cmake_minimum_required(VERSION 3.28)

project(chess CXX)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src)

file (GLOB_RECURSE CHESS CONFIGURE_DEPENDS ./src/main.cpp ./src/*.hpp)
file (GLOB_RECURSE CHESS_TEST CONFIGURE_DEPENDS ./test/*.cpp ./test/*.hpp)

add_compile_options(-march=native)
add_compile_options(-Wall)
# add_compile_options(-Wfunction-effects)

# add_compile_options(-fprofile-generate)
# add_link_options(-fprofile-generate)

add_compile_options(-fprofile-use=foo)
add_link_options(-fprofile-use=foo)

add_executable(chess ${CHESS})

add_executable(chess_test ${CHESS_TEST})

# & 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-profdata.exe' merge -output=foo *.profraw

