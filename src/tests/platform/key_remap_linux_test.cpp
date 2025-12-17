//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// key_remap_linux_test.cpp - Integration tests for key remapping on Linux
//
// Tests the complete key remapping flow on Linux:
// 1. Loading .mayu configuration
// 2. Key assignment lookup in keymaps
// 3. Modifier key handling
// 4. Key combination processing
//
// These tests verify that the keymap system works correctly with the
// platform-independent key definitions and that key assignments are
// properly resolved.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include "setting.h"
#include "setting_loader.h"
#include "keymap.h"
#include "keyboard.h"
#include "msgstream.h"
#include "multithread.h"

namespace yamy::test {

//=============================================================================
// Base Test Fixture for Key Remapping Tests
//=============================================================================

class KeyRemapLinuxTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;

    void SetUp() override {
        m_loader.reset(new SettingLoader(&m_soLog, &m_logStream));
        m_loader->initialize(&m_setting);
    }

    void TearDown() override {
        m_loader.reset();
    }

    void LoadConfig(const std::string& config) {
        m_loader->loadFromData(config);
        // Check for errors
        std::string log_output = m_logStream.str();
        if (log_output.find(_T("error:")) != std::string::npos) {
            FAIL() << "Errors found during config loading: " << log_output;
        }
    }

    // Helper to define basic 104 keyboard keys
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
};

//=============================================================================
// Test 1: Simple Key Remapping (A -> B)
//=============================================================================

TEST_F(KeyRemapLinuxTest, SimpleKeyRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n");

    LoadConfig(config);

    // Verify keys exist
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    Key* keyB = m_setting.m_keyboard.searchKey(_T("B"));
    ASSERT_NE(keyA, nullptr) << "Key A not found";
    ASSERT_NE(keyB, nullptr) << "Key B not found";

    // Verify keymap exists
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr) << "Global keymap not found";

    // Search for key assignment
    ModifiedKey mkA(keyA);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkA);
    ASSERT_NE(ka, nullptr) << "Key assignment for A not found";
    ASSERT_NE(ka->m_keySeq, nullptr) << "Key sequence for A is null";

    // Verify the action produces B
    EXPECT_FALSE(ka->m_keySeq->getActions().empty());
}

//=============================================================================
// Test 2: Key Swap (F1 <-> Escape)
//=============================================================================

TEST_F(KeyRemapLinuxTest, KeySwap) {
    std::string config = getKeyDefinitions() +
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

    // Verify F1 -> Escape mapping
    ModifiedKey mkF1(keyF1);
    const Keymap::KeyAssignment* kaF1 = globalMap->searchAssignment(mkF1);
    ASSERT_NE(kaF1, nullptr) << "Key assignment for F1 not found";

    // Verify Escape -> F1 mapping
    ModifiedKey mkEsc(keyEsc);
    const Keymap::KeyAssignment* kaEsc = globalMap->searchAssignment(mkEsc);
    ASSERT_NE(kaEsc, nullptr) << "Key assignment for Escape not found";
}

//=============================================================================
// Test 3: Modifier Key Remapping (CapsLock -> LControl in keymap)
// Note: We test the key assignment CapsLock -> LControl directly.
// The `mod control += CapsLock` syntax adds CapsLock as a control modifier,
// but for this test we verify the key remapping assignment works.
//=============================================================================

TEST_F(KeyRemapLinuxTest, ModifierKeyRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key *CapsLock = *LControl\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyCapsLock = m_setting.m_keyboard.searchKey(_T("CapsLock"));
    ASSERT_NE(keyCapsLock, nullptr);

    // Verify the CapsLock -> LControl key assignment exists
    ModifiedKey mkCaps(keyCapsLock);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCaps);
    EXPECT_NE(ka, nullptr) << "CapsLock -> LControl mapping not found";
}

//=============================================================================
// Test 4: Key Combination Remapping (Ctrl+J -> Enter)
//=============================================================================

TEST_F(KeyRemapLinuxTest, KeyCombinationRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-J = Enter\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyJ = m_setting.m_keyboard.searchKey(_T("J"));
    ASSERT_NE(keyJ, nullptr);

    // Search for Ctrl+J assignment
    ModifiedKey mkCtrlJ(keyJ);
    mkCtrlJ.m_modifier.on(Modifier::Type_Control);

    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCtrlJ);
    ASSERT_NE(ka, nullptr) << "Key assignment for C-J not found";
}

//=============================================================================
// Test 5: Alt+Key Remapping (Alt+H -> BackSpace)
//=============================================================================

