#include "foxy/client_session.hpp"

#include <catch/catch.hpp>

namespace asio = boost::asio;

using asio::ip::tcp;
using boost::system::error_code;

TEST_CASE("Our HTTP client session") {
  SECTION("should be able to callout to google") {
    asio::io_context io;

    auto s = foxy::client_session(io);

    // s.async_connect(
    //   [s](error_code const ec, tcp::endpoint const& endpoint) -> void {

    //   });

    io.run();
  }
}