#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <cstddef>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace executor {

class ThreadPool {
public: 
    explicit ThreadPool(std::size_t);
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&)     = delete;

    ThreadPool& operator=(const ThreadPool &)  = delete;
    ThreadPool& operator=(const ThreadPool &&) = delete;

    // execute will add a worker to the queue. a worker thread will execute it later on.
    void execute(std::function<void()>);

    // stop accepting further work, allow current workers to finish up.
    // you can call this multiple times externally.
    void shutdown();

    [[nodiscard]] std::size_t worker_count() const noexcept;

private:
    void worker_loop();

    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;

    // protect _tasks and _accepting_tasks
    std::mutex _queue_mutex;
    std::condition_variable _task_available;
    bool _accepting_tasks{true};

    // only one caller can perform the join sequence
    std::mutex _shutdown_mutex;
};

};

#endif