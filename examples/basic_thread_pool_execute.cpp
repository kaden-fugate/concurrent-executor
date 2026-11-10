#include "executor/thread_pool.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#define TASK_COUNT   64
#define THREAD_COUNT 8
#define SLEEP_MS     500

int main() {
    executor::ThreadPool pool(THREAD_COUNT);
    std::mutex output_mutex;

    for (int i = 0; i < TASK_COUNT; i++) {
        pool.execute([i, &output_mutex] {
            std::this_thread::sleep_for(std::chrono::milliseconds{SLEEP_MS});
            
            std::lock_guard output_lock(output_mutex);
            std::cout << "Task " << i << " ran on thread " << std::this_thread::get_id()
                << "\n";
        });
    }

    std::cout << "Shutting down the thread pool.\n";
    pool.shutdown();

    return 0;
}