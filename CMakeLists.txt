cmake_minimum_required(VERSION 3.6)
project(NetNinnyCPP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    clientSide.cpp
    proxy.cpp
        serverSide.cpp serverSide.h proxy.h clientSide.h filter.cpp filter.h)

add_executable(NetNinnyCPP ${SOURCE_FILES})