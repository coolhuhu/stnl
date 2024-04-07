#include "TcpConnection.h"
#include "logger.h"
#include <limits.h>
#include <unistd.h>
#include "TimeUtil.h"

using namespace stnl;
using namespace std::placeholders;

TcpConnection::TcpConnection(EventLoop *loop, int sockfd,
                             const SockAddr &localAddr,
                             const SockAddr &peerAddr)
    : loop_(loop),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      socketState_(SocketState::CONNECTING)
{
    channel_->setReadEventCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setCloseEventCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setWriteEventCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorEventCallback(std::bind(&TcpConnection::handleError, this));
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::connectionEstablish()
{
    assert(socketState_ == SocketState::CONNECTING);
    socketState_ = SocketState::CONNECTED;
    channel_->enableRead();

    if (connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectionDestory()
{
    if (socketState_ == SocketState::CONNECTED)
    {
        setSocketState(SocketState::DISCONNECTED);
        channel_->disableAll();
        // connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::send(const char *message, std::size_t len)
{
    if (socketState_ == SocketState::CONNECTED)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message, len);
        }
        else
        {
            void (TcpConnection::*fp)(const std::string_view message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, std::string(message, len)));
        }
    }
}

void TcpConnection::send(std::string_view message)
{
    send(message.data(), message.size());
}

void TcpConnection::send(NetBuffer *buf)
{
    if (socketState_ == SocketState::CONNECTED)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const std::string_view message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp,
                          this, 
                          buf->retrieveAllAsString()));
            
        }
    }
}

void TcpConnection::sendInLoop(const char *message, std::size_t len)
{
    assert(loop_->isInLoopThread());

    if (socketState_ == SocketState::DISCONNECTED)
    {
        // 断开连接
        return;
    }

    std::size_t remaining = len, written = 0;
    if (channel_->writeable() && outputBuffer_.readableBytes() == 0)
    {
        /**
         * outputBuffer_中没有数据，说明上一次一次性把outputBuffer_中的数据发送了出去。
         * 直接调用 write 发送数据，尝试一次性把数据发送出去。
         */
        written = ::write(channel_->fd(), message, len);
        if (written >= 0)
        {
            remaining = len - written;
            if (remaining == 0 && writeCompletionCallback_)
            {
                // 数据发送完成
                // loop_->queueInLoop(std::bind(writeCompletionCallback_, shared_from_this()));
                writeCompletionCallback_(shared_from_this());
            }
        }
        else // written < 0, error
        {
            written = 0;
            // FIXME: error handle
            // 可能是什么错误导致写错误，函数开始位置已经判断了 socket 是否可读。
            // 可能会在写的过程中，对方关闭了连接吗？
        }
    }

    if (remaining > 0)
    {
        // 一次 write 调用不能把数据全部发完，把未发出去的数据拷贝到 outputBuffer_ 中。
        // 思考：可能是什么原因导致了一次 write 调用未能把数据全部发送出去。
        outputBuffer_.append(message + written, remaining);
        if (!channel_->writeable())
        {
            channel_->enableWrite();
        }
    }
}

void TcpConnection::sendInLoop(const std::string_view message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFD(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        if (messageCallback_)
        {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
    }
    else if (n == 0)
    {
        // 文件描述符关闭
        handleClose();
    }
    else
    {
        errno = savedErrno;
        // logger
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->writeable())
    {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWrite();
            }
            /**
             * TODO: else, 一次write未能将outputBuffer_中的数据全部写入文件描述符中的缓冲区
             */

            if (socketState_ == SocketState::DISCONNECTING)
            {
                // NOTE:
                shutdownInLoop();
            }
        }
        else
        {
            // logger: handleWrite eror
        }
    }
    else
    {
        // TODO:
    }
}

void TcpConnection::handleClose()
{
    assert(socketState_ == SocketState::CONNECTED || socketState_ == SocketState::DISCONNECTING);
    setSocketState(SocketState::DISCONNECTED);
    channel_->disableAll();

    /**
     * TcpConnection由shared_ptr管理生命周期。
     * 这里创建一个临时变量，让TcpConnectionPtr的引用计数+1，
     * 延长期生命周期，保证closeCallback_能够被安全地执行完毕。
     */
    TcpConnectionPtr guardThis(shared_from_this());
    // connectionCallback_(guardThis);
    // closeCallback_ --> TcpConnection::connectionDestory()
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    // 如何处理异常
    int error = SocketUtil::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << peerAddr_.ip_str()
              << ":" << peerAddr_.port()
              << "] - SO_ERROR = " << error << " " << strerror(error);
}

void TcpConnection::shutdown()
{
    if (socketState_ == SocketState::CONNECTED)
    {
        socketState_ = SocketState::DISCONNECTING;
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::forceClose()
{
    if (socketState_ == SocketState::CONNECTED || socketState_ == SocketState::DISCONNECTING)
    {
        setSocketState(SocketState::DISCONNECTING);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{
    assert(loop_->isInLoopThread());
    if (!channel_->writeable())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceCloseInLoop()
{
    if (socketState_ == SocketState::CONNECTED || socketState_ == SocketState::DISCONNECTING)
    {
        handleClose();
    }
}
