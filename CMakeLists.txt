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
    src/task_group.cpp
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

add_executable(basic_thread_pool_execute
    examples/basic_thread_pool_execute.cpp
)

add_executable(basic_thread_pool_submit
    examples/basic_thread_pool_submit.cpp
)

add_executable(basic_task_group
    examples/basic_task_group.cpp
)

target_link_libraries(basic_thread_pool_execute PRIVATE executor)
target_link_libraries(basic_thread_pool_submit PRIVATE executor)
target_link_libraries(basic_task_group PRIVATE executor)

add_executable(thread_pool_tests_execute
    tests/thread_pool_tests_execute.cpp
)

add_executable(thread_pool_tests_submit
    tests/thread_pool_tests_submit.cpp
)

target_link_libraries(thread_pool_tests_execute PRIVATE executor)
target_link_libraries(thread_pool_tests_submit PRIVATE executor)

enable_testing()
add_test(NAME thread_pool_tests COMMAND thread_pool_tests)