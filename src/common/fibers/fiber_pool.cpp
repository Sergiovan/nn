#include "fiber_pool.h"
#include "common/defs.h"
#include "common/logger.h"

fiber_pool::fiber_pool() {
    
}

bool fiber_pool::run() {
    
    if (!queued_tasks.size()) {
        return true;
    }
    
    u64 fibers_done{0};
    u64 current_task{0};
    u64 current_fiber{0};
    
    // TODO How do we end this shit
    while (fibers_done < queued_tasks.size()) {
    
        // Still fibers to allocate
        if (fibers_done + running_fibers.size() < queued_tasks.size()) {
            task& t = queued_tasks[current_task++];
            
            fiber* fib = t.f = new fiber();
            fib->prepare(t.fun, t.args);

            bool res = fib->call();
            if (!res) {
                return res;
            }

            if (fib->done()) {
                fibers_done++;
            } else {
                running_fibers.push_back(fib);
            }
        } else {
            fiber* fib = running_fibers[current_fiber];
            
            bool res = fib->call();
            if (!res) {
                return res;
            }
            
            if (fib->done()) {
                fibers_done++;
                running_fibers.erase(running_fibers.begin() + current_fiber);
                if (running_fibers.size()) {
                    current_fiber %= running_fibers.size();
                } else {
                    current_fiber = 0;
                }
            } else {
                current_fiber = (current_fiber + 1) % running_fibers.size();
            }
        }
    }
    
    return true;
}

void fiber_pool::queue_task(task t) {
    t.id = queued_tasks.size();
    queued_tasks.push_back(t);
}

void fiber_pool::clean() {
    for (auto& t : queued_tasks) {
        if (t.f) {
            delete t.f;
        }
        t.f = nullptr;
    }
    queued_tasks.clear();
    running_fibers.clear();
}
