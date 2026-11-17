#include "executor/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

constexpr std::size_t worker_count = 8;
constexpr int branch_factor        = 6;
constexpr int tree_depth           = 7;
constexpr int benchmark_runs       = 30;

std::uint64_t cpu_work(std::uint64_t value, std::size_t iterations) {
    for (std::size_t i = 0; i < iterations; ++i) {
        value ^= value << 13;
        value ^= value >> 7;
        value ^= value << 17;
    }

    return value;
}

class RecursiveWork {
public:
    explicit RecursiveWork(executor::ThreadPool& pool)
        : _pool(pool) {}

    void start() {
        schedule(tree_depth, 1);
    }

    void wait() {
        while (_outstanding.load(std::memory_order_acquire) != 0) {
            const std::size_t observed =
                _outstanding.load(std::memory_order_relaxed);

            _outstanding.wait(
                observed,
                std::memory_order_relaxed
            );
        }
    }

    [[nodiscard]] std::uint64_t checksum() const {
        return _checksum.load(std::memory_order_relaxed);
    }

private:
    void schedule(int depth, std::uint64_t seed) {
        _pool.execute([this, depth, seed] {
            // most tasks are small, some are deliberately expensive.
            // this gives the runtime uneven work to rebalance.
            const std::size_t iterations =
                (seed % 17 == 0) ? 40'000 : 1'000;

            const std::uint64_t result =
                cpu_work(seed, iterations);

            _checksum.fetch_xor(
                result,
                std::memory_order_relaxed
            );

            if (depth > 0) {
                _outstanding.fetch_add(
                    branch_factor,
                    std::memory_order_relaxed
                );

                for (int child = 0; child < branch_factor; ++child) {
                    const std::uint64_t child_seed =
                        result +
                        static_cast<std::uint64_t>(child) +
                        1;

                    schedule(depth - 1, child_seed);
                }
            }

            const std::size_t remaining =
                _outstanding.fetch_sub(
                    1,
                    std::memory_order_release
                ) - 1;

            if (remaining == 0) {
                _outstanding.notify_one();
            }
        });
    }

    executor::ThreadPool& _pool;

    std::atomic<std::size_t> _outstanding{1};
    std::atomic<std::uint64_t> _checksum{0};
};

long long run_once(
    executor::SchedulerType scheduler_type,
    std::string_view scheduler_name
) {
    executor::ThreadPool pool{worker_count, scheduler_type};
    RecursiveWork work{pool};

    const auto start = std::chrono::steady_clock::now();

    work.start();
    work.wait();

    const auto end = std::chrono::steady_clock::now();

    pool.shutdown();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        ).count();

    std::cout << scheduler_name
              << ": " << elapsed << " ms"
              << ", checksum: " << work.checksum()
              << '\n';

    return elapsed;
}

} // namespace

int main() {
    std::cout << "Workers: " << worker_count
              << ", branch factor: " << branch_factor
              << ", tree depth: " << tree_depth
              << "\n\n";

    long long shared_sum = 0;
    long long stolen_sum = 0;
    for (int run = 0; run < benchmark_runs; ++run) {
        std::cout << "Run " << run + 1 << '\n';

        shared_sum += run_once(
            executor::SHARED_QUEUE,
            "Shared queue"
        );

        stolen_sum += run_once(
            executor::WORK_STEALING_QUEUE,
            "Work stealing"
        );

        std::cout << '\n';
    }
    std::cout << "Shared queue avg: " << shared_sum / benchmark_runs <<
        " Work stealing avg: " << stolen_sum / benchmark_runs << ".\n";
}