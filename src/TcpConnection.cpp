#include "TcpConnection.h"
#include <limits.h>
#include <unistd.h>

using namespace stnl;

TcpConnection::TcpConnection()
{
    channel_->setReadEventCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setCloseEventCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setWriteEventCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorEventCallback(std::bind(&TcpConnection::handleError, this));
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
        socketState_ == SocketState::DISCONNECTED;
        channel_->disableAll();
    }
    channel_->remove();
}

void TcpConnection::handleRead()
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFD(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        if (messageCallback_)
        {
            messageCallback_(shared_from_this(), &inputBuffer_);
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

            if (socketState_ == SocketState::DISCONNECTING) {
                // TODO:
            }
        }
        else {
            // logger: handleWrite eror
        }
    }
    else {
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
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    // 如何处理异常

}