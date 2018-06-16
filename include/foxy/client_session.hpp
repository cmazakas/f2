#ifndef FOXY_CLIENT_SESSION_HPP_
#define FOXY_CLIENT_SESSION_HPP_

#include <iostream>
#include <memory>
#include <utility>
#include <string_view>

#include <boost/asio/post.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/associated_executor.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include "foxy/coroutine.hpp"
#include "foxy/multi_stream.hpp"

namespace foxy {

struct client_session {
public:
  using timer_type  = boost::asio::steady_timer;
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = multi_stream;
  using strand_type = boost::asio::strand<boost::asio::executor>;

private:
  struct session_state {
    timer_type  timer;
    buffer_type buffer;
    stream_type stream;
    strand_type strand;

    session_state()                     = delete;
    session_state(session_state const&) = default;
    session_state(session_state&&)      = default;

    explicit
    session_state(boost::asio::io_context& io);

    explicit
    session_state(boost::asio::io_context& io, boost::asio::ssl::context& ctx);

    template <typename CompletionToken>
    auto post(CompletionToken&& token) -> void {
      auto executor =
        boost::asio::get_associated_executor(token, stream.get_executor());

      return boost::asio::post(executor, std::forward<CompletionToken>(token));
    }
  };

  std::shared_ptr<session_state> s_;

public:
  client_session()                      = delete;
  client_session(client_session const&) = default;
  client_session(client_session&&)      = default;

  explicit
  client_session(boost::asio::io_context& io);

  explicit
  client_session(boost::asio::io_context& io, boost::asio::ssl::context& ctx);

  template <typename CompletionToken>
  auto async_connect(
    std::string_view const host,
    std::string_view const service,
    CompletionToken&&      token
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    CompletionToken,
    void(boost::system::error_code, boost::asio::ip::tcp::endpoint)
  ) {
    namespace asio = boost::asio;
    namespace ssl  = asio::ssl;
    using asio::ip::tcp;

    asio::async_completion<
      CompletionToken,
      void(boost::system::error_code, boost::asio::ip::tcp::endpoint)
    >
    init(token);

    co_spawn(
      s_->strand,
      [
        host, service, s = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> awaitable<void, strand_type> {
        try {
          auto token = co_await this_coro::token();

          auto host_str = std::string(host);

          if (s->stream.encrypted()) {
            SSL_set_tlsext_host_name(
              s->stream.ssl_stream().native_handle(), host_str.c_str());
          }

          auto resolver  = tcp::resolver(s->stream.get_executor().context());
          auto endpoints = co_await resolver.async_resolve(
            asio::string_view(host.data(), host.size()),
            asio::string_view(service.data(), service.size()),
            token);

          auto endpoint = co_await asio::async_connect(
            s->stream.next_layer(), endpoints, token);

          if (s->stream.encrypted()) {
            (void ) co_await s->stream.ssl_stream().async_handshake(
              ssl::stream_base::client, token);
          }

          auto executor =
            asio::get_associated_executor(handler, s->stream.get_executor());

          co_return s->post(
            [endpoint = std::move(endpoint), handler = std::move(handler)]
            () mutable {
              handler({}, endpoint);
            });

        } catch(boost::system::error_code const& ec) {

          co_return s->post(
            [ec, handler = std::move(handler)]
            () mutable {
              handler(ec, tcp::endpoint());
            });
        }
      },
      detached);

    return init.result.get();
  }

  template <
    typename Message,
    typename Parser,
    typename CompletionToken
  >
  auto async_write(
    Message&          message,
    Parser&           parser,
    CompletionToken&& token
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    CompletionToken, void(boost::system::error_code)
  ) {
    namespace asio = boost::asio;
    namespace http = boost::beast::http;
    using asio::ip::tcp;

    asio::async_completion<CompletionToken, void(boost::system::error_code)>
    init(token);

    co_spawn(
      s_->strand,
      [
        &message, &parser, s = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> awaitable<void, strand_type> {
        try {
          auto token = co_await this_coro::token();

          (void ) co_await http::async_write(s->stream, message, token);
          (void ) co_await http::async_read(
            s->stream, s->buffer, parser, token);

          co_return s->post(
            [handler = std::move(handler)]() mutable { handler({}); });

        } catch(boost::system::error_code const& ec) {

          co_return s->post(
            [ec, handler = std::move(handler)]() mutable { handler(ec); });
        }
      },
      detached);

    return init.result.get();
  }

  auto shutdown() -> void;

  template <typename CompletionToken>
  auto async_ssl_shutdown(CompletionToken&& token) {
    namespace asio = boost::asio;
    namespace http = boost::beast::http;
    using asio::ip::tcp;

    asio::async_completion<CompletionToken, void(boost::system::error_code)>
    init(token);

    co_spawn(
      s_->strand,
      [
        s = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> awaitable<void> {

        auto ec    = boost::system::error_code();
        auto token = co_await this_coro::token();

        auto error_token =
          redirect_error_t<std::decay_t<decltype(token)>>(token, ec);

        (void) s->stream.ssl_stream().async_shutdown(error_token);

        s->post(
          [ec, handler = std::move(handler)]() mutable -> void {
            handler(ec);
          });
      },
      detached);

    return init.result.get();
  }
};

} // foxy

#endif // FOXY_CLIENT_SESSION_HPP_
