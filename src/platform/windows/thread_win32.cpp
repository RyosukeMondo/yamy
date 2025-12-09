#include "core/platform/thread.h"
#include <windows.h>

namespace yamy::platform {

void sleep_ms(uint32_t milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
}

} // namespace yamy::platform
