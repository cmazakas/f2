
#include "foxy/remove_header.hpp"
#include <boost/beast/http.hpp>
#include <catch/catch.hpp>

namespace http = boost::beast::http;

TEST_CASE("Our connection header removal function") {
  SECTION("should do as advertised") {
    auto fields = http::fields();
    fields.set(http::field::connection, "lol");
    fields.set(http::field::connection, "foo");

    foxy::remove_header(http::field::connection, fields);
  }
}