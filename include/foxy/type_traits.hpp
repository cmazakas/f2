#ifndef FOXY_TYPE_TRAITS_HPP_
#define FOXY_TYPE_TRAITS_HPP_

#include <boost/asio/strand.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/serializer.hpp>

#include <type_traits>

namespace foxy {

// is_strand
//
template <typename T>
struct is_strand : std::false_type {};

template <typename Executor>
struct is_strand<boost::asio::strand<Executor>> : std::true_type {};

template <typename T>
inline constexpr
bool is_strand_v = is_strand<T>::value;

// is_message
//
template <typename T>
struct is_message : std::false_type {};

template <bool isRequest, typename Body, typename Fields>
struct is_message<
  boost::beast::http::message<
    isRequest, Body, Fields
  >
> : std::true_type {};

template <typename T>
inline constexpr
bool is_message_v = is_message<T>::value;

// is_serializer
//
template <typename T>
struct is_serializer : std::false_type {};

template <bool isRequest, typename Body, typename Fields>
struct is_serializer<
  boost::beast::http::serializer<
    isRequest, Body, Fields
  >
> : std::true_type {};

template <typename T>
inline constexpr
bool is_serializer_v = is_serializer<T>::value;

} // foxy

#endif // FOXY_TYPE_TRAITS_HPP_