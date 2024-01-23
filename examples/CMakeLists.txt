cmake_minimum_required(VERSION 3.16)
project(stnl_tests)
set(CMAKE_CXX_STANDARD 20)

set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})

# link_directories(${LIB_PATH})

# 使用CMake父节点中设置的变量名作为生成的可执行程序的名称
add_executable(timerfd timerfd.cpp)
# target_link_libraries(timerfd ${STNL} pthread boost_timer)

# # 设置可执行程序的生成路径为当前构建目录
# set_target_properties(timerfd PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     )