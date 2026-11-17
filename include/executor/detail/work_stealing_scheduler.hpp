#ifndef WORK_STEALING_SCHEDULER_H
#define WORK_STEALING_SCHEDULER_H

#include "executor/detail/local_queue.hpp"
#include "executor/detail/scheduler.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>

namespace executor::detail {

class WorkStealingScheduler : public Scheduler {
public:
    WorkStealingScheduler(std::size_t);
    void enqueue(Task)                     override;
    bool wait_and_take(Task&, std::size_t) override;
    void close()                           override;

private:
    std::size_t      _n_workers;            // amount of workers
    std::queue<Task> _global_tasks;         // global task queue
    std::mutex       _mutex;                // global task queue mutex
    
    std::deque<LocalQueue>  _local_queues;  // each worker has a mutex and array
    std::condition_variable _task_available;

    bool _accepting_tasks{true};
};

} // namespace

#endif