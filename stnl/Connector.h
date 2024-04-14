#ifndef STNL_CONNECTOR_H
#define STNL_CONNECTOR_H

#include <memory>
#include <functional>
#include <atomic>

#include "noncopyable.h"
#include "Socket.h"

namespace stnl
{
    class EventLoop;
    class Channel;

    class Connector : public noncopyable, public std::enable_shared_from_this<Connector>
    {
    public:
        using NewConnectionCallback = std::function<void(int sockfd)>;

        enum class SocketState
        {
            DISCONNECTED,
            CONNECTING,
            CONNECTED,
        };

        static const int RetryDelayMs;
        static const int MaxRetryDelayMs;

        Connector(EventLoop *, const SockAddr &);

        ~Connector();

        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_ = cb;
        }

        void start();

        void restart();

        void stop();

        SockAddr serverAddr() const { return serverAddr_; }

    private:
        void setState(SocketState s) { state_ = s; }

        void startInLoop();

        void stopInLoop();

        void connect();

        void connecting(int sockfd);

        void handleWrite();

        void handleError();

        void retry(int sockfd);

        int removeAndResetChannel();

        void resetChannel();

    private:
        EventLoop *loop_;
        SockAddr serverAddr_;
        std::unique_ptr<Channel> channel_;
        SocketState state_;
        NewConnectionCallback newConnectionCallback_;
        std::atomic<bool> connect_;
        int retryDelayMs_;
    };
}

#endif