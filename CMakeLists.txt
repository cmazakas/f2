cmake_minimum_required(VERSION 3.11)

project(foxy)

find_path(CATCH_INCLUDE_DIR "catch.hpp")

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

find_package(
  Boost 1.67
  REQUIRED
    system
)

add_library(
  foxy

  INTERFACE
)

if (MSVC)
  target_compile_options(foxy INTERFACE "/await")

  # Win10
  target_compile_definitions(foxy INTERFACE _WIN32_WINNT=0x0A00)
endif()

target_compile_definitions(
  foxy

  INTERFACE
  BOOST_COROUTINES_NO_DEPRECATION_WARNING=1
  BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE=1
  _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING=1
)

target_compile_features(foxy INTERFACE cxx_std_17)

target_include_directories(
  foxy

  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  foxy

  INTERFACE
  Boost::boost
  Boost::system
)

# keep this around, just in case
if (MSVC)
  target_link_libraries(foxy INTERFACE Boost::disable_autolinking)
endif()

add_library(test_utils INTERFACE)
target_include_directories(test_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test/include)

add_executable(
  foxy_tests

  ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/client_test.cpp
)

target_link_libraries(
  foxy_tests

  PRIVATE
  foxy
  test_utils
  Catch
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake.modules/")
enable_testing()
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(foxy_tests)
