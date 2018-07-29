#include "foxy/client_session.hpp"

// foxy::client_session::session_state::session_state(boost::asio::io_context& io)
// : timer(io)
// , stream(io)
// {
// }

// foxy::client_session::session_state::session_state(
//   boost::asio::io_context&   io,
//   boost::asio::ssl::context& ctx)
// : timer(io)
// , stream(io, ctx)
// {
// }

foxy::client_session::client_session(boost::asio::io_context& io)
: detail::session(io)
{
}

foxy::client_session::client_session(
  boost::asio::io_context&   io,
  boost::asio::ssl::context& ctx)
: detail::session(io, ctx)
{
}

auto foxy::client_session::shutdown(boost::system::error_code& ec) -> void {
  auto& multi_stream = s_->stream;

  multi_stream
    .stream()
    .shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
}