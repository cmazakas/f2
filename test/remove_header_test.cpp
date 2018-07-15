#include "foxy/remove_header.hpp"
#include <boost/beast/http.hpp>
#include <catch/catch.hpp>

namespace http = boost::beast::http;

TEST_CASE("Our connection header removal function") {
  SECTION("should do as advertised") {

    auto const* upgrade_field = ", , ,, , , ,,upgrade";

    auto fields = http::fields();
    fields.insert(http::field::connection, upgrade_field);
    fields.insert(http::field::upgrade, "h2c, HTTPS/1.3, IRC/6.9, RTA/x11");

    auto proxy_fields = http::fields();

    foxy::partition_connection_options(fields, proxy_fields);

    CHECK(fields[http::field::connection] == "");
    CHECK(fields[http::field::upgrade] == "");

    CHECK(proxy_fields[http::field::connection] == upgrade_field);
    CHECK(
      proxy_fields[http::field::upgrade] ==
      "h2c, HTTPS/1.3, IRC/6.9, RTA/x11");
  }
}