//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy-inject - Synthetic keyboard event injector for testing
//
// Injects keyboard events via uinput for automated testing of YAMY daemon

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
#include <linux/uinput.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <chrono>
#include <thread>

using namespace yamy::test;
using namespace std::chrono;

/// Virtual keyboard for injecting events
class EventInjector {
private:
    int m_fd;

public:
    EventInjector() : m_fd(-1) {}

    ~EventInjector() {
        close();
    }

    bool initialize() {
        m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (m_fd < 0) {
            std::cerr << "Failed to open /dev/uinput: " << strerror(errno) << std::endl;
            std::cerr << "Try running with sudo or add user to 'input' group" << std::endl;
            return false;
        }

        // Enable key events
        if (ioctl(m_fd, UI_SET_EVBIT, EV_KEY) < 0) {
            std::cerr << "Failed to enable key events: " << strerror(errno) << std::endl;
            return false;
        }

        // Enable all keys
        for (int i = 0; i < KEY_MAX; i++) {
            ioctl(m_fd, UI_SET_KEYBIT, i);
        }

        // Enable sync events
        if (ioctl(m_fd, UI_SET_EVBIT, EV_SYN) < 0) {
            std::cerr << "Failed to enable sync events: " << strerror(errno) << std::endl;
            return false;
        }

        // Setup device info
        struct uinput_setup usetup;
        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor = 0x1234;
        usetup.id.product = 0x5679;
        strcpy(usetup.name, "YAMY Test Injector");

        if (ioctl(m_fd, UI_DEV_SETUP, &usetup) < 0) {
            std::cerr << "Failed to setup device: " << strerror(errno) << std::endl;
            return false;
        }

        if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
            std::cerr << "Failed to create device: " << strerror(errno) << std::endl;
            return false;
        }

        // Give kernel time to create device
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
        if (m_fd < 0) {
            std::cerr << "Device not initialized" << std::endl;
            return false;
        }

        struct input_event ev;
        memset(&ev, 0, sizeof(ev));

        // Key event
        ev.type = EV_KEY;
        ev.code = evdev_code;
        ev.value = (type == EventType::PRESS) ? 1 : 0;

        if (write(m_fd, &ev, sizeof(ev)) < 0) {
            std::cerr << "Failed to write key event: " << strerror(errno) << std::endl;
            return false;
        }

        // Sync event
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_SYN;
        ev.code = SYN_REPORT;
        ev.value = 0;

        if (write(m_fd, &ev, sizeof(ev)) < 0) {
            std::cerr << "Failed to write sync event: " << strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

    bool injectSequence(const std::vector<KeyEvent>& events, bool verbose = true) {
        auto start_time = steady_clock::now();

        for (const auto& event : events) {
            // Apply delay before this event
            if (event.delay_before_ms > 0) {
                std::this_thread::sleep_for(milliseconds(event.delay_before_ms));
            }

            // Send the event
            if (!sendEvent(event.evdev_code, event.type)) {
                return false;
            }

            if (verbose) {
                auto now = steady_clock::now();
                auto elapsed_us = duration_cast<microseconds>(now - start_time).count();

                const char* key_name = event.key_name.empty()
                    ? yamy::platform::getKeyName(event.evdev_code)
                    : event.key_name.c_str();

                std::cout << "[" << elapsed_us << " us] "
                          << "Injected: " << key_name
                          << " (evdev " << event.evdev_code << ") "
                          << eventTypeToString(event.type)
                          << std::endl;
            }
        }

        return true;
    }
};

/// CLI tool class
class YamyInjectTool {
private:
    EventInjector m_injector;

public:
    void printUsage() {
        std::cout << "yamy-inject - Synthetic keyboard event injector for YAMY testing\n\n";
        std::cout << "Usage:\n";
        std::cout << "  yamy-inject --key <evdev_code> [--press|--release|--sequence]\n";
        std::cout << "  yamy-inject --keys <code1,code2,...> [--sequence]\n";
        std::cout << "  yamy-inject --scenario <file.json>\n";
        std::cout << "  yamy-inject --help\n\n";

        std::cout << "Options:\n";
        std::cout << "  --key <code>        Inject single key (evdev code)\n";
        std::cout << "  --keys <codes>      Inject multiple keys (comma-separated evdev codes)\n";
        std::cout << "  --press             Inject press event only\n";
        std::cout << "  --release           Inject release event only\n";
        std::cout << "  --sequence          Inject press+release for each key (default)\n";
        std::cout << "  --delay <ms>        Delay between events (default: 50ms)\n";
        std::cout << "  --hold <ms>         Hold time for press before release (default: 50ms)\n";
        std::cout << "  --scenario <file>   Load test scenario from JSON file\n";
        std::cout << "  --test-case <name>  Run specific test case from scenario\n";
        std::cout << "  --quiet             Suppress output (except errors)\n";
        std::cout << "  --help              Show this help\n\n";

        std::cout << "Examples:\n";
        std::cout << "  yamy-inject --key 30                      # Inject KEY_A (press+release)\n";
        std::cout << "  yamy-inject --key 30 --press              # Inject KEY_A press only\n";
        std::cout << "  yamy-inject --keys 30,48,46               # Inject A, B, C keys\n";
        std::cout << "  yamy-inject --keys 30,15 --delay 100      # A, Tab with 100ms delay\n";
        std::cout << "  yamy-inject --scenario test.json          # Run all test cases\n";
        std::cout << "  yamy-inject --scenario test.json --test-case tc1\n\n";

        std::cout << "Common evdev codes:\n";
        std::cout << "  KEY_A=30, KEY_B=48, KEY_C=46, KEY_D=32, KEY_E=18\n";
        std::cout << "  KEY_TAB=15, KEY_ESC=1, KEY_ENTER=28, KEY_SPACE=57\n";
        std::cout << "  KEY_LEFTSHIFT=42, KEY_LEFTCTRL=29, KEY_LEFTALT=56\n\n";

        std::cout << "Note: Requires permission to access /dev/uinput\n";
        std::cout << "      Run with sudo or add user to 'input' group\n";
    }

