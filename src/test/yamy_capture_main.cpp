//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy-capture - Output verification tool for YAMY testing
//
// Monitors YAMY virtual keyboard output and captures events for verification

#include "test_scenario.h"
#include "test_scenario_json.h"
#include "../../platform/linux/keycode_mapping.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <poll.h>
#include <chrono>
#include <thread>
#include <mutex>

using namespace yamy::test;
using namespace std::chrono;

/// Output capturer - monitors YAMY virtual keyboard
class OutputCapturer {
private:
    int m_fd;
    std::vector<CapturedEvent> m_events;
    std::mutex m_mutex;
    bool m_running;
    std::thread m_captureThread;
    steady_clock::time_point m_start_time;

    std::string findYamyDevice() {
        DIR* dir = opendir("/dev/input");
        if (!dir) {
            std::cerr << "Failed to open /dev/input: " << strerror(errno) << std::endl;
            return "";
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "event", 5) != 0) {
                continue;
            }

            std::string devPath = std::string("/dev/input/") + entry->d_name;
            int fd = open(devPath.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            char name[256] = {0};
            if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
                std::string deviceName(name);
                if (deviceName.find("Yamy Virtual") != std::string::npos ||
                    deviceName.find("YAMY Virtual") != std::string::npos) {
                    close(fd);
                    closedir(dir);
                    return devPath;
                }
            }
            close(fd);
        }

        closedir(dir);
        return "";
    }

    void captureLoop() {
        struct pollfd pfd;
        pfd.fd = m_fd;
        pfd.events = POLLIN;

        while (m_running) {
            int ret = poll(&pfd, 1, 100); // 100ms timeout
            if (ret < 0) {
                if (errno != EINTR) {
                    std::cerr << "[Capturer] Poll error: " << strerror(errno) << std::endl;
                    break;
                }
                continue;
            }

            if (ret == 0) continue; // Timeout

            struct input_event ev;
            ssize_t n = read(m_fd, &ev, sizeof(ev));
            if (n == sizeof(ev) && ev.type == EV_KEY) {
                std::lock_guard<std::mutex> lock(m_mutex);

                CapturedEvent captured;
                captured.evdev_code = ev.code;
                captured.key_name = yamy::platform::getKeyName(ev.code);
                captured.type = (ev.value == 1) ? EventType::PRESS : EventType::RELEASE;

                auto now = steady_clock::now();
                captured.timestamp = duration_cast<microseconds>(now.time_since_epoch());
                captured.latency = duration_cast<microseconds>(now - m_start_time);

                m_events.push_back(captured);
            }
        }
    }

public:
    OutputCapturer() : m_fd(-1), m_running(false) {}

    ~OutputCapturer() {
        stop();
    }

    bool start(bool verbose = true) {
        std::string devPath = findYamyDevice();
        if (devPath.empty()) {
            std::cerr << "YAMY virtual keyboard not found. Is YAMY daemon running?" << std::endl;
            return false;
        }

        if (verbose) {
            std::cout << "Found YAMY virtual keyboard: " << devPath << std::endl;
        }

        m_fd = open(devPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (m_fd < 0) {
            std::cerr << "Failed to open device: " << strerror(errno) << std::endl;
            return false;
        }

        m_start_time = steady_clock::now();
        m_running = true;
        m_captureThread = std::thread(&OutputCapturer::captureLoop, this);

        if (verbose) {
            std::cout << "Capture started\n" << std::endl;
        }

        return true;
    }

    void stop() {
        if (m_running) {
            m_running = false;
            if (m_captureThread.joinable()) {
                m_captureThread.join();
            }
        }
        if (m_fd >= 0) {
            close(m_fd);
            m_fd = -1;
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_events.clear();
        m_start_time = steady_clock::now();
    }

    std::vector<CapturedEvent> getEvents() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events;
    }

    size_t getEventCount() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events.size();
    }

    bool waitForEvents(size_t count, uint32_t timeout_ms) {
        auto start = steady_clock::now();
        auto timeout = milliseconds(timeout_ms);

        while (steady_clock::now() - start < timeout) {
            if (getEventCount() >= count) {
                return true;
            }
            std::this_thread::sleep_for(milliseconds(10));
        }

        return false;
    }
};

