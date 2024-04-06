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
        using MessageCallback = std::function<void(const TcpConnectionPtr& conn, NetBuffer* buffer, Timestamp timestamp)>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr& conn)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr& conn)>;
        using WriteCompletionCallback = std::function<void(const TcpConnectionPtr& conn)>;


    public:
        TcpConnection(EventLoop* loop, int sockfd, const SockAddr& localAddr, const SockAddr& peerAddr);

        ~TcpConnection();

        EventLoop* getLoop() const { return loop_; }

        void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

        void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

        void setWriteCompleteCallback(const WriteCompletionCallback& cb) { writeCompletionCallback_ = cb; }

        /**
         * 当有新连接到来时调用该函数，并将socket上的读事件注册到epoll中
        */
        void connectionEstablish();
        
        void connectionDestory();

        void shutdown();

        void forceClose();

        void setSocketState(SocketState state) { socketState_ = state; }

        // FIXME: 使用 std::string, std::move()
        void send(const char* message, std::size_t len);
        void send(std::string_view message);

        // void enableRead();
        // void disableRead();

        void setTcpNoDelay(bool on);

        SockAddr& getLocalAddr()  { return localAddr_; }

        SockAddr& getPeerAddr()  { return peerAddr_; }

        bool isConnected() const { return socketState_ == SocketState::CONNECTED; }
        bool isDisconnected() const { return socketState_ == SocketState::DISCONNECTED; }


    private:
        // 处理已连接socket上的读、写、关闭和异常
        void handleRead(Timestamp);
        void handleWrite();
        void handleClose();
        void handleError();

        void sendInLoop(const char* message, std::size_t len);
        void sendInLoop(const std::string message);

        void shutdownInLoop();

        void forceCloseInLoop();

    private:
        EventLoop* loop_;
        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;
        SockAddr localAddr_;
        SockAddr peerAddr_;
        NetBuffer inputBuffer_;
        NetBuffer outputBuffer_;
        SocketState socketState_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        CloseCallback closeCallback_;
        WriteCompletionCallback writeCompletionCallback_;
    };
    
    
}

#endif