

#ifndef STNL_Channel_H
#define STNL_Channel_H

#include <functional>
#include "EventLoop.h"

namespace stnl
{

    /**
     * 管理文件描述符上事件的类,设置Selector关注的事件类型，事件发生后的回调函数等
     * 在生命周期内，拥有唯一不变的文件描述符。
     * EventLoop和Channel的关系为一对多。
     */
    class Channel
    {
    public:
        enum class EventState {
            NEW,
            ADDED,
            DELETED
        };

        using WriteEventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void()>;
        using ErrorEventCallback = std::function<void()>;
        using CloseEventCallback = std::function<void()>;

        Channel(EventLoop* loop, int fd);

        ~Channel();

        const int fd() const { return fd_; }

        void setWriteEventCallback(const WriteEventCallback &cb) { writeEventCallback_ = std::move(cb); }
        void setReadEventCallback(const ReadEventCallback &cb) { readEventCallback_ = std::move(cb); }
        void setErrorEventCallback(const ErrorEventCallback &cb) { errorEventCallback_ = std::move(cb); }
        void setCloseEventCallback(const CloseEventCallback &cb) { closeEventCallback_ = std::move(cb); }

        /**
         * 处理 fd_ 上已发生的事件。
         */
        void handleEvents();

        void enableRead()
        {
            requestedEvents_ |= kReadEvent;
            update();
        }

        void disableRead()
        {
            requestedEvents_ &= ~kReadEvent;
            update();
        }

        void enableWrite()
        {
            requestedEvents_ |= kWriteEvent;
            update();
        }

        void disableWrite()
        {
            requestedEvents_ &= ~kWriteEvent;
            update();
        }

        void disableAll()
        {
            requestedEvents_ = kNoneEvent;
            update();
        }

        void remove()
        {
            loop_->removeChannel(this);
        }

        int events() const { return requestedEvents_; }

        // 例如：当epoll_wait返回时，通过调用channel的void setReturnedEvent(int type)该方法来设置产生的事件类型
        void setReturnedEvent(int type) { returnedEvents_ = type; }

        EventState getEventState() { return eventState_; }

        void setEventState(EventState state) { eventState_ = state; }

        bool isNoEvent() const { return requestedEvents_ == kNoneEvent; }

        bool writeable() const { return requestedEvents_ & kWriteEvent; }

        bool readable() const { return requestedEvents_ & kReadEvent; }
        

    private:
        void update();


    private:
        EventLoop *loop_;
        const int fd_;

        int requestedEvents_; // 关注的事件类型
        int returnedEvents_;  // 发生的事件类型

        EventState eventState_; 

        WriteEventCallback writeEventCallback_;
        ReadEventCallback readEventCallback_;
        ErrorEventCallback errorEventCallback_;
        CloseEventCallback closeEventCallback_;

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;
    };


}   /* end namespace */

#endif