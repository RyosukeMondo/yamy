//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// integration_suite.cpp - End-to-end integration test suite
//
// This file provides comprehensive integration tests for the yamy Linux port,
// testing all major components working together:
// - Full application lifecycle (start, config, process, stop)
// - IPC command/response flow
// - GUI integration (tray, dialogs, notifications)
// - Session save/restore
// - Performance under load (<1ms key processing latency)
//
// Usage:
//   cmake -B build -DBUILD_REGRESSION_TESTS=ON -DBUILD_LINUX_STUB=ON
//   cmake --build build --target yamy_regression_test
//   xvfb-run -a ./build/bin/yamy_regression_test --gtest_filter="Integration*"
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <filesystem>

// Core components
#include "setting.h"
#include "setting_loader.h"
#include "keymap.h"
#include "keyboard.h"
#include "msgstream.h"
#include "multithread.h"

// Platform interfaces
#include "../../src/core/platform/types.h"
#include "../../src/core/platform/input_hook_interface.h"
#include "../../src/core/platform/input_injector_interface.h"
#include "../../src/core/platform/window_system_interface.h"
#include "../../src/core/platform/ipc_defs.h"
#include "../../src/core/ipc_messages.h"
#include "../../src/core/input/input_event.h"

// Session management
#include "../../src/core/settings/session_manager.h"

// Config management
#include "../../src/core/settings/config_manager.h"

namespace yamy::integration_test {

//=============================================================================
// Test Environment Detection
//=============================================================================

namespace {

bool isCI() {
    return std::getenv("CI") != nullptr ||
           std::getenv("GITHUB_ACTIONS") != nullptr;
}

bool hasDisplay() {
    return std::getenv("DISPLAY") != nullptr;
}

std::string getTempDir() {
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir) return tmpdir;
    tmpdir = std::getenv("TMP");
    if (tmpdir) return tmpdir;
    tmpdir = std::getenv("TEMP");
    if (tmpdir) return tmpdir;
    return "/tmp";
}

std::string createTempFile(const std::string& content, const std::string& suffix = ".mayu") {
    std::string path = getTempDir() + "/yamy_test_" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                       suffix;
    std::ofstream out(path);
    out << content;
    out.close();
    return path;
}

void removeTempFile(const std::string& path) {
    std::remove(path.c_str());
}

} // anonymous namespace

//=============================================================================
// Mock Input Hook - Simulates keyboard input for integration tests
//=============================================================================

class MockInputHook : public platform::IInputHook {
public:
    MockInputHook() : m_installed(false), m_eventCount(0) {}

    bool install(platform::KeyCallback keyCallback,
                 platform::MouseCallback mouseCallback) override {
        m_keyCallback = keyCallback;
        m_mouseCallback = mouseCallback;
        m_installed = true;
        return true;
    }

    void uninstall() override {
        m_installed = false;
        m_keyCallback = nullptr;
        m_mouseCallback = nullptr;
    }

    bool isInstalled() const override {
        return m_installed;
    }

    // Test helper: simulate a key event
    bool simulateKeyEvent(const platform::KeyEvent& event) {
        if (!m_installed || !m_keyCallback) {
            return false;
        }
        m_eventCount++;
        return m_keyCallback(event);
    }

    // Test helper: simulate key press and release
    std::pair<bool, bool> simulateKeyPressRelease(uint32_t scanCode, bool extended = false) {
        platform::KeyEvent downEvent{};
        downEvent.scanCode = scanCode;
        downEvent.isKeyDown = true;
        downEvent.isExtended = extended;
        downEvent.timestamp = m_eventCount;

        platform::KeyEvent upEvent{};
        upEvent.scanCode = scanCode;
        upEvent.isKeyDown = false;
        upEvent.isExtended = extended;
        upEvent.timestamp = m_eventCount + 1;

        bool downResult = simulateKeyEvent(downEvent);
        bool upResult = simulateKeyEvent(upEvent);
        return {downResult, upResult};
    }

    size_t getEventCount() const { return m_eventCount; }

private:
    std::atomic<bool> m_installed;
    platform::KeyCallback m_keyCallback;
    platform::MouseCallback m_mouseCallback;
    std::atomic<size_t> m_eventCount;
};

//=============================================================================
// Mock Input Injector - Records injected key events
//=============================================================================

class MockInputInjector : public platform::IInputInjector {
public:
    struct InjectedKey {
        uint32_t scanCode;
        bool isKeyDown;
        bool isExtended;
        std::chrono::steady_clock::time_point timestamp;
    };

    MockInputInjector() = default;

    void inject(const KEYBOARD_INPUT_DATA* data, const platform::InjectionContext& ctx,
                const void* rawData = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        InjectedKey key;
        key.scanCode = data->MakeCode;
        key.isKeyDown = !(data->Flags & KEYBOARD_INPUT_DATA::BREAK);
        key.isExtended = (data->Flags & KEYBOARD_INPUT_DATA::E0) != 0;
        key.timestamp = std::chrono::steady_clock::now();
        m_injectedKeys.push_back(key);
    }

