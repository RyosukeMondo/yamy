#pragma once

#include "log_entry.h"
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace yamy {
namespace logging {

class Logger {
public:
  using Listener = std::function<void(const LogEntry &)>;

  static Logger &getInstance();

  void log(LogLevel level, std::string category, std::string message);

  void addListener(Listener listener);

private:
  Logger();
  ~Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  std::mutex m_mutex;
  std::vector<Listener> m_listeners;
};

} // namespace logging
} // namespace yamy
