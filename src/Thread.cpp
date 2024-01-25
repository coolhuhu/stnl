#include "Thread.h"

using namespace stnl;

void Thread::threadFuncWarper(ThreadFunction func)
{
    {
        std::unique_lock<std::mutex> locker(mutex_);
        tid_ = thread_.get_id();  
    }
    cv_.notify_one();

    // FIXME: exception handling
    func();
}