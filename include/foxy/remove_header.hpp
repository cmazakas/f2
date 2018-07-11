#ifndef FOXY_DETAIL_REMOVE_HEADER_HPP_
#define FOXY_DETAIL_REMOVE_HEADER_HPP_

#include <boost/spirit/home/x3.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/type_traits.hpp>

#include <vector>
#include <string>
#include <algorithm>

namespace foxy {

template <typename Fields>
auto remove_connection_header(Fields& fields) {
  namespace x3   = boost::spirit::x3;
  namespace http = boost::beast::http;

  auto const field = http::field::connection;

  auto [begin, end] = fields.equal_range(field);

  auto tokens = std::vector<std::string>();

  std::for_each(begin, end, [&](auto const& field) mutable {
    auto const val = fields[field.name()];
    if (val == "") { return; }

    auto const token = +(x3::char_ - ',');

    x3::phrase_parse(
      val.begin(), val.end(),
      *x3::lit(',') >> token >> *(',' >> token),
      x3::ascii::space,
      tokens);
  });
}

} // foxy

#endif // FOXY_DETAIL_REMOVE_HEADER_HPP_