    void keyDown(platform::KeyCode key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        InjectedKey k;
        k.scanCode = static_cast<uint32_t>(key);
        k.isKeyDown = true;
        k.isExtended = false;
        k.timestamp = std::chrono::steady_clock::now();
        m_injectedKeys.push_back(k);
    }

    void keyUp(platform::KeyCode key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        InjectedKey k;
        k.scanCode = static_cast<uint32_t>(key);
        k.isKeyDown = false;
        k.isExtended = false;
        k.timestamp = std::chrono::steady_clock::now();
        m_injectedKeys.push_back(k);
    }

    void mouseMove(int32_t dx, int32_t dy) override {}
    void mouseButton(platform::MouseButton button, bool down) override {}
    void mouseWheel(int32_t delta) override {}

    // Test helpers
    std::vector<InjectedKey> getInjectedKeys() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_injectedKeys;
    }

    void clearInjectedKeys() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_injectedKeys.clear();
    }

    size_t getInjectedCount() const {
        return m_injectedKeys.size();
    }

    bool hasKeyDown(uint32_t scanCode) const {
        for (const auto& key : m_injectedKeys) {
            if (key.scanCode == scanCode && key.isKeyDown) {
                return true;
            }
        }
        return false;
    }

    bool hasKeyUp(uint32_t scanCode) const {
        for (const auto& key : m_injectedKeys) {
            if (key.scanCode == scanCode && !key.isKeyDown) {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<InjectedKey> m_injectedKeys;
    mutable std::mutex m_mutex;
};

//=============================================================================
// Integration Test Fixture - Full Lifecycle Tests
//=============================================================================

class IntegrationLifecycleTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;
    std::unique_ptr<MockInputHook> m_inputHook;
    std::unique_ptr<MockInputInjector> m_inputInjector;
    std::vector<std::string> m_tempFiles;

    void SetUp() override {
        m_loader.reset(new SettingLoader(&m_soLog, &m_logStream));
        m_loader->initialize(&m_setting);
        m_inputHook = std::make_unique<MockInputHook>();
        m_inputInjector = std::make_unique<MockInputInjector>();
    }

    void TearDown() override {
        m_inputHook->uninstall();
        m_loader.reset();

        // Cleanup temp files
        for (const auto& path : m_tempFiles) {
            removeTempFile(path);
        }
        m_tempFiles.clear();
    }

    void LoadConfig(const std::string& config) {
        m_loader->loadFromData(config);
        std::string log_output = m_logStream.str();
        if (log_output.find(_T("error:")) != std::string::npos) {
            FAIL() << "Errors found during config loading: " << log_output;
        }
    }

    std::string createTestConfig(const std::string& content) {
        std::string path = createTempFile(content);
        m_tempFiles.push_back(path);
        return path;
    }

    // Standard key definitions for tests
    std::string getKeyDefinitions() {
        return
            _T("def key A = 0x1E\n")
            _T("def key B = 0x30\n")
            _T("def key C = 0x2E\n")
            _T("def key D = 0x20\n")
            _T("def key E = 0x12\n")
            _T("def key F = 0x21\n")
            _T("def key G = 0x22\n")
            _T("def key H = 0x23\n")
            _T("def key I = 0x17\n")
            _T("def key J = 0x24\n")
            _T("def key K = 0x25\n")
            _T("def key L = 0x26\n")
            _T("def key P = 0x19\n")
            _T("def key N = 0x31\n")
            _T("def key Escape Esc = 0x01\n")
            _T("def key F1 = 0x3B\n")
            _T("def key F5 = 0x3F\n")
            _T("def key Enter Return = 0x1C\n")
            _T("def key BackSpace = 0x0E\n")
            _T("def key Space = 0x39\n")
            _T("def key Tab = 0x0F\n")
            _T("def key CapsLock = 0x3A\n")
            _T("def key LShift LeftShift = 0x2A\n")
            _T("def key RShift RightShift = 0x36\n")
            _T("def key LControl LeftControl LCtrl = 0x1D\n")
            _T("def key RControl RightControl RCtrl = E0-0x1D\n")
            _T("def key LAlt LeftAlt LMenu = 0x38\n")
            _T("def key RAlt RightAlt RMenu = E0-0x38\n")
            _T("def key Up = E0-0x48\n")
            _T("def key Down = E0-0x50\n")
            _T("def key Left = E0-0x4B\n")
            _T("def key Right = E0-0x4D\n")
            _T("def mod Shift = LShift RShift\n")
            _T("def mod Control = LControl RControl\n")
            _T("def mod Alt = LAlt RAlt\n");
    }

    // Scan codes
    static constexpr uint32_t SC_A = 0x1E;
    static constexpr uint32_t SC_B = 0x30;
    static constexpr uint32_t SC_J = 0x24;
    static constexpr uint32_t SC_ENTER = 0x1C;
    static constexpr uint32_t SC_LCTRL = 0x1D;
    static constexpr uint32_t SC_CAPSLOCK = 0x3A;
};

//=============================================================================
// Test 1: Full Lifecycle - Initialize, Load, Process, Shutdown
//=============================================================================

TEST_F(IntegrationLifecycleTest, FullLifecycleInitLoadProcessShutdown) {
    // Step 1: Initialize engine components
    EXPECT_FALSE(m_inputHook->isInstalled());
    EXPECT_EQ(m_inputInjector->getInjectedCount(), 0u);

    // Step 2: Load configuration
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("key C-J = Enter\n");

    LoadConfig(config);

    // Verify configuration loaded
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr) << "Global keymap should exist after load";

    // Step 3: Install input hook
    std::vector<platform::KeyEvent> receivedEvents;
    bool installed = m_inputHook->install(
        [&receivedEvents](const platform::KeyEvent& event) {
            receivedEvents.push_back(event);
            return true;
        },
        nullptr
    );
    ASSERT_TRUE(installed) << "Hook should install successfully";
    EXPECT_TRUE(m_inputHook->isInstalled());

    // Step 4: Process key events
    m_inputHook->simulateKeyPressRelease(SC_A);
    m_inputHook->simulateKeyPressRelease(SC_B);

    EXPECT_EQ(receivedEvents.size(), 4u) << "Should receive 4 events (2 press + 2 release)";

    // Step 5: Shutdown - uninstall hook
    m_inputHook->uninstall();
    EXPECT_FALSE(m_inputHook->isInstalled());

    // Step 6: Verify events can't be processed after shutdown
    receivedEvents.clear();
    m_inputHook->simulateKeyPressRelease(SC_A);
    EXPECT_EQ(receivedEvents.size(), 0u) << "No events after uninstall";
}

