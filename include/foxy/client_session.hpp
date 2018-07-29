#ifndef FOXY_CLIENT_SESSION_HPP_
#define FOXY_CLIENT_SESSION_HPP_

#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <boost/beast/core/bind_handler.hpp>

#include <boost/core/ignore_unused.hpp>

#include <memory>
#include <utility>
#include <iostream>
#include <string_view>

#include "foxy/coroutine.hpp"
#include "foxy/multi_stream.hpp"
#include "foxy/detail/session.hpp"

namespace foxy {

struct client_session : public detail::session {

public:
  using timer_type  = detail::session_state::timer_type;
  using buffer_type = detail::session_state::buffer_type;
  using stream_type = detail::session_state::stream_type;
  using strand_type = detail::session_state::strand_type;

  // client sessions cannot be default-constructed as they require an
  // `io_context`
  //
  client_session()                      = delete;

  client_session(client_session const&) = default;
  client_session(client_session&&)      = default;

  explicit
  client_session(boost::asio::io_context& io);

  // when constructed with an SSL context, the `client_session` will perform an
  // SSL handshake with the remote when calling `async_connect`
  // client sessions constructed with an SSL context need to be shutdown using
  // `async_ssl_shutdown`
  //
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

  // use `shutdown` in the case of a non-SSL `client_session`
  //
  auto shutdown(boost::system::error_code& ec) -> void;

  // `async_ssl_shutdown` is only meant to be called when the `client_session`
  // was constructed with a valid `asio::ssl::context&`
  // only this function must be called, calls to the `shutdown` will likely
  // result in undefined behavior
  //
  template <typename ShutdownHandler>
  auto async_ssl_shutdown(
    ShutdownHandler&& shutdown_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    ShutdownHandler, void(boost::system::error_code));
};

} // foxy

#include "foxy/impl/client_session.impl.hpp"

#endif // FOXY_CLIENT_SESSION_HPP_
