#ifndef FOXY_CLIENT_SESSION_HPP_
#define FOXY_CLIENT_SESSION_HPP_

#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/bind_handler.hpp>

#include <boost/core/ignore_unused.hpp>

#include <memory>
#include <utility>
#include <iostream>
#include <string_view>

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

    session_state(boost::asio::io_context& io, boost::asio::ssl::context& ctx);
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

  // `async_connect` performs forward name resolution on the specified host
  // and then attempts to form a TCP connection
  // `service` is the same as the original `asio::async_connect` function
  //
  template <typename ConnectHandler>
  auto async_connect(
    std::string      host,
    std::string      service,
    ConnectHandler&& connect_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    ConnectHandler,
    void(boost::system::error_code, boost::asio::ip::tcp::endpoint));

  // `async_write_header` mirrors the Beast function `http::async_write_header`
  // and will write the header portion of the Serializer to the session's
  // underlying stream object
  //
  template <
    typename Serializer,
    typename WriteHeaderHandler
  >
  auto async_write_header(
    Serializer&          serializer,
    WriteHeaderHandler&& write_header_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    WriteHeaderHandler, void(boost::system::error_code));

  // `async_write` mirrors the Beast function, `http::async_write` and writes
  // the input Serializer through the `client_session` to the currently
  // connected remote host
  //
  template <
    typename Serializer,
    typename WriteHandler
  >
  auto async_write(
    Serializer&    serializer,
    WriteHandler&& write_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
      WriteHandler, void(boost::system::error_code));

  // `async_request` writes a `http::request` to the remotely connected host
  // and then uses the supplied `http::response_parser` to store the response
  //
  template <
    typename Request,
    typename ResponseParser,
    typename WriteHandler
  >
  auto async_request(
    Request&        request,
    ResponseParser& parser,
    WriteHandler&&  write_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    WriteHandler, void(boost::system::error_code));

  template <typename ShutdownHandler>
  auto async_shutdown(
    ShutdownHandler&& shutdown_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    ShutdownHandler, void(boost::system::error_code)
  ) {
    namespace beast = boost::beast;
    namespace asio  = boost::asio;
    namespace http  = boost::beast::http;
    using asio::ip::tcp;
    using boost::ignore_unused;
    using boost::system::error_code;

    asio::async_completion<ShutdownHandler, void(boost::system::error_code)>
    init(shutdown_handler);

    co_spawn(
      s_->strand,
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

        if (multi_stream.is_ssl()) {
          ignore_unused(
            co_await multi_stream.ssl_stream().async_shutdown(error_token));

        } else {
          multi_stream
            .stream()
            .shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        }

        asio::post(executor, beast::bind_handler(std::move(handler), ec));
      },
      detached);

    return init.result.get();
  }
};

} // foxy

#include "foxy/impl/client_session.impl.hpp"

#endif // FOXY_CLIENT_SESSION_HPP_
