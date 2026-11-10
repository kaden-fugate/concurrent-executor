cmake_minimum_required(VERSION 3.20)

project(concurrent_executor
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(executor
    src/thread_pool.cpp
)

target_include_directories(executor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(executor PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(basic_thread_pool
    examples/basic_thread_pool.cpp
)

target_link_libraries(basic_thread_pool PRIVATE executor)

add_executable(thread_pool_tests
    tests/thread_pool_tests.cpp
)

target_link_libraries(thread_pool_tests PRIVATE executor)

enable_testing()
add_test(NAME thread_pool_tests COMMAND thread_pool_tests)