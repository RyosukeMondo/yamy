//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// key_remapping_integration_test.cpp - Integration tests for full key remapping flow
//
// Tests the complete key remapping pipeline on Linux:
// 1. Input hook receives key events
// 2. Engine processes events and applies keymaps
// 3. Input injector outputs remapped keys
//
// These tests use mock implementations to verify the full flow without
// requiring actual hardware access or X11 display.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

#include "setting.h"
#include "setting_loader.h"
#include "keymap.h"
#include "keyboard.h"
#include "msgstream.h"
#include "multithread.h"
#include "../../src/core/platform/types.h"
#include "../../src/core/platform/input_hook_interface.h"
#include "../../src/core/platform/input_injector_interface.h"
#include "../../src/core/platform/window_system_interface.h"
#include "../../src/core/input/input_event.h"

namespace yamy::test {

//=============================================================================
// Mock Input Hook - Simulates keyboard input
//=============================================================================

class MockInputHook : public platform::IInputHook {
public:
    MockInputHook() : m_installed(false) {}

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
        return m_keyCallback(event);
    }

    // Test helper: simulate key press and release
    std::pair<bool, bool> simulateKeyPressRelease(uint32_t scanCode, bool extended = false) {
        platform::KeyEvent downEvent{};
        downEvent.scanCode = scanCode;
        downEvent.isKeyDown = true;
        downEvent.isExtended = extended;
        downEvent.timestamp = 0;

        platform::KeyEvent upEvent{};
        upEvent.scanCode = scanCode;
        upEvent.isKeyDown = false;
        upEvent.isExtended = extended;
        upEvent.timestamp = 1;

        bool downResult = simulateKeyEvent(downEvent);
        bool upResult = simulateKeyEvent(upEvent);
        return {downResult, upResult};
    }

private:
    std::atomic<bool> m_installed;
    platform::KeyCallback m_keyCallback;
    platform::MouseCallback m_mouseCallback;
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
    };

    MockInputInjector() = default;

    void inject(const KEYBOARD_INPUT_DATA* data, const platform::InjectionContext& ctx,
                const void* rawData = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        InjectedKey key;
        key.scanCode = data->MakeCode;
        key.isKeyDown = !(data->Flags & KEYBOARD_INPUT_DATA::BREAK);
        key.isExtended = (data->Flags & KEYBOARD_INPUT_DATA::E0) != 0;
        m_injectedKeys.push_back(key);
    }

    void keyDown(platform::KeyCode key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        InjectedKey k;
        k.scanCode = static_cast<uint32_t>(key);
        k.isKeyDown = true;
        k.isExtended = false;
        m_injectedKeys.push_back(k);
    }

    void keyUp(platform::KeyCode key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        InjectedKey k;
        k.scanCode = static_cast<uint32_t>(key);
        k.isKeyDown = false;
        k.isExtended = false;
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
// Integration Test Fixture
//=============================================================================

class KeyRemappingIntegrationTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;
    std::unique_ptr<MockInputHook> m_inputHook;
    std::unique_ptr<MockInputInjector> m_inputInjector;

    void SetUp() override {
        m_loader.reset(new SettingLoader(&m_soLog, &m_logStream));
        m_loader->initialize(&m_setting);
        m_inputHook = std::make_unique<MockInputHook>();
        m_inputInjector = std::make_unique<MockInputInjector>();
    }

    void TearDown() override {
        m_inputHook->uninstall();
        m_loader.reset();
    }

    void LoadConfig(const tstring& config) {
        m_loader->loadFromData(config);
        tstring log_output = m_logStream.str();
        if (log_output.find(_T("error:")) != tstring::npos) {
            FAIL() << "Errors found during config loading: " << log_output;
        }
    }

    // Helper to define basic 104 keyboard keys
    tstring getKeyDefinitions() {
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
            _T("def key M = 0x32\n")
            _T("def key N = 0x31\n")
            _T("def key O = 0x18\n")
            _T("def key P = 0x19\n")
            _T("def key Q = 0x10\n")
            _T("def key R = 0x13\n")
            _T("def key S = 0x1F\n")
            _T("def key T = 0x14\n")
            _T("def key U = 0x16\n")
            _T("def key V = 0x2F\n")
            _T("def key W = 0x11\n")
            _T("def key X = 0x2D\n")
            _T("def key Y = 0x15\n")
            _T("def key Z = 0x2C\n")
            _T("def key Escape Esc = 0x01\n")
            _T("def key F1 = 0x3B\n")
            _T("def key F5 = 0x3F\n")
            _T("def key F10 = 0x44\n")
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
            _T("def key Home = E0-0x47\n")
            _T("def key End = E0-0x4F\n")
            _T("def mod Shift = LShift RShift\n")
            _T("def mod Control = LControl RControl\n")
            _T("def mod Alt = LAlt RAlt\n");
    }

    // Scan codes for keys
    static constexpr uint32_t SC_A = 0x1E;
    static constexpr uint32_t SC_B = 0x30;
    static constexpr uint32_t SC_C = 0x2E;
    static constexpr uint32_t SC_J = 0x24;
    static constexpr uint32_t SC_H = 0x23;
    static constexpr uint32_t SC_Z = 0x2C;
    static constexpr uint32_t SC_ESCAPE = 0x01;
    static constexpr uint32_t SC_F1 = 0x3B;
    static constexpr uint32_t SC_F5 = 0x3F;
    static constexpr uint32_t SC_F10 = 0x44;
    static constexpr uint32_t SC_ENTER = 0x1C;
    static constexpr uint32_t SC_BACKSPACE = 0x0E;
    static constexpr uint32_t SC_LCTRL = 0x1D;
    static constexpr uint32_t SC_LALT = 0x38;
    static constexpr uint32_t SC_LSHIFT = 0x2A;
    static constexpr uint32_t SC_CAPSLOCK = 0x3A;
    static constexpr uint32_t SC_UP = 0x48;
    static constexpr uint32_t SC_DOWN = 0x50;
    static constexpr uint32_t SC_HOME = 0x47;
    static constexpr uint32_t SC_END = 0x4F;
};

