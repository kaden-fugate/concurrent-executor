#include "executor/thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#define N_THREADS 2

int add(int x, int y) { return x + y; }

int main() {

    // return int, no params test
    {
        executor::ThreadPool pool(N_THREADS);

        auto result = pool.submit([] {
            return 22;
        });
        assert(result.get() == 22);

        pool.shutdown();
    }

    // return int, params test
    {
        executor::ThreadPool pool(N_THREADS);

        auto result = pool.submit(add, 10, 12);
        assert(result.get() == 22);

        pool.shutdown();
    }

    // return string, no params test
    {
        executor::ThreadPool pool(N_THREADS);

        auto result = pool.submit([] {
            return "Hello world!";
        });
        assert(std::string(result.get()) == "Hello world!");

        pool.shutdown();
    }

    // error test for submit -> exception escalation
    {
        executor::ThreadPool pool(N_THREADS);

        auto result = pool.submit([]() -> int {
            throw std::runtime_error("This is a test error.");
        });

        bool err_caught = false;
        try {
            int _result = result.get();
        }
        catch (const std::runtime_error &err) {
            assert(std::string(err.what()) == "This is a test error.");
            err_caught = true;
        }
        assert(err_caught);

        // make sure that the pool still works after an error is encountered
        auto _result = pool.submit([] {
            return 2;
        });
        assert(_result.get() == 2);

        pool.shutdown();
    }

    return 0;
}