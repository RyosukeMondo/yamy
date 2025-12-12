#include "logger.h"
#include "log_entry.h"
#include "../platform/platform_time.h"
#include <iostream>
#include <iomanip>

namespace yamy {
namespace logging {

LogEntry::LogEntry(LogLevel level, std::string category, std::string message)
    : timestamp(clock::now()), level(level), category(std::move(category)),
      message(std::move(message)) {}

std::string LogEntry::format() const {
  std::time_t time = clock::to_time_t(timestamp);
  std::tm tm;
  yamy::platform::localtime_safe(&time, &tm);

  char level_char;
  switch (level) {
  case LogLevel::Trace:
    level_char = 'T';
    break;
  case LogLevel::Info:
    level_char = 'I';
    break;
  case LogLevel::Warning:
    level_char = 'W';
    break;
  case LogLevel::Error:
    level_char = 'E';
    break;
  default:
    level_char = '?';
    break;
  }

  std::stringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [" << level_char << "] ["
     << category << "] " << message;
  return ss.str();
}

Logger &Logger::getInstance() {
  static Logger instance;
  return instance;
}

Logger::Logger() {
    addListener([](const LogEntry &entry) {
        std::cout << entry.format() << std::endl;
    });
}

void Logger::log(LogLevel level, std::string category, std::string message) {
  LogEntry entry(level, std::move(category), std::move(message));
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto &listener : m_listeners) {
    listener(entry);
  }
}

void Logger::addListener(Listener listener) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_listeners.push_back(std::move(listener));
}

} // namespace logging
} // namespace yamy
