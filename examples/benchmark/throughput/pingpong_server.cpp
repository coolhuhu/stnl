#include "stnl/TcpServer.h"
#include "stnl/Buffer.h"
#include "stnl/TimeUtil.h"
#include "stnl/logger.h"

#include <iostream>

using namespace stnl;
using namespace std::placeholders;


class PingPongServer {
public:
    PingPongServer(const SockAddr& serverAddr): 
                   server_(serverAddr, "PingPong-Server")
    {
        server_.setConnectionCallback(std::bind(&PingPongServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(&PingPongServer::onMessage, this, _1, _2, _3));
    }

    void start() { server_.start(); }

    void setThreadNums(int threadNum) { server_.setThreadNums(threadNum); }

private:
    void onConnection(const TcpConnection::TcpConnectionPtr& conn)
    {
        if (conn->isConnected()) {
            conn->setTcpNoDelay(true);
        }
    }

    void onMessage(const TcpConnection::TcpConnectionPtr& conn, NetBuffer* buf, Timestamp receiveTime) 
    {
        conn->send(buf);

    }

private:
    TcpServer server_;
};


int main(int argc, char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <ip> <port> <threads>\n", argv[0]);
        exit(1);
    }
    else {
        LOG_INFO << "PingPong Server...";

        SockAddr serverAddr(argv[1], atoi(argv[2]));
        int threadCount = atoi(argv[3]);

        PingPongServer server(serverAddr);

        if (threadCount > 1) {
            server.setThreadNums(threadCount);
        }

        server.start();
    }
}