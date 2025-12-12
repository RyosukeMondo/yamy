//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sync_linux.cpp - POSIX synchronization implementation (Track 6)

#include "../../core/platform/sync.h"
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <cstdlib>

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

// ========== Event primitives ==========
EventHandle createEvent(bool manual_reset, bool initial_state) {
    sem_t* sem = static_cast<sem_t*>(malloc(sizeof(sem_t)));
    if (!sem) return nullptr;

    if (sem_init(sem, 0, initial_state ? 1 : 0) == -1) {
        free(sem);
        return nullptr;
    }

    return sem;
}

bool setEvent(EventHandle event) {
    if (!event) return false;
    sem_t* sem = static_cast<sem_t*>(event);
    return sem_post(sem) == 0;
}

bool resetEvent(EventHandle event) {
    if (!event) return false;
    sem_t* sem = static_cast<sem_t*>(event);
    // Consume the semaphore value (non-blocking)
    while (sem_trywait(sem) == 0) {
        // Keep consuming until empty
    }
    return true;
}

bool destroyEvent(EventHandle event) {
    if (!event) return false;
    sem_t* sem = static_cast<sem_t*>(event);
    sem_destroy(sem);
    free(sem);
    return true;
}

// ========== Mutex primitives ==========
MutexHandle createMutex() {
    pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(malloc(sizeof(pthread_mutex_t)));
    if (!mutex) return nullptr;

    if (pthread_mutex_init(mutex, nullptr) != 0) {
        free(mutex);
        return nullptr;
    }

    return mutex;
}

WaitResult acquireMutex(MutexHandle mutex, uint32_t timeout_ms) {
    if (!mutex) return WaitResult::Failed;

    pthread_mutex_t* mtx = static_cast<pthread_mutex_t*>(mutex);

    if (timeout_ms == WAIT_INFINITE) {
        int result = pthread_mutex_lock(mtx);
        return (result == 0) ? WaitResult::Success : WaitResult::Failed;
    } else {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            return WaitResult::Failed;
        }

        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;

        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        int result = pthread_mutex_timedlock(mtx, &ts);
        if (result == 0) return WaitResult::Success;
        if (result == ETIMEDOUT) return WaitResult::Timeout;
        return WaitResult::Failed;
    }
}

bool releaseMutex(MutexHandle mutex) {
    if (!mutex) return false;
    pthread_mutex_t* mtx = static_cast<pthread_mutex_t*>(mutex);
    return pthread_mutex_unlock(mtx) == 0;
}

bool destroyMutex(MutexHandle mutex) {
    if (!mutex) return false;
    pthread_mutex_t* mtx = static_cast<pthread_mutex_t*>(mutex);
    pthread_mutex_destroy(mtx);
    free(mtx);
    return true;
}

} // namespace yamy::platform
