#include "foxy/forward_proxy.hpp"

#include <boost/system/error_code.hpp>

#include <boost/beast/http.hpp>

#include <iostream>

#include "foxy/log.hpp"
#include "foxy/coroutine.hpp"
#include "foxy/server_session.hpp"

namespace asio = boost::asio;
namespace http = boost::beast::http;
using boost::system::error_code;
using boost::ignore_unused;

namespace {

auto handle_request(foxy::multi_stream multi_stream) -> foxy::awaitable<void> {

  auto ec          = error_code();
  auto token       = co_await foxy::this_coro::token();
  auto error_token = foxy::redirect_error(token, ec);

  // HTTP/1.1 defaults to persistent connections
  //
  auto session    = foxy::server_session(std::move(multi_stream));
  auto keep_alive = true;

  while (keep_alive) {
    http::request_parser<http::empty_body>
    parser;

    ignore_unused(
      co_await session.async_read(parser, error_token));

    if (ec == http::error::end_of_stream) {
      break;
    }

    if (ec) {
      co_return foxy::log_error(ec, "forward proxy request read");
    }

    // TODO: find out if we need to handle is_header_done() returning false for
    // the parser/request (we probably do?)
    //
    auto request = parser.get();
    keep_alive   = request.keep_alive();

    // our forward proxy should only support the CONNECT method for the
    // foreseeable future
    //
    if (request.method() != http::verb::connect) {

      auto response = http::response<http::string_body>(
        http::status::method_not_allowed, 11,
        "Invalid HTTP request method. Only CONNECT is supported");

      response.prepare_payload();

      ignore_unused(
        co_await session.async_write(response, error_token));

      continue;
    }
  }

  session.shutdown();

  co_return;
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

      while(true) {
        co_await acceptor.async_accept(socket.stream(), error_token);
        if (ec) {
          log_error(ec, "proxy server connection acceptance");
          break;
        }

        co_spawn(
          io,
          [multi_stream = std::move(socket)]() mutable {
            return handle_request(std::move(multi_stream)); },
          detached);
      }
      co_return;
    },
    detached);
}