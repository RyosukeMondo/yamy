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

} // namespace yamy::platform

#endif // _WIN32
