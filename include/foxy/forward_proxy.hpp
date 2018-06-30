#ifndef FOXY_FORWARD_PROXY_HPP_
#define FOXY_FORWARD_PROXY_HPP_

#include <boost/asio/io_context.hpp>

#include <boost/asio/ip/tcp.hpp>

#include "foxy/multi_stream.hpp"

namespace foxy {

struct forward_proxy {

public:
  using acceptor_type = boost::asio::ip::tcp::acceptor;
  using stream_type   = multi_stream;
  using endpoint_type = boost::asio::ip::tcp::endpoint;

private:
  acceptor_type acceptor_;
  stream_type   socket_;

public:
  forward_proxy(
    boost::asio::io_context& io,
    endpoint_type const&     local_endpoint)
  : acceptor_(io, local_endpoint, false)
  , socket_(io)
  {}
};

} // foxy

#endif // FOXY_FORWARD_PROXY_HPP_