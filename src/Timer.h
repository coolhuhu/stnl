#ifndef STNL_TIMER_H
#define STNL_TIMER_H

#include <functional>
#include <atomic>
#include <map>
#include <unordered_set>
#include <set>
#include "Channel.h"
#include "TimeUtil.h"


namespace stnl
{

    using Seconds = double;
    using TimerCallback = std::function<void()>;

    class EventLoop;
    class Channel;

    class Timer;
    class TimerQueue;

    class TimerId
    {
    public:
        friend class TimerQueue;

    public:
        TimerId(): id_(0), timer_(nullptr) {}

        TimerId(int64_t id, Timer* timer): id_(id), timer_(timer) {}

        TimerId(const TimerId& timerId): id_(timerId.id_), timer_(timerId.timer_) {}

        TimerId& operator=(const TimerId& timerId)
        {
            if (this != &timerId)
            {
                id_ = timerId.id_;
                timer_ = timerId.timer_;
            }
            return *this;
        }

        const int64_t id() const { return id_; }

        Timer* getTimer() const { return timer_; }

    private:
        int64_t id_;
        Timer* timer_;
    };


    inline bool operator==(const TimerId& lhs, const TimerId& rhs)
    {
        return (lhs.id() == rhs.id()) && (lhs.getTimer() == rhs.getTimer());
    }

    inline bool operator<(const TimerId& lhs, const TimerId& rhs)
    {
        if (lhs.id() == rhs.id()) {
            return lhs.getTimer() < rhs.getTimer();
        }
        return lhs.id() < rhs.id();
    }


    /**
     * 定时器类，对超时时间、定时周期以及定时回调函数进行封装。
     */
    class Timer
    {
    public:
        explicit Timer(TimerCallback cb, Timestamp when, Seconds interval);

        ~Timer() { --timerCount_; }

        const int64_t id() const { return id_.id(); }

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
        const TimerId id_; // 定时器唯一标识

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
        using ActiveTimer = std::set<TimerId>;

        explicit TimerQueue(EventLoop *loop);

        ~TimerQueue();

        TimerId insert(TimerCallback cb, Timestamp &when, Seconds interval);

        /**
         * 供外层组件使用，根据指定的 TimerId 删除
        */
        void cancel(TimerId timerId);

    private:
        TimerVector getAllExpirationTimer(Timestamp when);

        void resetExpirationTimers(TimerVector &expirationTimers, Timestamp when);

        void timerReadingCallback();

        void resetTimerfd(Timestamp expiration);

        void insertInLoop(Timer*);

        void cancelInLoop(TimerId timerId);

        /**
         * 供 insertInLoop 函数调用
         *
         * @param timer
         * @return true 说明新添加的定时器的超时时间最小
         * @return false
         */
        bool insert(std::unique_ptr<Timer> timer);


    private:
        EventLoop *loop_;
        int timerfd_;
        Channel timerChannle_;
        TimerMap timers_;
        ActiveTimer activeTimers_;
        std::set<Timer*> cancelingTimers_;
        std::atomic_bool callingExpiredTimers_;
    };

}

#endif