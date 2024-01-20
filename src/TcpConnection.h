#ifndef STNL_TCPCONNECTION_H
#define STNL_TCPCONNECTION_H


#include <memory>
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"


namespace stnl
{
    /**
     * 一个TcpConnection类对象表示一次TCP连接。（表示的是已连接的socket）
     * TcpConnection功能的核心就是数据的发送与接收。
     * 
    */
    class TcpConnection: public std::enable_shared_from_this<TcpConnection>
    {
    public:
        enum class SocketState
        {
            DISCONNECTED,
            CONNECTING,
            CONNECTED,
            DISCONNECTING
        };
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using MessageCallback = std::function<void(const TcpConnectionPtr& conn, NetBuffer* buffer)>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr& conn)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr& conn)>;

    public:
        TcpConnection();

        ~TcpConnection();

        void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

        void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

        /**
         * 当有新连接到来时调用该函数，并将socket上的读事件注册到epoll中
        */
        void connectionEstablish();
        
        /**
         * 
        */
        void connectionDestory();

        void setSocketState(SocketState state) { socketState_ = state; }

        void send(const char* message, int len);
        void send(std::string_view message);

        void enableRead();
        void disableRead();

        void setTcpNoDelay(bool on);


    private:
        // 处理已连接socket上的读、写、关闭和异常
        void handleRead();
        void handleWrite();
        void handleClose();
        void handleError();


    private:
        EventLoop* loop_;
        std::unique_ptr<Channel> channel_;
        std::unique_ptr<Socket> socket_;
        SockAddr localAddr;
        SockAddr peerAddr;
        NetBuffer inputBuffer_;
        NetBuffer outputBuffer_;
        SocketState socketState_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        CloseCallback closeCallback_;
    };

}

#endif