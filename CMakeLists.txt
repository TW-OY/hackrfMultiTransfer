cmake_minimum_required(VERSION 3.5)
project(hackrfMultiTransfer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_SOURCE_DIR .)
set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_SOURCE_DIR}/cmake/modules)

aux_source_directory(./src SRC)
add_executable(tetst ${SRC})
target_link_libraries(tetst /usr/local/Cellar/hackrf/2015.07.2/lib -lhackrf)
