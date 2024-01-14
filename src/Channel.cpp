#include "Channel.h"
#include <sys/poll.h>


namespace stnl
{
    const int kNoneEvent = 0;
    const int kReadEvent = POLLIN | POLLPRI;
    const int kWriteEvent = POLLOUT;

    Channel::Channel(EventLoop* loop, int fd): loop_(loop), fd_(fd),
                                               watchedEventType_(0), activeEventType_(0)
    {
    }

    Channel::~Channel() {}


    void Channel::handleEvents()
    {
    }

    void Channel::update()
    {
        loop_->updateChannel(this);
    }
}