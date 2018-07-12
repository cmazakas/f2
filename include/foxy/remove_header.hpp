#ifndef FOXY_DETAIL_REMOVE_HEADER_HPP_
#define FOXY_DETAIL_REMOVE_HEADER_HPP_

#include <boost/spirit/home/x3.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/type_traits.hpp>

#include <boost/range/algorithm/for_each.hpp>

#include <vector>
#include <string>

namespace foxy {

template <typename Fields>
auto remove_connection_header(Fields& fields) {

  namespace x3    = boost::spirit::x3;
  namespace http  = boost::beast::http;
  namespace range = boost::range;

  auto tokens = std::vector<std::string>();

  range::for_each(
    fields.equal_range(http::field::connection),
    [&](auto const& field) {
      auto const val = field.value();
      if (val == "") { return; }

      auto const token = +(x3::char_ - ',');

      x3::phrase_parse(
        val.begin(), val.end(),
        *x3::lit(',') >> token >> *(',' >> token),
        x3::ascii::space,
        tokens);
    });

  range::for_each(tokens, [&](auto const& t) { fields.erase(t); });
}

} // foxy

#endif // FOXY_DETAIL_REMOVE_HEADER_HPP_
