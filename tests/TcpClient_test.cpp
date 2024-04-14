#include "stnl/TcpClient.h"
#include "stnl/logger.h"
#include "stnl/TimeUtil.h"

using namespace stnl;
using namespace std::placeholders;

int numThreads = 0;
class EchoClient;
std::vector<std::unique_ptr<EchoClient>> clients;
int current = 0;

class EchoClient
{
public:
    EchoClient(EventLoop* loop, SockAddr &serverAddr, const std::string &name)
        : client_(loop, serverAddr, name)
    {
        client_.setConnectionCallback(std::bind(&EchoClient::onConnection, this, _1));
        client_.setMessageCallback(std::bind(&EchoClient::onMessage, this, _1, _2, _3));
    }

    void connect()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpClient::TcpConnectionPtr &conn)
    {
        LOG_INFO << "EchoClient - " << conn->getLocalAddr().port() << " -> "
                 << conn->getPeerAddr().port() << " is "
                 << (conn->isConnected() ? "UP" : "DOWN");

        if (conn->isConnected())
        {
            ++current;
            if (current < clients.size())
            {
                clients[current]->connect();
            }
        }
    }

    void onMessage(const TcpClient::TcpConnectionPtr &conn, NetBuffer *buf, Timestamp receiveTime)
    {
        std::string msg(buf->retrieveAllAsString());
        LOG_INFO << " echo " << msg.size() << " bytes, "
                 << "data received at: " << msg;

        if (msg == "quit")
        {
            conn->send("bye\n");
            conn->shutdown();
        }
        else if (msg == "shutdown")
        {
            loop_->quit();
        }
        else
        {
            msg.clear();
            std::cin >> msg;
            conn->send(msg);
        }
    }

private:
    TcpClient client_;
    EventLoop *loop_;
};

/*
    ./TcpClient_test 127.0.0.1 8808
*/
int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        EventLoop loop;
        SockAddr serverAddr(argv[1], atoi(argv[2]));

        int n = 1;
        if (argc > 3)
        {
            n = atoi(argv[3]);
        }

        clients.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof buf, "%d", i + 1);
            clients.emplace_back(new EchoClient(&loop, serverAddr, buf));
        }

        clients[current]->connect();
        loop.loop();
    }
    else
    {
        printf("Usage: %s host_ip [current#]\n", argv[0]);
    }
}