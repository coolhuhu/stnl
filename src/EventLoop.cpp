#include "EventLoop.h"
#include "Epoll.h"
#include "Channel.h"
#include <cassert>
#include <sys/eventfd.h>
#include <thread>
#include "logger.h"
#include <unistd.h>

namespace stnl
{
    thread_local EventLoop* loopInCurrentThread = nullptr;

    int createEventfd()
    {
        int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
        if (fd < 0) {
            LOG_FATAL << "eventfd*() error";
        }
        return fd;
    }

    // FIXME: tid_初始化
    EventLoop::EventLoop():looping_(false), running_(false),
                           tid_(std::this_thread::get_id()), 
                           selector_(new Epoll(this)),
                           wakeupFd_(createEventfd()),
                           wakeupChannel_(new Channel(this, wakeupFd_)),
                           callingPendingFunctions_(false)

    {
        LOG_DEBUG << "EventLoop created.";

        if (loopInCurrentThread) {
            // 一个线程中最多创建一个 EventLoop 对象
            LOG_FATAL << "A thread can create a maximum of one EventLoop object.";
        }
        else {
            loopInCurrentThread = this;
        }

        wakeupChannel_->setReadEventCallback(std::bind(&EventLoop::wakeupReadCallback, this));
        wakeupChannel_->enableRead();
    }

    EventLoop::~EventLoop() {
        wakeupChannel_->disableAll();
        wakeupChannel_->remove();
        ::close(wakeupFd_);
        loopInCurrentThread = nullptr;
    }

    EventLoop* EventLoop::getEventLoopOfCurrentThread()
    {
        return loopInCurrentThread;
    }

    void EventLoop::doPendingFunctions()
    {
        std::vector<Func> functions;
        callingPendingFunctions_ = true;

        {
            std::unique_lock<std::mutex> locker(mutex_);
            functions.swap(pendingFunctions_);
        }

        for (auto& func : functions) {
            func();
        }

        callingPendingFunctions_ = false;
    }

    void EventLoop::wakeupReadCallback()
    {
        uint64_t one = 1;
        ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
        if (n != sizeof(one)) {
            LOG_ERROR << "EventLoop::wakeupReadCallback() should read 8 bytes, not " << n << " bytes";
        }
    }

    void EventLoop::abortNotInLoopThread()
    {
        LOG_FATAL << "EventLoop::abortNotInLoopThread(), " 
                  << "EventLoop was created in thread id = " << tid_
                  << ", current thread id = " << std::this_thread::get_id();
    }

    void EventLoop::wakeup()
    {
        uint64_t one = 1;
        ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
        if (n != sizeof(one)) {
            LOG_ERROR << "EventLoop::wakeup() should write 8 bytes, not " << n << " bytes";
        }
    }

    void EventLoop::loop()
    {
        assert(!looping_);
        assert(!running_);
        looping_ = true;
        running_ = true;

        ChannelVector activeChannels;
        while (running_)
        {
            activeChannels.clear();
            
            // 1. epoll_wait(), 获取有事件发生的events
            selector_->select(activeChannels, Epoll::EPOLL_TIMEOUT);

            // 2. 执行 events 上注册的回调函数
            for (auto channel : activeChannels)
            {
                channel->handleEvents();
            }

            doPendingFunctions();
        }

        looping_ = false;
    }

    void EventLoop::quit()
    {
        running_ = false;

        // FIXME: 若在其他线程（loop被创建的线程）调用quit()，需要进行哪些处理。
        if (!isInLoopThread()) {
            wakeup();
        }
    }

    void EventLoop::runInLoop(Func func)
    {
        if (func) {
            if (isInLoopThread()) {
                func();
            }
            else {
                queueInLoop(std::move(func));
            }
        }
    }

    void EventLoop::queueInLoop(Func func)
    {
        {
            std::unique_lock<std::mutex> locker(mutex_);
            pendingFunctions_.emplace_back(std::move(func));
        }

        if (!isInLoopThread() || callingPendingFunctions_) {
            wakeup();
        }
    }

    void EventLoop::updateChannel(Channel *channel)
    {
        selector_->updateChannel(channel);
    }

    void EventLoop::removeChannel(Channel* channel)
    {
        // TODO:

        selector_->removeChannel(channel);
    }
}
