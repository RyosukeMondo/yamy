#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sync.h - Platform-agnostic synchronization primitives

#ifndef _PLATFORM_SYNC_H
#define _PLATFORM_SYNC_H

#include <cstdint>

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

} // namespace yamy::platform

#endif // _PLATFORM_SYNC_H
