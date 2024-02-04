[《one-loop-per-thread核心原理》](one-loop-per-thread核心原理.md)中介绍了多线程下one loop per thread的核心原理，[《源码分析之Channel、EventLoop和Selector》](源码分析之Channel、EventLoop和Selector.md)中介绍了单线程下的 one loop per thread 具体实现，本文将继续介绍多线程下的 one loop per thread 具体实现。

one loop per thread也称为Reactor，为方便，下文使用 Reactor 替换 one loop per thread。

本文中多线程下的Reactor模型特指**多线程多Reactor模型**，小林coding 的[《高性能网络模式：Reactor 和 Proactor》](https://www.xiaolincoding.com/os/8_network_system/reactor.html)这篇文章中有介绍。多线程多Reactor模型的实现，主要涉及 Thread、EventLoopThread 和 EventLoopThreadPool 这三个类，下面依次进行介绍。

# Thread
Thread类是对C++标准库 std::thread 的进一步封装，主要提供给 EventLoopThread 类使用，用于将一个 EventLoop 实例对象和一个 Thread 实例对象绑定，即[《one-loop-per-thread核心原理》](one-loop-per-thread核心原理.md)中强调的一个 thread 中最多一个 loop，一个 loop 只属于一个 thread。

Thread 类成员变量和构造函数如下：
```cpp
class Thread
{
public:
    using ThreadFunction = std::function<void()>;

    Thread(std::string_view threadName, ThreadFunction func) : 
           threadName_(threadName), func_(func), running_(false) {}

    // ...
   
private:
    bool running_;
    std::string threadName_;
    ThreadFunction func_;
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::thread thread_;
    std::thread::id tid_;
};
```

Thread类的构造函数很简单，将线程待执行的主函数保存到 func_ 成员变量中，当 start() 函数被调用后，将 func_ 作为线程主函数传入 std::thread，真正开启一个线程进行执行。类成员变量中的 std::mutex 和 std::std::condition_variable_any 用于创建线程时进行同步，muduo中创建线程时同步使用的是CountDownLatch，本质上是一样。此外，muduo中采用了一个辅助的ThreadData结构体类来实现线程创建时同步，在我的实现中，我使用了一个简单的包装函数 `void threadFuncWarper(ThreadFunction func)` 来实现。

============================================

**关于线程标识的讨论**
关于线程标识的讨论，见muduo书中4.3节。这里给出我的一些浅显的看法。
C++标准库中的thread对线程的封装已经很完备了，在Linux环境中，thread本质上就是对POSIX线程的封装，`std::this_thread::get_id()` 等价于 `pthread_self()`。而 `std::thread::id`，也即 `std::this_thread::get_id()` 的返回值，重载了 operator<<，可以很方便的写入到输出流中，用于日志；此外，C++标准库也为 `std::thread::id` 提供了 [hash](https://en.cppreference.com/w/cpp/thread/thread/id/hash)，可以计算其 hash 值，使用 `std::thread::id` 可以作为线程唯一标识是可行的。


# EventLoopThread
EventLoopThread的实现较为简单，通过对象组合的方式将 Thread 和 EventLoop 组合起来，作为 EventLoopThread 的成员变量。EventLoopThread 的成员变量和构造函数如下：
```cpp
class EventLoopThread: public noncopyable
{
public:
    EventLoopThread::EventLoopThread(std::string_view threadName) :
                                     loop_(nullptr), mutex_(), cv_(),
                                     thread_(threadName, std::bind(&EventLoopThread::threadFunc, this))
    {
    }

private:
    Thread thread_;
    EventLoop* loop_;
    std::mutex mutex_;
    std::condition_variable_any cv_;
};
```

不要被 `EventLoop* loop_` 成员变量给欺骗，loop_ 是一个栈上变量，代码如下：
```cpp
EventLoop *EventLoopThread::startLoop()
{
    thread_.start();
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> locker(mutex_);
        cv_.wait(locker, [&]()
                    { return loop_ != nullptr; });
        loop = loop_;
    }
    
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    {
        std::unique_lock<std::mutex> locker(mutex_);
        loop_ = &loop;
        cv_.notify_one();
    }

    loop.loop();

    loop_ = nullptr;
}
```

# EventLoopThreadPool
EventLoopThreadPool类的实现也很简单，使用 `std::vector<std::unique_ptr<EventLoopThread>>` 管理多个 EventLoopThread 对象，主线程通过轮询的方法选择子线程来接收一个管理一个新连接。相关代码不在此展示。

一个改进点为，根据IO线程的负载情况分配IO线程，即每次选择IO负载最小的线程接受新的连接。试想一下这种场景，若在某段时间内，有几个线程的IO负载较高，此时恰好有大量的新连接请求到来，若新连接被分配到这些IO负载高的线程中，新连接的响应可能受到影响，为此应将新连接分配到负载较低的线程。这目前只是我的一种设想，后续尝试编写相关代码，并设计测试代码进行测试。


# 本文小结
在[《源码分析之Channel、EventLoop和Selector》](源码分析之Channel、EventLoop和Selector.md)的基础上实现多线程多Reactor模型变得简单的许多，至此多线程的事件处理框架搭建完成。后续在此基础上实现一个Tcp服务端程序。