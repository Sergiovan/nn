#include "common/threading/thread_pool.h"
#include "common/defs.h"

using tp = thread_pool;

tp::_promise::_promise() {
    
}

bool tp::_promise::get() {
    std::unique_lock l{lock};
    while (!done) {
        cond.wait(l);
    }
    
    return res;
}

bool tp::_promise::can_get() {
    return done;
}

void tp::_promise::stop() {
    stop_ = true;
}

bool tp::_promise::is_stopped() {
    return stop_;
}

void tp::_promise::set_value(bool value) {
    lock.lock();
    res = value;
    done = true;
    cond.notify_all();
    lock.unlock();
}

tp::thread::thread(thread_pool& tp) : tp{tp}, current{nullptr} { }

tp::thread::~thread() {
    kill();
    wait();
}

// Not very nice...
tp::thread::thread(thread&& o) : tp{o.tp}, current{o.current} {
    active = (bool) o.active;
    done = (bool) o.done;
    t.swap(o.t);
}

tp::promise tp::thread::start(task f, promise pr) {
    ASSERT(!active, "Thread was active");
    
    current = pr;
    
    active = true;
    t = std::thread{[=](){
        bool r = f(current);
        pr->set_value(r);
        
        done = true;
        tp.notify_thread_done();
    }};
    
    return pr;
}

void tp::thread::wait() {
    if (t.joinable()) {
        t.join();
    }
}

void tp::thread::kill() {
    if(current) {
        current->stop();
    }
}

bool tp::thread::is_active() {
    return active;
}

bool tp::thread::is_done() {
    return done;
}

void tp::thread::reset() {
    ASSERT(done, "Cannot reset active thread");
    
    active = false;
    done = false;
    
    current = {nullptr};
    
    if (t.joinable()) {
        t.join();
    }
}


struct thread_pool::task_internal {
    task fn{};
    promise p{new _promise{}};
};

thread_pool::thread_pool(u64 amount) {
    threads.reserve(amount);
    
    for (u64 i = 0; i < amount; ++i) {
        threads.emplace_back(*this);
        sem.notify();
    }
}

thread_pool::~thread_pool() {
    threads.clear();
}

std::pair<tp::task_handle, tp::promise> thread_pool::add_task(task t) {
    task_handle th = std::shared_ptr<task_internal>{new task_internal{t}};
    
    lock.lock();
    tasks.push(th);
    lock.unlock();
    wake_wait_any();
    
    return {th, th->p};
}

void thread_pool::wait_task(task_handle th) {                
    th->p->get(); // Will block until a value is gotten
    return; // Task done
}

void thread_pool::wait_all_tasks() {
    std::unique_lock l{lock};
    while (tasks.size()) {
        task_handle th = tasks.back();
        
        l.unlock();
        wait_task(th);
        l.lock();
    }
}

void thread_pool::start() {
    std::unique_lock l{lock};
    
    while (true) {        
        while (tasks.size()) {
            task_handle th = tasks.front();
            tasks.pop();
            
            lock.unlock();
            start_thread(th->fn, th->p); // Will block until there's a spot 
            lock.lock();
        }
        
        if (!wait_for_activity(l)) {
            break;
        }
    }
    
    l.unlock();
    wait_all_threads();
}

void thread_pool::stop_all() {
    lock.lock();
    while (tasks.size()) {
        tasks.pop();
    }
    lock.unlock();
    for (auto& t : threads) {
        t.kill();
    }
}

thread_pool::promise thread_pool::start_thread(task f, promise p) {
    auto& t = get_free(); // Will block and lock
    
    if (t.is_done()) {
        t.reset();
    }
    
    auto pp = t.start(f, p);
    lock.unlock();
    
    return pp;
}

bool thread_pool::wait_for_activity(std::unique_lock<std::mutex>& l) {
    ASSERT(l.owns_lock(), "Called wait_for_activity() without owning lock");
    
    if (tasks.size() || any_active_thread()) {
        volatile u64 sentinel_copy = sentinel;
        while (sentinel_copy == sentinel) {
            wait_any_cond.wait(l);
        }
        
        return true;
    }
    
    return false;
}

bool thread_pool::wait_thread() {
    std::unique_lock l{lock};
    
    if (!any_active_thread()) {
        return false; // No waiting done, no threads to wait for
    }
    
    u64 sentinel_copy = sentinel;
    
    while (sentinel_copy == sentinel) {
        wait_any_cond.wait(l);
    }
    
    return true; // Waiting done
}

void thread_pool::wait_thread(u64 thread) {
    ASSERT(thread < threads.size(), "Attempting wait on non-existing thread");
    
    threads[thread].wait();
}

void thread_pool::wait_all_threads() {
    while (wait_thread()) ; // This is fine because wait() blocks if there's some waiting
}

bool thread_pool::any_active_thread() {
    for (auto& t : threads) {
        if (t.is_active() && !t.is_done()) {
            return true;
        }
    }
    return false;
}

void thread_pool::notify_thread_done() {
    sem.notify();
    wake_wait_any();
}

void thread_pool::wake_wait_any() {
    std::unique_lock l{lock};
    sentinel = (sentinel + 1) & 0x7FFFFFFFFFFFFFFF;
    wait_any_cond.notify_all();
}

thread_pool::thread& thread_pool::get_free() {
    sem.wait();
    lock.lock(); // Unlocked outside
    
    for (auto& t : threads) {
        if (!t.is_active() || t.is_done()) {
            // This thread is free
            return t;
        }
    }
    
    ASSERT(false, "Semaphore was triggered without any free threads?");
    return threads[0];
}
