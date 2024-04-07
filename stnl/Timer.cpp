#include "Timer.h"
#include "logger.h"

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
                  id_(++timerCount_, this)
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
                                             timerChannle_(loop_, timerfd_),
                                             callingExpiredTimers_(false)
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

    void TimerQueue::cancel(TimerId timerId)
    {
        loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
    }

    void TimerQueue::cancelInLoop(TimerId timerId)
    {
        loop_->assertInLoopThread();
        assert(timers_.size() == activeTimers_.size());
        Timer *timer = timerId.getTimer();
        auto iter = activeTimers_.find(timerId);
        if (iter != activeTimers_.end())
        {
            auto range = timers_.equal_range(timer->expiration());
            auto ret = std::find_if(range.first, range.second, [&](auto& it){
                return it.second.get() == timer;
            });
            if (ret != range.second) {
                timers_.erase(ret);
            }
            activeTimers_.erase(timerId);
        }
        else if (callingExpiredTimers_) {
            cancelingTimers_.emplace(timer);
        }

        assert(timers_.size() == activeTimers_.size());
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

        // 从timers_和activeTimers中移除过期的定时器
        timers_.erase(timers_.begin(), iter);
        for (auto& timer: expirationTimers) {
            TimerId timerId(timer->id());
            auto iter = activeTimers_.find(timerId);
            if (iter != activeTimers_.end()) {
                activeTimers_.erase(timerId);
            }
            // else: error.
        }

        assert(timers_.size() == activeTimers_.size());
        return std::move(expirationTimers);
    }

    void TimerQueue::resetExpirationTimers(TimerVector& expirationTimers, Timestamp when)
    {
        for (auto& timer : expirationTimers) {
            // 该定时器为周期性定时器，修改过期时间，重新添加到 timers_ 中
            Timer* t = timer.get();
            if (timer->repeat() && cancelingTimers_.find(t) == cancelingTimers_.end()) {
                timer->restart(when);
                insert(std::move(timer));
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
         * 定时器到期，需要依次做哪些事情？
         * 1. 获取超时的所有定时器
         * 2. 执行超时的所有定时器对应的回调函数
         * 3. 重新设置下一个定时器的超时时间 nextExpiration
         * 4. 若超时的定时器中存在周期性定时，则 nextExpire 的设置会受此影响
        */

        TimerVector expirationTimers = getAllExpirationTimer(now_time);
        LOG_INFO << "expirationTimers.size() = " << expirationTimers.size();
        callingExpiredTimers_ = true;
        cancelingTimers_.clear();
        for (const auto& iter : expirationTimers) {
            iter->run();
        }
        callingExpiredTimers_ = false;

        resetExpirationTimers(expirationTimers, now_time);
    }

    TimerId TimerQueue::insert(TimerCallback cb, Timestamp& when, Seconds interval)
    {
        // 在IO线程中执行
        Timer* timer = new Timer(cb, when, interval);
        TimerId id(timer->id());
        loop_->runInLoop(std::bind(&TimerQueue::insertInLoop, this, timer));
        return id;
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

    void TimerQueue::insertInLoop(Timer* timer)
    {
        loop_->assertInLoopThread();

        // 使用 unique_ptr 管理 Timer
        // FIXME: unique_ptr 离开函数后，销毁，造成悬垂引用
        std::unique_ptr<Timer> unique_timer(timer);
        Timestamp when = unique_timer->expiration();
        
        bool earliestTimeout = insert(std::move(unique_timer));
        
        if (earliestTimeout) {
            resetTimerfd(when);
        }
    }

    bool TimerQueue::insert(std::unique_ptr<Timer> timer)
    {
        loop_->assertInLoopThread();
        assert(timers_.size() == activeTimers_.size());
        bool earliestTimeout = false;
        Timestamp expiration = timer->expiration();
        TimerId timerId(timer->id());

        auto iter = timers_.begin();
        // 新添加的定时器的超时时间最小
        if (iter == timers_.end() || expiration < iter->first) {
            earliestTimeout = true;
        }

        auto res = timers_.emplace(expiration, std::move(timer));
        assert(res->second);

        auto ret = activeTimers_.emplace(timerId);
        assert(ret.second);

        assert(timers_.size() == activeTimers_.size());
        return earliestTimeout;
    }

}