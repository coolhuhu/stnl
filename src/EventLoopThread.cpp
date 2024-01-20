#include "EventLoopThread.h"
#include "logger.h"

namespace stnl
{
    EventLoopThread::EventLoopThread(std::string_view threadName) : loop_(nullptr), mutex_(), cv_(),
                                                                    thread_(threadName, std::bind(&EventLoopThread::threadFunc, this))
    {
    }

    EventLoopThread::~EventLoopThread()
    {
        if (loop_ != nullptr)
        {
            loop_->quit();
            thread_.join();
        }
    }

    EventLoop *EventLoopThread::startLoop()
    {
        thread_.start();
        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> locker(mutex_);
            cv_.wait(locker, [&]()
                     { return loop_ != nullptr; });
            loop = loop_;
        }
        
        return loop;
    }

    void EventLoopThread::threadFunc()
    {
        EventLoop loop;

        {
            std::unique_lock<std::mutex> locker(mutex_);
            loop_ = &loop;
            cv_.notify_one();
        }

        loop.loop();

        loop_ = nullptr;
    }
}