    int run(int argc, char* argv[]) {
        if (argc < 2) {
            printUsage();
            return 1;
        }

        // Parse arguments
        std::string mode;
        uint16_t single_key = 0;
        std::vector<uint16_t> keys;
        bool press_only = false;
        bool release_only = false;
        uint32_t delay_ms = 50;
        uint32_t hold_ms = 50;
        std::string scenario_file;
        std::string test_case_name;
        bool verbose = true;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            }
            else if (arg == "--key" && i + 1 < argc) {
                mode = "single";
                single_key = static_cast<uint16_t>(std::stoi(argv[++i]));
            }
            else if (arg == "--keys" && i + 1 < argc) {
                mode = "multiple";
                keys = parseKeyCodes(argv[++i]);
            }
            else if (arg == "--press") {
                press_only = true;
            }
            else if (arg == "--release") {
                release_only = true;
            }
            else if (arg == "--sequence") {
                // Default behavior
            }
            else if (arg == "--delay" && i + 1 < argc) {
                delay_ms = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
            else if (arg == "--hold" && i + 1 < argc) {
                hold_ms = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
            else if (arg == "--scenario" && i + 1 < argc) {
                mode = "scenario";
                scenario_file = argv[++i];
            }
            else if (arg == "--test-case" && i + 1 < argc) {
                test_case_name = argv[++i];
            }
            else if (arg == "--quiet" || arg == "-q") {
                verbose = false;
            }
            else {
                std::cerr << "Unknown option: " << arg << std::endl;
                return 1;
            }
        }

        // Initialize injector
        if (verbose) {
            std::cout << "Initializing event injector..." << std::endl;
        }

        if (!m_injector.initialize()) {
            return 1;
        }

        if (verbose) {
            std::cout << "Event injector ready\n" << std::endl;
        }

        // Execute based on mode
        if (mode == "single") {
            return injectSingleKey(single_key, press_only, release_only, hold_ms, verbose);
        }
        else if (mode == "multiple") {
            return injectMultipleKeys(keys, delay_ms, hold_ms, verbose);
        }
        else if (mode == "scenario") {
            return injectScenario(scenario_file, test_case_name, verbose);
        }
        else {
            std::cerr << "No injection mode specified. Use --key, --keys, or --scenario" << std::endl;
            return 1;
        }
    }

private:
    std::vector<uint16_t> parseKeyCodes(const std::string& input) {
        std::vector<uint16_t> codes;
        size_t pos = 0;

        while (pos < input.length()) {
            size_t comma = input.find(',', pos);
            if (comma == std::string::npos) {
                comma = input.length();
            }

            std::string code_str = input.substr(pos, comma - pos);
            if (!code_str.empty()) {
                codes.push_back(static_cast<uint16_t>(std::stoi(code_str)));
            }

            pos = comma + 1;
        }

        return codes;
    }

