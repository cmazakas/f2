set(TESTING ON)
set(CMAKE_BUILD_TYPE Debug)
set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_C_COMPILER /home/chris/clang/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang)
set(CMAKE_CXX_COMPILER /home/chris/clang/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang++)
add_compile_options("-fcoroutines-ts" "-nostdinc++")
set(BOOST_ROOT "/home/chris/boosts/install-ftw")
include_directories(SYSTEM "/home/chris/clang/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-16.04/include/c++/v1/")
link_directories(SYSTEM "/home/chris/clang/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-16.04/lib/")
link_libraries("c++" "c++abi" "pthread")
include("/home/chris/vcpkg/scripts/buildsystems/vcpkg.cmake")
