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


#include "Epoll.h"
#include <unistd.h>
#include <cassert>

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
                                   eventFd_(epoll_create1(EPOLL_CLOEXEC)),
                                   events_(InitEventVectorSize)
    {
        if (eventFd_ < 0) {
            // TODO: error
        }
    }

    Epoll::~Epoll() {
        close(eventFd_);
    }

    void Epoll::select(ChannelVector& activeChannels, int timeout)
    {
        int activeEventNum = epoll_wait(eventFd_, events_.data(), static_cast<int>(events_.size()), timeout);

        if (activeEventNum < 0) {
            // FIXME: error
        }
        else if (activeEventNum == 0) {
            // no events happening, maybe timeout
        }
        else {
            // 取出发生的事件
            assert(static_cast<size_t>(activeEventNum) <= events_.size());
            for (int i = 0; i < activeEventNum; ++i) {
                // TODO:
            }

            /**
             * 当前用户态中用于接收事件的数组太小,不能一次性从内核态拷贝到用户态,把数组大小扩大一倍
            */
            if (static_cast<size_t>(activeEventNum) == events_.size()) {
                events_.resize(events_.size() * 2);
            }
        }
    }

}