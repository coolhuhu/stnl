#include "stnl/Timer.h"

#include <iostream>
#include <string>

using namespace stnl;

EventLoop* g_loop = nullptr;

void print_msg(std::string msg)
{
    std::cout << msg << std::endl;
}

void cancleTimer(TimerId timerId)
{
    g_loop->cancelTimer(timerId);
    std::cout << "cancel a Timer, its id is: " << timerId.id() << std::endl;
}

void quitLoop()
{
    std::cout << "quit loop." << std::endl;
    g_loop->quit();
}

int main()
{
    print_msg("start.");

    {
        EventLoop loop;
        g_loop = &loop;
        {
            TimerId timerId_1 = loop.runAfter(2, std::bind(print_msg, std::string("timeout once after 2s")));
            TimerId timerId_2 = loop.runAfter(3, std::bind(print_msg, std::string("timeout once after 3s")));
            TimerId timerId_3 = loop.runAfter(3.5, std::bind(print_msg, std::string("timeout once after 3.5s")));
            TimerId timerId_4 = loop.runAfter(6, std::bind(print_msg, std::string("timeout once after 6s")));
            std::cout << timerId_1.id() << std::endl;
            std::cout << timerId_2.id() << std::endl;
            std::cout << timerId_3.id() << std::endl;
            std::cout << timerId_4.id() << std::endl;
        }
        TimerId timerId_5 = loop.runEvery(3, std::bind(print_msg, std::string("timeout every 3s")));
        std::cout << timerId_5.id() << std::endl;
        
        loop.runAfter(10, std::bind(print_msg, std::string("timeout once after 10s")));
        loop.runAfter(20, std::bind(cancleTimer, timerId_5));
        loop.runAfter(45, quitLoop);

        loop.loop();
    }

    print_msg("loop exit");
}