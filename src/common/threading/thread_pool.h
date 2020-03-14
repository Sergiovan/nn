#pragma once

#include <atomic>
#include <functional>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <queue>

#include "common/threading/semaphore.h"

class tasker;

class thread_pool {    
private:
    class thread;
    
    class _promise {
    public:
        _promise();
        
        bool get();
        bool can_get();
        
        void stop();
        bool is_stopped();
    private:
        void set_value(bool value);
        
        bool res{false};
        std::atomic_bool done{false};
        std::atomic_bool stop_{false};
        
        std::mutex lock{};
        std::condition_variable cond{};
        
        friend class thread;
    };
   
public:
    using promise = std::shared_ptr<_promise>;
    using task = std::function<bool(promise)>;
        
private:
    class thread {
    public:
        thread(thread_pool& tp);
        
        ~thread();
        
        
        thread(thread&& o);
        
        promise start(task f, promise pr);
        
        void wait();
        void kill();
        
        bool is_active();
        bool is_done();
        
        void reset();
        
    private:
        thread_pool& tp;
        promise current;
        
        std::thread t{};

        bool active{false};
        bool done{false};
    };

    struct task_internal;
    using task_handle = std::shared_ptr<task_internal>;
    
public:
    thread_pool(u64 amount);
    ~thread_pool();
    
    std::pair<task_handle, promise> add_task(task t);
    void wait_task(task_handle th);
    void wait_all_tasks();
    
    void start();
    
    void stop_all();
private:
    promise start_thread(task f, promise p);
    bool wait_for_activity(std::unique_lock<std::mutex>& l);
    
    bool wait_thread();
    void wait_thread(u64 thread);
    // If this is called from within a pool thread there's a deadlock >:(
    void wait_all_threads();
    bool any_active_thread();
    void notify_thread_done();
    void wake_wait_any();
    
    thread& get_free();
    
    semaphore sem{}; // Triggered once per thread done
    
    std::mutex lock{}; // Protects all data
    std::condition_variable wait_any_cond{}; // Triggered on any thread done or task added
    u64 sentinel{0}; // wait_any_cond sentinel value
    
    std::vector<thread> threads{};
    std::queue<task_handle> tasks{};
    
    friend class thread;
};
