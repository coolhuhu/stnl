#include "TcpClient.h"
#include "Connector.h"
#include "logger.h"
#include "TimeUtil.h"
#include "TcpConnection.h"

using namespace stnl;
using namespace std::placeholders;


namespace stnl {
    void removeConnection(EventLoop* loop, const TcpConnection::TcpConnectionPtr& conn)
    {
        loop->queueInLoop(std::bind(&TcpConnection::connectionDestory, conn));
    }
}

void stnl::defaultConnectionCallback(const TcpConnection::TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->getLocalAddr().port() << " -> "
              << conn->getPeerAddr().port() << " is "
              << (conn->isConnected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void stnl::defaultMessageCallback(const TcpConnection::TcpConnectionPtr &conn, NetBuffer *buffer, Timestamp timestamp)
{
    buffer->retrieveAll();
}

TcpClient::TcpClient(EventLoop* loop, 
                     const SockAddr &addr,
                     const std::string &name) : loop_(loop),
                                                connector_(new Connector(loop_, addr)),
                                                name_(name),
                                                connectionCallback_(defaultConnectionCallback),
                                                messageCallback_(defaultMessageCallback),
                                                retry_(false),
                                                connect_(true),
                                                nextConnId_(1)
{
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, _1));
    // LOG_INFO;
}

TcpClient::~TcpClient()
{
    // LOG_INFO;
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::unique_lock<std::mutex> locker(mutex_);
        unique = connection_.use_count() == 1;
        conn = connection_;
    }
    if (conn)
    {
        assert(loop_ == conn->getLoop());
        // FIXME: not 100% safe, if we are in different thread
        CloseCallback cb = std::bind(&stnl::removeConnection, loop_, _1);
        loop_->runInLoop(
            std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique)
        {
            conn->forceClose();
        }
    }
    else
    {
        connector_->stop();
        // FIXME: HACK
        // loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void TcpClient::connect()
{
    // LOG_INFO;
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    // LOG_INFO;
    connect_ = false;
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}


void TcpClient::stop()
{
    // LOG_INFO;
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int socketFd)
{
    SockAddr peerAddr(SocketUtil::getPeerAddr(socketFd));

    SockAddr localAddr(SocketUtil::getLocalAddr(socketFd));
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            socketFd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompletionCallback_);
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection, this, _1));

    {
        std::unique_lock<std::mutex> locker(mutex_);
        connection_ = conn;
    }
    conn->connectionEstablish();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::unique_lock<std::mutex> locker(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectionDestory, conn));
    if (retry_ && connect_)
    {
        // LOG_INFO;
        connector_->restart();
    }
}
