#ifndef FOXY_CLIENT_SESSION_HPP_
#define FOXY_CLIENT_SESSION_HPP_

#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace foxy {

struct client_session {
public:
  using timer_type  = boost::asio::steady_timer;
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = boost::asio::ip::tcp::socket;
  using strand_type =
    boost::asio::strand<boost::asio::io_context::executor_type>;

private:
  struct session_state {
    timer_type  timer;
    buffer_type buffer;
    stream_type stream;
    strand_type strand;

    session_state(boost::asio::io_context& io)
    : timer(io)
    , stream(io)
    , strand(stream.get_executor())
    {
    }
  };

  std::shared_ptr<session_state> s_;

public:
  client_session()                      = delete;
  client_session(client_session const&) = default;
  client_session(client_session&&)      = default;

  explicit
  client_session(boost::asio::io_context& io)
  : s_(std::make_shared<session_state>(io))
  {
  }
};

} // foxy

#endif // FOXY_CLIENT_SESSION_HPP_