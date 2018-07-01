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

auto handle_request(
  asio::io_context&  io,
  foxy::multi_stream multi_stream) -> foxy::awaitable<void> {

  using parser_type = http::request_parser<http::empty_body>;

  auto ec          = error_code();
  auto token       = co_await foxy::this_coro::token();
  auto error_token = foxy::redirect_error(token, ec);

  auto session = foxy::server_session(io);
  parser_type parser;
  parser.skip(true);

  ignore_unused(
    co_await session.async_read(parser, error_token));

  if (ec) {
    co_return foxy::log_error(ec, "forward proxy request read");
  }

  auto request = parser.get();

  // TODO: find out if we need to handle is_header_done() returning false

  auto const request_method = request.method();
  if (request_method != http::verb::connect) {
    auto response = http::response<http::string_body>(
      http::status::method_not_allowed, 11,
      "Invalid HTTP request method. Only CONNECT is supported");

    response.prepare_payload();

    ignore_unused(co_await session.async_write(response, error_token));
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
          [&, multi_stream = std::move(socket)]() mutable {
            return handle_request(io, std::move(multi_stream)); },
          detached);
      }
      co_return;
    },
    detached);
}