/**
 * @file TcpServer.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef STNL_TCPSERVER_H
#define STNL_TCPSERVER_H

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <memory>

namespace stnl
{

    /**
     * 对服务端的监听socket的相关操作进行封装。
    */
    class Acceptor : public noncopyable
    {
    public:
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
        using NewConnectionCallback = std::function<void(int socketFd, const SockAddr&)>;
        NewConnectionCallback newConnectionCallback_;

        Socket listenSocket_;
        Channel listenChannel_;
        EventLoop* loop_;

        /**
         * 空闲的文件描述符，用于处理并发连接导致文件描述符已达上限的问题
        */
        int idleFd_;
        
    };

    class TcpServer : public noncopyable
    {
    private:

    };

}

#endif