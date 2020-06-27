#include "fiber.h"

#include "common/logger.h"

namespace {
    thread_local fiber* running_fiber{nullptr}; 
}

fiber* fiber::this_fiber() {
    return running_fiber;
}

bool fiber::yield(bool stall) {
    fiber* f = this_fiber();
    ASSERT(f, "Not in a fiber");
    ASSERT(f->state > PREINIT, "Fiber not ready to yield");
    
    if (stall) {
        f->state = STALLED;
    }
    
    return f->_yield();
}

bool fiber::crash() {
    fiber* f = this_fiber();
    ASSERT(f, "Not in a fiber");
    ASSERT(f->state > PREINIT, "Fiber not ready to yield");
    
    logger::error() << "Fiber crashed";
    f->state = CRASHED;
    
    return f->_yield();
}

fiber::fiber() {
    
}

fiber::~fiber() {
    if (stack) {
        delete [] stack;
    }
}

void fiber::prepare(fiber_func fun, void* args) {
    this->fun = fun;
    this->args = args;
    stack = new char[stack_size];
    
    getcontext(&this_context);
    
    this_context.uc_stack.ss_sp = this->stack;
    this_context.uc_stack.ss_size = stack_size;
    this_context.uc_link = &from_context;
    this_context.uc_flags = 0;
    
    makecontext(&this_context, &fiber::call_fiber, 0);
    
    state = READY;
}

bool fiber::call() {
    ASSERT(callable(), "Fiber not ready to be called");
    
    fiber* prev = this_fiber();
    running_fiber = this;
    running_fiber->state = RUNNING;
    int res = swapcontext(&from_context, &this_context);
    running_fiber = prev;
    
    if (!res) {
        return !crashed();
    } else {
        logger::error() << "Not enough memory for context swap in call()";
        return false;
    }
}

bool fiber::ready() {
    return state == READY;
}

bool fiber::running() {
    return state == RUNNING;
}

bool fiber::callable() {
    return running() || ready();
}

bool fiber::done() {
    return state == DONE;
}

bool fiber::crashed() {
    return state == CRASHED;
}

void fiber::call_fiber() {
    running_fiber->fun(running_fiber->args);
    running_fiber->clean();
}

bool fiber::_yield() {
    ASSERT(this_fiber() == this, "_yield() called from different fiber");
    
    int res = swapcontext(&this_context, &from_context);
    if (!res) {
        return !crashed();
    } else {
        logger::error() << "Not enough memory for context swap in yield()";
        return false;
    }
}

void fiber::clean() {
    state = DONE;
    delete [] stack;
    stack = nullptr;
    fun = nullptr;
    args = nullptr;
}
