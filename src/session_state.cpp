#include "foxy/detail/session_state.hpp"

foxy::detail::session_state::session_state(boost::asio::io_context& io)
: timer(io)
, stream(io)
{
}

foxy::detail::session_state::session_state(
  boost::asio::io_context&   io,
  boost::asio::ssl::context& ctx)
: timer(io)
, stream(io, ctx)
{
}
