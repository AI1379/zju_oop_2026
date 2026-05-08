cmake_minimum_required(VERSION 3.16)

project(lab5_fraction LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Configuring lab5-fraction")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "Using Clang compiler")
  add_compile_definitions(FRACTION_COMPILER_CLANG=1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Using GCC compiler")
  add_compile_definitions(FRACTION_COMPILER_GCC=1)
elseif(MSVC)
  message(STATUS "Using MSVC compiler")
  add_compile_definitions(FRACTION_COMPILER_MSVC=1)
else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
  add_compile_definitions(FRACTION_COMPILER_UNKNOWN=1)
endif()

# ============================================================
# Options
# ============================================================

option(FRACTION_ENABLE_TESTING "Build GoogleTest test targets" ON)
option(FRACTION_ENABLE_BENCHMARK "Build Google Benchmark target" ON)

# ============================================================
# Library & application
# ============================================================

add_library(fraction STATIC big_int.cpp)
target_include_directories(fraction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fraction_app calculator.cpp)
target_link_libraries(fraction_app PRIVATE fraction)

# ============================================================
# Testing
# ============================================================

if(FRACTION_ENABLE_TESTING)
  enable_testing()

  # 使用本地已安装/已构建的 GoogleTest。
  # 配置 CMake 前请设置 GTest_DIR 或 CMAKE_PREFIX_PATH 让 find_package 找到它。
  find_package(GTest CONFIG REQUIRED)

  add_executable(fraction_test fraction_test.cpp)
  target_link_libraries(fraction_test PRIVATE fraction GTest::gtest_main)

  add_executable(big_int_test big_int_test.cpp)
  target_link_libraries(big_int_test PRIVATE fraction GTest::gtest_main)

  add_executable(int128_test int128_test.cpp)
  target_link_libraries(int128_test PRIVATE fraction GTest::gtest_main)

  add_executable(fraction_bigint_test fraction_bigint_test.cpp)
  target_link_libraries(fraction_bigint_test PRIVATE fraction GTest::gtest_main)

  add_executable(fraction_int128_test fraction_int128_test.cpp)
  target_link_libraries(fraction_int128_test PRIVATE fraction GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(fraction_test)
  gtest_discover_tests(big_int_test)
  gtest_discover_tests(int128_test)
  gtest_discover_tests(fraction_bigint_test)
  gtest_discover_tests(fraction_int128_test)
endif()

# ============================================================
# Benchmark
# ============================================================

if(FRACTION_ENABLE_BENCHMARK)
  find_package(benchmark CONFIG QUIET)
  if(benchmark_FOUND)
    add_executable(big_int_bench big_int_bench.cpp)
    target_link_libraries(big_int_bench PRIVATE fraction benchmark::benchmark)
  else()
    message(WARNING "Google Benchmark not found. Set benchmark_DIR to enable the benchmark target.")
  endif()
endif()
