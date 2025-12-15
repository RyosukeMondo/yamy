//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy-test-runner - E2E test orchestrator for YAMY
//
// Executes complete test scenarios: inject input, capture output, verify results

#include "test_scenario.h"
#include "test_scenario_json.h"
#include "../../platform/linux/keycode_mapping.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <poll.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <signal.h>
#include <sys/wait.h>

using namespace yamy::test;
using namespace std::chrono;

// Forward declarations (reusing from inject/capture implementations)
class EventInjector {
private:
    int m_fd;

public:
    EventInjector() : m_fd(-1) {}
    ~EventInjector() { close(); }

    bool initialize() {
        m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (m_fd < 0) return false;

        ioctl(m_fd, UI_SET_EVBIT, EV_KEY);
        for (int i = 0; i < KEY_MAX; i++) {
            ioctl(m_fd, UI_SET_KEYBIT, i);
        }
        ioctl(m_fd, UI_SET_EVBIT, EV_SYN);

        struct uinput_setup usetup;
        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor = 0x1234;
        usetup.id.product = 0x5679;
        strcpy(usetup.name, "YAMY Test Injector");

        if (ioctl(m_fd, UI_DEV_SETUP, &usetup) < 0) return false;
        if (ioctl(m_fd, UI_DEV_CREATE) < 0) return false;

        usleep(100000);
        return true;
    }

    void close() {
        if (m_fd >= 0) {
            ioctl(m_fd, UI_DEV_DESTROY);
            ::close(m_fd);
            m_fd = -1;
        }
    }

    bool sendEvent(uint16_t evdev_code, EventType type) {
        if (m_fd < 0) return false;

        struct input_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_KEY;
        ev.code = evdev_code;
        ev.value = (type == EventType::PRESS) ? 1 : 0;

        if (write(m_fd, &ev, sizeof(ev)) < 0) return false;

        memset(&ev, 0, sizeof(ev));
        ev.type = EV_SYN;
        ev.code = SYN_REPORT;
        ev.value = 0;

        return write(m_fd, &ev, sizeof(ev)) >= 0;
    }

    bool injectSequence(const std::vector<KeyEvent>& events) {
        for (const auto& event : events) {
            if (event.delay_before_ms > 0) {
                std::this_thread::sleep_for(milliseconds(event.delay_before_ms));
            }
            if (!sendEvent(event.evdev_code, event.type)) {
                return false;
            }
        }
        return true;
    }
};

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
        if (!dir) return "";

        struct dirent* entry;
        std::string result;
        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "event", 5) != 0) continue;

            std::string devPath = std::string("/dev/input/") + entry->d_name;
            int fd = open(devPath.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            char name[256] = {0};
            if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
                std::string deviceName(name);
                if (deviceName.find("Yamy Virtual") != std::string::npos ||
                    deviceName.find("YAMY Virtual") != std::string::npos) {
                    result = devPath;
                    close(fd);
                    break;
                }
            }
            close(fd);
        }
        closedir(dir);
        return result;
    }

    void captureLoop() {
        struct pollfd pfd;
        pfd.fd = m_fd;
        pfd.events = POLLIN;

        while (m_running) {
            int ret = poll(&pfd, 1, 100);
            if (ret <= 0) continue;

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
    ~OutputCapturer() { stop(); }

    bool start() {
        std::string devPath = findYamyDevice();
        if (devPath.empty()) return false;

        m_fd = open(devPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (m_fd < 0) return false;

        m_start_time = steady_clock::now();
        m_running = true;
        m_captureThread = std::thread(&OutputCapturer::captureLoop, this);
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
            ::close(m_fd);
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

    bool waitForEvents(size_t count, uint32_t timeout_ms) {
        auto start = steady_clock::now();
        auto timeout = milliseconds(timeout_ms);

        while (steady_clock::now() - start < timeout) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_events.size() >= count) return true;
            }
            std::this_thread::sleep_for(milliseconds(10));
        }
        return false;
    }
};

