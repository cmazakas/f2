#include "foxy/client_session.hpp"

#include "foxy/type_traits.hpp"
#include "foxy/detail/get_strand.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/associated_executor.hpp>

#include <boost/smart_ptr/make_unique.hpp>
#include <boost/optional.hpp>

namespace foxy {
namespace detail {

template <class Executor, class Lambda>
auto lift(Executor executor, Lambda lambda) {
  struct callable : public Lambda {

    Executor executor_;

    callable(Executor e, Lambda l)
    : Lambda(std::move(l))
    , executor_(e)
    {}

    using Lambda::operator();
    using executor_type = Executor;
    auto get_executor() const noexcept -> executor_type {
      return executor_;
    }
  };

  return callable(executor, std::move(lambda));
}

template <class Handler>
struct async_connect_op : public boost::asio::coroutine {
private:

  struct frame;

  std::shared_ptr<session_state> s_;
  std::unique_ptr<frame>         frame_;

  // frame impl
  //
  struct frame {
    boost::optional<boost::asio::ip::tcp::resolver>               resolver;
    boost::optional<boost::asio::ip::tcp::resolver::results_type> endpoints;
    boost::optional<boost::asio::ip::tcp::endpoint>               endpoint;
    boost::optional<std::string>                                  host;
    boost::optional<std::string>                                  service;
    Handler                                                       handler;

    frame(Handler h_)
    : handler(std::move(h_))
    {};
  };

public:
  async_connect_op()                        = delete;
  async_connect_op(async_connect_op const&) = default;
  async_connect_op(async_connect_op&&)      = default;

  template <class DeducedHandler>
  async_connect_op(
    std::string                    host,
    std::string                    service,
    std::shared_ptr<session_state> s,
    DeducedHandler&&               handler)
  : s_(std::move(s))
  , frame_(boost::make_unique<frame>(std::forward<DeducedHandler>(handler)))
  {
    frame_->host.emplace(std::move(host));
    frame_->service.emplace(std::move(service));
  }

  using executor_type = boost::asio::associated_executor_t<
    Handler, decltype(s_->stream.get_executor())>;

  auto get_executor() const noexcept -> executor_type {
    return boost::asio::get_associated_executor(
      frame_->handler,
      s_->stream.get_executor());
  }

  #include <boost/asio/yield.hpp>
  auto operator()(boost::system::error_code ec = {}) -> void {

    using boost::asio::ip::tcp;
    using boost::system::error_code;

    namespace ssl  = boost::asio::ssl;
    namespace asio = boost::asio;

    reenter(this) {
      yield asio::post(std::move(*this));

      if (s_->stream.is_ssl()) {
        auto const res = SSL_set_tlsext_host_name(
          s_->stream.ssl_stream().native_handle(), frame_->host->c_str());

        if (res != 1) {
          ec.assign(
            static_cast<int>(::ERR_get_error()),
            asio::error::get_ssl_category());

          auto h = std::move(frame_->handler);
          frame_.release();
          return h(ec, tcp::endpoint());
        }
      }

      frame_->resolver.emplace(s_->stream.get_executor().context());

      yield {
        auto const& host     = *(frame_->host);
        auto const& svc      = *(frame_->service);
        auto&       resolver = *(frame_->resolver);
        auto        executor = this->get_executor();

        resolver.async_resolve(
          host, svc, lift(
            executor,
            [self = std::move(*this)] (error_code ec, tcp::resolver::results_type endpoints) mutable -> void {
              self.frame_->endpoints.emplace(std::move(endpoints));
              self(ec);
            }));
      }

      if (ec) {
        auto h = std::move(frame_->handler);
        frame_.release();
        return h(ec, tcp::endpoint());
      }

      yield {
        auto& stream    = s_->stream.stream();
        auto  endpoints = *(frame_->endpoints);
        auto  executor  = this->get_executor();

        asio::async_connect(
          stream, std::move(endpoints),
          lift(executor, [self = std::move(*this)](error_code ec, tcp::endpoint endpoint) mutable -> void {
            self.frame_->endpoint.emplace(std::move(endpoint));
            self(ec);
          }));
      }

      if (ec) {
        auto h = std::move(frame_->handler);
        frame_.release();
        return h(ec, tcp::endpoint());
      }

      if (s_->stream.is_ssl()) {
        yield {
          auto& ssl_stream = s_->stream.ssl_stream();
          auto  executor   = this->get_executor();

          ssl_stream.async_handshake(ssl::stream_base::client, std::move(*this));
        }

        if (ec) {
          auto h = std::move(frame_->handler);
          frame_.release();
          return h(ec, tcp::endpoint());
        }
      }

      auto h        = std::move(frame_->handler);
      auto endpoint = std::move(*(frame_->endpoint));
      frame_.release();
      return h({}, std::move(endpoint));
    }
  }
  #include <boost/asio/unyield.hpp>
};

} // detail
} // foxy

