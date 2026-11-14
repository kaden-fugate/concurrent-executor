#include "executor/thread_pool.hpp"
#include "executor/detail/shared_queue_scheduler.hpp"

#include <exception>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace executor {

ThreadPool::ThreadPool(std::size_t n_workers)
    : _scheduler(std::make_unique<detail::SharedQueueScheduler>()) {
    if (n_workers == 0)
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
void ThreadPool::execute(Task task) {
    this->_scheduler->enqueue(std::move(task));
}

void ThreadPool::shutdown() {
    std::lock_guard<std::mutex> shutdown_lock(_shutdown_mutex);

    this->_scheduler->close();
    for (std::thread &worker : _workers) {
        if (worker.joinable()) worker.join();
    }
}

std::size_t ThreadPool::worker_count() const noexcept {
    return this->_workers.size();
}

void ThreadPool::worker_loop() {
    Task task;
    while (this->_scheduler->wait_and_take(task)) {
        try { task(); }
        catch (...) { /* do not re-throw the error */ }
    }
}

}