/// Test executor
class TestExecutor {
private:
    EventInjector m_injector;
    OutputCapturer m_capturer;
    bool m_verbose;

public:
    TestExecutor(bool verbose = true) : m_verbose(verbose) {}

    TestCaseResult executeTestCase(const TestCase& test_case) {
        TestCaseResult result;
        result.name = test_case.name;

        auto start_time = steady_clock::now();

        if (m_verbose) {
            std::cout << "  Running: " << test_case.name;
            if (!test_case.description.empty()) {
                std::cout << " - " << test_case.description;
            }
            std::cout << std::endl;
        }

        // Clear previous events
        m_capturer.clear();

        // Inject input
        if (!m_injector.injectSequence(test_case.input)) {
            result.status = TestStatus::ERROR;
            result.error_message = "Failed to inject input events";
            result.duration_ms = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
            return result;
        }

        // Wait for expected output
        size_t expected_count = test_case.expected_output.size();
        bool captured_all = m_capturer.waitForEvents(expected_count, test_case.timeout_ms);

        auto end_time = steady_clock::now();
        result.duration_ms = duration_cast<milliseconds>(end_time - start_time).count();

        // Get captured events
        result.actual_output = m_capturer.getEvents();

        if (!captured_all) {
            result.status = TestStatus::TIMEOUT;
            result.error_message = "Timeout waiting for output events";
            if (m_verbose) {
                std::cout << "    ✗ TIMEOUT (expected " << expected_count << ", got " << result.actual_output.size() << ")" << std::endl;
            }
            return result;
        }

        // Verify output matches expected
        bool matches = verifyOutput(test_case.expected_output, result.actual_output, result.error_message);

        if (matches) {
            result.status = TestStatus::PASSED;

            // Calculate average latency
            if (!result.actual_output.empty()) {
                uint64_t total_latency = 0;
                for (const auto& event : result.actual_output) {
                    total_latency += event.latency.count();
                }
                result.latency_us = static_cast<uint32_t>(total_latency / result.actual_output.size());
            }

            if (m_verbose) {
                std::cout << "    ✓ PASSED (" << result.duration_ms << " ms, latency: " << result.latency_us << " us)" << std::endl;
            }
        } else {
            result.status = TestStatus::FAILED;
            if (m_verbose) {
                std::cout << "    ✗ FAILED: " << result.error_message << std::endl;
            }
        }

        return result;
    }

