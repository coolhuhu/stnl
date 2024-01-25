#include "../src/Thread.h"
#include <iostream>

using namespace stnl;

void threadFunc1(std::string msg)
{
    std::cout << "threadFunc runing... "
              << "current thread id: " << std::this_thread::get_id()
              << " " << msg << std::endl;
}

void threadFunc2(std::string msg)
{
    std::cout << "threadFunc2 starting..." << std::endl;
    std::cout << msg << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "threadFunc2 end." << std::endl;
}

class Foo
{
public:
    Foo(int x) : x_(x) {}

    void func1()
    {
        std::cout << "current thread id: " << std::this_thread::get_id()
                  << ". Foo::func1()" << std::endl;
    }

    void func2(const std::string &msg)
    {
        std::cout << "current thread id: " << std::this_thread::get_id()
                  << ". Foo::func2(" << msg << ")"
                  << std::endl;
    }

private:
    int x_;
};

int main()
{
    std::cout << "run test..." << std::endl;
    std::cout << "main thread tid: " << std::this_thread::get_id() << std::endl;

    Thread t1("thread1", std::bind(threadFunc1, "thread-1"));
    std::cout << "thread1 start..." << std::endl;
    t1.start();
    std::cout << t1.name() << " tid: " << t1.tid() << std::endl;
    t1.join();
    std::cout << t1.name() << " stop." << std::endl;

    std::cout << "-------------------------------" << std::endl;
    std::cout << std::endl;

    {
        Thread t2("thread2", std::bind(threadFunc2, "thread-2"));
        std::cout << "thread2 start..." << std::endl;
        t2.start();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        // 线程执行完毕
        std::cout << t2.name() << " tid: " << t2.tid() << std::endl;
        std::cout << t2.name() << " stop." << std::endl;
    }

    std::cout << "-------------------------------" << std::endl;
    std::cout << std::endl;

    {
        Thread t3("thread3", std::bind(threadFunc2, "thread-3"));
        std::cout << "thread3 start..." << std::endl;
        t3.start();
        std::cout << t3.name() << " tid: " << t3.tid() << std::endl;
        std::cout << t3.name() << " running..." << std::endl;
        // t3 析构了，线程挂在后台执行（detach)
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "-------------------------------" << std::endl;
    std::cout << std::endl;

    Foo foo(14);
    Thread t4("thread4", std::bind(&Foo::func1, &foo));
    std::cout << "thread4 start..." << std::endl;
    t4.start();
    std::cout << t4.name() << " tid: " << t4.tid() << std::endl;
    t4.join();
    std::cout << t4.name() << " stop." << std::endl;

    std::cout << "-------------------------------" << std::endl;
    std::cout << std::endl;

    std::string msg("hello");
    Thread t5("thread5", std::bind(&Foo::func2, &foo, std::ref(msg)));
    std::cout << "thread5 start..." << std::endl;
    t5.start();
    std::cout << t5.name() << " tid: " << t5.tid() << std::endl;
    t5.join();
    std::cout << t5.name() << " stop." << std::endl;

    std::cout << "-------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "test finish." << std::endl;
}