/**
 * @file EventLoop.h
 * @author lianghu (coolhuhu@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2024-01-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef STNL_EVENTLOOP_H
#define STNL_EVENTLOOP_H

#include <memory>
#include <vector>
#include <atomic>

namespace stnl
{

    class Channel;
    class Selector;

    /**
     * Reactor, one loop per thread.
     *
     */
    class EventLoop
    {
    public:

        EventLoop();

        ~EventLoop();

        void loop();

        void updateChannel(Channel*);

    private:
        using ChannelVector = std::vector<Channel*>;

        const int tid_;     // EventLoop所在线程的唯一标识
        std::unique_ptr<Selector> selector_;

        std::atomic_bool looping_;
        std::atomic_bool running_;
    };

}

#endif