#include "stnl/TimeUtil.h"

#include <iostream>

using namespace stnl;

void test_Timestamp()
{
    auto t = std::chrono::system_clock::now();
    std::cout << "current time: " << t.time_since_epoch().count() << std::endl;

    Timestamp now_t = Timestamp::now();
    std::cout << "nanoSecondsSinceEpoch: " << now_t.nanoSecondsSinceEpoch() << std::endl;
    std::cout << "mircoSecondsSinceEpoch: " << now_t.mircoSecondsSinceEpoch() << std::endl;
    std::cout << "milliSecondsSinceEpoch: " << now_t. milliSecondsSinceEpoch() << std::endl;
    std::cout << "secondsSinceEpoch: " << now_t.secondsSinceEpoch() << std::endl;
    std::cout << "nanosecond fraction: " << now_t.nanosecondFraction() << std::endl;

    Timestamp copy_now_t(now_t);
    std::cout << (copy_now_t == now_t) << std::endl;

    Timestamp now_t2 = Timestamp::now();
    std::cout << (now_t < now_t2) << std::endl;
}

void test_intervalFromNow()
{
    auto chrono_curr_time = std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "chrono_curr_time: " << chrono_curr_time << std::endl;
    Timestamp curr_time = Timestamp::now();
    std::cout << "curr_time: " << curr_time.nanoSecondsSinceEpoch() << std::endl;
    Timestamp future_time = addTime(curr_time, 5);
    std::cout << "future_time: " << future_time.nanoSecondsSinceEpoch() << std::endl;

    std::cout << "future_time - curr_time = " 
              << future_time.nanoSecondsSinceEpoch() - curr_time.nanoSecondsSinceEpoch() << std::endl;

    timespec interval = intervalFromNow(future_time);
    std::cout << "iterval: " << interval.tv_sec << " seconds, " 
              << interval.tv_nsec << "nanoseconds" << std::endl;
}

int main()
{
    test_Timestamp();
    std::cout << "---------------------------" << std::endl;
    test_intervalFromNow();
}
