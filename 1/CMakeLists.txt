cmake_minimum_required(VERSION 3.10)
project(lab1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_FLOAT "Use float instead of double" OFF)

add_executable(main main.cpp)

if (USE_FLOAT)
    target_compile_definitions(main PRIVATE USE_FLOAT)
endif()

