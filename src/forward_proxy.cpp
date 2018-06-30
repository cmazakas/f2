#include "foxy/forward_proxy.hpp"

#include <boost/system/error_code.hpp>

#include "foxy/log.hpp"
#include "foxy/coroutine.hpp"

using boost::system::error_code;

namespace {

auto handle_request() -> foxy::awaitable<void> {
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
          continue;
        }

        co_spawn(io, handle_request, detached);
      }
      co_return;
    },
    detached);
}