#ifndef FOXY_SERVER_SESSION_HPP_
#define FOXY_SERVER_SESSION_HPP_

#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/bind_handler.hpp>

#include <boost/core/ignore_unused.hpp>

#include <memory>
#include <utility>
#include <iostream>
#include <string_view>

#include "foxy/coroutine.hpp"
#include "foxy/multi_stream.hpp"

namespace foxy {

struct server_session {

public:
  using timer_type  = boost::asio::steady_timer;
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = multi_stream;
  using strand_type = boost::asio::strand<boost::asio::executor>;

private:
  struct session_state {
    timer_type  timer;
    buffer_type buffer;
    stream_type stream;
    strand_type strand;

    session_state()                     = delete;
    session_state(session_state const&) = default;
    session_state(session_state&&)      = default;

    explicit
    session_state(boost::asio::io_context& io);

    session_state(boost::asio::io_context& io, boost::asio::ssl::context& ctx);
  };

  std::shared_ptr<session_state> s_;

public:
  server_session()                      = delete;
  server_session(server_session const&) = default;
  server_session(server_session&&)      = default;

  explicit
  server_session(boost::asio::io_context& io);

  server_session(boost::asio::io_context& io, boost::asio::ssl::context& ctx);
};

} // foxy

#endif // FOXY_SERVER_SESSION_HPP_