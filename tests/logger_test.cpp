#include <thread>
#include <boost/timer/timer.hpp>

#include "../src/logger.h"

using namespace stnl;

void test_defaultLogger()
{
    for (int i = 0; i < 100; i++) {
        LOG_TRACE << "log message. " << i;
        LOG_INFO << "log message. " << i;
        LOG_DEBUG << "log message. " << i;
        LOG_WARN << "log message. " << i;
        LOG_ERROR << "log message. " << i;
        // LOG_FATAL << "log message. " << i;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void test_ConsoleLogger()
{
    std::shared_ptr<ConsoleHandler> consoleLogger = std::make_shared<ConsoleHandler>("Console Logger", LogLevel::INFO);
    Logger::instance().addHandler(consoleLogger);

    for (int i = 0; i < 100; i++) {
        LOG_TRACE << "log message. " << i;
        LOG_INFO << "log message. " << i;
        LOG_DEBUG << "log message. " << i;
        LOG_WARN << "log message. " << i;
        LOG_ERROR << "log message. " << i;
        // LOG_FATAL << "log message. " << i;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } 
}


void test_LogFile()
{
    std::shared_ptr<FileHandler> fileLogger = std::make_shared<FileHandler>("File Logger", LogLevel::INFO, "path", 2, 100, 20 * 100);
    std::shared_ptr<ConsoleHandler> consoleLogger = std::make_shared<ConsoleHandler>("Console Logger", LogLevel::INFO);

    Logger::instance().addHandler(fileLogger);
    Logger::instance().addHandler(consoleLogger);

    for (int i = 0; i < 1000; ++i)
    {
        LOG_INFO << "log message test ...." << i + 1;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}


void test_AsyncLogging()
{
    std::shared_ptr<AsyncLogging> asyncLogger = std::make_shared<AsyncLogging>(".", LogLevel::INFO, 1024);
    asyncLogger->start();
    Logger::instance().setAsyncLogger(asyncLogger);

    std::function<void()> threadFunc = []() {
        for (int i = 0; i < 1000; ++i) {
            LOG_INFO << "log message test ..." << i + 1 << " thread_id: " << std::this_thread::get_id();
            // std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    {
        boost::timer::auto_cpu_timer t;

        std::thread thread1(threadFunc);
        std::thread thread2(threadFunc);
        std::thread thread3(threadFunc);

        std::cout << "thread1 id = " << thread1.get_id() << std::endl;
        std::cout << "thread2 id = " << thread2.get_id() << std::endl;
        std::cout << "thread3 id = " << thread3.get_id() << std::endl;
        
        thread1.join();
        thread2.join();
        thread3.join();
    }

    
}

int main()
{
    test_defaultLogger();
    // test_AsyncLogging();
}