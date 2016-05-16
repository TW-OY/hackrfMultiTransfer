cmake_minimum_required(VERSION 3.5)
project(hackrfMultiTransfer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_subdirectory( src )
aux_source_directory(. DIR_SRCS)
add_executable(main ${DIR_SRCS})
target_link_libraries(main Mod)

set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/modules ${CMAKE_SOURCE_DIR}/cmake/modules)

