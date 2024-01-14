#include "EventLoop.h"
#include <cassert>

namespace stnl
{
    void EventLoop::loop()
    {
        assert(!looping_);
        looping_ = true;

        
    }
}
