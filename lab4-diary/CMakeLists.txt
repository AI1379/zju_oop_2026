cmake_minimum_required(VERSION 3.10)
project(lab4-diary)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLES pdadd pdremove pdshow pdlist)

option(DIARY_USE_BINARY_PERSISTENCE "Use binary persistence for diary entries" ON)

foreach(exec ${EXECUTABLES})
  message(STATUS "Add ${exec} with ${exec}.cpp and diary_base.cpp")
  add_executable(${exec} ${exec}.cpp diary_base.cpp)
  target_compile_options(${exec} PRIVATE -Wall -Wextra)
  if(DIARY_USE_BINARY_PERSISTENCE)
    target_compile_definitions(${exec} PRIVATE -DDIARY_USE_BINARY_PERSISTENCE)
  endif()
  if(WIN32)
    target_compile_definitions(${exec} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  endif()
endforeach()
