cmake_minimum_required(VERSION 3.16)
project(stnl_tests)
set(CMAKE_CXX_STANDARD 20)

set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})

link_directories(${LIB_PATH})

# 使用CMake父节点中设置的变量名作为生成的可执行程序的名称
add_executable(logger_test logger_test.cpp)
target_link_libraries(logger_test ${STNL} pthread boost_timer)

add_executable(EventLoop_test EventLoop_test.cpp)
target_link_libraries(EventLoop_test ${STNL} pthread)

add_executable(Socket_test Socket_test.cpp)
target_link_libraries(Socket_test ${STNL} pthread)

add_executable(EventLoopThread_test EventLoopThread_test.cpp)
target_link_libraries(EventLoopThread_test ${STNL} pthread)

add_executable(TimeUtil_test TimeUtil_test.cpp)
target_link_libraries(TimeUtil_test ${STNL} pthread)

add_executable(Timer_test Timer_test.cpp)
target_link_libraries(Timer_test ${STNL} pthread)