// yamy-test - Automated testing tool for YAMY keyboard remapping
// Enables systematic testing without manual UAT

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
#include <dirent.h>
#include <poll.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>

// Captured key event
struct CapturedKey {
    uint16_t code;
    bool pressed;
    std::chrono::steady_clock::time_point timestamp;
};

// Output capturer - listens to YAMY's virtual keyboard output
class OutputCapturer {
private:
    int m_fd;
    std::vector<CapturedKey> m_capturedKeys;
    std::mutex m_mutex;
    bool m_running;
    std::thread m_captureThread;

    std::string findYamyOutputDevice() {
        DIR* dir = opendir("/dev/input");
        if (!dir) {
            std::cerr << "Failed to open /dev/input" << std::endl;
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
                if (deviceName.find("Yamy Virtual") != std::string::npos) {
                    close(fd);
                    closedir(dir);
                    std::cout << "[OutputCapturer] Found YAMY output device: " << devPath
                              << " (" << deviceName << ")" << std::endl;
                    return devPath;
                }
            }
            close(fd);
        }

        closedir(dir);
        std::cerr << "[OutputCapturer] YAMY output device not found (is YAMY running?)" << std::endl;
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
                    std::cerr << "[OutputCapturer] poll error: " << strerror(errno) << std::endl;
                    break;
                }
                continue;
            }

            if (ret == 0) continue; // Timeout

            struct input_event ev;
            ssize_t n = read(m_fd, &ev, sizeof(ev));
            if (n == sizeof(ev) && ev.type == EV_KEY) {
                std::lock_guard<std::mutex> lock(m_mutex);
                CapturedKey key;
                key.code = ev.code;
                key.pressed = (ev.value == 1);
                key.timestamp = std::chrono::steady_clock::now();

                if (key.pressed) { // Only record key presses
                    m_capturedKeys.push_back(key);
                }
            }
        }
    }

public:
    OutputCapturer() : m_fd(-1), m_running(false) {}

    ~OutputCapturer() {
        stop();
    }

    bool start() {
        std::string devPath = findYamyOutputDevice();
        if (devPath.empty()) {
            return false;
        }

        m_fd = open(devPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (m_fd < 0) {
            std::cerr << "[OutputCapturer] Failed to open device: " << strerror(errno) << std::endl;
            return false;
        }

        m_running = true;
        m_captureThread = std::thread(&OutputCapturer::captureLoop, this);

        std::cout << "[OutputCapturer] Started capturing YAMY output" << std::endl;
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

    void clearCaptured() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_capturedKeys.clear();
    }

    std::vector<uint16_t> getCapturedKeyCodes() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<uint16_t> codes;
        for (const auto& key : m_capturedKeys) {
            codes.push_back(key.code);
        }
        return codes;
    }

    int getCapturedCount() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_capturedKeys.size();
    }
};

class VirtualKeyboard {
private:
    int m_fd;

public:
    VirtualKeyboard() : m_fd(-1) {}

    ~VirtualKeyboard() {
        if (m_fd >= 0) {
            ioctl(m_fd, UI_DEV_DESTROY);
            close(m_fd);
        }
    }

    bool initialize() {
        m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (m_fd < 0) {
            std::cerr << "Failed to open /dev/uinput: " << strerror(errno) << std::endl;
            std::cerr << "Try running with sudo or add user to input group" << std::endl;
            return false;
        }

        // Enable key events
        ioctl(m_fd, UI_SET_EVBIT, EV_KEY);

        // Enable all keys (for testing)
        for (int i = 0; i < KEY_MAX; i++) {
            ioctl(m_fd, UI_SET_KEYBIT, i);
        }

        // Enable sync events
        ioctl(m_fd, UI_SET_EVBIT, EV_SYN);

        // Setup device info
        struct uinput_setup usetup;
        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor = 0x1234;
        usetup.id.product = 0x5678;
        strcpy(usetup.name, "Test Keyboard for YAMY E2E");

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

        std::cout << "[VirtualKeyboard] Device created successfully" << std::endl;
        return true;
    }

    void sendKey(uint16_t keycode, bool press) {
        struct input_event ev;
        memset(&ev, 0, sizeof(ev));

        // Key event
        ev.type = EV_KEY;
        ev.code = keycode;
        ev.value = press ? 1 : 0;

        if (write(m_fd, &ev, sizeof(ev)) < 0) {
            std::cerr << "Failed to write key event: " << strerror(errno) << std::endl;
        }

        // Sync event
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_SYN;
        ev.code = SYN_REPORT;
        ev.value = 0;

        if (write(m_fd, &ev, sizeof(ev)) < 0) {
            std::cerr << "Failed to write sync event: " << strerror(errno) << std::endl;
        }
    }

