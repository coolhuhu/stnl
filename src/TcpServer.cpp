#include "TcpServer.h"
#include <fcntl.h>
#include <stdio.h>
#include <cassert>
#include <unistd.h>

using namespace stnl;
using namespace std::placeholders;

/* ------------------------------------------ Acceptor --------------------------------------------------- */

Acceptor::Acceptor(EventLoop *loop, const SockAddr &addr): loop_(loop), 
                                                           listenSocket_(SocketUtil::createNonblockSocket(addr.family())),
                                                           listenChannel_(loop, listenSocket_.fd()),
                                                           idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    // TODO: 设置socket选项，端口重用、地址重用
    // listenSocket_.setAddrReuse(true);
    // listenSocket_.setPortReuse(true);
    assert(idleFd_ >= 0);
    listenChannel_.setReadEventCallback(std::bind(&Acceptor::handleRead, this));
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
        // TcpServer中传入该回调函数
        if (newConnectionCallback_) {
            newConnectionCallback_(connect_fd, client_addr);
        }
        else {
            // 新连接没别处理的话就关闭
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

TcpServer::TcpServer(const SockAddr& listenAddr, std::string_view loopName, int threadNums)
                    : loop_(new EventLoop()),
                    threadPool_(new EventLoopThreadPool(loop_.get(), loopName, threadNums)),
                    acceptor_(new Acceptor(loop_.get(), listenAddr))
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnectionCallback, this, _1, _2));
}

TcpServer::~TcpServer()
{
    // TODO: 关闭所有连接
}


void TcpServer::newConnectionCallback(int socketFd, const SockAddr& peerAddr)
{

}