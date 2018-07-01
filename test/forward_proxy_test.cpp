#include <boost/asio/ip/address_v4.hpp>

#include "foxy/forward_proxy.hpp"

#include <catch.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;
using ip::tcp;

TEST_CASE("Our forward proxy") {
  SECTION("should forward requests on behalf of the client") {

    asio::io_context io;

    auto const src_addr     = ip::make_address_v4("127.0.0.1");
    auto const src_port     = static_cast<unsigned short>(1337);
    auto const src_endpoint = tcp::endpoint(src_addr, src_port);

    auto const reuse_addr = true;

    foxy::forward_proxy proxy(io, src_endpoint, reuse_addr);
    proxy.run();
    io.run();
  }
}