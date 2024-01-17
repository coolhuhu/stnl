/**
 * @file Selector.h
 * @author lianghu (coolhuhu@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2024-01-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef STNL_SELECTOR_H
#define STNL_SELECTOR_H

#include <map>
#include <vector>
#include "noncopyable.h"

namespace stnl
{

    class EventLoop;
    class Channel;

    /**
     * 抽象基类，为poll和epoll提供相同的接口。
     * 一个 EventLoop 对应一个 Selector, 一个 Selector 对应多个 Channel
     */
    class Selector: public noncopyable
    {
    public:
        using ChannelVector = std::vector<Channel*>;

        Selector(EventLoop* loop):loop_(loop) {}

        virtual ~Selector() = default;

        /**
         * TODO: 修改timeout的类型,使其更合理
        */
        virtual void select(ChannelVector& activeChannels, int timeout) = 0;

        virtual void updateChannel(Channel*) = 0;

        /**
         * 将指定的Channel从EventLoop中移除
        */
        virtual void removeChannel(Channel*) = 0;

    protected:
        // key为文件描述符, value为 EventManager管理的文件描述符
        using ChannelMap = std::map<int, Channel*>;
        ChannelMap channelMap_;

    private:
        EventLoop* loop_;
    };

}

#endif