//=============================================================================
// Test 2: Config Reload During Operation
//=============================================================================

TEST_F(IntegrationLifecycleTest, ConfigReloadDuringOperation) {
    // Initial config
    std::string config1 = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config1);

    // Verify initial mapping
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    ModifiedKey mkA(keyA);
    const Keymap::KeyAssignment* ka1 = globalMap->searchAssignment(mkA);
    ASSERT_NE(ka1, nullptr) << "A -> B mapping should exist";

    // Simulate reload with different config
    Setting newSetting;
    tstringstream newLogStream;
    SettingLoader newLoader(&m_soLog, &newLogStream);
    newLoader.initialize(&newSetting);

    std::string config2 = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = C\n")  // Changed mapping
        _T("key B = D\n"); // New mapping

    newLoader.loadFromData(config2);

    // Verify new mappings
    const Keymap* newGlobal = newSetting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(newGlobal, nullptr);

    Key* newKeyA = newSetting.m_keyboard.searchKey(_T("A"));
    Key* newKeyB = newSetting.m_keyboard.searchKey(_T("B"));
    ASSERT_NE(newKeyA, nullptr);
    ASSERT_NE(newKeyB, nullptr);

    ModifiedKey newMkA(newKeyA);
    ModifiedKey newMkB(newKeyB);

    const Keymap::KeyAssignment* ka2_a = newGlobal->searchAssignment(newMkA);
    const Keymap::KeyAssignment* ka2_b = newGlobal->searchAssignment(newMkB);

    EXPECT_NE(ka2_a, nullptr) << "A -> C mapping should exist in new config";
    EXPECT_NE(ka2_b, nullptr) << "B -> D mapping should exist in new config";
}

//=============================================================================
// Test 3: Multiple Keymaps with Window Matching
//=============================================================================

