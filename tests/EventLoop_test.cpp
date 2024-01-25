#include "../src/EventLoop.h"
#include "../src/logger.h"
#include <iostream>

using namespace stnl;

EventLoop *g_loop;

void callback_func(std::string msg)
{
    std::cout << "callback_func()..." << std::endl;
    std::cout << "msg: " << msg << std::endl;
    std::cout << std::endl;
}

void test_only_one_thread()
{
    std::cout << "current thread id = " << std::this_thread::get_id() << std::endl;

    // 线程安全测试
    assert(EventLoop::getEventLoopOfCurrentThread() == nullptr);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    assert(loop.isInLoopThread());
    loop.assertInLoopThread();

    loop.runInLoop(std::bind(callback_func, "test_only_one_thread"));
    loop.runInLoop(std::function<void()>());
}

void threadFunc()
{
    assert(EventLoop::getEventLoopOfCurrentThread() == nullptr);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    assert(loop.isInLoopThread());
    loop.assertInLoopThread();

    // 将 loop 传递给外部线程
    g_loop = &loop;
    loop.loop();
}

int main()
{
    std::cout << "run test..." << std::endl;
    std::cout << "-------------------------------" << std::endl;
    std::shared_ptr<ConsoleHandler> consoleLogger = std::make_shared<ConsoleHandler>("Console Logger", LogLevel::DEBUG);
    Logger::instance().addHandler(consoleLogger);

    std::cout << "main thread id = " << std::this_thread::get_id() << std::endl;

    std::cout << "test_only_one_thread start..." << std::endl;
    test_only_one_thread();
    std::cout << "test_only_one_thread finish..." << std::endl;
    std::cout << std::endl;

    std::cout << "test_multithread start..." << std::endl;

    Thread t("EventLoopInMultiThread", threadFunc);
    t.start();
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 此时 g_loop 已经拿到子线程传递出来的 EventLoop 对象
    // 在非EventLoop对象创建所在的线程执行 runInLoop
    for (int i = 1; i <= 5; ++i) {
        g_loop->runInLoop(std::bind(callback_func, std::to_string(i)));
    }

    g_loop->quit();

    t.join();
    std::cout << "test_multithread finish..." << std::endl;
}