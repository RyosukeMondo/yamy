// platform_paths.h - Platform-agnostic path utilities

#ifndef _PLATFORM_PATHS_H
#define _PLATFORM_PATHS_H

#include <string>

namespace yamy {
namespace platform {

/// Get the directory containing the current executable
/// @return Directory path (without trailing slash/backslash)
std::string getExecutableDirectory();

} // namespace platform
} // namespace yamy

#endif // _PLATFORM_PATHS_H
