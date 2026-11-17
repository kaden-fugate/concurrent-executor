#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <cstddef>
#include <functional>

namespace executor::detail {

using Task   = std::function<void()>;
using size_t = std::size_t;

class Scheduler {
public:
    virtual ~Scheduler() = default;

    // queue task, throw error if closed
    virtual void enqueue_global(Task)        = 0;
    virtual void enqueue_local(size_t, Task) = 0;

    // wait until work is avail or shutdown is complete
    //     true:  when work is obtained and assigned
    //     false: when closed and fully drained
    virtual bool wait_and_take(Task&, size_t) = 0;

    // reject submissions but make sure that queued work can drain
    virtual void close() = 0;
};

} // namespace

#endif