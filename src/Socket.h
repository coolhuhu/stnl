/**
 * @file Socket.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-17
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#ifndef STNL_SOCKET_H
#define STNL_SOCKET_H

#include <netinet/in.h>
#include <string_view>
#include <string>
#include "noncopyable.h"


namespace stnl
{
    class SockAddr;

    class SocketUtil
    {
    public:
        static void bindAddress(int socketFd, struct sockaddr* addr);

        static void listenSocket(int socketFd);

        static int acceptSocket(int socketFd, struct sockaddr* addr);

        static int connectSocket(int socketFd, const struct sockaddr* addr);

        static void closeSocket(int socketFd);

        static int createSocket(int domain, int type, int protocol);

        static int createNonblockSocket(int domain);

    };

    class SockAddr
    {
    public:
        SockAddr(): ipv6_(false) {}
        
        explicit SockAddr(std::string_view ip_str, uint16_t port, bool ipv6=false);
        
        explicit SockAddr(const struct sockaddr_in& addr): addr_(addr), ipv6_(false) {}

        explicit SockAddr(const struct sockaddr_in6& addr6): addr6_(addr6), ipv6_(true) {}

        ~SockAddr() = default;

        std::string ip_str();

        std::uint16_t port();

        bool is_ipv6() const { return ipv6_; }

        const struct sockaddr* getSockAddr() const;

        void setSockAddr6(const struct sockaddr_in6);

        void setSockAddr4(const struct sockaddr_in);

        sa_family_t family() const;
        

    private:
        /**
         * 让程序库同时兼容ipv4和ipv6的最佳方案是什么？
         * struct sockaddr_storage or union {struct sockaddr_in; struct sockaddr_in6;}
        */
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
        bool ipv6_;
    };


    class Socket: public noncopyable
    {   
    public:
        explicit Socket(int socketFd): socketFd_(socketFd) {}

        ~Socket() { SocketUtil::closeSocket(socketFd_); }

        int fd() const { return socketFd_; }

        void bindAddress(const SockAddr& sockaddr);

        void listen();

        int accept(SockAddr& addr);

        void shutdownWrite();

        void setAddrReuse(bool on);

        void setKeepAlive(bool on);

        void setPortReuse(bool on);

        /**
         * enable/disable Nagle's algorithm
        */
        void setTcpNoDelay(bool on);

    private:
        const int socketFd_;
    };

}


#endif