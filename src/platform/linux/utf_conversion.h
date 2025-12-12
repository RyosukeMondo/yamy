// utf_conversion.h - Linux stub (UTF-8 is native, no conversion needed)

#ifndef YAMY_PLATFORM_LINUX_UTF_CONVERSION_H
#define YAMY_PLATFORM_LINUX_UTF_CONVERSION_H

#include <string>

namespace yamy {
namespace platform {

// On Linux, UTF-8 is the native encoding, so these are identity/no-op functions
inline std::string utf8_to_wstring(const std::string& str) {
    return str;
}

inline std::string wstring_to_utf8(const std::string& str) {
    return str;
}

} // namespace platform
} // namespace yamy

#endif // YAMY_PLATFORM_LINUX_UTF_CONVERSION_H
