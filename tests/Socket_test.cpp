/**
 * @file Socket_test.cpp
 * @author lianghu (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#include <iostream>
#include "stnl/Socket.h"
#include "stnl/logger.h"

using namespace stnl;

void test_valid_SockAddr()
{
    SockAddr sockaddr("127.0.0.1", 8888);
    std::cout << "ip: " << sockaddr.ip_str() << std::endl;
    std::cout << "port: " << sockaddr.port() << std::endl;
}


void test_invalid_SockAddr_1()
{
    SockAddr sockaddr("256.22.23.11", 8888);
    std::cout << "ip: " << sockaddr.ip_str() << std::endl;
    std::cout << "port: " << sockaddr.port() << std::endl;
}

void test_invalid_SockAddr_2()
{
    SockAddr sockaddr("256.22,23.11", 8888);
    std::cout << "ip: " << sockaddr.ip_str() << std::endl;
    std::cout << "port: " << sockaddr.port() << std::endl;
}

void test_connectSocket()
{
    SockAddr serverAddr("127.0.0.1", 8888);
    int socketFd = SocketUtil::createNonblockSocket(serverAddr.family());
    SocketUtil::listenSocket(socketFd);
    int ret = SocketUtil::connectSocket(socketFd, serverAddr.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    std::cout << "saveErrno = " << savedErrno << std::endl;
    SocketUtil::closeSocket(socketFd);
}


int main()
{
    std::shared_ptr<ConsoleHandler> consoleLogger = std::make_shared<ConsoleHandler>("Console Logger", LogLevel::DEBUG);
    Logger::instance().addHandler(consoleLogger);
    
    test_valid_SockAddr();
    test_invalid_SockAddr_1();
    test_invalid_SockAddr_2();
    std::cout << "\n";

    test_connectSocket();
}