    void sendKeyPress(uint16_t keycode) {
        sendKey(keycode, true);   // Press
        usleep(50000);            // 50ms delay
        sendKey(keycode, false);  // Release
        usleep(50000);            // 50ms delay
    }

    void sendSequence(const std::vector<uint16_t>& keycodes) {
        for (uint16_t code : keycodes) {
            sendKeyPress(code);
        }
    }
};

class YamyTestTool {
public:
    void printUsage() {
        std::cout << "Usage: yamy-test <command> [options]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  inject <keycode>         - Inject a single key event\n";
        std::cout << "  sequence <keys>          - Inject a sequence of keys\n";
        std::cout << "  dry-run <keys>           - Show what would be injected (no actual injection)\n";
        std::cout << "  e2e <input> <expected>   - E2E test: inject input, verify output\n";
        std::cout << "  e2e-auto <input> <expected> - E2E test with auto YAMY restart\n";
        std::cout << "\nExamples:\n";
        std::cout << "  yamy-test inject 30                   # Inject KEY_A\n";
        std::cout << "  yamy-test sequence 30,48,46           # Inject A, B, C\n";
        std::cout << "  yamy-test dry-run 30,48,46            # Show A, B, C injection plan\n";
        std::cout << "  yamy-test e2e-auto 30,48,46 30,48,46  # Auto E2E: abc → abc\n";
        std::cout << "\nNotes:\n";
        std::cout << "  - Keycodes are evdev codes (see linux/input-event-codes.h)\n";
        std::cout << "  - KEY_A=30, KEY_B=48, KEY_C=46, KEY_TAB=15, etc.\n";
        std::cout << "  - Run with sudo or add user to input group\n";
        std::cout << "  - e2e-auto automatically restarts YAMY for testing\n";
    }

    bool e2eTestAuto(const std::vector<uint16_t>& inputKeys, const std::vector<uint16_t>& expectedKeys) {
        std::cout << "\n[E2E Auto] Automated end-to-end test with YAMY restart\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        // Display test case
        std::cout << "Input:    ";
        for (size_t i = 0; i < inputKeys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << inputKeys[i] << " (" << keyCodeToName(inputKeys[i]) << ")";
        }
        std::cout << "\nExpected: ";
        for (size_t i = 0; i < expectedKeys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << expectedKeys[i] << " (" << keyCodeToName(expectedKeys[i]) << ")";
        }
        std::cout << "\n\n";

        // Step 1: Create test keyboard FIRST
        std::cout << "[E2E Auto] Creating test keyboard...\n";
        VirtualKeyboard keyboard;
        if (!keyboard.initialize()) {
            std::cerr << "✗ FAILED: Cannot create virtual keyboard\n";
            return false;
        }

        // Step 2: Find build directory
        std::string buildDir;
        if (system("test -f ./bin/yamy 2>/dev/null") == 0) {
            buildDir = ".";  // Running from build dir
        } else if (system("test -f ../build/bin/yamy 2>/dev/null") == 0) {
            buildDir = "../build";  // Running from tests dir
        } else if (system("test -f ./build/bin/yamy 2>/dev/null") == 0) {
            buildDir = "./build";  // Running from root dir
        } else {
            std::cerr << "✗ FAILED: Cannot find YAMY binaries\n";
            return false;
        }

        // Step 3: Restart YAMY so it grabs our test keyboard
        std::cout << "[E2E Auto] Restarting YAMY to grab test keyboard...\n";
        system("killall -9 yamy 2>/dev/null");
        usleep(1000000); // 1 second

        std::string yamyCmd = buildDir + "/bin/yamy > /tmp/yamy_e2e_auto.log 2>&1 &";
        std::cout << "[E2E Auto] Starting YAMY: " << yamyCmd << "\n";
        system(yamyCmd.c_str());
        usleep(3000000); // 3 seconds for YAMY to start

        std::string yamyCtlStart = buildDir + "/bin/yamy-ctl start 2>&1";
        std::cout << "[E2E Auto] Starting engine: " << yamyCtlStart << "\n";
        int ctlResult = system(yamyCtlStart.c_str());
        if (ctlResult != 0) {
            std::cerr << "✗ WARNING: yamy-ctl start returned " << ctlResult << "\n";
        }
        usleep(1000000); // 1 second

        // Step 3: Start output capture
        std::cout << "[E2E Auto] Starting output capture...\n";
        OutputCapturer capturer;
        if (!capturer.start()) {
            std::cerr << "✗ FAILED: Cannot capture YAMY output\n";
            return false;
        }

        usleep(500000); // 500ms to let capturer stabilize
        capturer.clearCaptured();

        // Step 4: Inject keys
        std::cout << "[E2E Auto] Injecting " << inputKeys.size() << " key(s)...\n";
        for (uint16_t code : inputKeys) {
            keyboard.sendKeyPress(code);
        }

        // Step 5: Wait for YAMY output
        std::cout << "[E2E Auto] Waiting for YAMY output...\n";
        int maxWait = 20;
        int waitCount = 0;

        while (waitCount < maxWait) {
            usleep(100000); // 100ms
            if (capturer.getCapturedCount() >= (int)expectedKeys.size()) {
                break;
            }
            waitCount++;
        }

        // Step 6: Verify
        auto capturedKeys = capturer.getCapturedKeyCodes();

        std::cout << "\n[E2E Auto] Results:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Captured: ";
        for (size_t i = 0; i < capturedKeys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << capturedKeys[i] << " (" << keyCodeToName(capturedKeys[i]) << ")";
        }
        std::cout << "\n\n";

        bool passed = (capturedKeys.size() == expectedKeys.size());
        if (passed) {
            for (size_t i = 0; i < expectedKeys.size(); i++) {
                if (capturedKeys[i] != expectedKeys[i]) {
                    passed = false;
                    break;
                }
            }
        }

        if (passed) {
            std::cout << "✓ PASSED: Output matches expected!\n";
            return true;
        } else {
            std::cout << "✗ FAILED: Output does not match expected\n";
            if (capturedKeys.size() != expectedKeys.size()) {
                std::cout << "  Expected " << expectedKeys.size() << " keys, got " << capturedKeys.size() << "\n";
            }
            return false;
        }
    }

