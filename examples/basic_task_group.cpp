#include "executor/task_group.hpp"
#include "executor/thread_pool.hpp"

#include <iostream>
#include <stdint.h>
#include <chrono>

#define N_THREADS 8
#define N_TASKS   40

uint64_t sq_sums(int start, int count) { 
    uint64_t sum = 0;
    for (int i = start; i < start + count; i++)
        sum += (i / 2) * (i / 2);
    
    return sum;
}

int main() {
    executor::ThreadPool pool(N_THREADS);
    executor::TaskGroup  tgrp(pool);
    std::vector<std::future<uint64_t>> results;

    const auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < N_TASKS; i++)
        results.push_back( tgrp.submit(sq_sums, i + 1, N_TASKS * 1000000) );
    
    tgrp.wait();

    const auto end_time = std::chrono::steady_clock::now();
    
    for (int i = 0; i < N_TASKS; i++)
        std::cout << "Start: " << i + 1 << ",\tResult: " << results.at(i).get() << "\n";

    const auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );

    std::cout << "Elapsed: " << elapsed.count() << " ms\n";
}