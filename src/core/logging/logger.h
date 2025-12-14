#pragma once

/**
 * @file logger.h
 * @brief Thread-safe logging subsystem for YAMY.
 *
 * Provides the Logger singleton for emitting log messages to registered listeners.
 * Supports multiple listeners (console, file, GUI, etc.) via callback registration.
 */

#include "log_entry.h"
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace yamy {
namespace logging {

/**
 * @brief Thread-safe logging system (singleton).
 *
 * The Logger provides centralized logging with support for multiple output
 * listeners. Log messages are emitted to all registered listeners synchronously
 * under mutex protection.
 *
 * @note This is a transitional logger. For high-performance logging in the
 *       input processing hot path, use Quill instead.
 *
 * @code
 * // Emit a log message
 * Logger::getInstance().log(LogLevel::Info, "Engine", "Initialized");
 *
 * // Register a listener
 * Logger::getInstance().addListener([](const LogEntry& entry) {
 *     std::cout << entry.format() << std::endl;
 * });
 * @endcode
 */
class Logger {
public:
  /**
   * @brief Type for log listener callbacks.
   *
   * Listeners receive each log entry as it is emitted.
   */
  using Listener = std::function<void(const LogEntry &)>;

  /**
   * @brief Get the singleton Logger instance.
   *
   * @return Reference to the global Logger instance
   *
   * @note Thread-safe initialization (C++11 magic statics)
   */
  static Logger &getInstance();

  /**
   * @brief Emit a log message.
   *
   * Creates a LogEntry and dispatches it to all registered listeners.
   *
   * @param level Severity level of the message
   * @param category Log category (e.g., "Engine", "Input", "Config")
   * @param message The log message text
   *
   * @note Thread-safe. Multiple threads can call log() concurrently.
   *
   * @code
   * Logger::getInstance().log(LogLevel::Warning, "Driver", "Retrying connection");
   * @endcode
   */
  void log(LogLevel level, std::string category, std::string message);

  /**
   * @brief Register a log listener.
   *
   * The listener will be called for every log message emitted after registration.
   *
   * @param listener Callback function to receive log entries
   *
   * @note Thread-safe. Can be called while logging is active.
   * @note Listeners are called synchronously on the logging thread.
   *
   * @code
   * Logger::getInstance().addListener([](const LogEntry& entry) {
   *     if (entry.level >= LogLevel::Error) {
   *         std::cerr << entry.format() << std::endl;
   *     }
   * });
   * @endcode
   */
  void addListener(Listener listener);

private:
  Logger();
  ~Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  std::mutex m_mutex;           ///< Protects m_listeners access
  std::vector<Listener> m_listeners; ///< Registered log listeners
};

} // namespace logging
} // namespace yamy
