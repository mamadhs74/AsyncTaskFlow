#include "AsyncTaskFlow/AsyncTaskFlow.hpp"

// Example usage with different task types
int main() {
    AsyncTaskFlow::ThreadPool pool;
    AsyncTaskFlow::TaskDAG dag;
    
    // Add tasks with dependencies
    dag.add_task([] {
        std::cout << "Task 1: Loading resources..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    
    dag.add_task([] {
        std::cout << "Task 2: Initializing systems..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    });
    
    dag.add_task([] {
        std::cout << "Task 3: Processing data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }, 0, 1); // Depends on tasks 0 and 1
    
    dag.add_task([] {
        std::cout << "Task 4: Finalizing..." << std::endl;
    }, 2); // Depends on task 2
    
    std::cout << "Starting parallel execution with dependencies..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    dag.execute(pool);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "All tasks completed in " << duration.count() << "ms" << std::endl;
    
    return 0;
}