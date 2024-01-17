#ifndef STNL_EVENTLOOPTHREADPOOL_H
#define STNL_EVENTLOOPTHREADPOOL_H


#include "noncopyable.h"
#include <vector>
#include <memory>
#include "EventLoopThread.h"


namespace stnl
{

    class EventLoopThreadPool: public noncopyable
    {
    public:
        EventLoopThreadPool(EventLoop* mainLoop, std::string_view name, int threadNums=0);

        ~EventLoopThreadPool();

        EventLoop* getNextLoop();

        void start();

    private:
        std::string name_;
        EventLoop* mainLoop_;
        int threadNums_;
        std::vector<EventLoop*> loops_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        int nextLoopIndex_;
    };


}



#endif