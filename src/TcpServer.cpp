#include "TcpServer.h"
#include "TcpConnection.h"
#include <fcntl.h>
#include <stdio.h>
#include <cassert>
#include <unistd.h>

using namespace stnl;
using namespace std::placeholders;

/* ------------------------------------------ Acceptor --------------------------------------------------- */

Acceptor::Acceptor(EventLoop *loop, const SockAddr& addr): loop_(loop), 
                                                           listenSocket_(SocketUtil::createNonblockSocket(addr.family())),
                                                           listenChannel_(loop, listenSocket_.fd()),
                                                           idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    listenChannel_.setReadEventCallback(std::bind(&Acceptor::handleRead, this));
    listenSocket_.setAddrReuse(true);
    listenSocket_.setPortReuse(true);
    listenSocket_.bindAddress(addr);
}

Acceptor::~Acceptor()
{
    /**
     * Acceptor类需要管理文件描述的生命周期。
     * 1. 取消loop对listenSocket_上事件的关注
     * 2. 将与listenSocket_绑定的Channel从loop中移除
     * 3. 关闭该文件描述符
    */
   listenChannel_.disableAll();
   listenChannel_.remove();
   ::close(idleFd_);
}

/**
 * listen系统调用，使listenSocket_处于可监听的状态，
 * 然后将 listenSocket_ 的可读事件注册到loop中
*/
void Acceptor::listen()
{
    listenSocket_.listen();
    listenChannel_.enableRead();
    
    // listenChannel_ 的可读事件在Acceptor初始化时被设置
    // 可读事件的回调函数为 Acceptor::handleRead
}

/**
 * listenSocket_ 可读事件发生的回调函数，即有新连接到来时进行的回调操作。
 * 
*/
void Acceptor::handleRead()
{
    SockAddr client_addr;
    int connect_fd = listenSocket_.accept(client_addr);
    if (connect_fd >= 0) {
        /**
         * 新连接建立的回调函数，在TcpServer初始化被设置。
         * 为新连接创建一个TcpConnection对象，用来管理这个连接。
         */
        if (newConnectionCallback_) {
            newConnectionCallback_(connect_fd, client_addr);
        }
        else {
            // 新连接没别的处理的话就直接关闭
            SocketUtil::closeSocket(connect_fd);
        }
    }
    else {
        // FIXME：不是线程安全的
        if (errno == EMFILE) {
            ::close(idleFd_);
            idleFd_ = ::accept(listenSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}


/* -------------------------------------- TcpServer ---------------------------------------- */

TcpServer::TcpServer(const SockAddr& listenAddr, 
                    std::string_view name, 
                    int threadNums)
                    : loop_(new EventLoop()),
                    threadPool_(new EventLoopThreadPool(loop_.get(), name, threadNums)),
                    acceptor_(new Acceptor(loop_.get(), listenAddr)),
                    name_(name)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnectionCallback, this, _1, _2));
}

TcpServer::~TcpServer()
{
    // TODO: 关闭所有连接
    loop_->assertInLoopThread();

    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectionDestory, conn));
    }
}

void TcpServer::start()
{
    threadPool_->start();
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    loop_->loop();
}

/**
 * 处理一个新连接的步骤如下：
 * 1.根据新连接的 sock_fd 创建一个 TcpConnection 对象，
 *   用于管理该连接，并将其加入到 ConnectionMap 中。
 * 2.为新连接设置相应事件的回调函数，读、写事件
*/
void TcpServer::newConnectionCallback(int socketFd, SockAddr peerAddr)
{
    // 主线程负责处理
    loop_->assertInLoopThread();

    EventLoop* ioLoop = threadPool_->getNextLoop();
    SockAddr localAddr(SocketUtil::getLocalAddr(socketFd));
    TcpConnectionPtr conn(new TcpConnection(ioLoop, socketFd, localAddr, peerAddr));
    
    // 要保证生成的 connections_ 的 key 是唯一的
    char buf[64];
    snprintf(buf, sizeof buf, "-%s:%d", peerAddr.ip_str().c_str(), peerAddr.port());
    connections_.emplace(name_ + buf, conn);

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompletionCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectionEstablish, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    // size_t n = connections_.erase(conn->getSockFd());
    /**
     * 主线程管理TcpConnection, 建立连接和释放连接都由主线程管理。
    */
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectionDestory, conn));
}