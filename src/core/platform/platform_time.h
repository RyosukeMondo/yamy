// platform_time.h - Platform-agnostic time utilities

#ifndef _PLATFORM_TIME_H
#define _PLATFORM_TIME_H

#include <ctime>

namespace yamy {
namespace platform {

/// Cross-platform localtime (thread-safe)
/// Converts time_t to std::tm in a thread-safe manner
inline std::tm* localtime_safe(const std::time_t* timer, std::tm* buf) {
#ifdef _WIN32
    localtime_s(buf, timer);
    return buf;
#else
    return localtime_r(timer, buf);
#endif
}

} // namespace platform
} // namespace yamy

#endif // _PLATFORM_TIME_H
