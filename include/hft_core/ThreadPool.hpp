#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <type_traits>

#ifdef __linux__
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#endif

namespace hft::core {

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) 
        : stop_(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] { 
                            return stop_.load() || !tasks_.empty(); 
                        });
                        
                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_.load()) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return res;
    }

#ifdef __linux__
    void set_thread_affinity(size_t thread_idx, int cpu_id) {
        if (thread_idx >= workers_.size()) {
            throw std::out_of_range("Thread index out of range");
        }
        
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_id, &cpuset);
        
        int result = pthread_setaffinity_np(
            workers_[thread_idx].native_handle(),
            sizeof(cpu_set_t), &cpuset);
        
        if (result != 0) {
            throw std::runtime_error("Failed to set thread affinity");
        }
    }
#endif

    size_t size() const noexcept {
        return workers_.size();
    }

    size_t pending_tasks() const noexcept {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    ~ThreadPool() {
        stop_.store(true);
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

class HighPriorityThreadPool {
public:
    explicit HighPriorityThreadPool(size_t threads = 2) : stop_(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this, i] {
                set_thread_priority();
                set_thread_affinity(i);
                worker_loop();
            });
        }
    }

    template<class F, class... Args>
    auto enqueue_high_priority(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_.load()) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            high_priority_tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return res;
    }

    ~HighPriorityThreadPool() {
        stop_.store(true);
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    void worker_loop() {
        for (;;) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] { 
                    return stop_.load() || !high_priority_tasks_.empty(); 
                });
                
                if (stop_.load() && high_priority_tasks_.empty()) {
                    return;
                }
                
                task = std::move(high_priority_tasks_.front());
                high_priority_tasks_.pop();
            }
            
            task();
        }
    }

    void set_thread_priority() {
#ifdef __linux__
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        
        if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
            // Fallback to nice priority if real-time scheduling fails
            nice(-20);
        }
#endif
    }

    void set_thread_affinity(size_t thread_idx) {
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(thread_idx % std::thread::hardware_concurrency(), &cpuset);
        
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> high_priority_tasks_;
    
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

} // namespace hft::core