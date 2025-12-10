#include "core/platform/thread.h"
#include <thread>
#include <chrono>
#include <pthread.h>
#include <sched.h>

namespace yamy::platform {

void sleep_ms(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

ThreadHandle createThread(ThreadRoutine routine, void* arg) {
    pthread_t thread;
    // pthread_create returns 0 on success
    if (pthread_create(&thread, nullptr, routine, arg) != 0) {
        return nullptr;
    }
    // Cast thread ID (usually unsigned long) to void* handle
    return (ThreadHandle)thread;
}

bool joinThread(ThreadHandle handle) {
    pthread_t thread = (pthread_t)handle;
    return pthread_join(thread, nullptr) == 0;
}

bool detachThread(ThreadHandle handle) {
    pthread_t thread = (pthread_t)handle;
    return pthread_detach(thread) == 0;
}

bool setThreadPriority(ThreadHandle handle, int priority) {
    pthread_t thread = (pthread_t)handle;
    struct sched_param param;
    int policy;

    if (pthread_getschedparam(thread, &policy, &param) != 0) {
        return false;
    }

    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);

    if (min_prio == -1 || max_prio == -1) {
        return false;
    }

    if (priority < min_prio) priority = min_prio;
    if (priority > max_prio) priority = max_prio;

    param.sched_priority = priority;
    return pthread_setschedparam(thread, policy, &param) == 0;
}

} // namespace yamy::platform
