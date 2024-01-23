#include "Timer.h"

#include <unistd.h>
#include <sys/timerfd.h>
#include <cassert>
#include <sys/timerfd.h>
#include <cstring>

namespace stnl
{

    int createTimerfd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        if (timerfd < 0) {
            // FIXME: error handle
        }
        return timerfd;
    }

    void readTimerfd(int timerfd)
    {
        uint64_t many;
        ssize_t n = ::read(timerfd, &many, sizeof(many));
        if (n != sizeof(many)) {
            // FIXME: error handle
        }
    }

    std::atomic_int64_t Timer::timerCount_ = 0;

    Timer::Timer(TimerCallback cb, Timestamp when, Seconds interval)
                : callback_(std::move(cb)), expiration_(when), 
                  interval_(interval), repeat_(interval_ > 0.0),
                  id_(++timerCount_)
    {
    }

    void Timer::restart(Timestamp& when)
    {
        if (repeat_) {
            expiration_ = addTime(when, interval_);
        }
        else {
            expiration_ = Timestamp::invalid();
        }
    }

    /* --------------------------------- TimerQueue ------------------------------------ */

    TimerQueue::TimerQueue(EventLoop* loop): loop_(loop),
                                             timerfd_(createTimerfd()),
                                             timerChannle_(loop_, timerfd_)
    {
        timerChannle_.setReadEventCallback(std::bind(&TimerQueue::timerReadingCallback, this));
        timerChannle_.enableRead();
    }

    TimerQueue::~TimerQueue()
    {
        timerChannle_.disableAll();
        timerChannle_.remove();
        ::close(timerfd_);

        // 把 timers_ 中的所有定时器删除
        timers_.clear();    // 因为使用unique_ptr管理Timer，可以不需要显示调用 
    }

    /**
     * TODO: 这个函数务必通过测试
    */
    TimerQueue::TimerVector TimerQueue::getAllExpirationTimer(Timestamp when)
    {
        TimerVector expirationTimers;
        auto iter = timers_.upper_bound(when);
        assert(iter == timers_.end() || when < iter->first);
        std::transform(timers_.begin(), iter, std::back_inserter(expirationTimers),
                       [](auto& e) {
                            return std::move(e.second);
                       });

        // 一定要移除已到期的定时器
        timers_.erase(timers_.begin(), iter);
        return std::move(expirationTimers);
    }

    void TimerQueue::resetExpirationTimers(TimerVector& expirationTimers, Timestamp when)
    {
        for (auto& timer : expirationTimers) {
            // 该定时器为周期性定时器，修改过期时间，重新添加到 timers_ 中
            if (timer->repeat()) {
                timer->restart(when);
                insert(timer);
            }
            else {
                // delete expired timer
                // unique_ptr automatically destruct
            }
        }

        Timestamp nextExpiration;
        if (!timers_.empty()) {
            nextExpiration = timers_.begin()->second->expiration();
        }

        // FIXME: Check whether the value is reasonable.
        resetTimerfd(nextExpiration);
    }

    void TimerQueue::timerReadingCallback()
    {
        loop_->assertInLoopThread();
        Timestamp now_time = Timestamp::now();
        readTimerfd(timerfd_);

        /**
         * TODO: 定时器到期，需要依次做哪些事情？
         * 1. 获取超时的所有定时器
         * 2. 执行超时的所有定时器对应的回调函数
         * 3. 重新设置下一个定时器的超时时间 nextExpiration
         * 4. 若超时的定时器中存在周期性定时，则 nextExpire 的设置会受此影响
        */

        TimerVector expirationTimers = getAllExpirationTimer(now_time);

        for (const auto& iter : expirationTimers) {
            iter->run();
        }

        resetExpirationTimers(expirationTimers, now_time);
    }

    void TimerQueue::insert(TimerCallback& cb, Timestamp& when, Seconds& interval)
    {
        // 在IO线程中执行
        std::unique_ptr<Timer> timer = std::make_unique<Timer>(cb, when, interval);
        TimerId id = timer->id();

        // FIXME: error. timer will be destruct!!!
        // NOTE: must be std::move(std::bind(...))
        loop_->runInLoop(std::move(std::bind(&TimerQueue::insertInLoop, this, std::move(timer))));
        
    }

    void TimerQueue::resetTimerfd(Timestamp expiration)
    {
        struct itimerspec new_val;
        // struct itimerspec old_val;
        bzero(&new_val, sizeof(new_val));
        new_val.it_value = intervalFromNow(expiration);
        int ret = ::timerfd_settime(timerfd_, 0, &new_val, nullptr);
        if (ret != 0) {
            // FIXME: error handle
        }
    }

    void TimerQueue::insertInLoop(std::unique_ptr<Timer> timer)
    {
        loop_->assertInLoopThread();

        // std::unique_ptr<Timer> timer = std::make_unique<Timer>(cb, when, interval);
        Timestamp when = timer->expiration();
        // NOTE: 此时timer已经被移动
        bool earliestTimeout = insert(timer);
        
        if (earliestTimeout) {
            resetTimerfd(when);
        }
    }

    bool TimerQueue::insert(std::unique_ptr<Timer>& timer)
    {
        // loop_->assertInLoopThread();
        bool earliestTimeout = false;
        Timestamp expiration = timer->expiration();
        auto iter = timers_.begin();

        // 新添加的定时器的超时时间最小
        if (iter == timers_.end() || expiration < iter->first) {
            earliestTimeout = true;
        }

        auto res = timers_.emplace(expiration, std::move(timer));
        assert(res->second);

        return earliestTimeout;
    }

}