#include "../src/EventLoop.h"
#include "../src/logger.h"
#include <iostream>

using namespace stnl;


void func()
{
    std::cout << "func()..." << std::endl;
}

int main()
{
    std::shared_ptr<ConsoleHandler> consoleLogger = std::make_shared<ConsoleHandler>("Console Logger", LogLevel::DEBUG);
    Logger::instance().addHandler(consoleLogger);

    EventLoop loop;
    loop.runInLoop(func);
    loop.loop();
    
}