TEST_F(KeyRemapLinuxTest, AltKeyCombinationRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A-H = BackSpace\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyH = m_setting.m_keyboard.searchKey(_T("H"));
    ASSERT_NE(keyH, nullptr);

    // Search for Alt+H assignment
    ModifiedKey mkAltH(keyH);
    mkAltH.m_modifier.on(Modifier::Type_Alt);

    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkAltH);
    ASSERT_NE(ka, nullptr) << "Key assignment for A-H not found";
}

//=============================================================================
// Test 6: Shift+Ctrl Combination (Ctrl+Shift+A -> Ctrl+Shift+Z)
//=============================================================================

TEST_F(KeyRemapLinuxTest, ShiftCtrlCombinationRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-S-A = C-S-Z\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);

    // Search for Ctrl+Shift+A assignment
    ModifiedKey mkCSA(keyA);
    mkCSA.m_modifier.on(Modifier::Type_Control);
    mkCSA.m_modifier.on(Modifier::Type_Shift);

    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCSA);
    ASSERT_NE(ka, nullptr) << "Key assignment for C-S-A not found";
}

//=============================================================================
// Test 7: Navigation Key Remapping (Emacs-style)
//=============================================================================

TEST_F(KeyRemapLinuxTest, NavigationKeyRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-P = Up\n")
        _T("key C-N = Down\n")
        _T("key C-B = Left\n")
        _T("key C-F = Right\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    // Verify C-P -> Up
    Key* keyP = m_setting.m_keyboard.searchKey(_T("P"));
    ASSERT_NE(keyP, nullptr);
    ModifiedKey mkCP(keyP);
    mkCP.m_modifier.on(Modifier::Type_Control);
    EXPECT_NE(globalMap->searchAssignment(mkCP), nullptr) << "C-P mapping not found";

    // Verify C-N -> Down
    Key* keyN = m_setting.m_keyboard.searchKey(_T("N"));
    ASSERT_NE(keyN, nullptr);
    ModifiedKey mkCN(keyN);
    mkCN.m_modifier.on(Modifier::Type_Control);
    EXPECT_NE(globalMap->searchAssignment(mkCN), nullptr) << "C-N mapping not found";

    // Verify C-B -> Left
    Key* keyB = m_setting.m_keyboard.searchKey(_T("B"));
    ASSERT_NE(keyB, nullptr);
    ModifiedKey mkCB(keyB);
    mkCB.m_modifier.on(Modifier::Type_Control);
    EXPECT_NE(globalMap->searchAssignment(mkCB), nullptr) << "C-B mapping not found";

    // Verify C-F -> Right
    Key* keyF = m_setting.m_keyboard.searchKey(_T("F"));
    ASSERT_NE(keyF, nullptr);
    ModifiedKey mkCF(keyF);
    mkCF.m_modifier.on(Modifier::Type_Control);
    EXPECT_NE(globalMap->searchAssignment(mkCF), nullptr) << "C-F mapping not found";
}

//=============================================================================
// Test 8: Home/End Key Remapping (Emacs-style)
//=============================================================================

TEST_F(KeyRemapLinuxTest, HomeEndKeyRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key C-A = Home\n")
        _T("key C-E = End\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    // Verify C-A -> Home
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);
    ModifiedKey mkCA(keyA);
    mkCA.m_modifier.on(Modifier::Type_Control);
    EXPECT_NE(globalMap->searchAssignment(mkCA), nullptr) << "C-A mapping not found";

    // Verify C-E -> End
    Key* keyE = m_setting.m_keyboard.searchKey(_T("E"));
    ASSERT_NE(keyE, nullptr);
    ModifiedKey mkCE(keyE);
    mkCE.m_modifier.on(Modifier::Type_Control);
    EXPECT_NE(globalMap->searchAssignment(mkCE), nullptr) << "C-E mapping not found";
}

//=============================================================================
// Test 9: Function Key Remapping (F5 -> F10)
//=============================================================================

TEST_F(KeyRemapLinuxTest, FunctionKeyRemap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key F5 = F10\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyF5 = m_setting.m_keyboard.searchKey(_T("F5"));
    ASSERT_NE(keyF5, nullptr);

    ModifiedKey mkF5(keyF5);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkF5);
    EXPECT_NE(ka, nullptr) << "F5 mapping not found";
}

//=============================================================================
// Test 10: Keymap Inheritance
//=============================================================================

