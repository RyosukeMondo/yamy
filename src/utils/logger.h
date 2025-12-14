#pragma once

#include <quill/LogLevel.h>
#include <quill/Quill.h>

namespace yamy::log {

// Initializes the global Quill logger with YAMY defaults (RDTSC clock + JSON handler).
void init();

// Returns the initialized logger, initializing on first use.
quill::Logger* logger();

// Flushes all pending log messages.
void flush();

}  // namespace yamy::log

#define LOG_DEBUG(...) QUILL_LOG_DEBUG(::yamy::log::logger(), __VA_ARGS__)
#define LOG_INFO(...)  QUILL_LOG_INFO(::yamy::log::logger(), __VA_ARGS__)
#define LOG_ERROR(...) QUILL_LOG_ERROR(::yamy::log::logger(), __VA_ARGS__)