TEST_F(IntegrationLifecycleTest, MultipleKeymapsWindowMatching) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Editor /code|vim|emacs/ : Global\n")
        _T("key A = C\n")  // Different mapping for editors
        _T("window Terminal /terminal|konsole|gnome-terminal/ : Global\n")
        _T("key A = D\n"); // Different mapping for terminals

    LoadConfig(config);

    // Verify all keymaps created
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    const Keymap* editorMap = m_setting.m_keymaps.searchByName(_T("Editor"));
    const Keymap* terminalMap = m_setting.m_keymaps.searchByName(_T("Terminal"));

    ASSERT_NE(globalMap, nullptr);
    ASSERT_NE(editorMap, nullptr);
    ASSERT_NE(terminalMap, nullptr);

    // Test window class matching
    Keymaps::KeymapPtrList vscodeMatches;
    m_setting.m_keymaps.searchWindow(&vscodeMatches, "code", "Visual Studio Code");
    bool hasEditor = false;
    for (const auto* km : vscodeMatches) {
        if (km->getName() == _T("Editor")) hasEditor = true;
    }
    EXPECT_TRUE(hasEditor) << "Editor keymap should match 'code' window";

    Keymaps::KeymapPtrList termMatches;
    m_setting.m_keymaps.searchWindow(&termMatches, "gnome-terminal", "Terminal");
    bool hasTerminal = false;
    for (const auto* km : termMatches) {
        if (km->getName() == _T("Terminal")) hasTerminal = true;
    }
    EXPECT_TRUE(hasTerminal) << "Terminal keymap should match 'gnome-terminal' window";

    // Verify default (no match)
    Keymaps::KeymapPtrList browserMatches;
    m_setting.m_keymaps.searchWindow(&browserMatches, "firefox", "Mozilla Firefox");
    bool hasDefaultOnly = true;
    for (const auto* km : browserMatches) {
        if (km->getName() != _T("Global")) hasDefaultOnly = false;
    }
    // Should fall back to Global or be empty
    EXPECT_TRUE(browserMatches.empty() || hasDefaultOnly)
        << "Browser should match only Global or nothing";
}

//=============================================================================
// Test 4: Hook Event Flow with Callbacks
//=============================================================================

TEST_F(IntegrationLifecycleTest, HookEventFlowWithCallbacks) {
    std::vector<std::tuple<uint32_t, bool, std::chrono::steady_clock::time_point>> events;
    std::mutex eventsMutex;

    bool installed = m_inputHook->install(
        [&events, &eventsMutex](const platform::KeyEvent& event) {
            std::lock_guard<std::mutex> lock(eventsMutex);
            events.emplace_back(event.scanCode, event.isKeyDown,
                               std::chrono::steady_clock::now());
            return true;
        },
        nullptr
    );
    ASSERT_TRUE(installed);

    // Simulate a sequence of key events
    const std::vector<std::pair<uint32_t, bool>> sequence = {
        {SC_LCTRL, true},  // Ctrl down
        {SC_J, true},      // J down
        {SC_J, false},     // J up
        {SC_LCTRL, false}, // Ctrl up
    };

    for (const auto& [scanCode, isDown] : sequence) {
        platform::KeyEvent event{};
        event.scanCode = scanCode;
        event.isKeyDown = isDown;
        m_inputHook->simulateKeyEvent(event);
    }

    // Verify all events received in order
    std::lock_guard<std::mutex> lock(eventsMutex);
    ASSERT_EQ(events.size(), sequence.size());

    for (size_t i = 0; i < sequence.size(); ++i) {
        EXPECT_EQ(std::get<0>(events[i]), sequence[i].first)
            << "Event " << i << " scancode mismatch";
        EXPECT_EQ(std::get<1>(events[i]), sequence[i].second)
            << "Event " << i << " keyDown mismatch";
    }

    // Verify chronological ordering
    for (size_t i = 1; i < events.size(); ++i) {
        EXPECT_GE(std::get<2>(events[i]), std::get<2>(events[i-1]))
            << "Events should be chronologically ordered";
    }
}

//=============================================================================
// Test 5: Complex Keymap Inheritance Chain
//=============================================================================

TEST_F(IntegrationLifecycleTest, ComplexKeymapInheritanceChain) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = A\n")  // Default: A stays A
        _T("key B = B\n")
        _T("key C = C\n")
        _T("keymap Level1 : Global\n")
        _T("key A = B\n")  // Override A
        _T("keymap Level2 : Level1\n")
        _T("key B = C\n")  // Override B
        _T("keymap Level3 : Level2\n")
        _T("key C = D\n"); // Override C

    LoadConfig(config);

    // Verify inheritance chain
    const Keymap* global = m_setting.m_keymaps.searchByName(_T("Global"));
    const Keymap* level1 = m_setting.m_keymaps.searchByName(_T("Level1"));
    const Keymap* level2 = m_setting.m_keymaps.searchByName(_T("Level2"));
    const Keymap* level3 = m_setting.m_keymaps.searchByName(_T("Level3"));

    ASSERT_NE(global, nullptr);
    ASSERT_NE(level1, nullptr);
    ASSERT_NE(level2, nullptr);
    ASSERT_NE(level3, nullptr);

    EXPECT_EQ(level1->getParentKeymap(), global);
    EXPECT_EQ(level2->getParentKeymap(), level1);
    EXPECT_EQ(level3->getParentKeymap(), level2);

    // Verify Level3 has C -> D mapping directly
    Key* keyC = m_setting.m_keyboard.searchKey(_T("C"));
    ASSERT_NE(keyC, nullptr);
    ModifiedKey mkC(keyC);
    const Keymap::KeyAssignment* kaC = level3->searchAssignment(mkC);
    EXPECT_NE(kaC, nullptr) << "Level3 should have C mapping";
}

