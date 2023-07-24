cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_COMPILER "D:/msys64/mingw64/bin/g++.exe")
project(simpleJSON LANGUAGES CXX)
if (CMAKE_BUILD_TYPE STREQUAL Release)
    message("[INFO]Disable LOG...")
    add_definitions(-DNDEBUG)
else()
    message("[INFO]Enable LOG...")
endif()

add_library(simpleJSON simpleJSON.cpp converter.cpp)
add_executable(test test.cpp)
target_link_libraries(test PUBLIC simpleJSON)