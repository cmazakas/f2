#include "foxy/server_session.hpp"

foxy::server_session::session_state::session_state(boost::asio::io_context& io)
: timer(io)
, stream(io)
, strand(stream.get_executor())
{
}

foxy::server_session::session_state::session_state(
  boost::asio::io_context&   io,
  boost::asio::ssl::context& ctx)
: timer(io)
, stream(io, ctx)
, strand(stream.get_executor())
{
}

foxy::server_session::server_session(boost::asio::io_context& io)
: s_(std::make_shared<session_state>(io))
{
}

foxy::server_session::server_session(
  boost::asio::io_context&   io,
  boost::asio::ssl::context& ctx)
: s_(std::make_shared<session_state>(io, ctx))
{
}

auto foxy::server_session::shutdown() -> void {
  (s_->stream)
    .stream()
    .shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}