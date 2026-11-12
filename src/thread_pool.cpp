#include "executor/thread_pool.hpp"

#include <exception>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace executor {

ThreadPool::ThreadPool(std::size_t n_workers) {
    if (n_workers <= 0)
        throw std::invalid_argument("ThreadPool needs greater than zero workers.");

    _workers.reserve(n_workers);

    // each thread will run the worker loop
    for (std::size_t i = 0; i < n_workers; i++)
        _workers.emplace_back(&ThreadPool::worker_loop, this);
}

ThreadPool::~ThreadPool() {
    this->shutdown();
}

// execute will add a worker to the queue. a worker thread will execute it later on.
void ThreadPool::execute(std::function<void()> task) {
    if (!task)
        throw std::invalid_argument("Cannot execute an empty task.");

    {
        std::lock_guard queue_lock(_queue_mutex);
        if (!_accepting_tasks)
            throw std::invalid_argument("Cannot submit work after shutdown.");

        _tasks.push(std::move(task));
    }

    // now that the new task is visibile, we should wake a worker so that it can
    // handle it
    _task_available.notify_one();
}

void ThreadPool::shutdown() {
    std::lock_guard shutdown_lock(_shutdown_mutex);

    // queue mutex must be locked to set accepting tasks to false
    {
        std::lock_guard queue_lock(_queue_mutex);

        if (!_accepting_tasks) return;
        _accepting_tasks = false;
    }

    // let all workers finish up their work (or exit if no work queued)
    _task_available.notify_all();
    for (std::thread &worker : _workers) {
        if (worker.joinable()) worker.join();
    }
}

std::size_t ThreadPool::worker_count() const noexcept {
    return this->_workers.size();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock queue_lock(_queue_mutex);
            _task_available.wait(queue_lock, [this] {
                return !this->_tasks.empty() || !this->_accepting_tasks;
            });

            // when shutdown is in process and queue is empty, break out of the worker
            // loop
            if (this->_tasks.empty() && !this->_accepting_tasks) return;

            task = std::move(_tasks.front());
            _tasks.pop();
        }

        try {
            task();
        }
        catch (...) {
            throw;
        }
    }
}

}