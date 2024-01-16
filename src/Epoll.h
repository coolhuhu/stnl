/**
 * @file Epoll.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "Selector.h"
#include <sys/epoll.h>


namespace stnl
{
    /**
     * 封装 epoll
    */
    class Epoll: public Selector
    {
    public:
        Epoll(EventLoop* loop);

        ~Epoll() override;

        void select(ChannelVector& activeChannels, int timeout) override;

        void updateChannel(Channel* channel) override;

        void update(Channel* channel, int operation);

        static const int EPOLL_TIMEOUT = 5000;     // 5s

    private:
        using EventVector = std::vector<struct epoll_event>;

        EventVector events_;
        const int epollFd_;        

        static const int InitEventVectorSize = 16;

    };
}