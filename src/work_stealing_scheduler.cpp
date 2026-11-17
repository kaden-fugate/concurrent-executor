#include "executor/detail/work_stealing_scheduler.hpp"

#include <stdexcept>
#include <utility>

namespace executor::detail {

WorkStealingScheduler::WorkStealingScheduler(std::size_t) {
    throw std::runtime_error("Implementation incomplete.\n");
}

void WorkStealingScheduler::enqueue(Task task) {
    throw std::runtime_error("Implementation incomplete.\n");
}

bool WorkStealingScheduler::wait_and_take(Task &task, std::size_t) {
    throw std::runtime_error("Implementation incomplete.\n");
    return true;
}

void WorkStealingScheduler::close() {
    throw std::runtime_error("Implementation incomplete.\n");
}

} // namespace