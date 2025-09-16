#include "AsyncTaskFlow/AsyncTaskFlow.hpp"

int main() {
    AsyncTaskFlow::ThreadPool pool;
    AsyncTaskFlow::TaskDAG dag;
    
    dag.add_task([]{ /* Task 0 */ });
    dag.add_task([]{ /* Task 1 */ });
    dag.add_task([]{ /* Task 2 */ }, 0, 1); // Depends on 0 and 1
    dag.add_task([]{ /* Task 3 */ }, 2); // Depends on 2
    
    dag.execute(pool);
    return 0;
}