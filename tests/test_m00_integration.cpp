#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>

#include "../../src/core/engine/engine.h"
#include "../../src/core/settings/json_config_loader.h"
#include "../../src/core/platform/window_system_interface.h"
#include "../../src/core/platform/input_injector_interface.h"
#include "../../src/core/platform/input_hook_interface.h"
#include "../../src/core/platform/input_driver_interface.h"
#include "../../src/utils/msgstream.h"
#include "test_utils/event_simulator.h"

using namespace yamy::platform;
using namespace yamy::test;

// --- Test Config ---
const std::string TEST_CONFIG_M00 = R"({
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "B": "0x30",
      "S": "0x1f",
      "D": "0x20"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "A",
      "tap": "B",
      "holdThresholdMs": 200
    }
  },
  "mappings": [
    {
      "from": "M00-S",
      "to": "D",
      "comment": "When M00 is active (holding A), pressing S outputs D"
    }
  ]
})";

const std::string TEST_CONFIG_VIM = R"({
  "version": "2.0",
  "keyboard": {
    "keys": {
      "Semicolon": "0x27",
      "H": "0x23",
      "J": "0x24",
      "K": "0x25",
      "L": "0x26",
      "Left": "0xE04B",
      "Down": "0xE050",
      "Up": "0xE048",
      "Right": "0xE04D",
      "Escape": "0x01"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "Semicolon",
      "tap": "Semicolon",
      "holdThresholdMs": 200
    }
  },
  "mappings": [
    {
      "from": "M00-H",
      "to": "Left",
      "comment": "Hold Semicolon + H -> Left arrow"
    },
    {
      "from": "M00-J",
      "to": "Down",
      "comment": "Hold Semicolon + J -> Down arrow"
    },
    {
      "from": "M00-K",
      "to": "Up",
      "comment": "Hold Semicolon + K -> Up arrow"
    },
    {
      "from": "M00-L",
      "to": "Right",
      "comment": "Hold Semicolon + L -> Right arrow"
    }
  ]
})";

// --- Manual Mocks ---

class MockWindowSystem : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override { return nullptr; }
    WindowHandle windowFromPoint(const Point&) override { return nullptr; }
    bool getWindowRect(WindowHandle, Rect*) override { return false; }
    std::string getWindowText(WindowHandle) override { return ""; }
    std::string getClassName(WindowHandle) override { return "MockWindowClass"; }
    std::string getTitleName(WindowHandle) override { return "MockTitle"; }
    uint32_t getWindowThreadId(WindowHandle) override { return 1; }
    uint32_t getWindowProcessId(WindowHandle) override { return 1; }
    bool setForegroundWindow(WindowHandle) override { return true; }
    bool moveWindow(WindowHandle, const Rect&) override { return true; }
    bool showWindow(WindowHandle, int) override { return true; }
    bool closeWindow(WindowHandle) override { return true; }
    WindowHandle getParent(WindowHandle) override { return nullptr; }
    bool isMDIChild(WindowHandle) override { return false; }
    bool isChild(WindowHandle) override { return false; }
    WindowShowCmd getShowCommand(WindowHandle) override { return WindowShowCmd::Normal; }
    bool isConsoleWindow(WindowHandle) override { return false; }
    void getCursorPos(Point*) override {}
    void setCursorPos(const Point&) override {}
    int getMonitorCount() override { return 1; }
    bool getMonitorRect(int, Rect*) override { return false; }
    bool getMonitorWorkArea(int, Rect*) override { return false; }
    int getMonitorIndex(WindowHandle) override { return 0; }
    int getSystemMetrics(SystemMetric) override { return 0; }
    bool getWorkArea(Rect*) override { return false; }
    std::string getClipboardText() override { return ""; }
    bool setClipboardText(const std::string&) override { return true; }
    bool getClientRect(WindowHandle, Rect*) override { return false; }
    bool getChildWindowRect(WindowHandle, Rect*) override { return false; }
    unsigned int mapVirtualKey(unsigned int) override { return 0; }
    bool postMessage(WindowHandle, unsigned int, uintptr_t, intptr_t) override { return true; }
    unsigned int registerWindowMessage(const std::string&) override { return 0; }
    bool sendMessageTimeout(WindowHandle, unsigned int, uintptr_t, intptr_t, unsigned int, unsigned int, uintptr_t*) override { return true; }
    bool sendCopyData(WindowHandle, WindowHandle, const CopyData&, uint32_t, uint32_t, uintptr_t*) override { return true; }
    bool setWindowZOrder(WindowHandle, ZOrder) override { return true; }
    bool isWindowTopMost(WindowHandle) override { return false; }
    bool isWindowLayered(WindowHandle) override { return false; }
    bool setWindowLayered(WindowHandle, bool) override { return true; }
    bool setLayeredWindowAttributes(WindowHandle, unsigned long, unsigned char, unsigned long) override { return true; }
    bool redrawWindow(WindowHandle) override { return true; }
    bool enumerateWindows(WindowEnumCallback) override { return true; }
    int shellExecute(const std::string&, const std::string&, const std::string&, const std::string&, int) override { return 0; }
    bool disconnectNamedPipe(void*) override { return true; }
    bool connectNamedPipe(void*, void*) override { return true; }
    bool writeFile(void*, const void*, unsigned int, unsigned int*, void*) override { return true; }
    void* openMutex(const std::string&) override { return nullptr; }
    void* openFileMapping(const std::string&) override { return nullptr; }
    void* mapViewOfFile(void*) override { return nullptr; }
    bool unmapViewOfFile(void*) override { return true; }
    void closeHandle(void*) override {}
    void* loadLibrary(const std::string&) override { return nullptr; }
    void* getProcAddress(void*, const std::string&) override { return nullptr; }
    bool freeLibrary(void*) override { return true; }
    WindowHandle getToplevelWindow(WindowHandle, bool*) override { return nullptr; }
    bool changeMessageFilter(uint32_t, uint32_t) override { return true; }
};

