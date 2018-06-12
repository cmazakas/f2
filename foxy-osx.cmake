set(TESTING ON)
set(CMAKE_BUILD_TYPE Debug)
set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_C_COMPILER /Users/exbigboss/clang+llvm-6.0.0-x86_64-apple-darwin/bin/clang)
set(CMAKE_CXX_COMPILER /Users/exbigboss/clang+llvm-6.0.0-x86_64-apple-darwin/bin/clang++)
add_compile_options("-fcoroutines-ts")
link_libraries("pthread")
include("/Users/exbigboss/vcpkg/scripts/buildsystems/vcpkg.cmake")

