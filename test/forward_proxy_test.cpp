#include <boost/asio/ip/address_v4.hpp>

#include "foxy/forward_proxy.hpp"

#include <catch.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;
using ip::tcp;

TEST_CASE("Our forward proxy") {
  SECTION("should forward requests on behalf of the client") {

    asio::io_context io;

    auto const src_addr = ip::make_address_v4("127.0.0.1");
  }
}