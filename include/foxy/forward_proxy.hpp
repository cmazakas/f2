#ifndef FOXY_FORWARD_PROXY_HPP_
#define FOXY_FORWARD_PROXY_HPP_

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>

#include "foxy/multi_stream.hpp"

namespace foxy {

struct forward_proxy {

public:
  using acceptor_type = boost::asio::ip::tcp::acceptor;
  using stream_type   = multi_stream;
  using endpoint_type = boost::asio::ip::tcp::endpoint;

private:
  struct state {
    acceptor_type acceptor;
    stream_type   socket;

    state()             = delete;
    state(state const&) = delete;
    state(state&&)      = default;

    state(
      boost::asio::io_context& io,
      endpoint_type const&     local_endpoint,
      bool const               reuse_addr);
  };

  std::shared_ptr<state> s_;

public:
  forward_proxy()                     = delete;
  forward_proxy(forward_proxy const&) = delete;
  forward_proxy(forward_proxy&&)      = default;

  forward_proxy(
    boost::asio::io_context& io,
    endpoint_type const&     local_endpoint,
    bool const               reuse_addr);

  auto run() -> void;
};

} // foxy

#endif // FOXY_FORWARD_PROXY_HPP_