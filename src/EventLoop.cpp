#include "EventLoop.h"
#include <cassert>
#include <thread>

namespace stnl
{
    // FIXME: tid_初始化
    EventLoop::EventLoop():looping_(false), running_(false),
                           tid_(0), selector_()
    {
        
    }

    EventLoop::~EventLoop() {}


    void EventLoop::loop()
    {
        assert(!looping_);
        assert(!running_);
        looping_ = true;
        running_ = true;

        ChannelVector activeChannels;
        while (running_)
        {
            activeChannels.clear();
            
            // 1. epoll_wait(), 获取有事件发生的events
            selector_->select(activeChannels, 5);

            // 2. 执行 events 上注册的回调函数
            for (auto channel : activeChannels)
            {
                channel->handleEvents();
            }
        }

        looping_ = false;
    }

    void EventLoop::updateChannel(Channel *channel)
    {
        selector_->updateChannel(channel);
    }
}
