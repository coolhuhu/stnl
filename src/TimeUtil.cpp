#include "TimeUtil.h"
#include <string>
#include <inttypes.h>

using namespace stnl;

const int Timestamp::MillisecondsRatio = 1000;
const int Timestamp::MicrosecondsRatio = 1000000;
const int Timestamp::NanosecondsRatio = 1000000000;

/**
 * 使用 std::chrono::system_clock 精度可达纳秒
 */
Timestamp Timestamp::now()
{
    auto now_t = std::chrono::system_clock::now();
    return Timestamp(now_t.time_since_epoch().count());
}

std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = nanoSecondsSinceEpoch() / NanosecondsRatio;
    int64_t microseconds = mircoSecondsSinceEpoch() % MicrosecondsRatio;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
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