#ifndef SCHEDULER_TYPE_H
#define SCHEDULER_TYPE_H

namespace executor {

enum SchedulerType {
    SHARED_QUEUE,
    WORK_STEALING_QUEUE
};

} // namespace

#endif