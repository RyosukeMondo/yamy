#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// platform_logger.h - Platform-agnostic logging utility for YAMY
//
// Provides structured logging with levels for platform operations:
// - DEBUG: Frequent operations (key events, window queries)
// - INFO: Important operations (device open/close, connection status)
// - WARN: Non-fatal issues (fallback paths, degraded functionality)
// - ERROR: Failures (device errors, connection failures)
//
// Usage:
//   PLATFORM_LOG_DEBUG("window", "getForegroundWindow() returned 0x%lx", hwnd);
//   PLATFORM_LOG_INFO("input", "Device opened: %s", devNode.c_str());
//   PLATFORM_LOG_ERROR("ipc", "Connection failed: %s", strerror(errno));
//

#ifndef _PLATFORM_LOGGER_H
#define _PLATFORM_LOGGER_H

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>
#include <atomic>

namespace yamy::platform {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    NONE = 4
};

class PlatformLogger {
public:
    static PlatformLogger& instance() {
        static PlatformLogger logger;
        return logger;
    }

    void setLevel(LogLevel level) {
        m_level.store(level);
    }

    LogLevel getLevel() const {
        return m_level.load();
    }

    void log(LogLevel level, const char* component, const char* format, ...) {
        if (level < m_level.load()) {
            return;
        }

        va_list args;
        va_start(args, format);
        logImpl(level, component, format, args);
        va_end(args);
    }

private:
    PlatformLogger() : m_level(LogLevel::INFO) {}
    ~PlatformLogger() = default;

    PlatformLogger(const PlatformLogger&) = delete;
    PlatformLogger& operator=(const PlatformLogger&) = delete;

    void logImpl(LogLevel level, const char* component, const char* format, va_list args) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Get timestamp
        time_t now = time(nullptr);
        struct tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &now);  // Windows uses localtime_s with swapped arguments
#else
        localtime_r(&now, &tm_buf);  // POSIX uses localtime_r
#endif

        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm_buf);

        // Format message
        char msgBuf[1024];
        vsnprintf(msgBuf, sizeof(msgBuf), format, args);

        // Output structured log: [TIMESTAMP] [LEVEL] [COMPONENT] MESSAGE
        const char* levelStr = levelToString(level);
        fprintf(stderr, "[%s] [%s] [%s] %s\n", timeBuf, levelStr, component, msgBuf);
    }

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::mutex m_mutex;
    std::atomic<LogLevel> m_level;
};

// Convenience macros for logging
#define PLATFORM_LOG_DEBUG(component, ...) \
    yamy::platform::PlatformLogger::instance().log( \
        yamy::platform::LogLevel::DEBUG, component, __VA_ARGS__)

#define PLATFORM_LOG_INFO(component, ...) \
    yamy::platform::PlatformLogger::instance().log( \
        yamy::platform::LogLevel::INFO, component, __VA_ARGS__)

#define PLATFORM_LOG_WARN(component, ...) \
    yamy::platform::PlatformLogger::instance().log( \
        yamy::platform::LogLevel::WARN, component, __VA_ARGS__)

#define PLATFORM_LOG_ERROR(component, ...) \
    yamy::platform::PlatformLogger::instance().log( \
        yamy::platform::LogLevel::ERROR, component, __VA_ARGS__)

} // namespace yamy::platform

#endif // _PLATFORM_LOGGER_H
