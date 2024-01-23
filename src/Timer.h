#ifndef STNL_TIMER_H
#define STNL_TIMER_H

#include <functional>
#include "TimeUtil.h"
#include "Channel.h"
#include <atomic>
#include <map>

namespace stnl
{

    using Seconds = double;
    using TimerCallback = std::function<void()>;
    using TimerId = int64_t;

    /**
     * 定时器类，对超时时间、定时周期以及定时回调函数进行封装。
     */
    class Timer
    {
    public:
        Timer(TimerCallback cb, Timestamp when, Seconds interval);

        ~Timer() { --timerCount_; }

        const TimerId id() const { return id_; }

        void run()
        {
            if (callback_)
            {
                callback_();
            }
        }

        Timestamp expiration() const { return expiration_; }

        bool repeat() const { return repeat_; }

        static int64_t timerCount() { return timerCount_; }

        void restart(Timestamp &when);

    private:
        TimerCallback callback_;
        Timestamp expiration_;
        Seconds interval_; // 定时周期，单位为秒
        bool repeat_;
        TimerId id_; // 定时器唯一标识

        static std::atomic_int64_t timerCount_;
    };

    /**
     * 定时器管理类。
     * 要不要把这个类弄成抽象类，由实现类来实现具体的细节？？？
     */
    class TimerQueue
    {
    public:
        using TimerMap = std::multimap<Timestamp, std::unique_ptr<Timer>>;
        using TimerVector = std::vector<std::unique_ptr<Timer>>;

        explicit TimerQueue(EventLoop *loop);

        ~TimerQueue();

        void insert(TimerCallback &cb, Timestamp &when, Seconds &interval);

        /**
         * TODO:
         * how to cancel a timer?
        */
        // void cancel();

    private:
        TimerVector getAllExpirationTimer(Timestamp when);

        void resetExpirationTimers(TimerVector &expirationTimers, Timestamp when);

        void timerReadingCallback();

        void resetTimerfd(Timestamp expiration);

        void insertInLoop(std::unique_ptr<Timer> timer);

        /**
         * 供 insert(TimerCallback&, Timestamp&, Seconds&) 函数调用
         *
         * @param timer
         * @return true 说明新添加的定时器的超时时间最小
         * @return false
         */
        bool insert(std::unique_ptr<Timer> &timer);


    private:
        TimerMap timers_;
        int timerfd_;
        Channel timerChannle_;
        EventLoop *loop_;
    };

}

#endif