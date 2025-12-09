#include "core/platform/thread.h"
#include <thread>
#include <chrono>

namespace yamy::platform {

void sleep_ms(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

} // namespace yamy::platform
