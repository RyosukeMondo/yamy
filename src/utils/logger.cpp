#include "logger.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <quill/handlers/JsonFileHandler.h>

namespace yamy::log {
namespace {
std::once_flag init_flag;
quill::Logger* g_logger = nullptr;

std::shared_ptr<quill::Handler> make_json_file_handler() {
    // Ensure logs directory exists
    std::filesystem::create_directories("logs");

    // Create JsonFileHandler for Quill 3.9.0
    // Constructor: (filename, mode, append_to_filename, file_event_notifier, do_fsync)
    // JsonFileHandler automatically outputs JSON format
    auto handler = std::make_shared<quill::JsonFileHandler>(
        std::filesystem::path("logs/yamy.json"),
        "a",  // append mode for continuous logging
        quill::FilenameAppend::None,
        quill::FileEventNotifier{},
        false  // do_fsync - false for better performance
    );

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
        // Use 100us sleep to avoid busy-wait (0 causes 100% CPU usage)
        config.backend_thread_sleep_duration = std::chrono::microseconds{100};
        config.default_handlers = {make_json_file_handler()};

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
