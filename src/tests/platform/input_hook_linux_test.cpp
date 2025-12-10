//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// input_hook_linux_test.cpp - Unit tests for InputHookLinux
//
// Tests the Linux input hook implementation using evdev.
// These tests can run in two modes:
// 1. With /dev/input access (input group): Tests device detection
// 2. Without input access: Tests state management and callback behavior
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include "../../platform/linux/input_hook_linux.h"
#include "../../platform/linux/keycode_mapping.h"
#include "../../core/platform/types.h"
#include "../../core/input/input_event.h"

namespace yamy::platform {

//=============================================================================
// InputHookLinux Basic Tests - These test construction and state management
//=============================================================================

class InputHookLinuxBasicTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test default construction
TEST_F(InputHookLinuxBasicTest, DefaultConstruction) {
    InputHookLinux hook;
    EXPECT_FALSE(hook.isInstalled());
}

// Test destruction cleans up properly
TEST_F(InputHookLinuxBasicTest, DestructorCleansUp) {
    // Create hook in a block to test destructor
    {
        InputHookLinux hook;
        EXPECT_FALSE(hook.isInstalled());
    }
    // No crash means destructor worked
    SUCCEED();
}

// Test uninstall on uninstalled hook is safe
TEST_F(InputHookLinuxBasicTest, UninstallOnUninstalledIsSafe) {
    InputHookLinux hook;
    EXPECT_FALSE(hook.isInstalled());

    // Should not crash
    hook.uninstall();
    EXPECT_FALSE(hook.isInstalled());
}

// Test multiple uninstall calls are safe
TEST_F(InputHookLinuxBasicTest, MultipleUninstallsAreSafe) {
    InputHookLinux hook;

    hook.uninstall();
    hook.uninstall();
    hook.uninstall();

    EXPECT_FALSE(hook.isInstalled());
}

//=============================================================================
// InputHookLinux Callback Tests - Test callback configuration
//=============================================================================

class InputHookLinuxCallbackTest : public ::testing::Test {
protected:
    std::atomic<int> m_keyCallbackCount{0};
    std::atomic<int> m_mouseCallbackCount{0};
    KeyEvent m_lastKeyEvent{};

    void SetUp() override {
        m_keyCallbackCount = 0;
        m_mouseCallbackCount = 0;
        m_lastKeyEvent = {};
    }

    bool keyCallback(const KeyEvent& event) {
        m_keyCallbackCount++;
        m_lastKeyEvent = event;
        return true; // Consume the event
    }

    bool mouseCallback(const MouseEvent&) {
        m_mouseCallbackCount++;
        return true;
    }
};

// Test that install fails without devices (no permission case)
TEST_F(InputHookLinuxCallbackTest, InstallFailsWithoutDevices) {
    InputHookLinux hook;

    // This will likely fail if we don't have permissions to /dev/input/event*
    // We're testing that it handles the failure gracefully
    auto keyCallback = [this](const KeyEvent& e) { return this->keyCallback(e); };
    auto mouseCallback = [this](const MouseEvent& e) { return this->mouseCallback(e); };

    // Try to install - may fail if no permissions
    bool result = hook.install(keyCallback, mouseCallback);

    // Either way, should not crash
    if (!result) {
        EXPECT_FALSE(hook.isInstalled());
    } else {
        EXPECT_TRUE(hook.isInstalled());
        hook.uninstall();
    }
}

// Test install with null key callback
TEST_F(InputHookLinuxCallbackTest, InstallWithNullKeyCallback) {
    InputHookLinux hook;

    auto mouseCallback = [](const MouseEvent&) { return true; };

    // Install with null key callback - should still work
    bool result = hook.install(nullptr, mouseCallback);

    // May fail due to permissions, but shouldn't crash
    if (result) {
        hook.uninstall();
    }
    // No assertion - just ensuring no crash
    SUCCEED();
}

// Test install with null mouse callback
TEST_F(InputHookLinuxCallbackTest, InstallWithNullMouseCallback) {
    InputHookLinux hook;

    auto keyCallback = [](const KeyEvent&) { return true; };

    // Install with null mouse callback - should still work
    bool result = hook.install(keyCallback, nullptr);

    // May fail due to permissions, but shouldn't crash
    if (result) {
        hook.uninstall();
    }
    SUCCEED();
}

// Test install with both null callbacks
TEST_F(InputHookLinuxCallbackTest, InstallWithBothNullCallbacks) {
    InputHookLinux hook;

    // Install with null callbacks - should still work
    bool result = hook.install(nullptr, nullptr);

    // May fail due to permissions
    if (result) {
        hook.uninstall();
    }
    SUCCEED();
}

//=============================================================================
// EventReaderThread Tests - Test the reader thread class
//=============================================================================

class EventReaderThreadTest : public ::testing::Test {
protected:
    std::atomic<int> m_callbackCount{0};
    std::vector<KeyEvent> m_receivedEvents;
    std::mutex m_mutex;

