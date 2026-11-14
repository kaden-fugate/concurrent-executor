#ifndef TASK_GROUP_H
#define TASK_GROUP_H

#include "executor/thread_pool.hpp"

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

class TaskGroup {
public: 
    explicit TaskGroup(ThreadPool&);
    ~TaskGroup();

    TaskGroup(const TaskGroup&) = delete;
    TaskGroup(TaskGroup&&)      = delete;
    
    TaskGroup& operator=(const TaskGroup&) = delete;
    TaskGroup& operator=(TaskGroup&&)      = delete;

    template <typename Function, typename... Args>
    auto submit(Function&&, Args&&...) 
        -> std::future<std::invoke_result_t<Function, Args...>>;

    // stop accepting further work, allow current workers to finish up.
    // you can call this multiple times externally.
    void wait();

    [[nodiscard]] std::size_t pending() const noexcept;

private:
    ThreadPool &_pool;

    mutable std::mutex _mutex;
    std::condition_variable _all_done;
    std::size_t _pending{0};

    void task_finished();
};

template <typename Function, typename... Args>
auto TaskGroup::submit(Function&& fn, Args&&... args) 
    -> std::future<std::invoke_result_t<Function, Args...>>
{

    using Result = std::invoke_result_t<Function, Args...>;

    // convert function to a no-argument callable
    auto bound_function = std::bind_front(
        std::forward<Function>(fn),
        std::forward<Args>(args)...
    );

    // increment pending once the function is bound
    {
        std::lock_guard<std::mutex> lock(_mutex);
        ++_pending;
    }

    try {
        return this->_pool.submit(
            [this, function = std::move(bound_function)]() mutable -> Result {
                try {
                    if constexpr (std::is_void_v<Result>) {
                        function();
                        this->task_finished();
                    }
                    else {
                        Result result = function();
                        this->task_finished();
                        return result;
                    }
                }
                catch (...) {
                    this->task_finished();
                    throw;
                }
            }
        );
    }
    catch (...) {
        this->task_finished();
        throw;
    }

}

}

#endif