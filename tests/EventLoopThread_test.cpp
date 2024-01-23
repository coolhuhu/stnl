#include "../src/EventLoopThread.h"
#include <iostream>

using namespace stnl;

void threadFunc()
{
    std::cout << "threadFunc run in thread id: " << std::this_thread::get_id() << std::endl;
}



int main()
{
    std::cout << "main thread id: " << std::this_thread::get_id() << std::endl;
    // 在主线程执行 threadFunc
    threadFunc();

    EventLoopThread loopThread("EventLoopThreadTest");
    EventLoop* loop = loopThread.startLoop();

    // 将 threadFunc 传入到IO线程所在的线程中执行
    loop->runInLoop(threadFunc);
    std::this_thread::sleep_for(std::chrono::seconds(10));

    loop->quit();
}