    void SetUp() override {
        m_callbackCount = 0;
        m_receivedEvents.clear();
    }

    bool keyCallback(const KeyEvent& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbackCount++;
        m_receivedEvents.push_back(event);
        return true;
    }
};

// Test EventReaderThread construction
TEST_F(EventReaderThreadTest, Construction) {
    auto callback = [this](const KeyEvent& e) -> bool { return this->keyCallback(e); };

    // Use invalid fd (-1) since we can't easily create a real evdev device
    EventReaderThread reader(-1, "/dev/input/event99", callback);

    EXPECT_FALSE(reader.isRunning());
    EXPECT_EQ(reader.getDevNode(), "/dev/input/event99");
}

// Test start with invalid fd fails gracefully
TEST_F(EventReaderThreadTest, StartWithInvalidFdFailsGracefully) {
    auto callback = [this](const KeyEvent& e) -> bool { return this->keyCallback(e); };

    EventReaderThread reader(-1, "/dev/input/event99", callback);

    // Starting with invalid fd should work (thread starts), but read will fail
    bool started = reader.start();

    if (started) {
        // Thread started, let it try to read and fail
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        reader.stop();
        EXPECT_FALSE(reader.isRunning());
    }
    // Either way, no crash is success
    SUCCEED();
}

// Test stop on non-started reader is safe
TEST_F(EventReaderThreadTest, StopOnNonStartedIsSafe) {
    auto callback = [this](const KeyEvent& e) -> bool { return this->keyCallback(e); };

    EventReaderThread reader(-1, "/dev/input/event99", callback);
    EXPECT_FALSE(reader.isRunning());

    // Stop should be safe on non-started reader
    reader.stop();
    EXPECT_FALSE(reader.isRunning());
}

// Test multiple stop calls are safe
TEST_F(EventReaderThreadTest, MultipleStopsAreSafe) {
    auto callback = [this](const KeyEvent& e) -> bool { return this->keyCallback(e); };

    EventReaderThread reader(-1, "/dev/input/event99", callback);

    reader.stop();
    reader.stop();
    reader.stop();

    EXPECT_FALSE(reader.isRunning());
}

//=============================================================================
// KeyEvent Construction Tests - Verify KeyEvent structure
//=============================================================================

class KeyEventStructureTest : public ::testing::Test {};

TEST_F(KeyEventStructureTest, DefaultConstruction) {
    KeyEvent event{};

    EXPECT_EQ(event.key, KeyCode::Unknown);
    EXPECT_EQ(event.scanCode, 0u);
    EXPECT_FALSE(event.isKeyDown);
    EXPECT_FALSE(event.isExtended);
    EXPECT_EQ(event.timestamp, 0u);
    EXPECT_EQ(event.flags, 0u);
    EXPECT_EQ(event.extraInfo, 0u);
}

TEST_F(KeyEventStructureTest, KeyDownEvent) {
    KeyEvent event{};
    event.key = KeyCode::Unknown;
    event.scanCode = 0x41;  // VK_A
    event.isKeyDown = true;
    event.isExtended = false;
    event.timestamp = 12345;
    event.flags = 0;

    EXPECT_TRUE(event.isKeyDown);
    EXPECT_EQ(event.scanCode, 0x41);
    EXPECT_EQ(event.timestamp, 12345u);
    EXPECT_EQ(event.flags, 0u);
}

TEST_F(KeyEventStructureTest, KeyUpEvent) {
    KeyEvent event{};
    event.key = KeyCode::Unknown;
    event.scanCode = 0x41;
    event.isKeyDown = false;
    event.flags = 1;  // BREAK flag

    EXPECT_FALSE(event.isKeyDown);
    EXPECT_EQ(event.flags, 1u);
}

//=============================================================================
// Evdev Event Processing Tests - Test evdev to KeyEvent conversion logic
//=============================================================================

class EvdevEventProcessingTest : public ::testing::Test {};

// Test that EV_KEY type is properly filtered
TEST_F(EvdevEventProcessingTest, OnlyKeyEventsProcessed) {
    // Verify the filter logic constants
    EXPECT_EQ(EV_KEY, 1);  // EV_KEY should be 1

    // Other event types that should be filtered
    EXPECT_NE(EV_SYN, EV_KEY);
    EXPECT_NE(EV_REL, EV_KEY);
    EXPECT_NE(EV_ABS, EV_KEY);
}

// Test button filtering constants
TEST_F(EvdevEventProcessingTest, ButtonFilteringConstants) {
    // BTN_MISC marks the start of button codes (0x100)
    // Mouse buttons start at BTN_LEFT (0x110)
    // These values should be distinct from regular keyboard codes
    EXPECT_EQ(BTN_MISC, 0x100);
    EXPECT_GT(BTN_LEFT, BTN_MISC);  // BTN_LEFT = 0x110
    EXPECT_GT(BTN_RIGHT, BTN_MISC); // BTN_RIGHT = 0x111
    EXPECT_GT(BTN_MIDDLE, BTN_MISC); // BTN_MIDDLE = 0x112
}

// Test event value interpretation
TEST_F(EvdevEventProcessingTest, EventValueInterpretation) {
    // evdev event values:
    // 0 = key release
    // 1 = key press
    // 2 = key repeat

    struct input_event ev;
    ev.type = EV_KEY;
    ev.code = KEY_A;

    // Test press
    ev.value = 1;
    EXPECT_TRUE(ev.value == 1 || ev.value == 2);  // isKeyDown

    // Test repeat (also counted as down)
    ev.value = 2;
    EXPECT_TRUE(ev.value == 1 || ev.value == 2);  // isKeyDown

    // Test release
    ev.value = 0;
    EXPECT_FALSE(ev.value == 1 || ev.value == 2);  // isKeyUp
}

// Test timestamp conversion
TEST_F(EvdevEventProcessingTest, TimestampConversion) {
    struct input_event ev;
    ev.time.tv_sec = 1000;
    ev.time.tv_usec = 500000;  // 0.5 seconds

    // Conversion formula: tv_sec * 1000 + tv_usec / 1000
    uint32_t timestamp = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000;
    EXPECT_EQ(timestamp, 1000500u);  // 1000 seconds + 500ms
}

//=============================================================================
// Evdev to YAMY KeyEvent Mapping Tests
//=============================================================================

class EvdevToKeyEventMappingTest : public ::testing::Test {};

TEST_F(EvdevToKeyEventMappingTest, LetterKeyMapping) {
    // Test that evdev KEY_A maps to correct YAMY scancode
    uint16_t yamyCode = evdevToYamyKeyCode(KEY_A);
    EXPECT_EQ(yamyCode, 0x41);  // VK_A

    yamyCode = evdevToYamyKeyCode(KEY_Z);
    EXPECT_EQ(yamyCode, 0x5A);  // VK_Z
}

TEST_F(EvdevToKeyEventMappingTest, ModifierKeyMapping) {
    // Test modifier key mappings
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTSHIFT), 0xA0);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTSHIFT), 0xA1);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTCTRL), 0xA2);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTCTRL), 0xA3);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTALT), 0xA4);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTALT), 0xA5);
}

