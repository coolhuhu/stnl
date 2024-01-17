#ifndef STNL_EVENTLOOPTHREAD_H
#define STNL_EVENTLOOPTHREAD_H

#include "noncopyable.h"
#include "Thread.h"
#include "EventLoop.h"

#include <mutex>
#include <condition_variable>

namespace stnl
{
    /**
     * one loop per thread.
    */
    class EventLoopThread: public noncopyable
    {
    public:
        EventLoopThread(std::string_view threadName);
        
        ~EventLoopThread();

        EventLoop* startLoop();

    private:
        void threadFunc();

    private:
        Thread thread_;
        EventLoop* loop_;
        std::mutex mutex_;
        std::condition_variable_any cv_;
    };

}






#endif