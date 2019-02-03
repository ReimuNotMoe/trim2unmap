cmake_minimum_required(VERSION 2.8)
project(trim2unmap)

set(CMAKE_CXX_STANDARD 14)

add_executable(trim2unmap trim2unmap.c)
target_link_libraries(trim2unmap buse)
