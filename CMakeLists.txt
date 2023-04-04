cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

project(cpp_taskflow)
add_subdirectory(src)

set(CMAKE_CXX_FLAGS "-O3")