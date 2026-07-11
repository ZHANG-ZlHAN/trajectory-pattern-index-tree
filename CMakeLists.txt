cmake_minimum_required(VERSION 3.16)

project(
    trajectory_pattern_index_tree
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    trajectory_demo
    src/main.cpp
)

target_include_directories(
    trajectory_demo
    PRIVATE src
)

target_compile_options(
    trajectory_demo
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall
        -Wextra
        -Wpedantic
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
    >
)