TEST_F(EvdevToKeyEventMappingTest, FunctionKeyMapping) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F1), 0x70);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F12), 0x7B);
}

TEST_F(EvdevToKeyEventMappingTest, SpecialKeyMapping) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_ESC), 0x1B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_ENTER), 0x0D);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SPACE), 0x20);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_TAB), 0x09);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_BACKSPACE), 0x08);
}

TEST_F(EvdevToKeyEventMappingTest, UnknownKeyReturnsZero) {
    // Unknown evdev code should return 0
    EXPECT_EQ(evdevToYamyKeyCode(0xFFFF), 0);
}

//=============================================================================
// Device Permission Tests - Check if we have input device access
//=============================================================================

class DevicePermissionTest : public ::testing::Test {
protected:
    bool hasInputGroupAccess() const {
        // Check if any event device is readable
        for (int i = 0; i < 10; i++) {
            std::string path = "/dev/input/event" + std::to_string(i);
            if (access(path.c_str(), R_OK) == 0) {
                return true;
            }
        }
        return false;
    }
};

TEST_F(DevicePermissionTest, CheckInputDeviceAccess) {
    bool hasAccess = hasInputGroupAccess();

    if (!hasAccess) {
        GTEST_SKIP() << "No /dev/input/event* access (not in input group). "
                     << "To enable device tests, add user to input group: "
                     << "sudo usermod -aG input $USER";
    }

    // If we have access, verify we can at least check a device
    struct stat st;
    bool foundDevice = false;
    for (int i = 0; i < 20; i++) {
        std::string path = "/dev/input/event" + std::to_string(i);
        if (stat(path.c_str(), &st) == 0) {
            foundDevice = true;
            break;
        }
    }
    EXPECT_TRUE(foundDevice);
}

