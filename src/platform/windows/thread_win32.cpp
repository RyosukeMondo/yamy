#include "core/platform/thread.h"
#include <windows.h>

namespace yamy::platform {

void sleep_ms(uint32_t milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
}

bool destroyThread(ThreadHandle handle) {
    if (!handle) return false;
    return CloseHandle(static_cast<HANDLE>(handle)) != 0;
}

} // namespace yamy::platform
