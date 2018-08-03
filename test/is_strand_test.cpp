#include "foxy/type_traits.hpp"
#include "foxy/detail/get_strand.hpp"
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <catch2/catch.hpp>

namespace asio = boost::asio;

TEST_CASE("Our is_strand metafunction...") {
  SECTION("should assert that strands are strands and not-strands aren't") {

    static_assert(
      foxy::is_strand_v<asio::strand<asio::io_context::executor_type>>,
      "if you're reading this, it means you broke the `is_strand` "
      "metafunction");

    static_assert(!foxy::is_strand_v<int>, "int is not a strand, silly");

    asio::io_context io;

    auto handler  = []{};
    auto executor = io.get_executor();

    static_assert(!foxy::is_strand_v<decltype(handler)>);

    static_assert(
      foxy::is_strand_v<
        decltype(foxy::detail::get_strand(handler, executor))
      >,
      "if you're reading this, it means you broke `detail::get_strand` "
      "(uh oh)");
  }
}