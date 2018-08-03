#ifndef FOXY_DETAIL_SESSION_STATE_HPP_
#define FOXY_DETAIL_SESSION_STATE_HPP_

#include "foxy/multi_stream.hpp"

#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/asio/ssl/context.hpp>

#include <boost/beast/core/flat_buffer.hpp>

namespace foxy {
namespace detail {

struct session_state {
  using timer_type  = boost::asio::steady_timer;
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = multi_stream;
  using strand_type = boost::asio::strand<boost::asio::executor>;

  timer_type  timer;
  buffer_type buffer;
  stream_type stream;

  session_state()                     = delete;
  session_state(session_state const&) = default;
  session_state(session_state&&)      = default;

  explicit
  session_state(boost::asio::io_context& io);

  explicit
  session_state(stream_type stream_);

  session_state(boost::asio::io_context& io, boost::asio::ssl::context& ctx);
};

} // detail
} // foxy

#endif // FOXY_DETAIL_SESSION_STATE_HPP_