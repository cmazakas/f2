#include <iostream>
#include <boost/beast/http.hpp>
#include "foxy/client_session.hpp"

#include <catch/catch.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;

using asio::ip::tcp;
using boost::system::error_code;

TEST_CASE("Our HTTP client session") {
  SECTION("should be able to callout to google") {
    asio::io_context io;

    auto was_valid_request = false;

    auto s = foxy::client_session(io);
    s.async_connect(
      "www.google.com", "80",
      [s, &was_valid_request]
      (error_code const ec, tcp::endpoint const) mutable -> void {

        auto message = std::make_shared<
          http::request<http::empty_body>
        >(http::verb::get, "/", 11);

        auto parser = std::make_shared<
          http::response_parser<http::string_body>
        >();

        auto& m = *message;
        auto& p = *parser;

        s.async_send(
          m, p,
          [s, message, parser, &was_valid_request]
          (error_code const ec) -> void {
            auto msg = parser->release();

            auto is_correct_status = (msg.result_int() == 200);
            auto received_body     = (msg.body().size() > 0);

            CHECK(is_correct_status);
            CHECK(received_body);

            was_valid_request = true;
        });
      });

    io.run();
    REQUIRE(was_valid_request);
  }
}