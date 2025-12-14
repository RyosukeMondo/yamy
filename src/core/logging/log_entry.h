#pragma once

/**
 * @file log_entry.h
 * @brief Logging structures and level definitions for YAMY.
 *
 * Defines LogLevel enumeration and LogEntry structure for capturing
 * and formatting log messages.
 */

#include <chrono>
#include <string>

namespace yamy {
namespace logging {

/**
 * @brief Log severity levels.
 *
 * Ordered from lowest to highest severity.
 */
enum class LogLevel {
  Trace,   ///< Detailed trace information (verbose)
  Info,    ///< Informational messages
  Warning, ///< Warning messages (potential issues)
  Error,   ///< Error messages (actual failures)
};

/**
 * @brief Represents a single log entry.
 *
 * Immutable structure containing timestamp, severity level, category, and message.
 * Used by the Logger to emit structured log data to listeners.
 *
 * @code
 * LogEntry entry(LogLevel::Info, "Engine", "Started successfully");
 * std::string formatted = entry.format();
 * // Output: "[2025-01-15 14:30:00] [INFO] [Engine] Started successfully"
 * @endcode
 */
struct LogEntry {
  using clock = std::chrono::system_clock;

  /**
   * @brief Construct a log entry.
   *
   * @param level Severity level of the message
   * @param category Log category (e.g., "Engine", "Input", "Config")
   * @param message The log message text
   *
   * @post timestamp is set to current system time
   */
  LogEntry(LogLevel level, std::string category, std::string message);

  /**
   * @brief Format the log entry as a human-readable string.
   *
   * @return Formatted string: "[timestamp] [level] [category] message"
   *
   * @code
   * LogEntry entry(LogLevel::Error, "Driver", "Failed to open device");
   * std::cout << entry.format() << std::endl;
   * @endcode
   */
  std::string format() const;

  const clock::time_point timestamp; ///< Time when log entry was created
  const LogLevel level;              ///< Severity level
  const std::string category;        ///< Log category
  const std::string message;         ///< Log message text
};

} // namespace logging
} // namespace yamy