    std::string keyCodeToName(uint16_t code) {
        switch (code) {
            case 30: return "KEY_A";
            case 48: return "KEY_B";
            case 46: return "KEY_C";
            case 32: return "KEY_D";
            case 18: return "KEY_E";
            case 33: return "KEY_F";
            case 15: return "KEY_TAB";
            case 14: return "KEY_BACKSPACE";
            case 26: return "KEY_LEFTBRACE";
            case 27: return "KEY_RIGHTBRACE";
            case 42: return "KEY_LEFTSHIFT";
            case 40: return "KEY_APOSTROPHE";
            default: return "KEY_" + std::to_string(code);
        }
    }

    bool e2eTest(const std::vector<uint16_t>& inputKeys, const std::vector<uint16_t>& expectedKeys) {
        std::cout << "\n[E2E Test] Starting end-to-end test\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        // Display test case
        std::cout << "Input:    ";
        for (size_t i = 0; i < inputKeys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << inputKeys[i] << " (" << keyCodeToName(inputKeys[i]) << ")";
        }
        std::cout << "\nExpected: ";
        for (size_t i = 0; i < expectedKeys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << expectedKeys[i] << " (" << keyCodeToName(expectedKeys[i]) << ")";
        }
        std::cout << "\n\n";

        // Start output capture
        OutputCapturer capturer;
        if (!capturer.start()) {
            std::cerr << "✗ FAILED: Cannot capture YAMY output (is YAMY running?)\n";
            return false;
        }

        usleep(200000); // 200ms to let capturer stabilize

        // Clear any pre-existing events
        capturer.clearCaptured();

        // Create virtual keyboard and inject keys
        VirtualKeyboard keyboard;
        if (!keyboard.initialize()) {
            std::cerr << "✗ FAILED: Cannot create virtual keyboard\n";
            return false;
        }

        std::cout << "[E2E Test] Injecting " << inputKeys.size() << " key(s)...\n";
        for (uint16_t code : inputKeys) {
            keyboard.sendKeyPress(code);
        }

        // Wait for YAMY to process and output
        std::cout << "[E2E Test] Waiting for YAMY output...\n";
        int maxWait = 20; // 2 seconds max (20 * 100ms)
        int waitCount = 0;

        while (waitCount < maxWait) {
            usleep(100000); // 100ms
            if (capturer.getCapturedCount() >= (int)expectedKeys.size()) {
                break;
            }
            waitCount++;
        }

        // Get captured output
        auto capturedKeys = capturer.getCapturedKeyCodes();

        std::cout << "\n[E2E Test] Results:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Captured: ";
        for (size_t i = 0; i < capturedKeys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << capturedKeys[i] << " (" << keyCodeToName(capturedKeys[i]) << ")";
        }
        std::cout << "\n\n";

        // Verify output matches expected
        bool passed = (capturedKeys.size() == expectedKeys.size());
        if (passed) {
            for (size_t i = 0; i < expectedKeys.size(); i++) {
                if (capturedKeys[i] != expectedKeys[i]) {
                    passed = false;
                    break;
                }
            }
        }

        if (passed) {
            std::cout << "✓ PASSED: Output matches expected!\n";
            return true;
        } else {
            std::cout << "✗ FAILED: Output does not match expected\n";
            if (capturedKeys.size() != expectedKeys.size()) {
                std::cout << "  Expected " << expectedKeys.size() << " keys, got " << capturedKeys.size() << "\n";
            } else {
                for (size_t i = 0; i < expectedKeys.size(); i++) {
                    if (capturedKeys[i] != expectedKeys[i]) {
                        std::cout << "  Mismatch at position " << i << ": expected "
                                  << expectedKeys[i] << " (" << keyCodeToName(expectedKeys[i]) << "), got "
                                  << capturedKeys[i] << " (" << keyCodeToName(capturedKeys[i]) << ")\n";
                    }
                }
            }
            return false;
        }
    }

