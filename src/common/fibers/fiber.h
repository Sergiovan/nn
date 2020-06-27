#pragma once

#include <ucontext.h>
#include "common/defs.h"

class fiber {
public:
    using fiber_args = void*; // Yes, yes, I know
    using fiber_func = void(*)(fiber_args args);
    static constexpr u32 stack_size = 8 << 10;
    
    static fiber* this_fiber();
    static bool yield(bool stall = false);
    static bool crash();
    
    fiber();
    ~fiber();
    
    void prepare(fiber_func fun, void* args);
    
    bool call();
    
    bool ready();
    bool running();
    bool stalled();
    bool callable();
    bool done();
    bool crashed();
    
private:
    
    static void call_fiber();
    
    bool _yield();
    void clean();
    
    enum {PREINIT, READY, RUNNING, STALLED, DONE, CRASHED} state{PREINIT};
    
    fiber_func fun{nullptr};
    fiber_args args{nullptr};
    
    ucontext_t this_context{};
    ucontext_t from_context{};
    
    char* stack{nullptr};
};