    ScenarioResult executeScenario(const TestScenario& scenario) {
        ScenarioResult result;
        result.scenario_name = scenario.name;

        auto start_time = steady_clock::now();

        if (m_verbose) {
            std::cout << "\n═══════════════════════════════════════════════════════════" << std::endl;
            std::cout << "Scenario: " << scenario.name << std::endl;
            if (!scenario.description.empty()) {
                std::cout << "Description: " << scenario.description << std::endl;
            }
            std::cout << "Test cases: " << scenario.test_cases.size() << std::endl;
            std::cout << "═══════════════════════════════════════════════════════════\n" << std::endl;
        }

        // Initialize injector and capturer
        if (!m_injector.initialize()) {
            result.status = TestStatus::ERROR;
            std::cerr << "Failed to initialize event injector" << std::endl;
            return result;
        }

        if (!m_capturer.start()) {
            result.status = TestStatus::ERROR;
            std::cerr << "Failed to start output capturer" << std::endl;
            return result;
        }

        // Wait for capturer to stabilize
        std::this_thread::sleep_for(milliseconds(200));

        // Execute each test case
        bool all_passed = true;
        for (const auto& test_case : scenario.test_cases) {
            auto tc_result = executeTestCase(test_case);
            result.test_case_results.push_back(tc_result);

            if (tc_result.status != TestStatus::PASSED) {
                all_passed = false;
            }
        }

        m_capturer.stop();
        m_injector.close();

        result.status = all_passed ? TestStatus::PASSED : TestStatus::FAILED;
        result.duration_ms = duration_cast<milliseconds>(steady_clock::now() - start_time).count();

        if (m_verbose) {
            std::cout << "\n───────────────────────────────────────────────────────────" << std::endl;
            std::cout << "Scenario result: " << testStatusToString(result.status) << std::endl;
            std::cout << "Duration: " << result.duration_ms << " ms" << std::endl;
            std::cout << "═══════════════════════════════════════════════════════════\n" << std::endl;
        }

        return result;
    }

private:
    bool verifyOutput(const std::vector<KeyEvent>& expected, const std::vector<CapturedEvent>& actual, std::string& error_msg) {
        if (expected.size() != actual.size()) {
            error_msg = "Event count mismatch (expected " + std::to_string(expected.size()) + ", got " + std::to_string(actual.size()) + ")";
            return false;
        }

        for (size_t i = 0; i < expected.size(); i++) {
            if (expected[i].evdev_code != actual[i].evdev_code) {
                error_msg = "Key code mismatch at position " + std::to_string(i) +
                           " (expected " + std::to_string(expected[i].evdev_code) +
                           ", got " + std::to_string(actual[i].evdev_code) + ")";
                return false;
            }

            if (expected[i].type != actual[i].type) {
                error_msg = "Event type mismatch at position " + std::to_string(i) +
                           " (expected " + eventTypeToString(expected[i].type) +
                           ", got " + eventTypeToString(actual[i].type) + ")";
                return false;
            }
        }

        return true;
    }
};

/// CLI tool class
class YamyTestRunnerTool {
private:
    TestExecutor m_executor;

public:
    YamyTestRunnerTool(bool verbose) : m_executor(verbose) {}

    void printUsage() {
        std::cout << "yamy-test-runner - E2E test orchestrator for YAMY\n\n";
        std::cout << "Usage:\n";
        std::cout << "  yamy-test-runner --scenario <file.json>\n";
        std::cout << "  yamy-test-runner --suite <file.json>\n";
        std::cout << "  yamy-test-runner --help\n\n";

        std::cout << "Options:\n";
        std::cout << "  --scenario <file>   Run single test scenario\n";
        std::cout << "  --suite <file>      Run test suite (multiple scenarios)\n";
        std::cout << "  --test-case <name>  Run specific test case from scenario\n";
        std::cout << "  --report <file>     Save test report to file (JSON)\n";
        std::cout << "  --quiet             Suppress detailed output\n";
        std::cout << "  --help              Show this help\n\n";

        std::cout << "Examples:\n";
        std::cout << "  yamy-test-runner --scenario tests/scenarios/basic_remap.json\n";
        std::cout << "  yamy-test-runner --suite tests/suites/all_features.json\n";
        std::cout << "  yamy-test-runner --scenario test.json --report results.json\n\n";

        std::cout << "Prerequisites:\n";
        std::cout << "  - YAMY daemon must be running with appropriate config\n";
        std::cout << "  - User must have permission to access /dev/uinput\n";
    }