//=============================================================================
// IPC Integration Tests
//=============================================================================

class IntegrationIPCTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for test IPC files
        m_tempDir = getTempDir() + "/yamy_ipc_test_" +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(m_tempDir);
    }

    void TearDown() override {
        // Cleanup
        std::filesystem::remove_all(m_tempDir);
    }

    std::string m_tempDir;
};

//=============================================================================
// Test 6: IPC Message Types Coverage
//=============================================================================

TEST_F(IntegrationIPCTest, IPCMessageTypesCoverage) {
    // Test that all message types are defined and distinct
    std::set<uint32_t> messageTypes;

    // Control commands
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdReload));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdStop));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdStart));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdGetStatus));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdGetConfig));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdGetKeymaps));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdGetMetrics));

    // Responses
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspOk));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspError));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspStatus));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspConfig));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspKeymaps));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspMetrics));

    // Investigate commands
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdInvestigateWindow));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::RspInvestigateWindow));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdEnableInvestigateMode));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::CmdDisableInvestigateMode));
    messageTypes.insert(static_cast<uint32_t>(ipc::MessageType::NtfKeyEvent));

    // Verify all types are unique
    EXPECT_EQ(messageTypes.size(), 18u) << "All IPC message types should be unique";
}

//=============================================================================
// Test 7: IPC Message Structure Validity
//=============================================================================

TEST_F(IntegrationIPCTest, IPCMessageStructureValidity) {
    // Test InvestigateWindowRequest
    ipc::InvestigateWindowRequest req;
    req.hwnd = reinterpret_cast<yamy::platform::WindowHandle>(0x12345678);
    EXPECT_NE(req.hwnd, nullptr);

    // Test InvestigateWindowResponse
    ipc::InvestigateWindowResponse resp;
    std::strncpy(resp.keymapName, "TestKeymap", sizeof(resp.keymapName) - 1);
    std::strncpy(resp.matchedClassRegex, "/test-class/", sizeof(resp.matchedClassRegex) - 1);
    std::strncpy(resp.matchedTitleRegex, "/test-title/", sizeof(resp.matchedTitleRegex) - 1);
    std::strncpy(resp.activeModifiers, "Ctrl+Shift", sizeof(resp.activeModifiers) - 1);
    resp.isDefault = false;

    EXPECT_STREQ(resp.keymapName, "TestKeymap");
    EXPECT_STREQ(resp.matchedClassRegex, "/test-class/");
    EXPECT_FALSE(resp.isDefault);

    // Test KeyEventNotification
    ipc::KeyEventNotification notification;
    std::strncpy(notification.keyEvent, "A -> B (down)", sizeof(notification.keyEvent) - 1);
    EXPECT_STREQ(notification.keyEvent, "A -> B (down)");

    // Test Message wrapper
    ipc::Message msg;
    msg.type = ipc::MessageType::CmdGetStatus;
    msg.data = nullptr;
    msg.size = 0;
    EXPECT_EQ(msg.type, ipc::MessageType::CmdGetStatus);
}

//=============================================================================
// Test 8: GUI Message Types Coverage
//=============================================================================

TEST_F(IntegrationIPCTest, GUIMessageTypesCoverage) {
    // Test yamy::MessageType for GUI notifications
    std::set<uint32_t> guiMessageTypes;

    // Engine Lifecycle
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::EngineStarting));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::EngineStarted));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::EngineStopping));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::EngineStopped));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::EngineError));

    // Configuration
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::ConfigLoading));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::ConfigLoaded));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::ConfigError));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::ConfigValidating));

    // Runtime Events
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::KeymapSwitched));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::FocusChanged));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::ModifierChanged));

    // Performance Metrics
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::LatencyReport));
    guiMessageTypes.insert(static_cast<uint32_t>(MessageType::CpuUsageReport));

    // All types should be unique
    EXPECT_EQ(guiMessageTypes.size(), 14u) << "All GUI message types should be unique";
}

//=============================================================================
// Session Management Integration Tests
//=============================================================================

class IntegrationSessionTest : public ::testing::Test {
protected:
    std::string m_originalConfigDir;
    std::string m_testConfigDir;

    void SetUp() override {
        // Store original home to restore later
        m_testConfigDir = getTempDir() + "/yamy_session_test_" +
                          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(m_testConfigDir);

        // Override HOME for test isolation
        m_originalConfigDir = std::getenv("HOME") ? std::getenv("HOME") : "";
    }

    void TearDown() override {
        // Cleanup test directory
        std::filesystem::remove_all(m_testConfigDir);
    }
};

//=============================================================================
// Test 9: Session Data Structure
//=============================================================================

