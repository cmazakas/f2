#ifndef FOXY_DETAIL_GET_STRAND_HPP_
#define FOXY_DETAIL_GET_STRAND_HPP_

#include "foxy/type_traits.hpp"
#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <utility>
#include <iostream>

namespace foxy {
namespace detail {

template <typename Handler, typename Executor>
auto get_strand(Handler&& handler, Executor const& ex) {

  namespace asio = boost::asio;

  using handler_executor_type =
    std::decay_t<
      decltype(
        asio::get_associated_executor(
          std::forward<Handler>(handler), ex))>;

  if constexpr (foxy::is_strand_v<handler_executor_type>) {
    return asio::get_associated_executor(std::forward<Handler>(handler), ex);

  } else {

    return asio::strand<asio::executor>(
      asio::get_associated_executor(std::forward<Handler>(handler), ex));
  }
}

} // detail
} // foxy

#endif // FOXY_DETAIL_GET_STRAND_HPP_