#include "executor/detail/work_stealing_scheduler.hpp"

#include <random>
#include <stdexcept>
#include <utility>

namespace {

thread_local std::mt19937 worker_rng {
    std::random_device()()
};

}

namespace executor::detail {

using lock_guard  = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;

WorkStealingScheduler::WorkStealingScheduler(std::size_t n_workers) 
    : _n_workers(n_workers) 
{
    if (n_workers == 0)
        throw std::invalid_argument("WorkStealingQueue needs greater than zero workers.");
    
    _local_queues.resize(n_workers);
}

// enqueue into the global task deque. need to push to the BACK of the deque.
// increment _wake_generation as this is a reason for a worker to wake up.
void WorkStealingScheduler::enqueue_global(Task task) {
    if (!task)
        throw std::invalid_argument("Cannot execute an empty task");

    {
        lock_guard state_lock(_state_mutex);
        if (!_accepting_tasks)
            throw std::invalid_argument("Cannot submit work after shutdown.");
        
        {
            lock_guard global_lock(_global_mutex);
            _global_tasks.push_back(std::move(task));
        }

        ++_wake_generation;
    }

    _task_available.notify_one();
}

//
void WorkStealingScheduler::enqueue_local(size_t worker_id, Task task) {
    if (!task)
        throw std::invalid_argument("Cannot execute an empty task");

    {
        lock_guard state_lock(_state_mutex);
        if (!_accepting_tasks)
            throw std::invalid_argument("Cannot submit work after shutdown.");

        {
            LocalQueue *worker_queue = &_local_queues[worker_id];
            lock_guard worker_lock(worker_queue->_mutex);
            worker_queue->_tasks.push_back(std::move(task));
        }

        ++_wake_generation;
    }

    _task_available.notify_one();
}

bool WorkStealingScheduler::wait_and_take(Task &task, std::size_t worker_id) {

    std::uniform_int_distribution<size_t> victim_distribution{
        0,
        _n_workers - 1
    };
    const size_t victim = victim_distribution(worker_rng);

    std::size_t observed_generation = 0;
    while (true) {
        {
            lock_guard state_lock(_state_mutex);
            observed_generation = _wake_generation;
        }

        // try to do some local work
        {
            LocalQueue *worker_queue = &_local_queues[worker_id];
            lock_guard mutex_lock(worker_queue->_mutex);

            if (!worker_queue->_tasks.empty()) {
                task = std::move(worker_queue->_tasks.back());
                worker_queue->_tasks.pop_back();
                return true;
            }
        }

        // try to do some global work
        {
            lock_guard global_lock(_global_mutex);
            if (!_global_tasks.empty()) {
                task = std::move(_global_tasks.front());
                _global_tasks.pop_front();
                return true;
            }
        }

        // try to steal some work
        {
            for (size_t i = 0, id = victim - 1; i < _n_workers; i++) {
                id = (id + 1) % _n_workers;
                if (id == worker_id) continue;
                
                LocalQueue *worker_queue = &_local_queues[id];
                lock_guard mutex_lock(worker_queue->_mutex);
                
                if (!worker_queue->_tasks.empty()) {
                    task = std::move(worker_queue->_tasks.front());
                    worker_queue->_tasks.pop_front();
                    return true;
                }
            }
        }

        // no work, wait
        {
            unique_lock state_lock(_state_mutex);
            if (!_accepting_tasks)
                return false;

            _task_available.wait(state_lock, [this, observed_generation] {
                return observed_generation != _wake_generation || !_accepting_tasks;
            });
        }
    }
}

void WorkStealingScheduler::close() {

    {
        lock_guard state_lock(_state_mutex);
        if (!_accepting_tasks) return;

        _accepting_tasks = false;
        ++_wake_generation;
    }

    _task_available.notify_all();
}

} // namespace