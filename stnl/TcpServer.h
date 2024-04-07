
#ifndef STNL_TCPSERVER_H
#define STNL_TCPSERVER_H

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include <memory>
#include <map>

namespace stnl
{

    class TcpConnection;
    class NetBuffer;
    class Timestamp;

    /**
     * 对服务端的监听socket的相关操作进行封装。
    */
    class Acceptor : public noncopyable
    {
    public:
        using NewConnectionCallback = std::function<void(int socketFd, SockAddr)>;

        Acceptor(EventLoop* loop, const SockAddr& addr);

        ~Acceptor();

        void listen();

        void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    private:
        void handleRead();
    
    private:
        /**
         * 当一个新连接到来时的回调函数。
         * 在要给Tcp Server中，一般的做法为：将新连接的socket上的相关事件注册到epoll中。
         * 在 muduo 的实现中，newConnectionCallback_ 在TcpServer中实现并被设置
        */
        
        NewConnectionCallback newConnectionCallback_;

        Socket listenSocket_;
        Channel listenChannel_;
        EventLoop* loop_;

        /**
         * 空闲的文件描述符，用于处理并发连接导致文件描述符已达上限的问题
        */
        int idleFd_;
        
    };

    /**
     * TcpServer类表示一个完整的Tcp服务，应该具有以下成员：
     * 1. 包含一个EventLoop对象，作为主loop。
     * 2. 若支持多线程，一个EventLoopThreadPool，作为子loop。
     * 3. TcpConnection对象集合，一个TcpConnection对象表示一个Tcp连接
     * 4. 一个Acceptor对象，表示Tcp服务端的监听socket
     * 5. 各事件产生后的回调函数对象
     * 
    */
    class TcpServer : public noncopyable
    {
    public:
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

        TcpServer(const SockAddr& listenAddr, 
                  std::string_view name="Tcp-Server",
                  int threadNums=0);

        ~TcpServer();

        void setConnectionCallback(const TcpConnection::ConnectionCallback& cb) { connectionCallback_ = cb; }
        void setMessageCallback(const TcpConnection::MessageCallback& cb) { messageCallback_ = cb; }
        void setWriteCompletionCallback(const TcpConnection::WriteCompletionCallback& cb) { writeCompletionCallback_ = cb; }

        void start();

        void setThreadNums(int threadNums) {
            threadPool_->setThreadNums(threadNums);
        }

    private:
        /**
         * 新连接到来时的回调函数，传入Acceptor中，acceptor_->setNewConnectionCallback();
         * 
         * @param socketFd 对端文件描述符
         * @param peerAddr 对端SockAddr
         */
        void newConnectionCallback(int socketFd, SockAddr peerAddr);

        void removeConnection(const TcpConnectionPtr& conn);
        void removeConnectionInLoop(const TcpConnectionPtr& conn);


    private:
        std::unique_ptr<EventLoop> loop_;
        std::unique_ptr<EventLoopThreadPool> threadPool_;
        std::unique_ptr<Acceptor> acceptor_;
        ConnectionMap connections_;
        std::string name_;
        TcpConnection::ConnectionCallback connectionCallback_;
        TcpConnection::MessageCallback messageCallback_;
        TcpConnection::WriteCompletionCallback writeCompletionCallback_;
    };

}

#endif