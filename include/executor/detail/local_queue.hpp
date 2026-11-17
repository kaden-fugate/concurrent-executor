#ifndef LOCAL_QUEUE_H
#define LOCAL_QUEUE_H

#include <functional>
#include <mutex>
#include <queue>

namespace executor::detail {

using Task = std::function<void()>;

typedef struct LocalQueue {
    std::mutex _mutex;
    std::deque<Task> _tasks;
} LocalQueue;

}

#endif