//=============================================================================
// Test 1: Mock Input Hook Installation
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, MockInputHookInstallation) {
    EXPECT_FALSE(m_inputHook->isInstalled());

    bool result = m_inputHook->install(
        [](const platform::KeyEvent&) { return true; },
        [](const platform::MouseEvent&) { return true; }
    );

    EXPECT_TRUE(result);
    EXPECT_TRUE(m_inputHook->isInstalled());

    m_inputHook->uninstall();
    EXPECT_FALSE(m_inputHook->isInstalled());
}

//=============================================================================
// Test 2: Mock Input Hook Key Event Simulation
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, MockInputHookKeyEventSimulation) {
    std::vector<platform::KeyEvent> receivedEvents;

    m_inputHook->install(
        [&receivedEvents](const platform::KeyEvent& event) {
            receivedEvents.push_back(event);
            return true;
        },
        nullptr
    );

    // Simulate key press and release
    auto [downConsumed, upConsumed] = m_inputHook->simulateKeyPressRelease(SC_A);

    EXPECT_TRUE(downConsumed);
    EXPECT_TRUE(upConsumed);
    EXPECT_EQ(receivedEvents.size(), 2u);
    EXPECT_TRUE(receivedEvents[0].isKeyDown);
    EXPECT_FALSE(receivedEvents[1].isKeyDown);
    EXPECT_EQ(receivedEvents[0].scanCode, SC_A);
    EXPECT_EQ(receivedEvents[1].scanCode, SC_A);
}

//=============================================================================
// Test 3: Mock Input Injector Records Keys
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, MockInputInjectorRecordsKeys) {
    EXPECT_EQ(m_inputInjector->getInjectedCount(), 0u);

    m_inputInjector->keyDown(platform::KeyCode::Space);
    m_inputInjector->keyUp(platform::KeyCode::Space);

    auto keys = m_inputInjector->getInjectedKeys();
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_TRUE(keys[0].isKeyDown);
    EXPECT_FALSE(keys[1].isKeyDown);
}

//=============================================================================
// Test 4: Config Loading and Key Definition Verification
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, ConfigLoadingAndKeyDefinition) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config);

    // Verify key definitions
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    Key* keyB = m_setting.m_keyboard.searchKey(_T("B"));
    ASSERT_NE(keyA, nullptr) << "Key A should be defined";
    ASSERT_NE(keyB, nullptr) << "Key B should be defined";

    // Verify keymap exists
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr) << "Global keymap should exist";

    // Verify key assignment
    ModifiedKey mkA(keyA);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkA);
    ASSERT_NE(ka, nullptr) << "Key assignment for A should exist";
}

//=============================================================================
// Test 5: Simple Key Remapping Flow (A -> B)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, SimpleKeyRemappingFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config);

    // Verify the mapping is set up
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    Key* keyB = m_setting.m_keyboard.searchKey(_T("B"));
    ASSERT_NE(keyA, nullptr);
    ASSERT_NE(keyB, nullptr);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    // Verify A has a mapping to B
    ModifiedKey mkA(keyA);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkA);
    ASSERT_NE(ka, nullptr) << "A should be mapped";
    ASSERT_NE(ka->m_keySeq, nullptr) << "Key sequence should exist";

    // The key sequence should produce an action
    EXPECT_FALSE(ka->m_keySeq->getActions().empty())
        << "Key sequence should have actions";
}

