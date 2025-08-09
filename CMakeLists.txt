cmake_minimum_required(VERSION 3.28)

project(chess CXX)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 26)
# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3 -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# IF (WIN32)
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker /STACK:1000000000")
# ENDIF()

include_directories(src)

# add_link_options(-flto)

file (GLOB_RECURSE CHESS CONFIGURE_DEPENDS ./src/main.cpp ./src/*.hpp)
file (GLOB_RECURSE CHESS_TEST CONFIGURE_DEPENDS ./test/*.cpp ./test/*.hpp)

add_executable(chess ${CHESS})
add_executable(chess_test ${CHESS_TEST})
