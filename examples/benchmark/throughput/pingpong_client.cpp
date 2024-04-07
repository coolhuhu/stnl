#include "stnl/TcpClient.h"
#include "stnl/logger.h"
#include "stnl/TimeUtil.h"
#include "stnl/Timer.h"
#include "stnl/EventLoopThreadPool.h"

#include <string>
#include <iostream>

using namespace std;
using namespace stnl;
using namespace std::placeholders;

class Client;

class Session : noncopyable
{
public:
    Session(EventLoop *loop,
            const SockAddr &serverAddr,
            const string &name,
            Client *owner)
        : client_(loop, serverAddr, name),
          owner_(owner),
          bytesRead_(0),
          bytesWritten_(0),
          messagesRead_(0)
    {
        client_.setConnectionCallback(
            std::bind(&Session::onConnection, this, _1));
        client_.setMessageCallback(
            std::bind(&Session::onMessage, this, _1, _2, _3));
    }

    void start()
    {
        client_.connect();
    }

    void stop()
    {
        client_.disconnect();
    }

    int64_t bytesRead() const
    {
        return bytesRead_;
    }

    int64_t messagesRead() const
    {
        return messagesRead_;
    }

private:
    void onConnection(const TcpConnection::TcpConnectionPtr &conn);

    void onMessage(const TcpConnection::TcpConnectionPtr &conn, NetBuffer *buf, Timestamp receiveTime)
    {
        ++messagesRead_;
        bytesRead_ += buf->readableBytes();
        bytesWritten_ += buf->readableBytes();
        conn->send(buf);
    }

    TcpClient client_;
    Client *owner_;
    int64_t bytesRead_;
    int64_t bytesWritten_;
    int64_t messagesRead_;
};

class Client : noncopyable
{
public:
    Client(EventLoop *loop,
           const SockAddr &serverAddr,
           int blockSize,
           int sessionCount,
           int timeout,
           int threadCount)
        : loop_(loop),
          threadPool_(loop, "pingpong-client"),
          sessionCount_(sessionCount),
          timeout_(timeout)
    {
        loop_->runAfter(timeout, std::bind(&Client::handleTimeout, this));
        if (threadCount > 1)
        {
            threadPool_.setThreadNums(threadCount);
        }
        threadPool_.start();

        for (int i = 0; i < blockSize; ++i)
        {
            message_.push_back(static_cast<char>(i % 128));
        }

        for (int i = 0; i < sessionCount; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof buf, "C%05d", i);
            Session *session = new Session(threadPool_.getNextLoop(), serverAddr, buf, this);
            session->start();
            sessions_.emplace_back(session);
        }
    }

    const string &message() const
    {
        return message_;
    }

    void onConnect()
    {
        if (numConnected_.fetch_add(1) == sessionCount_)
        {
            LOG_INFO << "all connected";
        }
    }

    void onDisconnect(const TcpConnection::TcpConnectionPtr &conn)
    {
        if (numConnected_.fetch_sub(1) == 0)
        {
            LOG_INFO << "all disconnected";

            int64_t totalBytesRead = 0;
            int64_t totalMessagesRead = 0;
            for (const auto &session : sessions_)
            {
                totalBytesRead += session->bytesRead();
                totalMessagesRead += session->messagesRead();
            }
            LOG_INFO << totalBytesRead << " total bytes read";
            LOG_INFO << totalMessagesRead << " total messages read";
            LOG_INFO << static_cast<double>(totalBytesRead) / static_cast<double>(totalMessagesRead)
                     << " average message size";
            LOG_INFO << static_cast<double>(totalBytesRead) / (timeout_ * 1024 * 1024)
                     << " MiB/s throughput";
            conn->getLoop()->queueInLoop(std::bind(&Client::quit, this));
        }
    }

private:
    void quit()
    {
        loop_->queueInLoop(std::bind(&EventLoop::quit, loop_));
    }

    void handleTimeout()
    {
        LOG_INFO << "stop";
        for (auto &session : sessions_)
        {
            session->stop();
        }
    }

    EventLoop *loop_;
    EventLoopThreadPool threadPool_;
    int sessionCount_;
    int timeout_;
    std::vector<std::unique_ptr<Session>> sessions_;
    string message_;
    std::atomic_int32_t numConnected_;
};

void Session::onConnection(const TcpConnection::TcpConnectionPtr &conn)
{
    if (conn->isConnected())
    {
        conn->setTcpNoDelay(true);
        conn->send(owner_->message());
        owner_->onConnect();
    }
    else
    {
        owner_->onDisconnect(conn);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "Usage: client <host_ip> <port> <threads> <blocksize> ");
        fprintf(stderr, "<sessions> <time>\n");
    }
    else
    {
        LOG_INFO << "client start...";
        

        const char *ip = argv[1];
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        int threadCount = atoi(argv[3]);
        int blockSize = atoi(argv[4]);
        int sessionCount = atoi(argv[5]);
        int timeout = atoi(argv[6]);

        EventLoop loop;
        SockAddr serverAddr(ip, port);

        Client client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
        loop.loop();
    }
}