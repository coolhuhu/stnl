#include "../src/TimeUtil.h"

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
    auto time = Timestamp::now().nanoSecondsSinceEpoch();
    Timestamp futureTime(time + 6000000000);    // 未来5s
    struct timespec interval = intervalFromNow(futureTime);
    std::cout << "interval " << interval.tv_sec << " seconds, "
              << interval.tv_nsec << " nanoseconds" << std::endl;
}

int main()
{
    test_Timestamp();
    test_intervalFromNow();
}
