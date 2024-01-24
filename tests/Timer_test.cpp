#include "../src/Timer.h"

#include <iostream>
#include <string>

using namespace stnl;


void print_msg(std::string msg)
{
    std::cout << msg << std::endl;
}


int main()
{
    EventLoop loop;
    print_msg("start.");
    loop.runAfter(1, std::bind(print_msg, std::string("timeout once after 1s")));
    loop.runAfter(2, std::bind(print_msg, std::string("timeout once after 2s")));
    loop.runAfter(3.5, std::bind(print_msg, std::string("timeout once after 3.5s")));
    loop.runAfter(4, std::bind(print_msg, std::string("timeout once after 4s")));

    loop.loop();
    print_msg("loop exit");
}