#include "Channel.h"
#include <sys/poll.h>
#include "EventLoop.h"


namespace stnl
{
    const int Channel::kNoneEvent = 0;
    const int Channel::kReadEvent = POLLIN | POLLPRI;
    const int Channel::kWriteEvent = POLLOUT;

    Channel::Channel(EventLoop* loop, int fd): loop_(loop), fd_(fd), eventState_(EventState::NEW),
                                               requestedEvents_(0), returnedEvents_(0)
    {
    }

    Channel::~Channel() {}


    /**
     * 根据 activeEventType 值来判断处理哪种类型的事件。
     * 这里对于poll和epoll，大多数的事件类型的宏定义的值是相同的，这里使用了poll中对应的宏定义。
     * TODO: 一种更好的做法是，自定义一个类来封装poll和epoll的底层事件类型，做到对外统一，待实现。
    */
    void Channel::handleEvents()
    {
        if (returnedEvents_ & POLLOUT) {
            if (writeEventCallback_) {
                writeEventCallback_();
            }
        }

        if (returnedEvents_ & POLLNVAL) {
            // POLLNVAL - Invalid request: fd not open (only returned in revents; ignored in events).
        }

        if (returnedEvents_ & POLLERR) {
            if (errorEventCallback_) {
                errorEventCallback_();
            }
        }

        if (returnedEvents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
            if (readEventCallback_) {
                readEventCallback_();
            }
        }

        if (returnedEvents_ & POLLHUP && !(returnedEvents_ & POLLIN)) {
            if (closeEventCallback_) {
                closeEventCallback_();
            }
        }
    }

    void Channel::update()
    {
        loop_->updateChannel(this);
    }
}