    int injectSingleKey(uint16_t key, bool press_only, bool release_only, uint32_t hold_ms, bool verbose) {
        std::vector<KeyEvent> events;

        const char* key_name = yamy::platform::getKeyName(key);

        if (press_only) {
            KeyEvent evt;
            evt.evdev_code = key;
            evt.key_name = key_name;
            evt.type = EventType::PRESS;
            evt.delay_before_ms = 0;
            events.push_back(evt);
        }
        else if (release_only) {
            KeyEvent evt;
            evt.evdev_code = key;
            evt.key_name = key_name;
            evt.type = EventType::RELEASE;
            evt.delay_before_ms = 0;
            events.push_back(evt);
        }
        else {
            // Sequence: press + release
            KeyEvent press_evt;
            press_evt.evdev_code = key;
            press_evt.key_name = key_name;
            press_evt.type = EventType::PRESS;
            press_evt.delay_before_ms = 0;
            events.push_back(press_evt);

            KeyEvent release_evt;
            release_evt.evdev_code = key;
            release_evt.key_name = key_name;
            release_evt.type = EventType::RELEASE;
            release_evt.delay_before_ms = hold_ms;
            events.push_back(release_evt);
        }

        return m_injector.injectSequence(events, verbose) ? 0 : 1;
    }

    int injectMultipleKeys(const std::vector<uint16_t>& keys, uint32_t delay_ms, uint32_t hold_ms, bool verbose) {
        std::vector<KeyEvent> events;

        for (size_t i = 0; i < keys.size(); i++) {
            const char* key_name = yamy::platform::getKeyName(keys[i]);

            // Press
            uint32_t delay_before = (i == 0) ? 0 : delay_ms;
            KeyEvent press_evt;
            press_evt.evdev_code = keys[i];
            press_evt.key_name = key_name;
            press_evt.type = EventType::PRESS;
            press_evt.delay_before_ms = delay_before;
            events.push_back(press_evt);

            // Release
            KeyEvent release_evt;
            release_evt.evdev_code = keys[i];
            release_evt.key_name = key_name;
            release_evt.type = EventType::RELEASE;
            release_evt.delay_before_ms = hold_ms;
            events.push_back(release_evt);
        }

        return m_injector.injectSequence(events, verbose) ? 0 : 1;
    }

    int injectScenario(const std::string& filename, const std::string& test_case_name, bool verbose) {
        try {
            if (verbose) {
                std::cout << "Loading scenario from: " << filename << std::endl;
            }

            TestScenario scenario = loadScenarioFromJson(filename);

            if (verbose) {
                std::cout << "Scenario: " << scenario.name << std::endl;
                std::cout << "Test cases: " << scenario.test_cases.size() << "\n" << std::endl;
            }

            // Run specific test case or all
            bool found = false;
            for (const auto& test_case : scenario.test_cases) {
                if (!test_case_name.empty() && test_case.name != test_case_name) {
                    continue;
                }

                found = true;

                if (verbose) {
                    std::cout << "═══════════════════════════════════════" << std::endl;
                    std::cout << "Test case: " << test_case.name << std::endl;
                    if (!test_case.description.empty()) {
                        std::cout << "Description: " << test_case.description << std::endl;
                    }
                    std::cout << "Input events: " << test_case.input.size() << std::endl;
                    std::cout << "═══════════════════════════════════════\n" << std::endl;
                }

                if (!m_injector.injectSequence(test_case.input, verbose)) {
                    std::cerr << "Failed to inject test case: " << test_case.name << std::endl;
                    return 1;
                }

                if (verbose) {
                    std::cout << "\n✓ Test case completed\n" << std::endl;
                }
            }

            if (!test_case_name.empty() && !found) {
                std::cerr << "Test case not found: " << test_case_name << std::endl;
                return 1;
            }

            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
};

int main(int argc, char* argv[]) {
    YamyInjectTool tool;
    return tool.run(argc, argv);
}
