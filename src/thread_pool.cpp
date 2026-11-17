#include "executor/thread_pool.hpp"
#include "executor/detail/shared_queue_scheduler.hpp"
#include "executor/detail/work_stealing_scheduler.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

typedef struct WorkerContext {
    executor::ThreadPool *_pool = nullptr;
    std::size_t _worker_id      = 0;
} WorkerContext;

thread_local WorkerContext context;

} // namespace

namespace executor {

ThreadPool::ThreadPool(std::size_t n_workers)
    : _scheduler(make_scheduler(n_workers, SHARED_QUEUE)) 
{
    _workers.reserve(n_workers);

    // each thread will run the worker loop
    for (std::size_t i = 0; i < n_workers; i++)
        _workers.emplace_back(&ThreadPool::worker_loop, this, i);
}

ThreadPool::ThreadPool(std::size_t n_workers, SchedulerType type)
    : _scheduler(make_scheduler(n_workers, type)) 
{
    _workers.reserve(n_workers);

    // each thread will run the worker loop
    for (std::size_t i = 0; i < n_workers; i++)
        _workers.emplace_back(&ThreadPool::worker_loop, this, i);
}

ThreadPool::~ThreadPool() {
    this->shutdown();
}

// execute will add a worker to the queue. a worker thread will execute it later on.
void ThreadPool::execute(Task task) {
    if (context._pool == this)
        this->_scheduler->enqueue_local(context._worker_id, std::move(task));
    else
        this->_scheduler->enqueue_global(std::move(task));
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

void ThreadPool::worker_loop(std::size_t worker_id) {
    const WorkerContext prev_context = context;
    context = {._pool = this, ._worker_id = worker_id};

    Task task;
    while (this->_scheduler->wait_and_take(task, worker_id)) {
        try { task(); }
        catch (...) { /* do not re-throw the error */ }
    }

    context = prev_context; // restore previous context
}

std::unique_ptr<detail::Scheduler> ThreadPool::make_scheduler(std::size_t n_workers, 
    SchedulerType type) 
{
    if (n_workers == 0)
        throw std::invalid_argument("ThreadPool needs greater than zero workers.");

    switch (type) {
        case SHARED_QUEUE:
            return std::make_unique<detail::SharedQueueScheduler>();
        case WORK_STEALING_QUEUE:
            return std::make_unique<detail::WorkStealingScheduler>(n_workers);
        default:
            throw std::invalid_argument("Invalid scheduler type");
    }
}

}