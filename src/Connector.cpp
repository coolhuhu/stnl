#include "Connector.h"
#include "EventLoop.h"
#include "logger.h"
#include "Channel.h"
#include "Timer.h"

#include <cassert>
#include <cerrno>
#include <cstring>

using namespace stnl;

const int Connector::RetryDelayMs = 500;
const int Connector::MaxRetryDelayMs = 30000;

Connector::Connector(EventLoop *loop,
                     const SockAddr &serverAddr) : loop_(loop),
                                                   serverAddr_(serverAddr),
                                                   connect_(false),
                                                   state_(SocketState::DISCONNECTED),
                                                   retryDelayMs_(RetryDelayMs)

{
}

Connector::~Connector()
{
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(SocketState::DISCONNECTED);
    retryDelayMs_ = RetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == SocketState::DISCONNECTED);
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == SocketState::CONNECTING)
    {
        setState(SocketState::DISCONNECTED);
        int socketfd = removeAndResetChannel();
        retry(socketfd);
    }
}

void Connector::connect()
{
    int sockfd = SocketUtil::createNonblockSocket(serverAddr_.family());
    int ret = SocketUtil::connectSocket(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH: /* 网络不可达，间隔重试 */
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK: /* 尝试在一个不是 socket 文件描述符上执行 socket 相关操作 */
        LOG_ERROR << "connect error in Connector::startInLoop " << savedErrno;
        SocketUtil::closeSocket(sockfd);
        break;

    default:
        LOG_ERROR << "Unexpected error in Connector::startInLoop " << savedErrno;
        SocketUtil::closeSocket(sockfd);
        // connectErrorCallback_();
        break;
    }
}

void Connector::connecting(int sockfd)
{
    setState(SocketState::CONNECTING);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteEventCallback(
        std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
    channel_->setErrorEventCallback(
        std::bind(&Connector::handleError, this)); // FIXME: unsafe

    channel_->enableWrite();
}

void Connector::handleWrite()
{
    if (state_ == SocketState::CONNECTING)
    {
        int socketfd = removeAndResetChannel();
        int err = SocketUtil::getSocketError(socketfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err;
            retry(socketfd);
        }
        else if (SocketUtil::isSelfConnect(socketfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(socketfd);
        }
        else
        {
            setState(SocketState::CONNECTED);
            if (connect_)
            {
                newConnectionCallback_(socketfd);
            }
            else
            {
                SocketUtil::closeSocket(socketfd);
            }
        }
    }
    else
    {
        assert(state_ == SocketState::DISCONNECTED);
    }
}

void Connector::handleError()
{
    // LOG_ERROR << "Connector::handleError = " << state_;
    if (state_ == SocketState::CONNECTING)
    {
        int socketfd = removeAndResetChannel();
        int err = SocketUtil::getSocketError(socketfd);
        LOG_TRACE << "SO_ERROR = " << err;
        retry(socketfd);
    }
}

void Connector::retry(int sockfd)
{
    SocketUtil::closeSocket(sockfd);
    setState(SocketState::DISCONNECTED);
    if (connect_)
    {
        LOG_DEBUG << "Connector::retry - Retry connecting to " << serverAddr_.port()
                  << " in " << retryDelayMs_ << " milliseconds. ";

        loop_->runAfter(retryDelayMs_ / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, MaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int socketfd = channel_->fd();
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return socketfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}
