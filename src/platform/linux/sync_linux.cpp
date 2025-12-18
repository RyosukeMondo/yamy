//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sync_linux.cpp - POSIX synchronization implementation (Track 6)

// Define _GNU_SOURCE for pthread_tryjoin_np
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../../core/platform/sync.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <cstdlib>

namespace yamy::platform {

// Event structure using condition variables for proper manual/auto-reset semantics
struct Event {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool signaled;
    bool manual_reset;
};

WaitResult waitForObject(void* handle, uint32_t timeout_ms) {
    if (!handle) {
        return WaitResult::Failed;
    }

    // Try to detect if this is a ThreadHandle or an Event
    // ThreadHandles are pthread_t values cast to void*
    // Events are pointers to our Event struct
    // We use a heuristic: if the value looks like a valid pthread_t (large integer),
    // and pthread_tryjoin_np succeeds or would wait, it's likely a thread
    pthread_t thread = (pthread_t)handle;

    // Try non-blocking join to detect if it's a valid thread
    int tryjoin_result = pthread_tryjoin_np(thread, nullptr);

    if (tryjoin_result == 0) {
        // Thread already exited
        return WaitResult::Success;
    } else if (tryjoin_result == EBUSY) {
        // Valid thread, still running - wait for it
        if (timeout_ms == WAIT_INFINITE) {
            // Join with infinite wait
            int result = pthread_join(thread, nullptr);
            return (result == 0) ? WaitResult::Success : WaitResult::Failed;
        } else {
            // pthread_join doesn't support timeouts, so we need to poll
            // This is not ideal but necessary for timeout support
            uint32_t elapsed = 0;
            const uint32_t poll_interval = 10; // Poll every 10ms

            while (elapsed < timeout_ms) {
                int result = pthread_tryjoin_np(thread, nullptr);
                if (result == 0) {
                    return WaitResult::Success;
                } else if (result != EBUSY) {
                    return WaitResult::Failed;
                }

                // Sleep for poll interval
                struct timespec ts;
                ts.tv_sec = 0;
                ts.tv_nsec = poll_interval * 1000000;
                nanosleep(&ts, nullptr);
                elapsed += poll_interval;
            }
            return WaitResult::Timeout;
        }
    }

    // Not a thread, assume it's an Event
    Event* event = static_cast<Event*>(handle);

    pthread_mutex_lock(&event->mutex);

    // Wait while not signaled
    while (!event->signaled) {
        if (timeout_ms == WAIT_INFINITE) {
            // Infinite wait
            int result = pthread_cond_wait(&event->cond, &event->mutex);
            if (result != 0 && result != EINTR) {
                pthread_mutex_unlock(&event->mutex);
                return WaitResult::Failed;
            }
        } else {
            // Timed wait
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
                pthread_mutex_unlock(&event->mutex);
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

            int result = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
            if (result == ETIMEDOUT) {
                pthread_mutex_unlock(&event->mutex);
                return WaitResult::Timeout;
            }
            if (result != 0 && result != EINTR) {
                pthread_mutex_unlock(&event->mutex);
                return WaitResult::Failed;
            }
        }
    }

    // If auto-reset event, clear the signal after waking one waiter
    if (!event->manual_reset) {
        event->signaled = false;
    }

    pthread_mutex_unlock(&event->mutex);
    return WaitResult::Success;
}

// ========== Event primitives ==========
EventHandle createEvent(bool manual_reset, bool initial_state) {
    Event* event = new Event();
    if (!event) return nullptr;

    // Create mutex attributes to disable priority protection
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        delete event;
        return nullptr;
    }

    // Disable priority protection protocol to avoid privilege requirements
    if (pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE) != 0) {
        pthread_mutexattr_destroy(&attr);
        delete event;
        return nullptr;
    }

    if (pthread_mutex_init(&event->mutex, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        delete event;
        return nullptr;
    }

    pthread_mutexattr_destroy(&attr);

    if (pthread_cond_init(&event->cond, nullptr) != 0) {
        pthread_mutex_destroy(&event->mutex);
        delete event;
        return nullptr;
    }

    event->signaled = initial_state;
    event->manual_reset = manual_reset;

    return event;
}

bool setEvent(EventHandle event) {
    if (!event) return false;

    Event* ev = static_cast<Event*>(event);
    pthread_mutex_lock(&ev->mutex);

    ev->signaled = true;

    // For manual-reset events, wake all waiters
    // For auto-reset events, wake one waiter
    if (ev->manual_reset) {
        pthread_cond_broadcast(&ev->cond);
    } else {
        pthread_cond_signal(&ev->cond);
    }

    pthread_mutex_unlock(&ev->mutex);
    return true;
}

bool resetEvent(EventHandle event) {
    if (!event) return false;

    Event* ev = static_cast<Event*>(event);
    pthread_mutex_lock(&ev->mutex);
    ev->signaled = false;
    pthread_mutex_unlock(&ev->mutex);

    return true;
}

bool destroyEvent(EventHandle event) {
    if (!event) return false;

    Event* ev = static_cast<Event*>(event);
    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
    delete ev;

    return true;
}

// ========== Mutex primitives ==========
MutexHandle createMutex() {
    pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(malloc(sizeof(pthread_mutex_t)));
    if (!mutex) return nullptr;

    // Create mutex attributes to disable priority protection
    // This prevents glibc pthread_tpp_change_priority errors in test environments
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        free(mutex);
        return nullptr;
    }

    // Disable priority protection protocol (TPP) to avoid privilege requirements
    if (pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE) != 0) {
        pthread_mutexattr_destroy(&attr);
        free(mutex);
        return nullptr;
    }

    if (pthread_mutex_init(mutex, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        free(mutex);
        return nullptr;
    }

    pthread_mutexattr_destroy(&attr);
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
