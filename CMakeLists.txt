cmake_minimum_required(VERSION 3.10)
project(debugio CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#SET (CMAKE_VERBOSE_MAKEFILE 1)

option(BUILD_EXAMPLE "Build the example tree." ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
