#ifndef FOXY_SERVER_SESSION_HPP_
#define FOXY_SERVER_SESSION_HPP_

#include "foxy/coroutine.hpp"
#include "foxy/multi_stream.hpp"
#include "foxy/detail/session.hpp"

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

namespace foxy {

struct server_session : public detail::session {

public:
  using timer_type  = detail::session_state::timer_type;
  using buffer_type = detail::session_state::buffer_type;
  using stream_type = detail::session_state::stream_type;
  using strand_type = detail::session_state::strand_type;

  server_session()                      = delete;
  server_session(server_session const&) = default;
  server_session(server_session&&)      = default;

  explicit
  server_session(multi_stream stream);

  auto shutdown() -> void;
};

} // foxy

#endif // FOXY_SERVER_SESSION_HPP_