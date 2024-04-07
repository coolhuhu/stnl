#ifndef STNL_TCPCLIENT_H
#define STNL_TCPCLIENT_H

#include "TcpConnection.h"

namespace stnl
{
    class Timestamp;
    class Connector;

    void defaultConnectionCallback(const TcpConnection::TcpConnectionPtr &conn);

    void defaultMessageCallback(const TcpConnection::TcpConnectionPtr &conn, NetBuffer* buffer, Timestamp timestamp);

    
    class TcpClient : public noncopyable
    {
    public:
        using ConnectorPtr = std::shared_ptr<Connector>;
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using WriteCompletionCallback = std::function<void(const TcpConnectionPtr &)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr &conn, NetBuffer *buffer, Timestamp timestamp)>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr &)>;

        TcpClient(EventLoop* loop,
                  const SockAddr &addr,
                  const std::string &name);

        ~TcpClient();

        void connect();

        void disconnect();

        void stop();

        TcpConnectionPtr connection() const
        {
            std::unique_lock<std::mutex> locker(mutex_);
            return connection_;
        }

        EventLoop *getLoop() const { return loop_; }
        bool retry() const { return retry_; }
        void enableRetry() { retry_ = true; }

        const std::string &name() const
        {
            return name_;
        }

        void setConnectionCallback(ConnectionCallback cb)
        {
            connectionCallback_ = std::move(cb);
        }

        void setMessageCallback(MessageCallback cb)
        {
            messageCallback_ = std::move(cb);
        }

        void setWriteCompleteCallback(WriteCompletionCallback cb)
        {
            writeCompletionCallback_ = std::move(cb);
        }

    private:
        void newConnection(int socketFd);

        void removeConnection(const TcpConnectionPtr &conn);

    private:
        EventLoop* loop_;
        ConnectorPtr connector_;
        const std::string name_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompletionCallback writeCompletionCallback_;
        std::atomic<bool> retry_;
        std::atomic<bool> connect_;
        int nextConnId_;
        mutable std::mutex mutex_;
        TcpConnectionPtr connection_;
    };

}

#endif // STNL_TCPCLIENT_H