template <typename ConnectHandler>
auto foxy::client_session::async_connect(
  std::string      host,
  std::string      service,
  ConnectHandler&& connect_handler
) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
  ConnectHandler,
  void(boost::system::error_code, boost::asio::ip::tcp::endpoint)
) {

  using boost::asio::ip::tcp;
  using boost::ignore_unused;
  using boost::system::error_code;

  namespace beast = boost::beast;
  namespace asio  = boost::asio;
  namespace ssl   = asio::ssl;

  asio::async_completion<
    ConnectHandler,
    void(boost::system::error_code, boost::asio::ip::tcp::endpoint)
  >
  init(connect_handler);

  auto strand = foxy::detail::get_strand(
    init.completion_handler, s_->stream.get_executor());

  using handler_type = BOOST_ASIO_HANDLER_TYPE(
    ConnectHandler, void(error_code, tcp::endpoint));

  foxy::detail::async_connect_op<handler_type>(
    std::move(host), std::move(service), s_,
    std::move(init.completion_handler))();

  // foxy::co_spawn(
  //   strand,
  //   [
  //     s       = s_,
  //     host    = std::move(host),
  //     service = std::move(service),
  //     handler = std::move(init.completion_handler)
  //   ]() mutable -> foxy::awaitable<void, strand_type> {

  //     auto executor =
  //       asio::get_associated_executor(handler, s->stream.get_executor());

  //     auto token       = co_await this_coro::token();
  //     auto ec          = error_code();
  //     auto error_token = redirect_error(token, ec);

  //     if (s->stream.is_ssl()) {
  //       auto const res = SSL_set_tlsext_host_name(
  //         s->stream.ssl_stream().native_handle(), host.c_str());

  //       if (res != 1) {
  //         ec.assign(
  //           static_cast<int>(::ERR_get_error()),
  //           asio::error::get_ssl_category());

  //         co_return asio::post(
  //           executor,
  //           beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
  //       }
  //     }

  //     auto resolver  = tcp::resolver(s->stream.get_executor().context());
  //     auto endpoints =
  //       co_await resolver.async_resolve(host, service, error_token);

  //     if (ec) {
  //       co_return asio::post(
  //         executor,
  //         beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
  //     }

  //     auto endpoint = co_await asio::async_connect(
  //       s->stream.stream(), endpoints, error_token);

  //     if (ec) {
  //       co_return asio::post(
  //         executor,
  //         beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
  //     }

  //     if (s->stream.is_ssl()) {
  //       ignore_unused(
  //         co_await (s->stream)
  //           .ssl_stream()
  //           .async_handshake(ssl::stream_base::client, error_token));

  //       if (ec) {
  //         co_return asio::post(
  //           executor,
  //           beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
  //       }
  //     }

  //     co_return asio::post(
  //       executor,
  //       beast::bind_handler(std::move(handler), error_code(), endpoint));
  //   },
  //   detached);

  return init.result.get();
}

template <
  typename Request,
  typename ResponseParser,
  typename WriteHandler
>
auto foxy::client_session::async_request(
  Request&        request,
  ResponseParser& parser,
  WriteHandler&&  write_handler
) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
  WriteHandler, void(boost::system::error_code)
) {
  using boost::asio::ip::tcp;
  using boost::ignore_unused;
  using boost::system::error_code;

  namespace beast = boost::beast;
  namespace asio  = boost::asio;
  namespace http  = boost::beast::http;

  asio::async_completion<WriteHandler, void(boost::system::error_code)>
  init(write_handler);

  auto strand = foxy::detail::get_strand(
    init.completion_handler, s_->stream.get_executor());

  co_spawn(
    strand,
    [
      &request, &parser, s = s_,
      handler = std::move(init.completion_handler)
    ]() mutable -> awaitable<void, strand_type> {

      auto token       = co_await this_coro::token();
      auto ec          = error_code();
      auto error_token = redirect_error(token, ec);

      auto executor =
        asio::get_associated_executor(handler, s->stream.get_executor());

      ignore_unused(
        co_await http::async_write(s->stream, request, error_token));

      if (ec) {
        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), ec));
      }

      ignore_unused(
        co_await http::async_read(
          s->stream,
          s->buffer,
          parser,
          error_token));

      if (ec) {
        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), ec));
      }

      co_return asio::post(
        executor,
        beast::bind_handler(std::move(handler), error_code()));
    },
    detached);

  return init.result.get();
}

template <typename ShutdownHandler>
auto foxy::client_session::async_ssl_shutdown(
  ShutdownHandler&& shutdown_handler
) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
  ShutdownHandler, void(boost::system::error_code)
) {
  using boost::asio::ip::tcp;
  using boost::ignore_unused;
  using boost::system::error_code;

  namespace beast = boost::beast;
  namespace asio  = boost::asio;
  namespace http  = boost::beast::http;

  asio::async_completion<ShutdownHandler, void(boost::system::error_code)>
  init(shutdown_handler);

  auto strand = foxy::detail::get_strand(
    init.completion_handler, s_->stream.get_executor());

  co_spawn(
    strand,
    [
      s = s_,
      handler = std::move(init.completion_handler)
    ]() mutable -> awaitable<void> {

      auto& multi_stream = s->stream;

      auto executor =
        asio::get_associated_executor(handler, multi_stream.get_executor());

      auto token       = co_await this_coro::token();
      auto ec          = error_code();
      auto error_token = redirect_error(token, ec);

      ignore_unused(
        co_await multi_stream.ssl_stream().async_shutdown(error_token));

      asio::post(executor, beast::bind_handler(std::move(handler), ec));
    },
    detached);

  return init.result.get();
}