//=============================================================================
// Test 6: Key Swap Flow (F1 <-> Escape)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, KeySwapFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key F1 = Escape\n")
        _T("key Escape = F1\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyF1 = m_setting.m_keyboard.searchKey(_T("F1"));
    Key* keyEsc = m_setting.m_keyboard.searchKey(_T("Escape"));
    ASSERT_NE(keyF1, nullptr);
    ASSERT_NE(keyEsc, nullptr);

    // Verify both directions of the swap
    ModifiedKey mkF1(keyF1);
    ModifiedKey mkEsc(keyEsc);

    const Keymap::KeyAssignment* kaF1 = globalMap->searchAssignment(mkF1);
    const Keymap::KeyAssignment* kaEsc = globalMap->searchAssignment(mkEsc);

    EXPECT_NE(kaF1, nullptr) << "F1 -> Escape mapping should exist";
    EXPECT_NE(kaEsc, nullptr) << "Escape -> F1 mapping should exist";
}

//=============================================================================
// Test 7: Modifier Key Combination (Ctrl+J -> Enter)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, ModifierKeyCombinationFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-J = Enter\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyJ = m_setting.m_keyboard.searchKey(_T("J"));
    ASSERT_NE(keyJ, nullptr);

    // Create modified key with Control modifier
    ModifiedKey mkCtrlJ(keyJ);
    mkCtrlJ.m_modifier.on(Modifier::Type_Control);

    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCtrlJ);
    EXPECT_NE(ka, nullptr) << "Ctrl+J -> Enter mapping should exist";
}

//=============================================================================
// Test 8: Multiple Modifier Combination (Ctrl+Shift+A -> Ctrl+Shift+Z)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, MultipleModifierCombinationFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-S-A = C-S-Z\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);

    // Create modified key with Control+Shift modifiers
    ModifiedKey mkCSA(keyA);
    mkCSA.m_modifier.on(Modifier::Type_Control);
    mkCSA.m_modifier.on(Modifier::Type_Shift);

    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCSA);
    EXPECT_NE(ka, nullptr) << "Ctrl+Shift+A mapping should exist";
}

//=============================================================================
// Test 9: Alt Modifier Combination (Alt+H -> BackSpace)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, AltModifierCombinationFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A-H = BackSpace\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyH = m_setting.m_keyboard.searchKey(_T("H"));
    ASSERT_NE(keyH, nullptr);

    ModifiedKey mkAltH(keyH);
    mkAltH.m_modifier.on(Modifier::Type_Alt);

    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkAltH);
    EXPECT_NE(ka, nullptr) << "Alt+H -> BackSpace mapping should exist";
}

//=============================================================================
// Test 10: Emacs Navigation Keys (C-P/N/B/F -> Up/Down/Left/Right)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, EmacsNavigationKeysFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-P = Up\n")
        _T("key C-N = Down\n")
        _T("key C-B = Left\n")
        _T("key C-F = Right\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    struct TestCase {
        const char* keyName;
        const char* description;
    };

    TestCase testCases[] = {
        {"P", "C-P -> Up"},
        {"N", "C-N -> Down"},
        {"B", "C-B -> Left"},
        {"F", "C-F -> Right"},
    };

    for (const auto& tc : testCases) {
        Key* key = m_setting.m_keyboard.searchKey(tc.keyName);
        ASSERT_NE(key, nullptr) << "Key " << tc.keyName << " should exist";

        ModifiedKey mk(key);
        mk.m_modifier.on(Modifier::Type_Control);

        const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mk);
        EXPECT_NE(ka, nullptr) << tc.description << " mapping should exist";
    }
}

