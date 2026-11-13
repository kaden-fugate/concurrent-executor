cmake_minimum_required(VERSION 3.20)

project(concurrent_executor
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EXECUTOR_ENABLE_SANITIZERS
    "Build with sanitizers enabled"
    OFF
)

set(EXECUTOR_SANITIZERS
    "address,undefined"
    CACHE STRING
    "Comma-separated sanitizers: address,undefined or thread"
)

function(enable_executor_sanitizers target_name)
    if (EXECUTOR_ENABLE_SANITIZERS)
        target_compile_options(${target_name} PRIVATE
            -fsanitize=${EXECUTOR_SANITIZERS}
            -fno-omit-frame-pointer
        )

        target_link_options(${target_name} PRIVATE
            -fsanitize=${EXECUTOR_SANITIZERS}
        )
    endif()
endfunction()

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

add_executable(concurrency_stress_tests
    tests/concurrency_stress_tests.cpp
)

target_link_libraries(thread_pool_tests_execute PRIVATE executor)
target_link_libraries(thread_pool_tests_submit PRIVATE executor)
target_link_libraries(concurrency_stress_tests PRIVATE executor)

enable_executor_sanitizers(executor)

enable_executor_sanitizers(basic_thread_pool_execute)
enable_executor_sanitizers(basic_thread_pool_submit)
enable_executor_sanitizers(basic_task_group)

enable_executor_sanitizers(thread_pool_tests_execute)
enable_executor_sanitizers(thread_pool_tests_submit)
enable_executor_sanitizers(concurrency_stress_tests)

enable_testing()
add_test(
    NAME thread_pool_tests_execute
    COMMAND thread_pool_tests_execute
)

add_test(
    NAME thread_pool_tests_submit
    COMMAND thread_pool_tests_submit
)

add_test(
    NAME concurrency_stress_tests
    COMMAND concurrency_stress_tests
)

set_tests_properties(concurrency_stress_tests PROPERTIES
    TIMEOUT 60
)