class MockInputInjector : public IInputInjector {
public:
    void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData) override {
        lastMakeCode = data->MakeCode;
        lastFlags = data->Flags;
        injectCallCount++;
    }
    void keyDown(KeyCode) override {}
    void keyUp(KeyCode) override {}
    void mouseMove(int32_t, int32_t) override {}
    void mouseButton(MouseButton, bool) override {}
    void mouseWheel(int32_t) override {}

    uint16_t lastMakeCode = 0;
    uint16_t lastFlags = 0;
    int injectCallCount = 0;

    void reset() {
        lastMakeCode = 0;
        lastFlags = 0;
        injectCallCount = 0;
    }
};

class MockInputHook : public IInputHook {
public:
    bool install(KeyCallback keyCallback, MouseCallback mouseCallback) override {
        capturedKeyCallback = keyCallback;
        return true;
    }
    void uninstall() override {}
    bool isInstalled() const override { return true; }

    KeyCallback capturedKeyCallback;
};

class MockInputDriver : public IInputDriver {
public:
    bool open(void*) override { return true; }
    void close() override {}
    void manageExtension(const std::string&, const std::string&, bool, void**) override {}
};

// --- Test Fixture ---

class M00IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        logStream = new tomsgstream(0);

        mockWindowSystem = new MockWindowSystem();
        mockInputInjector = new MockInputInjector();
        mockInputHook = new MockInputHook();
        mockInputDriver = new MockInputDriver();

        engine = new Engine(*logStream, mockWindowSystem, nullptr, mockInputInjector, mockInputHook, mockInputDriver);

        // Initialize EventSimulator with default config
        simulator = new EventSimulator();
    }

    void TearDown() override {
        engine->stop();
        delete engine;
        delete logStream;

        delete mockWindowSystem;
        delete mockInputInjector;
        delete mockInputHook;
        delete mockInputDriver;
        delete simulator;
    }

    void loadJsonConfig(const std::string& jsonContent) {
        // Write JSON to temp file
        const std::string tempPath = "/tmp/yamy_test_config.json";
        std::ofstream ofs(tempPath);
        ofs << jsonContent;
        ofs.close();

        // Load using JsonConfigLoader
        Setting* setting = new Setting;
        yamy::settings::JsonConfigLoader loader;
        bool success = loader.load(setting, tempPath);
        ASSERT_TRUE(success) << "Failed to load JSON config";

        // Start engine and apply setting
        engine->start();

        // Wait for engine to be fully initialized and ready
        bool isReady = simulator->waitForEngineReady(engine);
        ASSERT_TRUE(isReady) << "Engine failed to become ready within timeout";

        engine->setSetting(setting);

        // Wait for engine to fully process the setting and stabilize
        // This ensures EventProcessor, ModifierHandler, and rule tables are ready
        // Increased from 300ms to 500ms for more reliable EventProcessor registration
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void injectKey(uint16_t yamyScanCode, bool isKeyDown) {
        ASSERT_TRUE(mockInputHook->capturedKeyCallback) << "InputHook callback not captured";

        // Convert YAMY scan code to evdev code for proper event processing
        uint16_t evdevCode = EventSimulator::yamyToEvdev(yamyScanCode);

        KeyEvent event;
        event.scanCode = evdevCode;
        event.isKeyDown = isKeyDown;
        event.extraInfo = 0;

        mockInputHook->capturedKeyCallback(event);
    }

    void waitForProcessing(int maxRetries = 10) {
        int initialCount = mockInputInjector->injectCallCount;
        int retries = maxRetries;
        while (retries-- > 0 && mockInputInjector->injectCallCount == initialCount) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    tomsgstream* logStream;
    MockWindowSystem* mockWindowSystem;
    MockInputInjector* mockInputInjector;
    MockInputHook* mockInputHook;
    MockInputDriver* mockInputDriver;
    Engine* engine;
    EventSimulator* simulator;
};

// --- Tests ---

/**
 * Test 1: Tap A <200ms → should output B
 * CRITICAL: Verifies hold-vs-tap detection works in full engine context
 */
TEST_F(M00IntegrationTest, DISABLED_TapAShouldOutputB) {
    loadJsonConfig(TEST_CONFIG_M00);
    mockInputInjector->reset();

    // Create event sequence: Press A, wait 100ms, Release A
    std::vector<EventSimulator::Event> events = {
        EventSimulator::Event(30, true, 100),   // Press A (evdev=30), wait 100ms
        EventSimulator::Event(30, false, 0)     // Release A
    };

    // Inject sequence with proper timing
    simulator->injectSequence(mockInputHook->capturedKeyCallback, events);

    // Wait for async processing to complete
    bool outputReceived = simulator->waitForOutput(mockInputInjector, 1);
    ASSERT_TRUE(outputReceived) << "No output generated within timeout";

    // Should output B (0x30)
    EXPECT_EQ(mockInputInjector->lastMakeCode, 0x30)
        << "CRITICAL BUG: Tap A should output B (0x30)! Got 0x"
        << std::hex << mockInputInjector->lastMakeCode;
}

/**
 * Test 2: Hold A >200ms + press S → should output D
 * CRITICAL: Verifies M00 activation and rule matching through full engine
 */
TEST_F(M00IntegrationTest, DISABLED_HoldAPlusShouldOutputD) {
    loadJsonConfig(TEST_CONFIG_M00);
    mockInputInjector->reset();

    // Create event sequence: Press A, wait 250ms (exceed threshold), Press S
    std::vector<EventSimulator::Event> events = {
        EventSimulator::Event(30, true, 250),   // Press A (evdev=30), wait 250ms to exceed threshold
        EventSimulator::Event(31, true, 0)      // Press S (evdev=31)
    };

    // Inject sequence with proper timing
    simulator->injectSequence(mockInputHook->capturedKeyCallback, events);

    // Wait for async processing to complete
    bool outputReceived = simulator->waitForOutput(mockInputInjector, 1);
    ASSERT_TRUE(outputReceived) << "No output generated within timeout";

    // Should output D (0x20)
    EXPECT_EQ(mockInputInjector->lastMakeCode, 0x20)
        << "CRITICAL BUG: M00+S should output D (0x20)! Got 0x"
        << std::hex << mockInputInjector->lastMakeCode;
}

/**
 * Test 3: Vim-mode Semicolon+H → LEFT arrow
 * CRITICAL: Verifies M00 works with arrow keys (extended scan codes)
 */
TEST_F(M00IntegrationTest, DISABLED_VimModeSemicolonPlusHOutputsLeft) {
    loadJsonConfig(TEST_CONFIG_VIM);
    mockInputInjector->reset();

    // Create event sequence: Press Semicolon, wait 250ms (exceed threshold), Press H
    std::vector<EventSimulator::Event> events = {
        EventSimulator::Event(39, true, 250),   // Press Semicolon (evdev=39), wait 250ms
        EventSimulator::Event(35, true, 0)      // Press H (evdev=35)
    };

    // Inject sequence with proper timing
    simulator->injectSequence(mockInputHook->capturedKeyCallback, events);

    // Wait for async processing to complete
    bool outputReceived = simulator->waitForOutput(mockInputInjector, 1);
    ASSERT_TRUE(outputReceived) << "No output generated within timeout";

    // Should output LEFT (0xE04B)
    EXPECT_EQ(mockInputInjector->lastMakeCode, 0xE04B)
        << "CRITICAL BUG: M00+H should output LEFT (0xE04B)! Got 0x"
        << std::hex << mockInputInjector->lastMakeCode;
}

/**
 * Test 4: Vim-mode Semicolon tap → Semicolon
 * Verifies tap detection for Semicolon trigger
 */
TEST_F(M00IntegrationTest, DISABLED_VimModeSemicolonTapOutputsSemicolon) {
    loadJsonConfig(TEST_CONFIG_VIM);
    mockInputInjector->reset();

    // Create event sequence: Press Semicolon, wait 100ms (under threshold), Release
    std::vector<EventSimulator::Event> events = {
        EventSimulator::Event(39, true, 100),   // Press Semicolon (evdev=39), wait 100ms
        EventSimulator::Event(39, false, 0)     // Release Semicolon
    };

    // Inject sequence with proper timing
    simulator->injectSequence(mockInputHook->capturedKeyCallback, events);

    // Wait for async processing to complete
    bool outputReceived = simulator->waitForOutput(mockInputInjector, 1);
    ASSERT_TRUE(outputReceived) << "No output generated within timeout";

    // Should output Semicolon (0x27)
    EXPECT_EQ(mockInputInjector->lastMakeCode, 0x27)
        << "Tap Semicolon should output Semicolon (0x27)! Got 0x"
        << std::hex << mockInputInjector->lastMakeCode;
}

/**
 * Test 5: All vim arrow keys (HJKL → Left/Down/Up/Right)
 * Comprehensive test for all 4 arrow key mappings
 */
TEST_F(M00IntegrationTest, DISABLED_VimModeAllArrowKeys) {
    loadJsonConfig(TEST_CONFIG_VIM);

    struct TestCase {
        uint16_t inputEvdev;
        uint16_t expectedScan;
        std::string name;
    };

    TestCase cases[] = {
        {35, 0xE04B, "H→LEFT"},    // H evdev=35
        {36, 0xE050, "J→DOWN"},    // J evdev=36
        {37, 0xE048, "K→UP"},      // K evdev=37
        {38, 0xE04D, "L→RIGHT"}    // L evdev=38
    };

    for (const auto& tc : cases) {
        mockInputInjector->reset();

        // Create event sequence: Press Semicolon, wait 250ms, Press key (H/J/K/L), Release all
        std::vector<EventSimulator::Event> events = {
            EventSimulator::Event(39, true, 250),       // Press Semicolon (evdev=39), wait 250ms
            EventSimulator::Event(tc.inputEvdev, true, 50),  // Press test key, wait 50ms
            EventSimulator::Event(tc.inputEvdev, false, 10), // Release test key, wait 10ms
            EventSimulator::Event(39, false, 0)         // Release Semicolon
        };

        // Inject sequence with proper timing
        simulator->injectSequence(mockInputHook->capturedKeyCallback, events);

        // Wait for async processing to complete
        bool outputReceived = simulator->waitForOutput(mockInputInjector, 1);
        ASSERT_TRUE(outputReceived) << "No output generated for " << tc.name;

        // Verify output
        EXPECT_EQ(mockInputInjector->lastMakeCode, tc.expectedScan)
            << "CRITICAL BUG: " << tc.name << " failed! Expected 0x"
            << std::hex << tc.expectedScan << " got 0x" << mockInputInjector->lastMakeCode;

        // Wait between test cases for engine to stabilize
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
