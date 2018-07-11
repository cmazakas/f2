cmake_minimum_required(VERSION 3.11)

project(foxy LANGUAGES CXX)

find_package(
  Boost 1.67
  REQUIRED
    system
    date_time
)

find_package(
  OpenSSL REQUIRED
)

add_library(
  foxy

  STATIC

  ${CMAKE_CURRENT_SOURCE_DIR}/src/multi_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/client_session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/server_session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/forward_proxy.cpp
)

if (MSVC)
  target_compile_options(foxy PUBLIC "/await")

  # Win10
  target_compile_definitions(foxy PUBLIC _WIN32_WINNT=0x0A00)
endif()

target_compile_definitions(
  foxy

  PUBLIC
  BOOST_COROUTINES_NO_DEPRECATION_WARNING=1
  BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE=1
  _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING=1
)

target_compile_features(foxy PUBLIC cxx_std_17)

target_include_directories(
  foxy

  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  foxy

  PUBLIC
  Boost::boost
  Boost::system
  Boost::date_time
  OpenSSL::SSL
)

if (TESTING)

  find_package(
    Boost 1.67
    REQUIRED
      coroutine
  )

  find_package(Catch2 CONFIG REQUIRED)

  add_library(test_utils INTERFACE)
  target_include_directories(
    test_utils
    INTERFACE
      ${CMAKE_CURRENT_SOURCE_DIR}/test/include
  )

  add_executable(
    foxy_tests

    ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/client_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/forward_proxy_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/remove_header_test.cpp
  )

  target_link_libraries(
    foxy_tests

    PRIVATE
    foxy
    test_utils
    Catch2::Catch
    Boost::coroutine
  )

  set(
    CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/cmake.modules/"
  )

  enable_testing()
  include(ParseAndAddCatchTests)
  ParseAndAddCatchTests(foxy_tests)

endif()
