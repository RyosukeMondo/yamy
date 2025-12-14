#include "logger.h"

#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <quill/handlers/JsonConsoleHandler.h>

namespace yamy::log {
namespace {
std::once_flag init_flag;
quill::Logger* g_logger = nullptr;

std::shared_ptr<quill::Handler> make_json_console_handler() {
    auto handler = quill::create_handler<quill::JsonConsoleHandler>(
        "yamy_json_console", std::string{"stdout"}, stdout, quill::ConsoleColours{});
    handler->set_log_level(quill::LogLevel::Debug);
    return handler;
}
}  // namespace

void init() {
    std::call_once(init_flag, [] {
        quill::Config config;
        config.backend_thread_name = "yamy-log-backend";
        config.default_timestamp_clock_type = quill::TimestampClockType::Tsc;
        config.rdtsc_resync_interval = std::chrono::milliseconds{250};
        config.backend_thread_sleep_duration = std::chrono::nanoseconds{0};
        config.default_handlers = {make_json_console_handler()};

        quill::configure(config);
        quill::start();

        g_logger = quill::get_root_logger();
        g_logger->set_log_level(quill::LogLevel::Debug);
    });
}

quill::Logger* logger() {
    init();
    return g_logger;
}

void flush() {
    if (g_logger) {
        quill::flush();
    }
}

}  // namespace yamy::log
