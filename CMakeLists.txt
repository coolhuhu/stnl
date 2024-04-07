cmake_minimum_required(VERSION 3.16)
project(stnl)
set(CMAKE_CXX_STANDARD 20)

# set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
# set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(EXEC_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIB_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR})

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/stnl/*.cpp)

set(STNL stnl)

add_compile_definitions(NDEBUG)

add_subdirectory(stnl)
add_subdirectory(tests)
add_subdirectory(examples)

