#pragma once

#include "fiber.h"
#include <vector>


class fiber_pool {
public:
    struct task {
        fiber::fiber_func fun;
        fiber::fiber_args args;
        
        u64 id{0};
        fiber* f{nullptr};
    };
    
    fiber_pool();
    
    bool run();
    void queue_task(task t);
    
private:
    void clean();
    
    std::vector<task> queued_tasks{};
    std::vector<fiber*> running_fibers{};
};
