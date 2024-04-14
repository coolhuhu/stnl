set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})

link_directories(${LIB_PATH})

add_executable(pingpong_server pingpong_server.cpp)
target_link_libraries(pingpong_server ${STNL} pthread)

add_executable(pingpong_client pingpong_client.cpp)
target_link_libraries(pingpong_client ${STNL} pthread)