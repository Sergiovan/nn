#pragma once

// Original from https://stackoverflow.com/a/4793662/4918348

#include <mutex>
#include <condition_variable>

#include "common/defs.h"

class semaphore {    
public:
    void notify();
    void wait();
    bool try_wait();
    
private:
    std::mutex m;
    std::condition_variable c;
    u64 count{0};
};
