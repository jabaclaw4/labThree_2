cmake_minimum_required(VERSION 3.16)
project(labOne_tree CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(headers)

add_executable(labOne_tree
        src/main.cpp
        src/Tests.cpp
)