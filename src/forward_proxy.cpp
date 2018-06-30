#include "foxy/forward_proxy.hpp"

#include <boost/system/error_code.hpp>

#include "foxy/log.hpp"
#include "foxy/coroutine.hpp"

using boost::system::error_code;

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
  co_spawn(
    s_->socket.get_executor().context(),
    [s = s_]() mutable -> awaitable<void> {

      auto token       = co_await this_coro::token();
      auto ec          = error_code();
      auto error_token = redirect_error(token, ec);

      auto& acceptor = s->acceptor;
      auto& socket   = s->socket;

      while(true) {
        co_await acceptor.async_accept(socket.stream(), error_token);
        if (ec) {
          log_error(ec, "proxy server connection acceptance");
          continue;
        }
      }
      co_return;
    },
    detached);
}