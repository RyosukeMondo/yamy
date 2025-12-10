//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sync_linux.cpp - POSIX synchronization implementation (Track 6)

#include "../../core/platform/sync.h"
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

namespace yamy::platform {

WaitResult waitForObject(void* handle, uint32_t timeout_ms) {
    if (!handle) {
        return WaitResult::Failed;
    }

    sem_t* sem = static_cast<sem_t*>(handle);
    int result = 0;

    if (timeout_ms == WAIT_INFINITE) {
        // Infinite wait
        do {
            result = sem_wait(sem);
        } while (result == -1 && errno == EINTR);
    } else {
        // Timed wait
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            return WaitResult::Failed;
        }

        // Add timeout to current time
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;

        // Handle nanosecond overflow
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        do {
            result = sem_timedwait(sem, &ts);
        } while (result == -1 && errno == EINTR);
    }

    if (result == 0) {
        return WaitResult::Success;
    }

    if (errno == ETIMEDOUT) {
        return WaitResult::Timeout;
    }

    return WaitResult::Failed;
}

} // namespace yamy::platform
