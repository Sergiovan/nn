#include "common/threading/semaphore.h"

void semaphore::notify() {
    std::lock_guard lock{m};
    ++count;
    c.notify_one();
}

void semaphore::wait() {
    std::unique_lock lock{m};
    while(!count) // Handle spurious wake-ups.
        c.wait(lock);
    --count;
}

bool semaphore::try_wait() {
    std::lock_guard lock{m};
    if(count) {
        --count;
        return true;
    }
    return false;
}
