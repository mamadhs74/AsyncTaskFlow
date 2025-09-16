#ifndef ASYNCTASKFLOW_HPP
#define ASYNCTASKFLOW_HPP

#include <iostream>
#include <vector>
#include <future>
#include <functional>
#include <type_traits>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace AsyncTaskFlow {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

class TaskDAG {
public:
    template<typename F, typename... Dependencies>
    void add_task(F&& func, Dependencies&&... deps);

    void execute(ThreadPool& pool);

private:
    struct TaskInfo {
        std::function<void()> function;
        std::unordered_set<size_t> dependencies;
        size_t dependent_count{0};
        bool completed{false};
    };

    std::unordered_map<size_t, TaskInfo> tasks;
    size_t next_task_id{0};

    void add_dependency(size_t task_id, size_t dep_id);
    std::future<void> execute_task(ThreadPool& pool, size_t task_id, std::mutex& futures_mutex);
};

// ThreadPool Implementation

inline ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    for(size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] { 
                        return stop || !tasks.empty(); 
                    });
                    if(stop && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if(stop) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker : workers) {
        worker.join();
    }
}

// TaskDAG Implementation

template<typename F, typename... Dependencies>
void TaskDAG::add_task(F&& func, Dependencies&&... deps) {
    auto task_id = next_task_id++;
    
    // Create task function
    auto task_func = [func = std::forward<F>(func)]() mutable {
        return func();
    };
    
    tasks[task_id] = {task_func, {deps...}, 0, false};
    
    // Update dependencies
    (add_dependency(task_id, deps), ...);
}

inline void TaskDAG::execute(ThreadPool& pool) {
    std::vector<std::future<void>> futures;
    std::mutex futures_mutex;
    
    for(auto& [id, task] : tasks) {
        if(task.dependencies.empty()) {
            futures.push_back(execute_task(pool, id, futures_mutex));
        }
    }
    
    // Wait for all tasks to complete
    for(auto& future : futures) {
        future.wait();
    }
}

inline void TaskDAG::add_dependency(size_t task_id, size_t dep_id) {
    tasks[dep_id].dependent_count++;
}

inline std::future<void> TaskDAG::execute_task(ThreadPool& pool, size_t task_id, std::mutex& futures_mutex) {
    return pool.enqueue([this, task_id, &futures_mutex] {
        auto& task = tasks[task_id];
        task.function();
        task.completed = true;
        
        std::vector<std::future<void>> new_futures;
        
        // Find tasks that depend on this one and check if they're ready
        for(auto& [id, other_task] : tasks) {
            if(other_task.dependencies.contains(task_id)) {
                other_task.dependencies.erase(task_id);
                if(other_task.dependencies.empty()) {
                    new_futures.push_back(execute_task(pool, id, futures_mutex));
                }
            }
        }
        
        // Add new futures to the collection (thread-safe)
        {
            std::lock_guard lock(futures_mutex);
            // In a real implementation, we'd add these to the main futures list
            // For simplicity, we just wait for them here
            for(auto& future : new_futures) {
                future.wait();
            }
        }
    });
}

} // namespace AsyncTaskFlow

#endif // ASYNCTASKFLOW_HPP