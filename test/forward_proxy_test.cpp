#include <boost/system/error_code.hpp>

#include <boost/asio/ip/address_v4.hpp>

#include <boost/beast/http.hpp>

#include "foxy/coroutine.hpp"
#include "foxy/forward_proxy.hpp"
#include "foxy/client_session.hpp"

#include <catch.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace ip   = asio::ip;
using ip::tcp;
using boost::system::error_code;

TEST_CASE("Our forward proxy") {
  SECTION("should forward requests on behalf of the client") {

    asio::io_context io;

    auto const src_addr     = ip::make_address_v4("127.0.0.1");
    auto const src_port     = static_cast<unsigned short>(1337);
    auto const src_endpoint = tcp::endpoint(src_addr, src_port);

    auto const reuse_addr = true;

    auto was_valid_request = true;

    foxy::co_spawn(
      io,
      [&]() mutable -> foxy::awaitable<void> {

        foxy::forward_proxy proxy(io, src_endpoint, reuse_addr);
        proxy.run();

        auto token = co_await foxy::this_coro::token();

        auto const num_reqs = 10;
        auto session        = foxy::client_session(io);

        (void ) co_await session.async_connect("127.0.0.1", "1337", token);

        for (int i = 0; i < num_reqs; ++i) {
          auto req = http::request<http::empty_body>(http::verb::get, "/", 11);
          req.keep_alive(i != (num_reqs - 1));

          http::response_parser<http::string_body>
          res_parser;

          (void ) co_await session.async_write(req, res_parser, token);

          auto res = res_parser.release();

          auto const invalid_method =
            res.result() == http::status::method_not_allowed;

          auto const is_valid_body =
              res.body() ==
              "Invalid HTTP request method. Only CONNECT is supported\n\n";

          CHECK(invalid_method);
          CHECK(is_valid_body);

          was_valid_request =
            was_valid_request && invalid_method && is_valid_body;
        }

        session.async_shutdown(token);

        io.stop();
        co_return;
      },
      foxy::detached);

    io.run();

    REQUIRE(was_valid_request);
  }
}