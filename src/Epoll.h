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



    private:
        using EventVector = std::vector<struct epoll_event>;

        EventVector events_;
        const int eventFd_;        

        static const int InitEventVectorSize = 16;
    };
}