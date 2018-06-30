#ifndef FOXY_LOG_HPP_
#define FOXY_LOG_HPP_

#include <string_view>
#include <boost/system/error_code.hpp>

namespace foxy {
  auto log_error(
    boost::system::error_code const ec,
    std::string_view const what
  ) -> void;
} // foxy

#endif // FOXY_LOG_HPP_