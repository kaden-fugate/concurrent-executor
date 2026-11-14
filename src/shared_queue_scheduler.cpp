#include "executor/detail/shared_queue_scheduler.hpp"

#include <stdexcept>
#include <utility>

namespace executor::detail {

void SharedQueueScheduler::enqueue(Task task) {
    if (!task)
        throw std::invalid_argument("Cannot execute an empty task");
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_accepting_tasks)
            throw std::invalid_argument("Cannot submit work after shutdown.");
        
        _tasks.push(std::move(task));
    }

    _task_available.notify_one();
}

bool SharedQueueScheduler::wait_and_take(Task &task) {
    std::unique_lock<std::mutex> lock(_mutex);

    _task_available.wait(lock, [this] {
        return !_tasks.empty() || !_accepting_tasks;
    });

    if (_tasks.empty()) return false;

    task = std::move(_tasks.front());
    _tasks.pop();

    return true;
}

void SharedQueueScheduler::close() {
    
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_accepting_tasks) return;
        _accepting_tasks = false;
    }

    _task_available.notify_all();
}

} // namespace