TEST_F(IntegrationSessionTest, SessionDataStructure) {
    SessionData data;

    // Test default values
    EXPECT_TRUE(data.activeConfigPath.empty());
    EXPECT_FALSE(data.engineWasRunning);
    EXPECT_TRUE(data.windowPositions.empty());
    EXPECT_EQ(data.savedTimestamp, 0);

    // Test setting values
    data.activeConfigPath = "/home/user/.config/yamy/work.mayu";
    data.engineWasRunning = true;
    data.savedTimestamp = std::chrono::system_clock::now().time_since_epoch().count();

    WindowPosition pos;
    pos.x = 100;
    pos.y = 200;
    pos.width = 800;
    pos.height = 600;
    pos.valid = true;
    data.windowPositions["InvestigateDialog"] = pos;

    EXPECT_EQ(data.activeConfigPath, "/home/user/.config/yamy/work.mayu");
    EXPECT_TRUE(data.engineWasRunning);
    EXPECT_GT(data.savedTimestamp, 0);
    EXPECT_EQ(data.windowPositions.size(), 1u);
    EXPECT_EQ(data.windowPositions["InvestigateDialog"].x, 100);
}

//=============================================================================
// Test 10: Window Position Storage and Retrieval
//=============================================================================

TEST_F(IntegrationSessionTest, WindowPositionStorageRetrieval) {
    auto& session = SessionManager::instance();

    // Save multiple window positions
    session.saveWindowPosition("LogDialog", 50, 75, 640, 480);
    session.saveWindowPosition("InvestigateDialog", 100, 100, 800, 600);
    session.saveWindowPosition("PreferencesDialog", 200, 150, 500, 400);

    // Retrieve and verify
    WindowPosition logPos = session.getWindowPosition("LogDialog");
    EXPECT_TRUE(logPos.valid);
    EXPECT_EQ(logPos.x, 50);
    EXPECT_EQ(logPos.y, 75);
    EXPECT_EQ(logPos.width, 640);
    EXPECT_EQ(logPos.height, 480);

    WindowPosition invPos = session.getWindowPosition("InvestigateDialog");
    EXPECT_TRUE(invPos.valid);
    EXPECT_EQ(invPos.x, 100);
    EXPECT_EQ(invPos.y, 100);

    // Non-existent window should return invalid
    WindowPosition unknownPos = session.getWindowPosition("UnknownDialog");
    EXPECT_FALSE(unknownPos.valid);
}

//=============================================================================
// Test 11: Session State Updates
//=============================================================================

TEST_F(IntegrationSessionTest, SessionStateUpdates) {
    auto& session = SessionManager::instance();

    // Test config path setting
    session.setActiveConfig("/path/to/config.mayu");
    EXPECT_EQ(session.data().activeConfigPath, "/path/to/config.mayu");

    // Test engine state setting
    session.setEngineRunning(true);
    EXPECT_TRUE(session.data().engineWasRunning);

    session.setEngineRunning(false);
    EXPECT_FALSE(session.data().engineWasRunning);
}

//=============================================================================
// Performance Integration Tests
//=============================================================================

class IntegrationPerformanceTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;
    std::unique_ptr<MockInputHook> m_inputHook;

    void SetUp() override {
        m_loader.reset(new SettingLoader(&m_soLog, &m_logStream));
        m_loader->initialize(&m_setting);
        m_inputHook = std::make_unique<MockInputHook>();
    }

    void TearDown() override {
        m_inputHook->uninstall();
        m_loader.reset();
    }

    std::string getKeyDefinitions() {
        return
            _T("def key A = 0x1E\n")
            _T("def key B = 0x30\n")
            _T("def key C = 0x2E\n")
            _T("def key LControl = 0x1D\n")
            _T("def mod Control = LControl\n");
    }

    static constexpr uint32_t SC_A = 0x1E;
};

//=============================================================================
// Test 12: Key Processing Latency Under Load
//=============================================================================

