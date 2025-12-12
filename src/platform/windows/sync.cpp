//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sync.cpp - Windows synchronization implementation

#include "../../core/platform/sync.h"

#ifdef _WIN32
#include <windows.h>

namespace yamy::platform {

WaitResult waitForObject(void* handle, uint32_t timeout_ms) {
    if (!handle) {
        return WaitResult::Failed;
    }

    DWORD result = WaitForSingleObject(static_cast<HANDLE>(handle), timeout_ms);

    switch (result) {
        case WAIT_OBJECT_0:
            return WaitResult::Success;
        case WAIT_TIMEOUT:
            return WaitResult::Timeout;
        case WAIT_ABANDONED:
            return WaitResult::Abandoned;
        case WAIT_FAILED:
        default:
            return WaitResult::Failed;
    }
}

// ========== Event primitives ==========
EventHandle createEvent(bool manual_reset, bool initial_state) {
    return CreateEvent(nullptr, manual_reset ? TRUE : FALSE, initial_state ? TRUE : FALSE, nullptr);
}

bool setEvent(EventHandle event) {
    if (!event) return false;
    return SetEvent(static_cast<HANDLE>(event)) != 0;
}

bool resetEvent(EventHandle event) {
    if (!event) return false;
    return ResetEvent(static_cast<HANDLE>(event)) != 0;
}

bool destroyEvent(EventHandle event) {
    if (!event) return false;
    return CloseHandle(static_cast<HANDLE>(event)) != 0;
}

// ========== Mutex primitives ==========
MutexHandle createMutex() {
    return CreateMutex(nullptr, FALSE, nullptr);
}

WaitResult acquireMutex(MutexHandle mutex, uint32_t timeout_ms) {
    return waitForObject(mutex, timeout_ms);
}

bool releaseMutex(MutexHandle mutex) {
    if (!mutex) return false;
    return ReleaseMutex(static_cast<HANDLE>(mutex)) != 0;
}

bool destroyMutex(MutexHandle mutex) {
    if (!mutex) return false;
    return CloseHandle(static_cast<HANDLE>(mutex)) != 0;
}

} // namespace yamy::platform

#endif // _WIN32
