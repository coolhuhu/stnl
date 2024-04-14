#include "stnl/Timer.h"
#include "stnl/logger.h"

#include <iostream>
#include <string>

using namespace stnl;

EventLoop *g_loop = nullptr;

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


/**
 * 测试 Timer 的基本功能
*/
void test1()
{
    print_msg("test1 start.");

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

    print_msg("loop exit.\n test1 end.");
}


void test2_func(double time)
{
    std::cout << "timeout once after " << time << "s." << std::endl;
}


void test2()
{
    print_msg("test2 start.");

    int startTimeMicroseconds = 500;
    int MaxRetryTimeMicroseconds = 30000; 

    EventLoop loop;

    auto f = [&]() {

        while (1) {
            TimerId tid = loop.runAfter(startTimeMicroseconds / 1000.0, 
                                     std::bind(test2_func, startTimeMicroseconds / 1000.0));

            startTimeMicroseconds = std::min(startTimeMicroseconds * 2, MaxRetryTimeMicroseconds);
            std::cout << "startTimeMicroseconds = " << startTimeMicroseconds << std::endl;

            if (startTimeMicroseconds >= MaxRetryTimeMicroseconds) {
                break;
            }
        }
    
        loop.quit();
    };

    loop.runAfter(3, f);

    loop.loop();
    
    print_msg("test2 end.");
}



/*
    测试，当定时器的设置的到期时间小于等于0时，定时器会失效。
*/
void test3()
{
    EventLoop loop;

    loop.runAfter(-1, [&]() {
        std::cout << "timeout once after 1s." << std::endl;
    });

    loop.loop();
}


int main()
{
    std::shared_ptr<ConsoleHandler> consoleHandlerPtr = std::make_shared<ConsoleHandler>("pingpong_client", LogLevel::DEBUG);
    Logger::instance().addHandler(consoleHandlerPtr);

    test3();
}