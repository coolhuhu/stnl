/**
 * 对 std::thread 进行了简单的封装
 */

#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <string_view>
#include <string>
#include <functional>
#include <cassert>
#include <mutex>
#include <condition_variable>

namespace stnl
{

class Thread
{
public:
    using ThreadFunction = std::function<void()>;

    Thread(std::string_view threadName, ThreadFunction func) : threadName_(threadName), func_(func), running_(false) {}

    // TODO: make moveable

    ~Thread()
    {
        if (running_ && thread_.joinable())
        {
            thread_.detach();
        }
    }

    void start()
    {
        assert(!running_);
        assert(!thread_.joinable());
        running_ = true;
        thread_ = std::move(std::thread(&Thread::threadFuncWarper, this, std::move(func_)));
        
        {
            std::unique_lock<std::mutex> locker(mutex_);
            cv_.wait(locker);
        }
    }

    void join()
    {
        assert(running_);
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    bool joinable() const { return thread_.joinable(); }

    const std::string& name() { return threadName_; }

    const std::thread::id tid() const { return tid_; }

private:
    void threadFuncWarper(ThreadFunction func);

private:
    bool running_;
    std::string threadName_;
    ThreadFunction func_;
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::thread thread_;
    std::thread::id tid_;
};

}

#endif