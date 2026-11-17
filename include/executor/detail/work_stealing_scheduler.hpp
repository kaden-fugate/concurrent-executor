#ifndef WORK_STEALING_SCHEDULER_H
#define WORK_STEALING_SCHEDULER_H

#include "executor/detail/local_queue.hpp"
#include "executor/detail/scheduler.hpp"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <deque>

namespace executor::detail {

using condition_variable = std::condition_variable;
using LocalQueues        = std::deque<LocalQueue>;
using mutex              = std::mutex;
using size_t             = std::size_t;
using Tasks              = std::deque<Task>;

class WorkStealingScheduler : public Scheduler {
public:
    WorkStealingScheduler(size_t);
    void enqueue_global(Task)         override;
    void enqueue_local(size_t, Task)  override;
    bool wait_and_take(Task&, size_t) override;
    void close()                      override;

private:
    size_t _n_workers;            // amount of workers
    size_t _wake_generation{0};   // records things that can give a reason to wake
    
    Tasks  _global_tasks;         // global task deque
    
    mutex  _global_mutex;         // protects _global_tasks
    mutex  _state_mutex;          // protects _accepting_tasks and _wake_generation 

    LocalQueues        _local_queues;   // each worker has a mutex and deque
    condition_variable _task_available;

    bool _accepting_tasks{true};
};

} // namespace

#endif