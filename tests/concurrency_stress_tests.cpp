#include "executor/task_group.hpp"
#include "executor/thread_pool.hpp"

#include <atomic>
#include <cstdlib>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "TEST FAILED: " << message << '\n';
        std::abort();
    }
}

void test_many_concurrent_producers() {
    constexpr int producer_count = 8;
    constexpr int tasks_per_producer = 2'000;

    executor::ThreadPool pool{4};

    std::atomic<int> completed{0};
    std::atomic<bool> start{false};
    std::vector<std::thread> producers;

    producers.reserve(producer_count);

    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) {
                // Wait until every producer is ready.
            }

            for (int task = 0; task < tasks_per_producer; ++task) {
                pool.execute([&completed] {
                    completed.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    start.store(true, std::memory_order_release);

    for (std::thread& producer : producers) {
        producer.join();
    }

    pool.shutdown();

    check(
        completed.load(std::memory_order_relaxed) ==
            producer_count * tasks_per_producer,
        "every concurrently submitted task should run exactly once"
    );
}

void test_repeated_pool_lifecycle() {
    constexpr int iterations = 200;
    constexpr int tasks_per_iteration = 100;

    for (int iteration = 0; iteration < iterations; ++iteration) {
        executor::ThreadPool pool{4};
        std::atomic<int> completed{0};

        for (int task = 0; task < tasks_per_iteration; ++task) {
            pool.execute([&completed] {
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }

        pool.shutdown();

        check(
            completed.load(std::memory_order_relaxed) ==
                tasks_per_iteration,
            "every task should finish before shutdown returns"
        );
    }
}

void test_future_exception_does_not_kill_pool() {
    executor::ThreadPool pool{2};

    auto failing = pool.submit([]() -> int {
        throw std::runtime_error{"expected failure"};
    });

    bool received_exception = false;

    try {
        static_cast<void>(failing.get());
    } catch (const std::runtime_error& error) {
        received_exception =
            std::string{error.what()} == "expected failure";
    }

    check(received_exception, "future.get() should rethrow task exception");

    auto successful = pool.submit([] {
        return 42;
    });

    check(
        successful.get() == 42,
        "worker should remain usable after another task throws"
    );

    pool.shutdown();
}

void test_task_group_completion_and_exception() {
    executor::ThreadPool pool{4};

    {
        executor::TaskGroup group{pool};
        std::atomic<int> completed{0};

        constexpr int task_count = 5'000;

        for (int task = 0; task < task_count; ++task) {
            group.submit([&completed] {
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }

        auto failing = group.submit([]() -> int {
            throw std::runtime_error{"group task failed"};
        });

        group.wait();

        check(
            completed.load(std::memory_order_relaxed) == task_count,
            "group.wait() should return only after every normal task finishes"
        );

        bool received_exception = false;

        try {
            static_cast<void>(failing.get());
        } catch (const std::runtime_error&) {
            received_exception = true;
        }

        check(
            received_exception,
            "a failed group task should still complete group bookkeeping"
        );
    }

    pool.shutdown();
}

void test_group_submission_after_pool_shutdown() {
    executor::ThreadPool pool{2};
    executor::TaskGroup group{pool};

    pool.shutdown();

    bool rejected = false;

    try {
        static_cast<void>(group.submit([] {
            return 42;
        }));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    check(rejected, "a group should reject work once its pool is shut down");

    group.wait();
}

int main() {
    test_many_concurrent_producers();
    test_repeated_pool_lifecycle();
    test_future_exception_does_not_kill_pool();
    test_task_group_completion_and_exception();
    test_group_submission_after_pool_shutdown();

    std::cout << "All concurrency stress tests passed.\n";
}