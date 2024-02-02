#include "../src/TcpServer.h"
#include "../src/logger.h"
#include <memory>
#include <iostream>

using namespace std::placeholders;

using namespace stnl;

class EchoServer
{
public:
    EchoServer(const SockAddr& listenSockAddr): server_(listenSockAddr, "Echo-Server")
    {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2));
    }

    void start() { server_.start(); }

private:
    void onMessage(const TcpConnection::TcpConnectionPtr& conn, NetBuffer* buf)
    {
        std::string msg(buf->retrieveAllAsString());
        LOG_INFO << "recv data: " << msg;
        if (msg == "quit\r\n" || msg == "exit\r\n") {
            conn->send("bye.");
            conn->shutdown();
        }
        conn->send(msg);
    }

    void onConnection(const TcpConnection::TcpConnectionPtr& conn)
    {
        conn->send("welcome to echo-server.\n");
    }

private:
    TcpServer server_;
};


/**
 * usage: 
 * ./TcpServer_test 127.0.0.1 8808
 * telnet 127.0.0.1 8808
*/
int main(int argc, char* argv[])
{
    SockAddr listenSockAddr(argv[1], atoi(argv[2]));
    EchoServer server(listenSockAddr);
    server.start();
}