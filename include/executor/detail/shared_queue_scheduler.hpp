#ifndef SHARED_QUEUE_SCHEDULER_H
#define SHARED_QUEUE_SCHEDULER_H

#include "executor/detail/scheduler.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>

namespace executor::detail {

class SharedQueueScheduler : public Scheduler {
public:
    void enqueue(Task)        override;
    bool wait_and_take(Task&) override;
    void close()              override;

private:
    std::queue<Task> _tasks;
    
    std::mutex _mutex;
    std::condition_variable _task_available;

    bool _accepting_tasks{true};
};

} // namespace

#endif