#include "EventLoopThreadPool.h"

stnl::EventLoopThreadPool::EventLoopThreadPool(EventLoop *mainLoop, std::string_view name, int threadNums)
    : mainLoop_(mainLoop),
      name_(name),
      threadNums_(threadNums),
      nextLoopIndex_(0)
{
}

stnl::EventLoopThreadPool::~EventLoopThreadPool()
{
}

/**
 * mainLoop 从子 loops 选择一个 loop。简单的轮询。
 * TODO: 根据每个子loop所在的线程的负载情况，分配负载最小的loop进行服务。
 */
stnl::EventLoop *stnl::EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = mainLoop_;

    if (!loops_.empty())
    {
        loop = loops_[nextLoopIndex_];
        ++nextLoopIndex_;
        if (static_cast<size_t>(nextLoopIndex_) >= loops_.size())
        {
            nextLoopIndex_ = 0;
        }
    }

    return loop;
}

void stnl::EventLoopThreadPool::start()
{
    for (int i = 0; i < threadNums_; ++i)
    {
        char threadName[name_.size() + 36];
        snprintf(threadName, sizeof(threadName), "%s - %d", name_.c_str(), i);
        EventLoopThread *thread = new EventLoopThread(threadName);
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(thread));
        loops_.emplace_back(thread->startLoop());
    }
}
