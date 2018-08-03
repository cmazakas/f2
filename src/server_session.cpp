#include "foxy/server_session.hpp"

#include <boost/asio/post.hpp>

namespace asio = boost::asio;

foxy::server_session::server_session(stream_type stream_)
: detail::session(std::move(stream_))
{
}

auto foxy::server_session::shutdown() -> void {
  auto& multi_stream = s_->stream;

  multi_stream
    .stream()
    .shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}