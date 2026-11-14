#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <functional>

namespace executor::detail {

using Task = std::function<void()>;

class Scheduler {
public:
    virtual ~Scheduler() = default;

    // queue task, throw error if closed
    virtual void enqueue(Task) = 0;

    // wait until work is avail or shutdown is complete
    //     true:  when work is obtained and assigned
    //     false: when closed and fully drained
    virtual bool wait_and_take(Task&) = 0;

    // reject submissions but make sure that queued work can drain
    virtual void close() = 0;
};

} // namespace

#endif