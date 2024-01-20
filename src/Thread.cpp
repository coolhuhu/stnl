#include "Thread.h"

using namespace stnl;

void Thread::threadFuncWarper(ThreadFunction func)
{
    {
        std::unique_lock<std::mutex> locker(mutex_);
        
    }
}