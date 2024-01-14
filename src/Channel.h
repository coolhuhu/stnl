/**
 * @file Channel.h
 * @author lianghu (coolhuhu@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2024-01-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef STNL_Channel_H
#define STNL_Channel_H

#include <functional>

namespace stnl
{

    class EventLoop;

    /**
     * 管理文件描述符上事件的类,设置Selector关注的事件类型，事件发生后的回调函数等
     * 在生命周期内，拥有唯一不变的文件描述符。
     * EventLoop和Channel的关系为一对多。
     */
    class Channel
    {
    public:
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
            // TODO: 设置 watchedEvent_
            // watchedEvent_ |= 
            // 将 fd_ 上关注事件的更新更新到 Selector 中
            watchedEventType_ |= kReadEvent;
            update();
        }

        void disableRead()
        {
            // TODO: 设置 watchedEvent_
            // watchedEvent_ |= 
            // 将 fd_ 上关注事件的更新更新到 Selector 中
            watchedEventType_ &= ~kReadEvent;
            update();
        }

        void enableWrite()
        {
            // TODO: 设置 watchedEvent_
            // watchedEvent_ |= 
            // 将 fd_ 上关注事件的更新更新到 Selector 中
            watchedEventType_ |= kWriteEvent;
            update();
        }

        void disableWrite()
        {
            // TODO: 设置 watchedEvent_
            // watchedEvent_ |= 
            // 将 fd_ 上关注事件的更新更新到 Selector 中
            watchedEventType_ &= ~kWriteEvent;
            update();
        }

        int events() const { return activeEventType_; }
        

    private:
        void update();


    private:
        EventLoop *loop_;
        const int fd_;

        int watchedEventType_; // 关注的事件类型
        int activeEventType_;  // 发生的事件类型

        WriteEventCallback writeEventCallback_;
        ReadEventCallback readEventCallback_;
        ErrorEventCallback errorEventCallback_;
        CloseEventCallback closeEventCallback_;
    };


}   /* end namespace */

#endif