    void dryRun(const std::vector<uint16_t>& keycodes) {
        std::cout << "[DRY-RUN] Would inject " << keycodes.size() << " key(s):\n";

        for (size_t i = 0; i < keycodes.size(); i++) {
            std::cout << "[DRY-RUN]   Key " << (i+1) << ": evdev code " << keycodes[i];

            // Map common keycodes to names
            switch (keycodes[i]) {
                case 30: std::cout << " (KEY_A)"; break;
                case 48: std::cout << " (KEY_B)"; break;
                case 46: std::cout << " (KEY_C)"; break;
                case 15: std::cout << " (KEY_TAB)"; break;
                case 14: std::cout << " (KEY_BACKSPACE)"; break;
                case 26: std::cout << " (KEY_LEFTBRACE '[')"; break;
                case 42: std::cout << " (KEY_LEFTSHIFT)"; break;
                default: std::cout << " (unknown)"; break;
            }
            std::cout << "\n";
        }

        std::cout << "[DRY-RUN] No actual injection performed (dry-run mode)\n";
    }

    bool injectKeys(const std::vector<uint16_t>& keycodes) {
        VirtualKeyboard keyboard;

        if (!keyboard.initialize()) {
            return false;
        }

        std::cout << "Injecting " << keycodes.size() << " key(s)...\n";

        for (size_t i = 0; i < keycodes.size(); i++) {
            std::cout << "  Injecting key " << (i+1) << "/" << keycodes.size()
                      << ": evdev code " << keycodes[i] << std::endl;
            keyboard.sendKeyPress(keycodes[i]);
        }

        std::cout << "✓ All keys injected successfully\n";
        std::cout << "\nCheck YAMY metrics:\n";
        std::cout << "  yamy-ctl metrics\n";

        return true;
    }

    std::vector<uint16_t> parseKeycodes(const std::string& input) {
        std::vector<uint16_t> codes;
        size_t pos = 0;

        while (pos < input.length()) {
            size_t comma = input.find(',', pos);
            if (comma == std::string::npos) {
                comma = input.length();
            }

            std::string code_str = input.substr(pos, comma - pos);
            uint16_t code = std::stoi(code_str);
            codes.push_back(code);

            pos = comma + 1;
        }

        return codes;
    }
};

int main(int argc, char* argv[]) {
    YamyTestTool tool;

    if (argc < 2) {
        tool.printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "inject" && argc >= 3) {
        uint16_t keycode = std::stoi(argv[2]);
        return tool.injectKeys({keycode}) ? 0 : 1;

    } else if (command == "sequence" && argc >= 3) {
        auto keycodes = tool.parseKeycodes(argv[2]);
        return tool.injectKeys(keycodes) ? 0 : 1;

    } else if (command == "dry-run" && argc >= 3) {
        auto keycodes = tool.parseKeycodes(argv[2]);
        tool.dryRun(keycodes);
        return 0;

    } else if (command == "e2e" && argc >= 4) {
        auto inputKeys = tool.parseKeycodes(argv[2]);
        auto expectedKeys = tool.parseKeycodes(argv[3]);
        return tool.e2eTest(inputKeys, expectedKeys) ? 0 : 1;

    } else if (command == "e2e-auto" && argc >= 4) {
        auto inputKeys = tool.parseKeycodes(argv[2]);
        auto expectedKeys = tool.parseKeycodes(argv[3]);
        return tool.e2eTestAuto(inputKeys, expectedKeys) ? 0 : 1;

    } else {
        tool.printUsage();
        return 1;
    }
}
