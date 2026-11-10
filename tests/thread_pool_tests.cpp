#include "executor/thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#define TASK_COUNT 10000

int main() {
    executor::ThreadPool pool(4);

    std::atomic<int> counter(0);

    for (int i = 0; i < TASK_COUNT; i++) {
        pool.execute([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.shutdown();
    assert(counter.load(std::memory_order_relaxed) == TASK_COUNT);

    // test that pool throws an error when adding to it after shutdowns
    bool failed = false;
    try {
        pool.execute([] {});
    }
    catch (std::invalid_argument e) {
        failed = true;
    }
    
    assert(failed);
    std::cout << "[THREAD_POOL_TEST]: PASS\n";

    return 0;
}