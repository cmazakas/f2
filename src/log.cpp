#include "foxy/log.hpp"
#include <iostream>

auto foxy::log_error(
  boost::system::error_code const ec,
  std::string_view const what
) -> void {

  std::cerr << what << " : " << ec << "\n\n";
}