

#include "Socket.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <cstring>
#include <unistd.h>
#include "logger.h"

using namespace stnl;

/* -------------------------------------- SocketUtil ------------------------------------------------*/

void SocketUtil::bindAddress(int socketFd, const struct sockaddr *addr)
{
    int res = ::bind(socketFd, addr, sizeof(struct sockaddr_in6));
    if (res < 0)
    {
        // FIXME: how to handle bind error.
        LOG_ERROR << "bind error.";
    }
}

void SocketUtil::listenSocket(int socketFd)
{
    int res = ::listen(socketFd, SOMAXCONN);
    // FIXME:
    if (res < 0)
    {
        LOG_ERROR << "listen error.";
    }
}

int SocketUtil::acceptSocket(int socketFd, struct sockaddr *addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));
    int fd = ::accept4(socketFd, addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (fd < 0)
    {
        // FIXME: 如何处理 accept 失败
        LOG_ERROR << "accept error.";
    }
    return fd;
}

int SocketUtil::connectSocket(int socketFd, const struct sockaddr *addr)
{
    int res = ::connect(socketFd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (res < 0)
    {
        // FIXME:
        LOG_ERROR << "connect error.";
    }
    return res;
}

void SocketUtil::closeSocket(int socketFd)
{
    if (::close(socketFd) < 0)
    {
        // FIXME:
        LOG_ERROR << "socket close error.";
    }
}

int SocketUtil::createSocket(int domain, int type, int protocol)
{
    int fd = ::socket(domain, type, protocol);
    if (fd < 0)
    {
        // FIXME:
        LOG_ERROR << "create a socket error.";
    }
    return fd;
}

int SocketUtil::createNonblockSocket(int domain)
{
    return createSocket(domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
}

void SocketUtil::shutdownWrite(int socketFd)
{
    if (::shutdown(socketFd, SHUT_WR) < 0)
    {
        // FIXME:
    }
}

void SocketUtil::setTcpNoDelay(int socketFd, bool on)
{
    int optval = on ? 1 : 0;
    if (::setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, 
        &optval, static_cast<socklen_t>(sizeof(optval))) < 0)
    {
        // FIXME:
    }
}

void SocketUtil::setAddrReuse(int socketFd, bool on)
{
    int optval = on ? 1 : 0;
    if (::setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, 
        &optval, static_cast<socklen_t>(sizeof(optval))) < 0)
    {
        // FIXME:
    }
}

void SocketUtil::setPortReuse(int socketFd, bool on)
{
    int optval = on ? 1 : 0;
    if (::setsockopt(socketFd, SOL_SOCKET, SO_REUSEPORT, 
        &optval, static_cast<socklen_t>(sizeof(optval))) < 0)
    {
        // FIXME:
    }
}

void SocketUtil::setKeepAlive(int socketFd, bool on)
{
    int optval = on ? 1 : 0;
    if (::setsockopt(socketFd, SOL_SOCKET, SO_KEEPALIVE, 
        &optval, static_cast<socklen_t>(sizeof(optval))) < 0)
    {
        // FIXME:
    }
}

SockAddr SocketUtil::getLocalAddr(int socketFd)
{
    // FIXME: 修改为 struct sockaddr_storage 处理 ipv4 和 ipv6
    // NOTE: 目前只能处理 ipv4
    struct sockaddr_in localAddr;
    bzero(&localAddr, sizeof(localAddr));
    socklen_t addrLen = static_cast<socklen_t>(sizeof(localAddr));
    if (::getsockname(socketFd, reinterpret_cast<struct sockaddr*>(&localAddr), &addrLen) < 0) {
        // FIXME: error handle
    }
    return SockAddr(localAddr);
}

int SocketUtil::getSocketError(int socketFd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
    if (::getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    }
    else {
        return optval;
    }
}

/* ----------------------------------------- SockAddr ----------------------------------------*/

SockAddr::SockAddr(std::string_view ip_str, uint16_t port, bool ipv6) : ipv6_(ipv6)
{
    if (ipv6)
    {
        bzero(&addr6_, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        int res = inet_pton(AF_INET6, ip_str.data(), &addr6_.sin6_addr);
        if (res == 0)
        {
            // FIXME: invalid network address
            LOG_DEBUG << "invalid network address";
        }

        addr6_.sin6_port = htons(port);
    }
    else
    {
        bzero(&addr_, sizeof(addr_));
        addr_.sin_family = AF_INET;
        int res = inet_pton(AF_INET, ip_str.data(), &addr_.sin_addr);
        if (res == 0)
        {
            // FIXME: invalid network address
            LOG_DEBUG << "invalid network address";
        }
        addr_.sin_port = htons(port);
    }
}

std::string SockAddr::ip_str()
{
    char buf[64];
    if (ipv6_)
    {
        inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof(buf));
    }
    else
    {
        inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    }
    return buf;
}

std::uint16_t SockAddr::port()
{
    uint16_t port = 0;
    if (ipv6_)
    {
        port = ntohs(addr6_.sin6_port);
    }
    else
    {
        port = ntohs(addr6_.sin6_port);
    }
    return port;
}

const struct sockaddr *SockAddr::getSockAddr() const
{
    if (ipv6_)
    {
        return static_cast<const struct sockaddr*>(static_cast<const void*>(&addr6_));
    }
    else
    {
        return static_cast<const struct sockaddr*>(static_cast<const void*>(&addr_));
    }
}

// struct sockaddr *SockAddr::getSockAddr()
// {
//     if (ipv6_)
//     {
//         return static_cast<struct sockaddr*>(static_cast<void*>(&addr6_));
//     }
//     else
//     {
//         return static_cast<struct sockaddr*>(static_cast<void*>(&addr_));
//     }
// }

void SockAddr::setSockAddr6(const sockaddr_in6 addr6)
{
    addr6_ = addr6;
    ipv6_ = true;
}

void SockAddr::setSockAddr4(const sockaddr_in addr)
{
    addr_ = addr;
    ipv6_ = false;
}

sa_family_t stnl::SockAddr::family() const
{
    if (ipv6_)
    {
        return addr6_.sin6_family;
    }
    else
    {
        return addr_.sin_family;
    }
}

/* --------------------------------------- Socket ------------------------------------- */

void Socket::bindAddress(const SockAddr& sockaddr)
{
    SocketUtil::bindAddress(socketFd_, sockaddr.getSockAddr());
}

void Socket::listen()
{
    SocketUtil::listenSocket(socketFd_);
}

int Socket::accept(SockAddr &addr)
{

    struct sockaddr_in6 addr6;
    bzero(&addr6, sizeof(addr6));
    int connect_fd = SocketUtil::acceptSocket(socketFd_, static_cast<struct sockaddr*>(static_cast<void*>(&addr6)));
    if (connect_fd >= 0)
    {
        addr.setSockAddr6(addr6);
    }
    return connect_fd;
}

void Socket::shutdownWrite()
{
    SocketUtil::shutdownWrite(socketFd_);
}

void Socket::setAddrReuse(bool on)
{
    SocketUtil::setAddrReuse(socketFd_, on);
}

void Socket::setKeepAlive(bool on)
{
    SocketUtil::setKeepAlive(socketFd_, on);
}

void Socket::setPortReuse(bool on)
{
    SocketUtil::setPortReuse(socketFd_, on);
}

void Socket::setTcpNoDelay(bool on)
{
    SocketUtil::setTcpNoDelay(socketFd_, on);
}