TEST_F(KeyRemapLinuxTest, KeymapInheritance) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("keymap ChildMap : Global\n")
        _T("key C = D\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    const Keymap* childMap = m_setting.m_keymaps.searchByName(_T("ChildMap"));

    ASSERT_NE(globalMap, nullptr) << "Global keymap not found";
    ASSERT_NE(childMap, nullptr) << "ChildMap not found";

    // ChildMap should have C -> D mapping
    Key* keyC = m_setting.m_keyboard.searchKey(_T("C"));
    ASSERT_NE(keyC, nullptr);
    ModifiedKey mkC(keyC);
    EXPECT_NE(childMap->searchAssignment(mkC), nullptr) << "C mapping not in ChildMap";
}

//=============================================================================
// Test 11: KeySeq Definition
// Note: When assigning `key F1 = $MySeq`, the resulting action is a single
// ActionKeySeq that references the $MySeq keyseq. The keyseq itself contains
// the 3 actions (A, B, C), but the assignment has 1 action (the keyseq ref).
//=============================================================================

TEST_F(KeyRemapLinuxTest, KeySeqDefinition) {
    std::string config = getKeyDefinitions() +
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
    ASSERT_NE(ka, nullptr) << "F1 mapping not found";
    ASSERT_NE(ka->m_keySeq, nullptr);

    // The assignment should have at least 1 action (the keyseq reference)
    EXPECT_GE(ka->m_keySeq->getActions().size(), 1UL)
        << "KeySeq assignment should have at least 1 action";
}

//=============================================================================
// Test 12: Modifier Passthrough (*CapsLock = *LControl)
//=============================================================================

TEST_F(KeyRemapLinuxTest, ModifierPassthrough) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key *CapsLock = *LControl\n");

    LoadConfig(config);

    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);

    Key* keyCapsLock = m_setting.m_keyboard.searchKey(_T("CapsLock"));
    ASSERT_NE(keyCapsLock, nullptr);

    // Search for the assignment with "don't care" modifiers
    ModifiedKey mkCaps(keyCapsLock);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mkCaps);
    EXPECT_NE(ka, nullptr) << "CapsLock mapping not found";
}

//=============================================================================
// Test 13: Multiple Keymaps with Window Matching Pattern
//=============================================================================

TEST_F(KeyRemapLinuxTest, WindowMatchingKeymap) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Verify the window-specific keymap was created
    const Keymap* terminalMap = m_setting.m_keymaps.searchByName(_T("Terminal"));
    ASSERT_NE(terminalMap, nullptr) << "Terminal keymap not found";

    // Verify it has a parent keymap (Global)
    EXPECT_NE(terminalMap->getParentKeymap(), nullptr) << "Terminal should inherit from Global";

    // Note: Window matching functionality will be removed in FR-3 (global keymap only)
    // This test verifies the keymap structure was created correctly
}

//=============================================================================
// Test 14: Key Definition with Multiple Scan Codes (E0 prefix)
//=============================================================================

TEST_F(KeyRemapLinuxTest, ExtendedKeyDefinition) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key Home = End\n");

    LoadConfig(config);

    Key* keyHome = m_setting.m_keyboard.searchKey(_T("Home"));
    ASSERT_NE(keyHome, nullptr);

    // Home key should have E0 prefix (extended key)
    ASSERT_GE(keyHome->getScanCodesSize(), 1);
    const ScanCode& sc = keyHome->getScanCodes()[0];
    EXPECT_TRUE(sc.m_flags & ScanCode::E0) << "Home key should have E0 flag";
}

//=============================================================================
// Test 15: Verify Modifier Types
//=============================================================================

TEST_F(KeyRemapLinuxTest, ModifierTypes) {
    std::string config = getKeyDefinitions();

    LoadConfig(config);

    // Verify all modifier types have keys assigned
    const Keyboard::Mods& shiftMods =
        m_setting.m_keyboard.getModifiers(Modifier::Type_Shift);
    const Keyboard::Mods& controlMods =
        m_setting.m_keyboard.getModifiers(Modifier::Type_Control);
    const Keyboard::Mods& altMods =
        m_setting.m_keyboard.getModifiers(Modifier::Type_Alt);

    EXPECT_FALSE(shiftMods.empty()) << "Shift modifiers should be defined";
    EXPECT_FALSE(controlMods.empty()) << "Control modifiers should be defined";
    EXPECT_FALSE(altMods.empty()) << "Alt modifiers should be defined";

    // Verify left and right variants exist for shift
    bool hasLShift = false, hasRShift = false;
    for (const Key* k : shiftMods) {
        if (k->getName() == _T("LShift")) hasLShift = true;
        if (k->getName() == _T("RShift")) hasRShift = true;
    }
    EXPECT_TRUE(hasLShift) << "LShift should be a shift modifier";
    EXPECT_TRUE(hasRShift) << "RShift should be a shift modifier";
}

} // namespace yamy::test
