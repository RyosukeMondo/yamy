#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sync.h - Platform-agnostic synchronization primitives

#ifndef _PLATFORM_SYNC_H
#define _PLATFORM_SYNC_H

#include <cstdint>
#include "types.h"

namespace yamy::platform {
    /// Wait result enumeration
    enum class WaitResult {
        Success,    /// Object signaled
        Timeout,    /// Wait timed out
        Failed,     /// Wait failed
        Abandoned   /// Mutex abandoned (if applicable)
    };

    /// Infinite wait timeout constant
    constexpr uint32_t WAIT_INFINITE = 0xFFFFFFFF;

    /// Wait for an object to be signaled
    /// @param handle Object handle (EventHandle, MutexHandle, ThreadHandle, etc.)
    /// @param timeout_ms Timeout in milliseconds (use WAIT_INFINITE for no timeout)
    /// @return WaitResult indicating success, timeout, or failure
    WaitResult waitForObject(void* handle, uint32_t timeout_ms);

    // ========== Event primitives ==========
    /// Create an event object
    /// @param manual_reset If true, event requires manual reset; if false, auto-resets after signaling
    /// @param initial_state If true, event starts in signaled state
    /// @return Event handle, or nullptr on failure
    EventHandle createEvent(bool manual_reset, bool initial_state);

    /// Signal an event (wake waiting threads)
    /// @param event Event handle
    /// @return true on success, false on failure
    bool setEvent(EventHandle event);

    /// Reset an event to non-signaled state
    /// @param event Event handle
    /// @return true on success, false on failure
    bool resetEvent(EventHandle event);

    /// Destroy an event
    /// @param event Event handle
    /// @return true on success, false on failure
    bool destroyEvent(EventHandle event);

    // ========== Mutex primitives ==========
    /// Create a mutex object
    /// @return Mutex handle, or nullptr on failure
    MutexHandle createMutex();

    /// Acquire a mutex (lock)
    /// @param mutex Mutex handle
    /// @param timeout_ms Timeout in milliseconds (use WAIT_INFINITE for no timeout)
    /// @return WaitResult indicating success, timeout, or failure
    WaitResult acquireMutex(MutexHandle mutex, uint32_t timeout_ms = WAIT_INFINITE);

    /// Release a mutex (unlock)
    /// @param mutex Mutex handle
    /// @return true on success, false on failure
    bool releaseMutex(MutexHandle mutex);

    /// Destroy a mutex
    /// @param mutex Mutex handle
    /// @return true on success, false on failure
    bool destroyMutex(MutexHandle mutex);

} // namespace yamy::platform

#endif // _PLATFORM_SYNC_H
