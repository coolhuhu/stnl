cmake_minimum_required(VERSION 3.16)
project(stnl)
set(CMAKE_CXX_STANDARD 20)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_subdirectory(src)
add_subdirectory(tests)