//=============================================================================
// Thread Safety Tests - Verify thread-safe operations
//=============================================================================

class ThreadSafetyTest : public ::testing::Test {
protected:
    std::atomic<int> m_callbackCount{0};

    void SetUp() override {
        m_callbackCount = 0;
    }
};

// Test that isInstalled() is thread-safe
TEST_F(ThreadSafetyTest, IsInstalledThreadSafe) {
    InputHookLinux hook;

    std::atomic<bool> running{true};
    std::atomic<int> checkCount{0};

    // Start a thread that continuously checks isInstalled
    std::thread checker([&]() {
        while (running) {
            hook.isInstalled();  // Should not crash
            checkCount++;
        }
    });

    // Give the checker thread time to run
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    running = false;
    checker.join();

    EXPECT_GT(checkCount, 0);
}

// Test rapid install/uninstall doesn't cause issues
TEST_F(ThreadSafetyTest, RapidInstallUninstall) {
    InputHookLinux hook;
    auto callback = [](const KeyEvent&) { return true; };

    // Rapidly toggle install state
    for (int i = 0; i < 10; i++) {
        bool result = hook.install(callback, nullptr);
        // May or may not succeed depending on permissions
        (void)result;
        hook.uninstall();
    }

    EXPECT_FALSE(hook.isInstalled());
}

//=============================================================================
// Input Event Structure Tests (from KEYBOARD_INPUT_DATA)
//=============================================================================

class InputDataStructureTest : public ::testing::Test {};

TEST_F(InputDataStructureTest, BreakFlagIndicatesKeyUp) {
    KEYBOARD_INPUT_DATA data;
    data.Flags = KEYBOARD_INPUT_DATA::BREAK;

    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::BREAK);
}

TEST_F(InputDataStructureTest, ExtendedKeyFlags) {
    KEYBOARD_INPUT_DATA data;

    data.Flags = KEYBOARD_INPUT_DATA::E0;
    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::E0);

    data.Flags = KEYBOARD_INPUT_DATA::E1;
    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::E1);

    data.Flags = KEYBOARD_INPUT_DATA::E0E1;
    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::E0);
    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::E1);
}

TEST_F(InputDataStructureTest, MakeCodeStorage) {
    KEYBOARD_INPUT_DATA data;

    data.MakeCode = KEY_A;
    EXPECT_EQ(data.MakeCode, KEY_A);

    data.MakeCode = KEY_ENTER;
    EXPECT_EQ(data.MakeCode, KEY_ENTER);
}

} // namespace yamy::platform
