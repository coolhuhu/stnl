#ifndef COUNTDOWNLATCH_
#define COUNTDOWNLATCH_

#include "noncopyable.h"
#include <mutex>
#include <condition_variable>


class CountDownLatch: public noncopyable {
public:
    explicit CountDownLatch(int count): mutex_(), condition_(), count_(count) {}

    void wait() {
        std::unique_lock<std::mutex> locker(mutex_);
        condition_.wait(locker, [&]() {return count_ == 0;});
    }

    void countDown() {
        std::unique_lock<std::mutex> locker(mutex_);
        --count_;
        if (count_ == 0) {
            condition_.notify_all();
        }
    }

    int getCount() const {
        std::unique_lock<std::mutex> locker(mutex_);
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable_any condition_;
    int count_;
};


#endif