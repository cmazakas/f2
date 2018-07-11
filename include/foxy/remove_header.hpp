#ifndef FOXY_DETAIL_REMOVE_HEADER_HPP_
#define FOXY_DETAIL_REMOVE_HEADER_HPP_

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/type_traits.hpp>

namespace foxy {

template <typename Fields>
auto remove_header(boost::beast::http::field const field, Fields& fields) {
  auto const [begin, end] = fields.equal_range(field);

  // we need to now begin parsing each field value, checking that it follows
  // our grammars and then gather them all up so we can erase them
  //

}

} // foxy

#endif // FOXY_DETAIL_REMOVE_HEADER_HPP_