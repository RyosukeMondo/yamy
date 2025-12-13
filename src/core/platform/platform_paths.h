// platform_paths.h - Platform-agnostic path utilities

#pragma once

#include <string>

namespace yamy {
namespace platform {

/// Get the directory containing the current executable
/// @return Directory path (without trailing slash/backslash)
std::string getExecutableDirectory();

} // namespace platform
} // namespace yamy
