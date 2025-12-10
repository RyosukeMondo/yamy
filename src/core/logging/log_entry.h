#pragma once

#include <chrono>
#include <string>

namespace yamy {
namespace logging {

enum class LogLevel {
  Trace,
  Info,
  Warning,
  Error,
};

struct LogEntry {
  using clock = std::chrono::system_clock;

  LogEntry(LogLevel level, std::string category, std::string message);

  std::string format() const;

  const clock::time_point timestamp;
  const LogLevel level;
  const std::string category;
  const std::string message;
};

} // namespace logging
} // namespace yamy