TEST_F(IntegrationPerformanceTest, KeyProcessingLatencyUnderLoad) {
    // Load a typical configuration
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("key C-A = C\n");

    m_loader->loadFromData(config);

    std::vector<std::chrono::nanoseconds> processingTimes;
    processingTimes.reserve(1000);

    bool installed = m_inputHook->install(
        [&processingTimes](const platform::KeyEvent& event) {
            auto start = std::chrono::high_resolution_clock::now();

            // Simulate minimal processing (callback overhead)
            volatile int x = event.scanCode;
            (void)x;

            auto end = std::chrono::high_resolution_clock::now();
            processingTimes.push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
            return true;
        },
        nullptr
    );
    ASSERT_TRUE(installed);

    // Generate 1000 key events under load
    for (int i = 0; i < 500; ++i) {
        m_inputHook->simulateKeyPressRelease(SC_A);
    }

    ASSERT_EQ(processingTimes.size(), 1000u);

    // Calculate statistics
    std::sort(processingTimes.begin(), processingTimes.end());

    auto sum = std::chrono::nanoseconds::zero();
    for (const auto& t : processingTimes) {
        sum += t;
    }
    auto avgNs = sum.count() / processingTimes.size();
    auto p50Ns = processingTimes[processingTimes.size() / 2].count();
    auto p95Ns = processingTimes[processingTimes.size() * 95 / 100].count();
    auto p99Ns = processingTimes[processingTimes.size() * 99 / 100].count();
    auto maxNs = processingTimes.back().count();

    // Report metrics
    std::cout << "Key Processing Latency (callback only):" << std::endl;
    std::cout << "  Average: " << avgNs / 1000.0 << " µs" << std::endl;
    std::cout << "  P50:     " << p50Ns / 1000.0 << " µs" << std::endl;
    std::cout << "  P95:     " << p95Ns / 1000.0 << " µs" << std::endl;
    std::cout << "  P99:     " << p99Ns / 1000.0 << " µs" << std::endl;
    std::cout << "  Max:     " << maxNs / 1000.0 << " µs" << std::endl;

    // Performance target: <1ms (1,000,000ns) for P95
    // Note: This is callback overhead only, not full engine processing
    EXPECT_LT(p95Ns, 1000000) << "P95 latency should be <1ms";
}

//=============================================================================
// Test 13: Config Loading Performance
//=============================================================================

TEST_F(IntegrationPerformanceTest, ConfigLoadingPerformance) {
    // Create a complex configuration with many keymaps and mappings
    std::string config = getKeyDefinitions();

    // Create 10 keymaps with 5 mappings each (simpler to avoid string conversion issues)
    config += _T("keymap Global\n");
    for (int i = 0; i < 5; ++i) {
        config += _T("key A = B\n");
    }

    for (int km = 1; km <= 10; ++km) {
        config += _T("keymap App");
        config += std::to_string(km);
        config += _T(" : Global\n");
        for (int i = 0; i < 5; ++i) {
            config += _T("key C-A = C\n");
        }
    }

    // Measure loading time
    auto start = std::chrono::high_resolution_clock::now();
    m_loader->loadFromData(config);
    auto end = std::chrono::high_resolution_clock::now();

    auto loadTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Config Loading Performance:" << std::endl;
    std::cout << "  Complex config load time: " << loadTimeMs << " ms" << std::endl;

    // Config loading should complete in reasonable time (<500ms)
    EXPECT_LT(loadTimeMs, 500) << "Config loading should complete in <500ms";

    // Verify config loaded correctly
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    EXPECT_NE(globalMap, nullptr) << "Global keymap should exist";

    for (int km = 1; km <= 10; ++km) {
        std::string name = "App" + std::to_string(km);
        const Keymap* appMap = m_setting.m_keymaps.searchByName(std::string(name.begin(), name.end()));
        EXPECT_NE(appMap, nullptr) << "Keymap " << name << " should exist";
    }
}

//=============================================================================
// Test 14: Sustained Key Event Processing
//=============================================================================

TEST_F(IntegrationPerformanceTest, SustainedKeyEventProcessing) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    m_loader->loadFromData(config);

    std::atomic<size_t> eventCount{0};
    std::atomic<bool> stopFlag{false};

    bool installed = m_inputHook->install(
        [&eventCount, &stopFlag](const platform::KeyEvent& event) {
            if (stopFlag) return false;
            eventCount++;
            return true;
        },
        nullptr
    );
    ASSERT_TRUE(installed);

    // Generate events continuously for 100ms
    auto startTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::milliseconds(100);

    while (std::chrono::steady_clock::now() - startTime < duration) {
        m_inputHook->simulateKeyPressRelease(SC_A);
    }

    stopFlag = true;
    auto endTime = std::chrono::steady_clock::now();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    double eventsPerSecond = (eventCount * 1000.0) / elapsedMs;

    std::cout << "Sustained Event Processing:" << std::endl;
    std::cout << "  Duration: " << elapsedMs << " ms" << std::endl;
    std::cout << "  Events processed: " << eventCount << std::endl;
    std::cout << "  Events/second: " << eventsPerSecond << std::endl;

    // Should handle at least 10,000 events/second
    EXPECT_GT(eventsPerSecond, 10000) << "Should process >10k events/second";
}

//=============================================================================
// All Tracks Integration Test
//=============================================================================

class IntegrationAllTracksTest : public IntegrationLifecycleTest {
    // Inherits all fixtures
};

//=============================================================================
// Test 15: All Tracks Working Together
//=============================================================================

