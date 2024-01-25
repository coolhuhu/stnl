#ifndef STNL_TIMEUTIL_H
#define STNL_TIMEUTIL_H

#include <chrono>


/**
 * chrono使用指南：https://zhuanlan.zhihu.com/p/662738124
*/


namespace stnl
{

    class Timestamp
    {
    public:
        Timestamp(): nanoSecondsSinceEpoch_(0) {}

        explicit Timestamp(int64_t nanoSecondsSinceEpoch): nanoSecondsSinceEpoch_(nanoSecondsSinceEpoch) {}

        Timestamp(const Timestamp& t): nanoSecondsSinceEpoch_(t.nanoSecondsSinceEpoch_) {}

        Timestamp& operator=(const Timestamp& rhs)
        {
            if (this != &rhs) {
                this->nanoSecondsSinceEpoch_ = rhs.nanoSecondsSinceEpoch_;
            }
            return *this;
        }

        decltype(auto) secondsSinceEpoch() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(nanoSecondsSinceEpoch_).count();
        }

        decltype(auto) mircoSecondsSinceEpoch() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(nanoSecondsSinceEpoch_).count();
        }

        decltype(auto) milliSecondsSinceEpoch() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(nanoSecondsSinceEpoch_).count();
        }

        decltype(auto) nanoSecondsSinceEpoch() const
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(nanoSecondsSinceEpoch_).count();
        }

        decltype(auto) nanosecondFraction()
        {
            return nanoSecondsSinceEpoch() - mircoSecondsSinceEpoch() * MillisecondsRatio;
        }

        static Timestamp now();

        static Timestamp invalid()
        {
            return Timestamp();
        }

    public:
        static const int MillisecondsRatio = 1000;
        static const int MicrosecondsRatio = 1000000;
        static const int NanosecondsRatio = 1000000000;
    
    private:
        std::chrono::nanoseconds nanoSecondsSinceEpoch_;
    };

    inline bool operator<(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.nanoSecondsSinceEpoch() < rhs.nanoSecondsSinceEpoch();
    }

    inline bool operator==(const Timestamp& lhs, const Timestamp& rhs)
    {
        return lhs.nanoSecondsSinceEpoch() == rhs.nanoSecondsSinceEpoch();
    }

    struct timespec intervalFromNow(Timestamp when);

    inline Timestamp addTime(Timestamp timestamp, double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::NanosecondsRatio);
        return Timestamp(timestamp.nanoSecondsSinceEpoch() + delta);
    }

}


#endif