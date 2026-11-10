#include "executor/thread_pool.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#define N_THREADS 2

int add(int x, int y) { return x + y; }

int main() {
    executor::ThreadPool pool(N_THREADS);

    // test that submit can receive multiple types of functions with different arguments
    auto answer = pool.submit([] {
        return 22;
    });

    auto sum = pool.submit(add, 10, 12);

    auto str = pool.submit([] {
        return "Hello world!";
    });

    std::cout << "Expected:\t" << answer.get() << "\nActual:\t\t" << sum.get()
        << "\nString:\t\t" << str.get() << "\n";
    
    // test a failing task
    auto fails = pool.submit([]() -> int {
        throw std::runtime_error("This is a test error.");
    });

    try {
        int result = fails.get();
    }
    catch (const std::runtime_error &err) {
        std::cerr << "Task failed:\t" << err.what() << "\n";
    }

    pool.shutdown();

    return 0;
}