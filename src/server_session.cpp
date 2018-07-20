#include "foxy/server_session.hpp"

foxy::server_session::session_state::session_state(stream_type stream_)
: stream(std::move(stream_))
, strand(stream.get_executor())
, timer(stream.get_executor().context())
{
}

foxy::server_session::server_session(stream_type stream)
: s_(std::make_shared<session_state>(std::move(stream)))
{
}

auto foxy::server_session::shutdown() -> void {
  (s_->stream)
    .stream()
    .shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}

auto foxy::server_session::stream() & -> stream_type& {
  return s_->stream;
}