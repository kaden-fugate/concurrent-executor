#include "executor/task_group.hpp"

namespace executor {

TaskGroup::TaskGroup(ThreadPool &pool) : _pool(pool){}

TaskGroup::~TaskGroup() {
    this->wait();
}

// return once there are no more tasks pending
void TaskGroup::wait() {
    std::unique_lock<std::mutex> lock(_mutex);

    _all_done.wait(lock, [this] {
        return !_pending;
    });
}

std::size_t TaskGroup::pending() const noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    return _pending;
}

void TaskGroup::task_finished() {
    
    bool all_done = false;

    { 
        std::lock_guard<std::mutex> lock(_mutex);
        all_done = (--_pending == 0);
    }

    if (all_done) _all_done.notify_all();
}

}