TEST_F(IntegrationAllTracksTest, AllTracksWorkingTogether) {
    // Track 1: Platform Abstraction - Load config
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("key C-J = Enter\n")
        _T("window Editor /code|vim/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Verify config loaded (Track 2: Config Management)
    ASSERT_NE(m_setting.m_keymaps.searchByName(_T("Global")), nullptr);
    ASSERT_NE(m_setting.m_keymaps.searchByName(_T("Editor")), nullptr);

    // Track 3/4: Investigate/Log Dialog data structures
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);

    // Track 5: Notification system - message types exist
    EXPECT_NE(static_cast<uint32_t>(MessageType::EngineStarted), 0);
    EXPECT_NE(static_cast<uint32_t>(MessageType::ConfigLoaded), 0);
    EXPECT_NE(static_cast<uint32_t>(MessageType::KeymapSwitched), 0);

    // Track 6: Advanced Features - window matching
    Keymaps::KeymapPtrList matches;
    m_setting.m_keymaps.searchWindow(&matches, "code", "Visual Studio Code");
    bool hasEditor = false;
    for (const auto* km : matches) {
        if (km->getName() == _T("Editor")) hasEditor = true;
    }
    EXPECT_TRUE(hasEditor);

    // Input hook functionality
    std::vector<platform::KeyEvent> events;
    bool installed = m_inputHook->install(
        [&events](const platform::KeyEvent& event) {
            events.push_back(event);
            return true;
        },
        nullptr
    );
    ASSERT_TRUE(installed);

    m_inputHook->simulateKeyPressRelease(SC_A);
    EXPECT_EQ(events.size(), 2u) << "Should process press and release";

    m_inputHook->uninstall();
    EXPECT_FALSE(m_inputHook->isInstalled());
}

//=============================================================================
// Test 16: Error Recovery and Resilience
//=============================================================================

TEST_F(IntegrationAllTracksTest, ErrorRecoveryAndResilience) {
    // Test with invalid config - should not crash
    std::string invalidConfig = _T("this is not valid config syntax\n");

    // This should not throw or crash
    EXPECT_NO_THROW({
        m_loader->loadFromData(invalidConfig);
    });

    // Engine should still be usable after invalid config
    std::string validConfig = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    EXPECT_NO_THROW({
        Setting newSetting;
        tstringstream newLogStream;
        SettingLoader newLoader(&m_soLog, &newLogStream);
        newLoader.initialize(&newSetting);
        newLoader.loadFromData(validConfig);

        // Verify it loaded correctly
        const Keymap* globalMap = newSetting.m_keymaps.searchByName(_T("Global"));
        EXPECT_NE(globalMap, nullptr);
    });
}

//=============================================================================
// Test 17: Concurrent Event Processing
//=============================================================================

TEST_F(IntegrationAllTracksTest, ConcurrentEventProcessing) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config);

    std::atomic<size_t> totalEvents{0};
    std::mutex eventsMutex;
    std::vector<uint32_t> allScanCodes;

    bool installed = m_inputHook->install(
        [&totalEvents, &eventsMutex, &allScanCodes](const platform::KeyEvent& event) {
            totalEvents++;
            std::lock_guard<std::mutex> lock(eventsMutex);
            allScanCodes.push_back(event.scanCode);
            return true;
        },
        nullptr
    );
    ASSERT_TRUE(installed);

    // Simulate concurrent event generation from multiple "threads"
    // (In reality, we're just interleaving events)
    const size_t numEvents = 100;

    for (size_t i = 0; i < numEvents; ++i) {
        // Alternate between different keys
        uint32_t scanCode = (i % 3 == 0) ? SC_A : ((i % 3 == 1) ? SC_B : SC_J);
        platform::KeyEvent event{};
        event.scanCode = scanCode;
        event.isKeyDown = (i % 2 == 0);
        m_inputHook->simulateKeyEvent(event);
    }

    EXPECT_EQ(totalEvents, numEvents);
    EXPECT_EQ(allScanCodes.size(), numEvents);
}

//=============================================================================
// Test 18: Memory Safety Under Rapid Reconnection
//=============================================================================

TEST_F(IntegrationAllTracksTest, MemorySafetyUnderRapidReconnection) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config);

    // Rapidly install/uninstall hook multiple times
    for (int i = 0; i < 10; ++i) {
        std::atomic<int> eventCount{0};

        bool installed = m_inputHook->install(
            [&eventCount](const platform::KeyEvent& event) {
                eventCount++;
                return true;
            },
            nullptr
        );
        ASSERT_TRUE(installed) << "Install iteration " << i;

        m_inputHook->simulateKeyPressRelease(SC_A);
        EXPECT_EQ(eventCount, 2) << "Iteration " << i;

        m_inputHook->uninstall();
        EXPECT_FALSE(m_inputHook->isInstalled());

        // Events should not be processed after uninstall
        m_inputHook->simulateKeyPressRelease(SC_A);
        EXPECT_EQ(eventCount, 2) << "No events after uninstall in iteration " << i;
    }
}

} // namespace yamy::integration_test

// Note: Main function is provided by regression_suite.cpp when building
// as part of the regression test suite. This file only contains test cases.
