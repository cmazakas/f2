#ifndef FOXY_DETAIL_REMOVE_HEADER_HPP_
#define FOXY_DETAIL_REMOVE_HEADER_HPP_

#include <boost/spirit/home/x3.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/type_traits.hpp>

#include <boost/range/algorithm/for_each.hpp>

#include <vector>
#include <string>

namespace foxy {

/**
 * partition_connection_options is used to partition the Connection header
 * field and any enumerated options from in_fields to out_fields
 *
 * This is primarily intended for use in intermediary servers where hop-by-hop
 * semantics must be respected
 */
template <typename Fields>
auto partition_connection_options(Fields& in_fields, Fields& out_fields) {

  namespace x3    = boost::spirit::x3;
  namespace http  = boost::beast::http;
  namespace range = boost::range;

  auto options = std::vector<std::string>();

  range::for_each(
    in_fields.equal_range(http::field::connection),
    [&](auto const& field) {
      auto const val = field.value();
      if (val == "") { return; }

      auto const option_rule = +(x3::char_ - ',');

      // the ABNF grammar listed in rfc 7230 is as follows:
      //
      // BWS = OWS
      // Connection = *( "," OWS ) connection-option *( OWS "," [ OWS
      // connection-option ] )
      //
      // this then gives us our parsed list of connection options from every
      // Connection header field listed in the input fields object
      //
      x3::phrase_parse(
        val.begin(), val.end(),
        *x3::lit(',') >> option_rule >> *(',' >> option_rule),
        x3::ascii::space,
        options);

      out_fields.insert(http::field::connection, field.value());
    });

  // iterate the list of connection options, finding all headers from in_fields
  // and then copying them into out_fields
  //
  range::for_each(
    options,
    [&](auto const& opt) {

      range::for_each(
        in_fields.equal_range(opt),
        [&](auto const& in_field) {
          out_fields.insert(opt, in_field.value());
        });
    });

  range::for_each(options, [&](auto const& opt) { in_fields.erase(opt); });
  in_fields.erase(http::field::connection);
}

} // foxy

#endif // FOXY_DETAIL_REMOVE_HEADER_HPP_
