cmake_minimum_required(VERSION 3.16)

project(lab5_fraction LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Configuring lab5-fraction")

enable_testing()

# 使用本地已安装/已构建的 GoogleTest。
# 配置 CMake 前请准备以下环境变量之一，让 find_package 能找到它：
#   1) GTest_DIR：指向包含 GTestConfig.cmake 的目录
#   2) CMAKE_PREFIX_PATH：指向 GoogleTest 的安装前缀
# 示例：
#   set GTest_DIR=C:\path\to\gtest\lib\cmake\GTest
#   set CMAKE_PREFIX_PATH=C:\path\to\gtest\install
find_package(GTest CONFIG REQUIRED)

set(FRACTION_LIB_SOURCES
  big_int.cpp
)

set(FRACTION_LIB_INCLUDES
  fraction.hpp
  big_int.hpp
  concepts.hpp
  utility.hpp
)

# 1) 静态库目标
add_library(fraction STATIC
  ${FRACTION_LIB_SOURCES}
)
target_include_directories(fraction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 2) 可执行文件目标
add_executable(fraction_app
  calculator.cpp
)
target_link_libraries(fraction_app PRIVATE fraction)

# 3) gtest 测试目标
add_executable(fraction_test
  fraction_test.cpp
)
target_link_libraries(fraction_test PRIVATE fraction GTest::gtest_main)

add_executable(big_int_test
  big_int_test.cpp
)
target_link_libraries(big_int_test PRIVATE fraction GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(fraction_test)
gtest_discover_tests(big_int_test)

