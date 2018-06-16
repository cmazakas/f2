#include <iostream>
#include <thread>

#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/use_future.hpp>

#include "foxy/coroutine.hpp"
#include "foxy/client_session.hpp"

#include <catch/catch.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace ssl  = asio::ssl;

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

        s.async_write(
          m, p,
          [s, message, parser, &was_valid_request]
          (error_code const ec) mutable -> void {

            s.shutdown();

            auto msg = parser->release();

            auto is_correct_status = (msg.result_int() == 200);
            auto received_body     = (msg.body().size() > 0);

            CHECK(is_correct_status);
            CHECK(received_body);

            was_valid_request = is_correct_status && received_body;
        });
      });

    io.run();
    REQUIRE(was_valid_request);
  }

  SECTION("should work with coros as well") {

    asio::io_context io;

    auto was_valid_request = false;

    asio::spawn(
      io,
      [&](asio::yield_context yield_ctx) -> void {

        auto s = foxy::client_session(io);

        auto message =
          http::request<http::empty_body>(http::verb::get, "/", 11);

        http::response_parser<http::string_body>
        parser;

        s.async_connect("www.google.com", "80", yield_ctx);
        s.async_write(message, parser, yield_ctx);
        s.shutdown();

        auto msg = parser.release();

        auto is_correct_status = (msg.result_int() == 200);
        auto received_body     = (msg.body().size() > 0);

        CHECK(is_correct_status);
        CHECK(received_body);

        was_valid_request = is_correct_status && received_body;

      });

    io.run();

    REQUIRE(was_valid_request);
  }

  SECTION("should support SSL") {

    asio::io_context io;

    auto was_valid_request = false;

    foxy::co_spawn(
      io,
      [&]() -> foxy::awaitable<void> {

        auto token = co_await foxy::this_coro::token();

        auto ctx = ssl::context(ssl::context::sslv23_client);
        auto s   = foxy::client_session(io, ctx);

        auto message =
          http::request<http::empty_body>(http::verb::get, "/", 11);

        http::response_parser<http::string_body>
        parser;

        (void ) co_await s.async_connect("www.google.com", "443", token);
        (void ) co_await s.async_write(message, parser, token);
        (void ) co_await s.async_ssl_shutdown(token);

        auto msg = parser.release();

        auto is_correct_status = (msg.result_int() == 200);
        auto received_body     = (msg.body().size() > 0);

        CHECK(is_correct_status);
        CHECK(received_body);

        was_valid_request = is_correct_status && received_body;

        co_return;
      },
      foxy::detached);

    io.run();

    REQUIRE(was_valid_request);
  }

  SECTION("should support SSL with futures") {

    asio::io_context io;

    auto was_valid_request = false;

    asio::spawn(
      io,
      [&](asio::yield_context yield_ctx) -> void {
        auto ctx = ssl::context(ssl::context::sslv23_client);
        auto s   = foxy::client_session(io, ctx);

        auto message =
          http::request<http::empty_body>(http::verb::get, "/", 11);

        http::response_parser<http::string_body>
        parser;

        auto connect_token
          = s.async_connect("www.google.com", "443", asio::use_future);

        connect_token.get();

        auto write_token = s.async_write(message, parser, asio::use_future);
        write_token.get();

        auto ssl_token = s.async_ssl_shutdown(asio::use_future);
        ssl_token.get();

        auto msg = parser.release();

        auto is_correct_status = (msg.result_int() == 200);
        auto received_body     = (msg.body().size() > 0);

        CHECK(is_correct_status);
        CHECK(received_body);

        was_valid_request = is_correct_status && received_body;
      });

    std::thread t([&]() { io.run(); });
    io.run();
    t.join();

    REQUIRE(was_valid_request);
  }
}