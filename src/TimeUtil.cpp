#include "TimeUtil.h"

using namespace stnl;


/**
 * 使用 std::chrono::system_clock 精度可达纳秒
*/
Timestamp Timestamp::now()
{
    auto now_t = std::chrono::system_clock::now();
    return Timestamp(now_t.time_since_epoch().count());
}


struct timespec stnl::intervalFromNow(Timestamp when)
{
    auto diff = when.nanoSecondsSinceEpoch() - Timestamp::now().nanoSecondsSinceEpoch();

    // FIXME: if diff <= 0
    struct timespec interval;
    interval.tv_sec = static_cast<time_t>(diff / Timestamp::NanosecondsRatio);
    interval.tv_nsec = static_cast<long>(diff % Timestamp::NanosecondsRatio);
    
    return interval;
}