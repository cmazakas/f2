#include "foxy/partition.hpp"

#include <boost/beast/http.hpp>
#include <catch/catch.hpp>

namespace http = boost::beast::http;

TEST_CASE("Our connection header removal function") {
  SECTION("should do as advertised") {

    auto const* upgrade_field = ", , ,, , , ,,upgrade";

    auto fields = http::fields();
    fields.insert(http::field::connection, upgrade_field);
    fields.insert(http::field::connection, "lol");
    fields.insert(http::field::upgrade, "h2c, HTTPS/1.3, IRC/6.9, RTA/x11");
    fields.insert("lol", "lol-tastic value");

    auto proxy_fields = http::fields();

    foxy::partition_connection_options(fields, proxy_fields);

    CHECK(fields[http::field::connection] == "");
    CHECK(fields[http::field::upgrade]    == "");
    CHECK(fields["lol"]                   == "");

    CHECK(proxy_fields[http::field::connection] == upgrade_field);
    CHECK(
      proxy_fields[http::field::upgrade] ==
      "h2c, HTTPS/1.3, IRC/6.9, RTA/x11");

    CHECK(proxy_fields["lol"] == "lol-tastic value");
  }

  SECTION("should support a variation of the Connection value") {

    auto const* connection_options = ", , ,, , , ,,upgrade , lol";

    auto fields = http::fields();
    fields.insert(http::field::connection, connection_options);
    fields.insert(http::field::upgrade, "h2c, HTTPS/1.3, IRC/6.9, RTA/x11");
    fields.insert("lol", "lol-tastic value");

    auto proxy_fields = http::fields();

    foxy::partition_connection_options(fields, proxy_fields);

    CHECK(fields[http::field::connection] == "");
    CHECK(fields[http::field::upgrade]    == "");
    CHECK(fields["lol"]                   == "");

    CHECK(proxy_fields[http::field::connection] == connection_options);
    CHECK(
      proxy_fields[http::field::upgrade] ==
      "h2c, HTTPS/1.3, IRC/6.9, RTA/x11");

    CHECK(proxy_fields["lol"] == "lol-tastic value");
  }
}