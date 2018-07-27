#include "foxy/forward_proxy.hpp"

#include <boost/system/error_code.hpp>

#include <boost/beast/http.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/container/vector.hpp>

#include <array>
#include <string>
#include <iostream>

#include "foxy/log.hpp"
#include "foxy/coroutine.hpp"
#include "foxy/partition.hpp"
#include "foxy/server_session.hpp"
#include "foxy/client_session.hpp"

namespace x3     = boost::spirit::x3;
namespace asio   = boost::asio;
namespace beast  = boost::beast;
namespace http   = beast::http;
namespace fusion = boost::fusion;

using boost::system::error_code;
using boost::ignore_unused;

namespace {

// TODO: enforce finite message lengths because we so heavily rely on
// persistence in the case of our proxy and messages are only considered
// finite via Content-Length and Transfer-Encoding: chunked otherwise
// messages are only considered to end when the connection is closed
//

auto init(
  foxy::server_session& server_session,
  foxy::client_session& client_session,
  error_code&           ec)-> foxy::awaitable<void> {

  auto token       = co_await foxy::this_coro::token();
  auto error_token = foxy::redirect_error(token, ec);

  while (true) {
    http::request_parser<http::empty_body>
    parser;

    ignore_unused(
      co_await server_session.async_read(parser, error_token));

    if (ec) {
      break;
    }

    // TODO: find out if we need to handle is_header_done() returning false for
    // the parser/request (we probably do?)
    //
    auto request = parser.get();

    // our forward proxy should only support the CONNECT method for the
    // foreseeable future
    //
    if (request.method() != http::verb::connect) {
      auto response = http::response<http::string_body>(
        http::status::method_not_allowed, 11,
        "Invalid HTTP request method. Only CONNECT is supported\n\n");

      response.prepare_payload();

      ignore_unused(
        co_await server_session.async_write(response, error_token));

      continue;
    }

    // if we have the correct verb but the connection was signalled to _not_ be
    // persistent, gracefully end the connection now
    //
    if (!request.keep_alive()) {
      auto response = http::response<http::string_body>(
        http::status::bad_request, 11,
        "Connection must be persistent to allow proper tunneling\n\n");

      response.prepare_payload();

      ignore_unused(
        co_await server_session.async_write(response, error_token));

      break;
    }

    // attempt to establish the external connection and form the tunnel
    //
    auto const target = request.target();

    auto host = std::string();
    auto port = std::string();

    auto host_and_port =
      fusion::vector<std::string&, std::string&>(host, port);

    x3::parse(
      target.begin(), target.end(),
      +(x3::char_ - ":") >> -(":" >> +x3::digit),
      host_and_port);

    ignore_unused(
      co_await client_session.async_connect(host, port, error_token));

    if (ec) {
      auto response = http::response<http::string_body>(
        http::status::bad_request, 11,
        "Unable to establish connection with remote host\n\n");

      response.prepare_payload();

      ignore_unused(
        co_await server_session.async_write(response, error_token));

      continue;
    }

    // we were able to successfully connect to the remote, reply with a 200
    // and thus begin the tunneling
    //
    auto response = http::response<http::empty_body>(http::status::ok, 11);
    ignore_unused(
      co_await server_session.async_write(response, error_token));

    break;
  }
}

auto tunnel(
  foxy::server_session& server_session,
  foxy::client_session& client_session)-> foxy::awaitable<void> {

  auto ec          = error_code();
  auto token       = co_await foxy::this_coro::token();
  auto error_token = foxy::redirect_error(token, ec);

  auto buf = std::array<char, 2048>();

  while (true) {

    auto fields = http::fields();

    http::request_parser<http::buffer_body>
    parser;

    http::request_serializer<http::buffer_body, http::fields>
    serializer(parser.get());

    ignore_unused(
      co_await server_session.async_read_header(parser, error_token));
    if (ec) { break; }

    http::fields& req_fields = parser.get().base();
    foxy::partition_connection_options(req_fields, fields);

    ignore_unused(
      co_await client_session.async_write_header(serializer, error_token));
    if (ec) { break; }

    // and then we kind of copy-paste some code from the Beast HTTP relay
    // example
    //
    auto& body = parser.get().body();
    if (!parser.is_done()) {

      body.data = buf.data();
      body.size = buf.size();

      ignore_unused(
        co_await server_session.async_read(parser, error_token));
      if (ec == http::error::need_buffer) {
        ec = {};
      }
      if (ec) { break; }

      body.size = buf.size() - body.size;
      body.data = buf.data();
      body.more = !parser.is_done();

    } else {
      body.data = nullptr;
      body.size = 0;
    }


  }
}

auto handle_request(
  foxy::multi_stream multi_stream,
  asio::io_context&  io) -> foxy::awaitable<void> {

  auto ec          = error_code();
  auto token       = co_await foxy::this_coro::token();
  auto error_token = foxy::redirect_error(token, ec);

  auto server_session = foxy::server_session(std::move(multi_stream));

  // TODO: add SSL context
  //
  auto client_session = foxy::client_session(io);

  co_await init(server_session, client_session, ec);
  if (!ec) {
    co_await tunnel(server_session, client_session);
  }

  server_session.shutdown();
}

} // anonymous

foxy::forward_proxy::state::state(
  boost::asio::io_context& io,
  endpoint_type const&     local_endpoint,
  bool const               reuse_addr)
: acceptor(io, local_endpoint, reuse_addr)
, socket(io)
{
}

foxy::forward_proxy::forward_proxy(
  boost::asio::io_context& io,
  endpoint_type const&     local_endpoint,
  bool const               reuse_addr)
: s_(std::make_shared<state>(io, local_endpoint, reuse_addr))
{
}

auto foxy::forward_proxy::run() -> void {

  auto& acceptor = s_->acceptor;
  auto& socket   = s_->socket;
  auto& io       = socket.get_executor().context();

  co_spawn(
    io,
    [&, s = s_]() mutable -> awaitable<void> {

      auto token       = co_await this_coro::token();
      auto ec          = error_code();
      auto error_token = redirect_error(token, ec);

      while (true) {
        ignore_unused(
          co_await acceptor.async_accept(socket.stream(), error_token));
        if (ec) {
          log_error(ec, "proxy server connection acceptance");
          break;
        }

        co_spawn(
          io,
          [&, multi_stream = std::move(socket)]() mutable {
            return handle_request(std::move(multi_stream), io); },
          detached);
      }
      co_return;
    },
    detached);
}