    int run(int argc, char* argv[]) {
        if (argc < 2) {
            printUsage();
            return 1;
        }

        // Parse arguments
        std::string mode;
        std::string file;
        std::string test_case_name;
        std::string report_file;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            }
            else if (arg == "--scenario" && i + 1 < argc) {
                mode = "scenario";
                file = argv[++i];
            }
            else if (arg == "--suite" && i + 1 < argc) {
                mode = "suite";
                file = argv[++i];
            }
            else if (arg == "--test-case" && i + 1 < argc) {
                test_case_name = argv[++i];
            }
            else if (arg == "--report" && i + 1 < argc) {
                report_file = argv[++i];
            }
            else {
                std::cerr << "Unknown option: " << arg << std::endl;
                return 1;
            }
        }

        // Execute based on mode
        try {
            if (mode == "scenario") {
                return runScenario(file, test_case_name, report_file);
            }
            else if (mode == "suite") {
                return runSuite(file, report_file);
            }
            else {
                std::cerr << "No mode specified. Use --scenario or --suite" << std::endl;
                return 1;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

private:
    int runScenario(const std::string& filename, const std::string& test_case_filter, const std::string& report_file) {
        TestScenario scenario = loadScenarioFromJson(filename);

        // Filter test cases if needed
        if (!test_case_filter.empty()) {
            std::vector<TestCase> filtered;
            for (const auto& tc : scenario.test_cases) {
                if (tc.name == test_case_filter) {
                    filtered.push_back(tc);
                }
            }
            if (filtered.empty()) {
                std::cerr << "Test case not found: " << test_case_filter << std::endl;
                return 1;
            }
            scenario.test_cases = filtered;
        }

        auto result = m_executor.executeScenario(scenario);

        // Save report if requested
        if (!report_file.empty()) {
            std::ofstream out(report_file);
            out << serializeScenarioResult(result);
            std::cout << "\nReport saved to: " << report_file << std::endl;
        }

        return (result.status == TestStatus::PASSED) ? 0 : 1;
    }

    int runSuite(const std::string& filename, const std::string& report_file) {
        TestSuite suite = loadSuiteFromJson(filename);

        TestSuiteResult suite_result;
        suite_result.suite_name = suite.name;
        suite_result.timestamp = getCurrentTimestamp();

        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Test Suite: " << suite.name << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;

        auto suite_start = steady_clock::now();

        for (const auto& scenario_file : suite.scenario_files) {
            TestScenario scenario = loadScenarioFromJson(scenario_file);
            auto result = m_executor.executeScenario(scenario);

            suite_result.scenario_results.push_back(result);
            suite_result.total_scenarios++;
            suite_result.total_test_cases += result.test_case_results.size();

            for (const auto& tc_result : result.test_case_results) {
                if (tc_result.status == TestStatus::PASSED) {
                    suite_result.passed++;
                } else {
                    suite_result.failed++;
                }
            }
        }

        suite_result.duration_ms = duration_cast<milliseconds>(steady_clock::now() - suite_start).count();

        // Print summary
        printSuiteSummary(suite_result);

        // Save report if requested
        if (!report_file.empty()) {
            std::ofstream out(report_file);
            out << serializeTestSuiteResult(suite_result);
            std::cout << "\nReport saved to: " << report_file << std::endl;
        }

        return (suite_result.failed == 0) ? 0 : 1;
    }

    void printSuiteSummary(const TestSuiteResult& result) {
        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Test Suite Summary" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "Suite: " << result.suite_name << std::endl;
        std::cout << "Timestamp: " << result.timestamp << std::endl;
        std::cout << "Duration: " << result.duration_ms << " ms" << std::endl;
        std::cout << "\nScenarios: " << result.total_scenarios << std::endl;
        std::cout << "Test cases: " << result.total_test_cases << std::endl;
        std::cout << "Passed: " << result.passed << std::endl;
        std::cout << "Failed: " << result.failed << std::endl;

        if (result.failed == 0) {
            std::cout << "\n✓ ALL TESTS PASSED" << std::endl;
        } else {
            std::cout << "\n✗ " << result.failed << " TEST(S) FAILED" << std::endl;
        }

        std::cout << "═══════════════════════════════════════════════════════════\n" << std::endl;
    }

    std::string getCurrentTimestamp() {
        auto now = system_clock::now();
        auto time_t = system_clock::to_time_t(now);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&time_t));
        return std::string(buffer);
    }
};

int main(int argc, char* argv[]) {
    bool verbose = true;

    // Check for --quiet flag
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--quiet" || std::string(argv[i]) == "-q") {
            verbose = false;
            break;
        }
    }

    YamyTestRunnerTool tool(verbose);
    return tool.run(argc, argv);
}
