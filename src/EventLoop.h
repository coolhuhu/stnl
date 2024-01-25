#ifndef STNL_EVENTLOOP_H
#define STNL_EVENTLOOP_H

#include <memory>
#include <vector>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>

namespace stnl
{

    class Channel;
    class Selector;
    class TimerId;
    class TimerQueue;
    class Timestamp;
    using TimerCallback = std::function<void()>;

    /**
     * Reactor, one loop per thread.
     *
     */
    class EventLoop
    {
    public:
        using Func = std::function<void()>;

        EventLoop();

        ~EventLoop();

        void loop();

        void quit();

        void runInLoop(Func func);

        void queueInLoop(Func func);

        void updateChannel(Channel*);

        void removeChannel(Channel*);

        static EventLoop* getEventLoopOfCurrentThread();

        bool isInLoopThread() const { return tid_ == std::this_thread::get_id(); }

        void assertInLoopThread()
        {
            if (!isInLoopThread()) {
                abortNotInLoopThread();
            }
        }

        TimerId runAt(Timestamp time, TimerCallback cb);

        TimerId runAfter(double delay, TimerCallback cb);

        TimerId runEvery(double interval, TimerCallback cb);

        void cancelTimer(TimerId timerId);

    private:
        void doPendingFunctions();

        /**
         * wakeupFd_上可读事件的回调函数。从wakeupFd_上读取数据。
        */
        void wakeupReadCallback();

        void abortNotInLoopThread();

        void wakeup();

    private:
        using ChannelVector = std::vector<Channel*>;

        const std::thread::id tid_;     // EventLoop所在线程的唯一标识
        std::unique_ptr<Selector> selector_;

        std::atomic_bool looping_;
        std::atomic_bool running_;

        std::unique_ptr<TimerQueue> timerQueue_;

        std::mutex mutex_;
        int wakeupFd_;
        std::unique_ptr<Channel> wakeupChannel_;
        std::vector<Func> pendingFunctions_;
        bool callingPendingFunctions_;
    };

}

#endif