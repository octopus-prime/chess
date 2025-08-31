cmake_minimum_required(VERSION 3.28)

project(chess CXX)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3 -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src)

file (GLOB_RECURSE CHESS CONFIGURE_DEPENDS ./src/main.cpp ./src/*.hpp)
file (GLOB_RECURSE CHESS_TEST CONFIGURE_DEPENDS ./test/*.cpp ./test/*.hpp)

add_executable(chess ${CHESS})
add_executable(chess_test ${CHESS_TEST})
