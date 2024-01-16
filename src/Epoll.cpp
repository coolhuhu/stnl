/**
 * @file Epoll.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-01-14
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/poll.h>
#include "Channel.h"
#include "Epoll.h"
#include <unistd.h>
#include <cassert>
#include <cstring>
#include "logger.h"

namespace stnl
{
    static_assert(EPOLLIN == POLLIN);
    static_assert(EPOLLPRI == POLLPRI);
    static_assert(EPOLLOUT == POLLOUT);
    static_assert(EPOLLRDHUP == POLLRDHUP);
    static_assert(EPOLLERR == POLLERR);
    static_assert(EPOLLHUP == POLLHUP);


    class EventLoop;

    Epoll::Epoll(EventLoop* loop): Selector(loop), 
                                   epollFd_(epoll_create1(EPOLL_CLOEXEC)),
                                   events_(InitEventVectorSize)
    {
        if (epollFd_ < 0) {
            // TODO: error
        }
    }

    Epoll::~Epoll() {
        close(epollFd_);
    }

    void Epoll::select(ChannelVector& activeChannels, int timeout)
    {
        int returnedEventsNum = epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeout);
        LOG_DEBUG << "epoll_wait once...";
        if (returnedEventsNum < 0) {
            // FIXME: error
        }
        else if (returnedEventsNum == 0) {
            // no events happening, maybe timeout
        }
        else {
            // 取出发生的事件
            assert(static_cast<size_t>(returnedEventsNum) <= events_.size());
            for (int i = 0; i < returnedEventsNum; ++i) {
                Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

            #ifndef NDEBUG
                int fd = channel->fd();
                auto iter = channelMap_.find(fd);
                assert(iter != channelMap_.end());
                assert(iter->second == channel);
            #endif
            
                channel->setReturnedEvent(events_[i].events);
                activeChannels.emplace_back(channel);
            }

            /**
             * 当前用户态中用于接收事件的数组太小,不能一次性从内核态拷贝到用户态,把数组大小扩大一倍
            */
            if (static_cast<size_t>(returnedEventsNum) == events_.size()) {
                events_.resize(events_.size() * 2);
            }
        }
    }

    /**
     * 根据channel中的watchedEventType_成员变量的值，将对应的事件添加到epoll中
    */
    void Epoll::updateChannel(Channel *channel)
    {
        Channel::EventState currentEventState = channel->getEventState();
        int fd = channel->fd();
        if (currentEventState == Channel::EventState::NEW) {
            assert(channelMap_.find(fd) == channelMap_.end());
            channelMap_[fd] = channel;
            channel->setEventState(Channel::EventState::ADDED);
            update(channel, EPOLL_CTL_ADD);
            
        }
        else if (currentEventState == Channel::EventState::ADDED) {
            assert(channelMap_.find(fd) != channelMap_.end());
            assert(channelMap_[fd] == channel);
            if (channel->isNoEvent()) {
                channel->setEventState(Channel::EventState::DELETED);
                update(channel, EPOLL_CTL_DEL);
            }
            else {
                update(channel, EPOLL_CTL_MOD);
            }
        }
        else {
            // currentEventState == EventState::DELETED
            assert(channelMap_.find(fd) != channelMap_.end());
            assert(channelMap_[fd] == channel);
            channel->setEventState(Channel::EventState::ADDED);
            update(channel, EPOLL_CTL_ADD);
        }
    }

    void Epoll::update(Channel *channel, int operation)
    {
        struct epoll_event event;
        bzero(&event, sizeof(event));
        event.events = channel->events();
        event.data.ptr = channel;       // NOTE: channel的生命周期问题
        int fd = channel->fd();

        // logging operation

        if (::epoll_ctl(epollFd_, operation, fd, &event) < 0) {
            // FIXME: epoll_ctl error
            
        }

    }
}