#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "executor/detail/scheduler.hpp"

#include <cstddef>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace executor {

using Task = std::function<void()>;

class ThreadPool {
public: 
    explicit ThreadPool(std::size_t);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&)      = delete;

    ThreadPool& operator=(const ThreadPool&)  = delete;
    ThreadPool& operator=(ThreadPool&&)       = delete;

    // execute will add a worker to the queue. a worker thread will execute it later on.
    void execute(Task);

    template <typename Function, typename... Args>
    auto submit(Function&&, Args&&...) 
        -> std::future<std::invoke_result_t<Function, Args...>>;

    // stop accepting further work, allow current workers to finish up.
    // you can call this multiple times externally.
    void shutdown();

    [[nodiscard]] std::size_t worker_count() const noexcept;

private:
    void worker_loop();

    std::vector<std::thread> _workers;
    std::unique_ptr<detail::Scheduler> _scheduler;

    // only one caller can perform the join sequence
    std::mutex _shutdown_mutex;
};

template <typename Function, typename... Args>
auto ThreadPool::submit(Function&& fn, Args&&... args) 
    -> std::future<std::invoke_result_t<Function, Args...>>
{
    using Result = std::invoke_result_t<Function, Args...>;

    // convert function to a no-argument callable (only format that execute accepts)
    auto bound_function = std::bind_front(
        std::forward<Function>(fn),
        std::forward<Args>(args)...
    );

    // wrap the bound function, connect it to a future, return as a shared pointer
    auto pkg_task = std::make_shared<std::packaged_task<Result()>>(
        std::move(bound_function)
    );

    std::future<Result> res = pkg_task->get_future();

    this->execute([pkg_task] {
        (*pkg_task)();
    });

    return res;
}

}

#endif