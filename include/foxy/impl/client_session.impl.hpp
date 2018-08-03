#include "foxy/client_session.hpp"
#include "foxy/type_traits.hpp"
#include "foxy/detail/get_strand.hpp"

#include <chrono>

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

  foxy::co_spawn(
    strand,
    [
      s       = s_,
      host    = std::move(host),
      service = std::move(service),
      handler = std::move(init.completion_handler)
    ]() mutable -> foxy::awaitable<void, strand_type> {

      auto executor =
        asio::get_associated_executor(handler, s->stream.get_executor());

      auto token       = co_await this_coro::token();
      auto ec          = error_code();
      auto error_token = redirect_error(token, ec);

      if (s->stream.is_ssl()) {
        auto const res = SSL_set_tlsext_host_name(
          s->stream.ssl_stream().native_handle(), host.c_str());

        if (res != 1) {
          ec.assign(
            static_cast<int>(::ERR_get_error()),
            asio::error::get_ssl_category());

          co_return asio::post(
            executor,
            beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
        }
      }

      auto resolver  = tcp::resolver(s->stream.get_executor().context());
      auto endpoints =
        co_await resolver.async_resolve(host, service, error_token);

      if (ec) {
        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
      }

      auto endpoint = co_await asio::async_connect(
        s->stream.stream(), endpoints, error_token);

      if (ec) {
        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
      }

      if (s->stream.is_ssl()) {
        ignore_unused(
          co_await (s->stream)
            .ssl_stream()
            .async_handshake(ssl::stream_base::client, error_token));

        if (ec) {
          co_return asio::post(
            executor,
            beast::bind_handler(std::move(handler), ec, tcp::endpoint()));
        }
      }

      co_return asio::post(
        executor,
        beast::bind_handler(std::move(handler), error_code(), endpoint));
    },
    detached);

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