/// CLI tool class
class YamyCaptureTool {
private:
    OutputCapturer m_capturer;

public:
    void printUsage() {
        std::cout << "yamy-capture - Output verification tool for YAMY testing\n\n";
        std::cout << "Usage:\n";
        std::cout << "  yamy-capture [options]\n\n";

        std::cout << "Options:\n";
        std::cout << "  --timeout <ms>      Capture timeout in milliseconds (default: 1000)\n";
        std::cout << "  --count <n>         Stop after capturing N events\n";
        std::cout << "  --until-key <code>  Stop when specific key is captured\n";
        std::cout << "  --format <type>     Output format: json, human (default: json)\n";
        std::cout << "  --stream            Stream events in real-time\n";
        std::cout << "  --quiet             Suppress informational output\n";
        std::cout << "  --help              Show this help\n\n";

        std::cout << "Examples:\n";
        std::cout << "  yamy-capture --timeout 2000                # Capture for 2 seconds\n";
        std::cout << "  yamy-capture --count 10                    # Capture 10 events\n";
        std::cout << "  yamy-capture --until-key 1                 # Capture until ESC\n";
        std::cout << "  yamy-capture --format human                # Human-readable output\n";
        std::cout << "  yamy-capture --stream                      # Real-time streaming\n\n";

        std::cout << "Output:\n";
        std::cout << "  JSON format includes:\n";
        std::cout << "    - captured_events: Array of captured events with timing\n";
        std::cout << "    - summary: Event count, duration, average latency\n\n";

        std::cout << "Note: Requires YAMY daemon to be running\n";
    }

    int run(int argc, char* argv[]) {
        if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            printUsage();
            return 0;
        }

        // Parse arguments
        uint32_t timeout_ms = 1000;
        size_t count = 0;
        uint16_t until_key = 0;
        std::string format = "json";
        bool stream = false;
        bool verbose = true;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--timeout" && i + 1 < argc) {
                timeout_ms = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
            else if (arg == "--count" && i + 1 < argc) {
                count = static_cast<size_t>(std::stoi(argv[++i]));
            }
            else if (arg == "--until-key" && i + 1 < argc) {
                until_key = static_cast<uint16_t>(std::stoi(argv[++i]));
            }
            else if (arg == "--format" && i + 1 < argc) {
                format = argv[++i];
            }
            else if (arg == "--stream") {
                stream = true;
            }
            else if (arg == "--quiet" || arg == "-q") {
                verbose = false;
            }
            else {
                std::cerr << "Unknown option: " << arg << std::endl;
                return 1;
            }
        }

        // Start capture
        if (!m_capturer.start(verbose)) {
            return 1;
        }

        // Wait a bit for capture to stabilize
        std::this_thread::sleep_for(milliseconds(100));

        if (stream) {
            return runStreaming(verbose);
        }
        else if (count > 0) {
            return runCount(count, timeout_ms, format, verbose);
        }
        else if (until_key > 0) {
            return runUntilKey(until_key, timeout_ms, format, verbose);
        }
        else {
            return runTimeout(timeout_ms, format, verbose);
        }
    }

