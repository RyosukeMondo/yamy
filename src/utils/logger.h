#pragma once

// X11 headers (included transitively via Qt) define macros that conflict with Quill's LogLevel enum
// Save the X11 macro values, undef them for Quill, then restore them
#ifdef Dynamic
#define YAMY_X11_DYNAMIC_SAVED Dynamic
#undef Dynamic
#endif
#ifdef None
#define YAMY_X11_NONE_SAVED None
#undef None
#endif

// Configure Quill
#ifndef QUILL_QUIET
#define QUILL_QUIET
#endif

#define QUILL_USE_EXCEPTIONS
#include <quill/LogLevel.h>
#include <quill/Quill.h>

// Restore X11 macros after Quill headers are processed
// Note: We must restore the original value (0L for None, 0 for Dynamic) directly
// rather than using the saved macro name, since we're about to undefine it
#ifdef YAMY_X11_DYNAMIC_SAVED
#undef YAMY_X11_DYNAMIC_SAVED
#define Dynamic 0
#endif
#ifdef YAMY_X11_NONE_SAVED
#undef YAMY_X11_NONE_SAVED
#define None 0L
#endif

namespace yamy::log {

// Initializes the global Quill logger with YAMY defaults (RDTSC clock + JSON handler).
void init();

// Returns the initialized logger, initializing on first use.
quill::Logger* logger();

// Flushes all pending log messages.
void flush();

}  // namespace yamy::log

// Undefine Quill's convenience macros to define our own with automatic logger()
#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_WARN
#undef LOG_WARN
#endif
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

#define LOG_DEBUG(...) QUILL_LOG_DEBUG(::yamy::log::logger(), __VA_ARGS__)
#define LOG_INFO(...)  QUILL_LOG_INFO(::yamy::log::logger(), __VA_ARGS__)
#define LOG_WARN(...)  QUILL_LOG_WARNING(::yamy::log::logger(), __VA_ARGS__)
#define LOG_ERROR(...) QUILL_LOG_ERROR(::yamy::log::logger(), __VA_ARGS__)
