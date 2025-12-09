#pragma once
#include <cstdint>

namespace yamy::platform {

// Cross-platform sleep function
// Sleeps for the specified number of milliseconds
void sleep_ms(uint32_t milliseconds);

} // namespace yamy::platform