private:
    int runTimeout(uint32_t timeout_ms, const std::string& format, bool verbose) {
        if (verbose) {
            std::cout << "Capturing for " << timeout_ms << " ms..." << std::endl;
        }

        std::this_thread::sleep_for(milliseconds(timeout_ms));

        m_capturer.stop();

        auto events = m_capturer.getEvents();

        if (verbose) {
            std::cout << "\nCaptured " << events.size() << " events\n" << std::endl;
        }

        printEvents(events, format);

        return 0;
    }

    int runCount(size_t count, uint32_t timeout_ms, const std::string& format, bool verbose) {
        if (verbose) {
            std::cout << "Capturing " << count << " events (timeout: " << timeout_ms << " ms)..." << std::endl;
        }

        bool success = m_capturer.waitForEvents(count, timeout_ms);

        m_capturer.stop();

        auto events = m_capturer.getEvents();

        if (!success) {
            if (verbose) {
                std::cerr << "\nTimeout: Only captured " << events.size() << " of " << count << " events\n" << std::endl;
            }
            printEvents(events, format);
            return 1;
        }

        if (verbose) {
            std::cout << "\n✓ Captured " << events.size() << " events\n" << std::endl;
        }

        // Trim to exact count
        if (events.size() > count) {
            events.resize(count);
        }

        printEvents(events, format);

        return 0;
    }

    int runUntilKey(uint16_t key_code, uint32_t timeout_ms, const std::string& format, bool verbose) {
        if (verbose) {
            const char* key_name = yamy::platform::getKeyName(key_code);
            std::cout << "Capturing until " << key_name << " (evdev " << key_code << ")..." << std::endl;
        }

        auto start = steady_clock::now();
        auto timeout = milliseconds(timeout_ms);

        bool found = false;
        while (steady_clock::now() - start < timeout) {
            auto events = m_capturer.getEvents();
            for (const auto& event : events) {
                if (event.evdev_code == key_code) {
                    found = true;
                    break;
                }
            }
            if (found) break;

            std::this_thread::sleep_for(milliseconds(10));
        }

        m_capturer.stop();

        auto events = m_capturer.getEvents();

        if (!found) {
            if (verbose) {
                std::cerr << "\nTimeout: Key not found. Captured " << events.size() << " events\n" << std::endl;
            }
            printEvents(events, format);
            return 1;
        }

        if (verbose) {
            std::cout << "\n✓ Target key found. Captured " << events.size() << " events\n" << std::endl;
        }

        printEvents(events, format);

        return 0;
    }

    int runStreaming(bool verbose) {
        if (verbose) {
            std::cout << "Streaming events (Ctrl+C to stop)...\n" << std::endl;
        }

        size_t last_count = 0;

        while (true) {
            std::this_thread::sleep_for(milliseconds(50));

            auto events = m_capturer.getEvents();

            // Print new events
            for (size_t i = last_count; i < events.size(); i++) {
                const auto& event = events[i];
                std::cout << "[" << event.timestamp.count() << " us] "
                          << event.key_name << " (evdev " << event.evdev_code << ") "
                          << eventTypeToString(event.type)
                          << " [latency: " << event.latency.count() << " us]"
                          << std::endl;
            }

            last_count = events.size();
        }

        return 0;
    }

    void printEvents(const std::vector<CapturedEvent>& events, const std::string& format) {
        if (format == "json") {
            std::cout << serializeCapturedEvents(events) << std::endl;
        }
        else if (format == "human") {
            printHumanReadable(events);
        }
        else {
            std::cerr << "Unknown format: " << format << std::endl;
        }
    }

    void printHumanReadable(const std::vector<CapturedEvent>& events) {
        std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
        std::cout << "Captured Events: " << events.size() << std::endl;
        std::cout << "═══════════════════════════════════════════════════════════" << std::endl;

        for (size_t i = 0; i < events.size(); i++) {
            const auto& event = events[i];
            std::cout << "[" << (i + 1) << "] "
                      << event.key_name << " (evdev " << event.evdev_code << ") "
                      << eventTypeToString(event.type)
                      << " @ " << event.timestamp.count() << " us"
                      << " [+" << event.latency.count() << " us]"
                      << std::endl;
        }

        if (!events.empty()) {
            uint64_t total_latency = 0;
            for (const auto& event : events) {
                total_latency += event.latency.count();
            }

            auto first_ts = events.front().timestamp;
            auto last_ts = events.back().timestamp;
            auto duration_us = (last_ts - first_ts).count();

            std::cout << "───────────────────────────────────────────────────────────" << std::endl;
            std::cout << "Duration: " << duration_us << " us (" << (duration_us / 1000.0) << " ms)" << std::endl;
            std::cout << "Average latency: " << (total_latency / events.size()) << " us" << std::endl;
            std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    YamyCaptureTool tool;
    return tool.run(argc, argv);
}
