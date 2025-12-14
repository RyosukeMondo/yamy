#pragma once

#include "logger.h"
#include <fmt/printf.h>

// Compatibility layer to route legacy PLATFORM_LOG_* macros through Quill.
// Uses fmt::sprintf to support existing printf-style format strings.
#define PLATFORM_LOG_DEBUG(component, fmt_str, ...) \
    LOG_DEBUG("[{}] {}", component, fmt::sprintf(fmt_str, ##__VA_ARGS__))

#define PLATFORM_LOG_INFO(component, fmt_str, ...) \
    LOG_INFO("[{}] {}", component, fmt::sprintf(fmt_str, ##__VA_ARGS__))

#define PLATFORM_LOG_WARN(component, fmt_str, ...) \
    LOG_WARN("[{}] {}", component, fmt::sprintf(fmt_str, ##__VA_ARGS__))

#define PLATFORM_LOG_ERROR(component, fmt_str, ...) \
    LOG_ERROR("[{}] {}", component, fmt::sprintf(fmt_str, ##__VA_ARGS__))
