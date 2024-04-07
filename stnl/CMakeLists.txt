cmake_minimum_required(VERSION 3.16)
project(stnl_src)
set(CMAKE_CXX_STANDARD 20)



set(LIBRARY_OUTPUT_PATH ${LIB_PATH})
add_library(${STNL}_static STATIC ${SRC})
add_library(${STNL}_shared SHARED ${SRC})


# 修改生成的静态库和动态库的名字
set_target_properties(${STNL}_static PROPERTIES OUTPUT_NAME ${STNL})
set_target_properties(${STNL}_shared PROPERTIES OUTPUT_NAME ${STNL})