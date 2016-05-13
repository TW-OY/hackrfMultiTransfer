cmake_minimum_required(VERSION 3.5)
project(hackrfMultiTransfer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES fmMod.cpp transfer.cpp wavOperator.cpp)
add_executable(hackrfMultiTransfer ${SOURCE_FILES})