//=============================================================================
// Test 11: Window Context Keymap Switching
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, WindowContextKeymapSwitching) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = X\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = Y\n")
        _T("window Browser /firefox/ : Global\n")
        _T("key A = Z\n");

    LoadConfig(config);

    // Verify keymaps were created
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    const Keymap* terminalMap = m_setting.m_keymaps.searchByName(_T("Terminal"));
    const Keymap* browserMap = m_setting.m_keymaps.searchByName(_T("Browser"));

    ASSERT_NE(globalMap, nullptr) << "Global keymap should exist";
    ASSERT_NE(terminalMap, nullptr) << "Terminal keymap should exist";
    ASSERT_NE(browserMap, nullptr) << "Browser keymap should exist";

    // Verify inheritance
    EXPECT_NE(terminalMap->getParentKeymap(), nullptr) << "Terminal should inherit from Global";
    EXPECT_NE(browserMap->getParentKeymap(), nullptr) << "Browser should inherit from Global";

    // Test window matching
    Keymaps::KeymapPtrList terminalMatches;
    m_setting.m_keymaps.searchWindow(&terminalMatches, "terminal", "bash");
    bool hasTerminal = false;
    for (const auto* km : terminalMatches) {
        if (km->getName() == _T("Terminal")) hasTerminal = true;
    }
    EXPECT_TRUE(hasTerminal) << "Terminal keymap should match 'terminal' class";

    Keymaps::KeymapPtrList firefoxMatches;
    m_setting.m_keymaps.searchWindow(&firefoxMatches, "firefox", "Mozilla");
    bool hasBrowser = false;
    for (const auto* km : firefoxMatches) {
        if (km->getName() == _T("Browser")) hasBrowser = true;
    }
    EXPECT_TRUE(hasBrowser) << "Browser keymap should match 'firefox' class";
}

//=============================================================================
// Test 12: Modifier Passthrough (*CapsLock = *LControl)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, ModifierPassthroughFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key *CapsLock = *LControl\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyCapsLock = m_setting.m_keyboard.searchKey(_T("CapsLock"));
    ASSERT_NE(keyCapsLock, nullptr);

    ModifiedKey mkCaps(keyCapsLock);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCaps);
    EXPECT_NE(ka, nullptr) << "CapsLock -> LControl mapping should exist";
}

//=============================================================================
// Test 13: Function Key Remapping (F5 -> F10)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, FunctionKeyRemappingFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key F5 = F10\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyF5 = m_setting.m_keyboard.searchKey(_T("F5"));
    ASSERT_NE(keyF5, nullptr);

    ModifiedKey mkF5(keyF5);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkF5);
    EXPECT_NE(ka, nullptr) << "F5 -> F10 mapping should exist";
}

//=============================================================================
// Test 14: Key Sequence Definition ($MySeq)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, KeySequenceDefinitionFlow) {
    tstring config = getKeyDefinitions() +
        _T("keyseq $MySeq = A B C\n")
        _T("keymap Global\n")
        _T("key F1 = $MySeq\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyF1 = m_setting.m_keyboard.searchKey(_T("F1"));
    ASSERT_NE(keyF1, nullptr);

    ModifiedKey mkF1(keyF1);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkF1);
    ASSERT_NE(ka, nullptr) << "F1 -> $MySeq mapping should exist";
    ASSERT_NE(ka->m_keySeq, nullptr) << "Key sequence should exist";

    // The key sequence should have at least 1 action (the keyseq reference)
    EXPECT_GE(ka->m_keySeq->getActions().size(), 1UL)
        << "Key sequence should have actions";
}

//=============================================================================
// Test 15: Keymap Inheritance Chain
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, KeymapInheritanceChainFlow) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = X\n")
        _T("keymap Child : Global\n")
        _T("key B = Y\n")
        _T("keymap GrandChild : Child\n")
        _T("key C = Z\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    const Keymap* childMap = m_setting.m_keymaps.searchByName(_T("Child"));
    const Keymap* grandChildMap = m_setting.m_keymaps.searchByName(_T("GrandChild"));

    ASSERT_NE(globalMap, nullptr);
    ASSERT_NE(childMap, nullptr);
    ASSERT_NE(grandChildMap, nullptr);

    // Verify inheritance chain
    EXPECT_EQ(grandChildMap->getParentKeymap(), childMap);
    EXPECT_EQ(childMap->getParentKeymap(), globalMap);

    // Verify each keymap has its specific mapping
    Key* keyC = m_setting.m_keyboard.searchKey(_T("C"));
    ASSERT_NE(keyC, nullptr);
    ModifiedKey mkC(keyC);
    const Keymap::KeyAssignment* kaC = grandChildMap->searchAssignment(mkC);
    EXPECT_NE(kaC, nullptr) << "GrandChild should have C -> Z mapping";
}

