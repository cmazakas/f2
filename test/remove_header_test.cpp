#include "foxy/remove_header.hpp"
#include <boost/beast/http.hpp>
#include <catch/catch.hpp>

namespace http = boost::beast::http;

TEST_CASE("Our connection header removal function") {
  SECTION("should do as advertised") {

    auto fields = http::fields();
    fields.insert(http::field::connection, "lol");
    fields.insert(http::field::connection, "foo");

    fields.insert("lol", "muahahaha");
    fields.insert("lol", "and another one too!");

    fields.insert("foo", "we'll only test one instance of this field");

    foxy::remove_connection_header(fields);

    REQUIRE(fields["lol"] == "");
    REQUIRE(fields["foo"] == "");
    REQUIRE(fields[http::field::connection] != "");
  }
}