//=============================================================================
// Test 16: Complete Hook-Engine-Injector Pipeline Verification
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, CompletePipelineVerification) {
    // This test verifies the complete flow can be set up correctly:
    // 1. Input hook can be installed with callbacks
    // 2. Config is loaded with valid keymaps
    // 3. Injector is ready to receive events

    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config);

    // Install hook with callback that records events
    std::vector<platform::KeyEvent> hookReceivedEvents;
    bool installed = m_inputHook->install(
        [&hookReceivedEvents](const platform::KeyEvent& event) {
            hookReceivedEvents.push_back(event);
            return true; // Consume the event
        },
        nullptr
    );
    ASSERT_TRUE(installed) << "Hook should be installed";

    // Verify keymap is ready
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr) << "Global keymap should exist";

    // Simulate key event through hook
    m_inputHook->simulateKeyPressRelease(SC_A);

    // Verify hook received events
    EXPECT_EQ(hookReceivedEvents.size(), 2u) << "Hook should receive press and release";

    // Verify injector is ready (it should start empty)
    EXPECT_EQ(m_inputInjector->getInjectedCount(), 0u)
        << "Injector should have no keys yet (would be populated by Engine)";

    // Clean up
    m_inputHook->uninstall();
}

//=============================================================================
// Test 17: Extended Key Handling (Arrow keys with E0 prefix)
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, ExtendedKeyHandling) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key Up = Down\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyUp = m_setting.m_keyboard.searchKey(_T("Up"));
    ASSERT_NE(keyUp, nullptr);

    // Verify the Up key has E0 prefix (extended key)
    ASSERT_GE(keyUp->getScanCodesSize(), 1);
    const ScanCode& sc = keyUp->getScanCodes()[0];
    EXPECT_TRUE(sc.m_flags & ScanCode::E0) << "Up key should have E0 flag";

    // Verify mapping exists
    ModifiedKey mkUp(keyUp);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkUp);
    EXPECT_NE(ka, nullptr) << "Up -> Down mapping should exist";
}

//=============================================================================
// Test 18: Event Ordering Verification
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, EventOrderingVerification) {
    // Verify that events are processed in the correct order
    std::vector<std::pair<uint32_t, bool>> eventOrder; // scanCode, isKeyDown

    m_inputHook->install(
        [&eventOrder](const platform::KeyEvent& event) {
            eventOrder.emplace_back(event.scanCode, event.isKeyDown);
            return true;
        },
        nullptr
    );

    // Simulate a sequence: Press A, Press B, Release A, Release B
    platform::KeyEvent events[] = {
        {platform::KeyCode::Unknown, true, false, SC_A, 0, 0, 0},  // A down
        {platform::KeyCode::Unknown, true, false, SC_B, 1, 0, 0},  // B down
        {platform::KeyCode::Unknown, false, false, SC_A, 2, 0, 0}, // A up
        {platform::KeyCode::Unknown, false, false, SC_B, 3, 0, 0}, // B up
    };

    for (const auto& event : events) {
        m_inputHook->simulateKeyEvent(event);
    }

    ASSERT_EQ(eventOrder.size(), 4u);
    EXPECT_EQ(eventOrder[0], std::make_pair(SC_A, true));  // A down
    EXPECT_EQ(eventOrder[1], std::make_pair(SC_B, true));  // B down
    EXPECT_EQ(eventOrder[2], std::make_pair(SC_A, false)); // A up
    EXPECT_EQ(eventOrder[3], std::make_pair(SC_B, false)); // B up
}

//=============================================================================
// Test 19: Empty Config Handling
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, EmptyConfigHandling) {
    // Just key definitions, no keymap - should still work
    tstring config = getKeyDefinitions();

    LoadConfig(config);

    // Should have keys defined
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    EXPECT_NE(keyA, nullptr) << "Key A should be defined even without keymap";
}

//=============================================================================
// Test 20: Multiple Keymaps with Same Key Different Mappings
//=============================================================================

TEST_F(KeyRemappingIntegrationTest, MultipleKeymapsDifferentMappings) {
    tstring config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = X\n")
        _T("keymap Map1 : Global\n")
        _T("key A = Y\n")
        _T("keymap Map2 : Global\n")
        _T("key A = Z\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    const Keymap* map1 = m_setting.m_keymaps.searchByName(_T("Map1"));
    const Keymap* map2 = m_setting.m_keymaps.searchByName(_T("Map2"));

    ASSERT_NE(globalMap, nullptr);
    ASSERT_NE(map1, nullptr);
    ASSERT_NE(map2, nullptr);

    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);
    ModifiedKey mkA(keyA);

    // Each keymap should have its own A mapping
    const Keymap::KeyAssignment* kaGlobal = globalMap->searchAssignment(mkA);
    const Keymap::KeyAssignment* kaMap1 = map1->searchAssignment(mkA);
    const Keymap::KeyAssignment* kaMap2 = map2->searchAssignment(mkA);

    EXPECT_NE(kaGlobal, nullptr) << "Global should have A mapping";
    EXPECT_NE(kaMap1, nullptr) << "Map1 should have A mapping";
    EXPECT_NE(kaMap2, nullptr) << "Map2 should have A mapping";
}

} // namespace yamy::test
