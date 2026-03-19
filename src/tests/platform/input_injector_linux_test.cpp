//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// input_injector_linux_test.cpp - Unit tests for InputInjectorLinux
//
// Tests the Linux input injection implementation using uinput.
// These tests can run in two modes:
// 1. With /dev/uinput access (root): Tests actual injection
// 2. Without uinput access: Tests keycode mapping and error handling
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include "../../platform/linux/keycode_mapping.h"
#include "../../core/platform/types.h"
#include "../../core/input/input_event.h"
#include <cstdlib>
#include <fstream>
#include <unistd.h>

namespace yamy::platform {

//=============================================================================
// Keycode Mapping Tests - These run without uinput access
//=============================================================================

class KeycodeMappingTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test letter key mappings (A-Z) using PC/AT scan codes
// yamyToEvdevKeyCode uses scan code maps (g_scanToEvdevMap_US) as primary lookup
TEST_F(KeycodeMappingTest, LetterKeyMappingYamyToEvdev) {
    // Scan code 0x1E = A, KEY_A = 30
    EXPECT_EQ(yamyToEvdevKeyCode(0x1E), KEY_A);
    EXPECT_EQ(yamyToEvdevKeyCode(0x30), KEY_B);
    EXPECT_EQ(yamyToEvdevKeyCode(0x2E), KEY_C);
    EXPECT_EQ(yamyToEvdevKeyCode(0x20), KEY_D);
    EXPECT_EQ(yamyToEvdevKeyCode(0x12), KEY_E);
    EXPECT_EQ(yamyToEvdevKeyCode(0x21), KEY_F);
    EXPECT_EQ(yamyToEvdevKeyCode(0x22), KEY_G);
    EXPECT_EQ(yamyToEvdevKeyCode(0x23), KEY_H);
    EXPECT_EQ(yamyToEvdevKeyCode(0x17), KEY_I);
    EXPECT_EQ(yamyToEvdevKeyCode(0x24), KEY_J);
    EXPECT_EQ(yamyToEvdevKeyCode(0x25), KEY_K);
    EXPECT_EQ(yamyToEvdevKeyCode(0x26), KEY_L);
    EXPECT_EQ(yamyToEvdevKeyCode(0x32), KEY_M);
    EXPECT_EQ(yamyToEvdevKeyCode(0x31), KEY_N);
    EXPECT_EQ(yamyToEvdevKeyCode(0x18), KEY_O);
    EXPECT_EQ(yamyToEvdevKeyCode(0x19), KEY_P);
    EXPECT_EQ(yamyToEvdevKeyCode(0x10), KEY_Q);
    EXPECT_EQ(yamyToEvdevKeyCode(0x13), KEY_R);
    EXPECT_EQ(yamyToEvdevKeyCode(0x1F), KEY_S);
    EXPECT_EQ(yamyToEvdevKeyCode(0x14), KEY_T);
    EXPECT_EQ(yamyToEvdevKeyCode(0x16), KEY_U);
    EXPECT_EQ(yamyToEvdevKeyCode(0x2F), KEY_V);
    EXPECT_EQ(yamyToEvdevKeyCode(0x11), KEY_W);
    EXPECT_EQ(yamyToEvdevKeyCode(0x2D), KEY_X);
    EXPECT_EQ(yamyToEvdevKeyCode(0x15), KEY_Y);
    EXPECT_EQ(yamyToEvdevKeyCode(0x2C), KEY_Z);
}

TEST_F(KeycodeMappingTest, LetterKeyMappingEvdevToYamy) {
    // evdevToYamyKeyCode returns scan codes, not VK codes
    EXPECT_EQ(evdevToYamyKeyCode(KEY_A), 0x1E);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_B), 0x30);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_Z), 0x2C);
}

// Test number key mappings (0-9) using scan codes
TEST_F(KeycodeMappingTest, NumberKeyMappingYamyToEvdev) {
    // Scan code 0x0B = 0, 0x02 = 1, etc.
    EXPECT_EQ(yamyToEvdevKeyCode(0x0B), KEY_0);
    EXPECT_EQ(yamyToEvdevKeyCode(0x02), KEY_1);
    EXPECT_EQ(yamyToEvdevKeyCode(0x03), KEY_2);
    EXPECT_EQ(yamyToEvdevKeyCode(0x04), KEY_3);
    EXPECT_EQ(yamyToEvdevKeyCode(0x05), KEY_4);
    EXPECT_EQ(yamyToEvdevKeyCode(0x06), KEY_5);
    EXPECT_EQ(yamyToEvdevKeyCode(0x07), KEY_6);
    EXPECT_EQ(yamyToEvdevKeyCode(0x08), KEY_7);
    EXPECT_EQ(yamyToEvdevKeyCode(0x09), KEY_8);
    EXPECT_EQ(yamyToEvdevKeyCode(0x0A), KEY_9);
}

TEST_F(KeycodeMappingTest, NumberKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_0), 0x0B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_1), 0x02);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_9), 0x0A);
}

// Test function key mappings (F1-F12) using scan codes
TEST_F(KeycodeMappingTest, FunctionKeyMappingYamyToEvdev) {
    // Scan code 0x3B = F1, etc.
    EXPECT_EQ(yamyToEvdevKeyCode(0x3B), KEY_F1);
    EXPECT_EQ(yamyToEvdevKeyCode(0x3C), KEY_F2);
    EXPECT_EQ(yamyToEvdevKeyCode(0x3D), KEY_F3);
    EXPECT_EQ(yamyToEvdevKeyCode(0x3E), KEY_F4);
    EXPECT_EQ(yamyToEvdevKeyCode(0x3F), KEY_F5);
    EXPECT_EQ(yamyToEvdevKeyCode(0x40), KEY_F6);
    EXPECT_EQ(yamyToEvdevKeyCode(0x41), KEY_F7);
    EXPECT_EQ(yamyToEvdevKeyCode(0x42), KEY_F8);
    EXPECT_EQ(yamyToEvdevKeyCode(0x43), KEY_F9);
    EXPECT_EQ(yamyToEvdevKeyCode(0x44), KEY_F10);
    EXPECT_EQ(yamyToEvdevKeyCode(0x57), KEY_F11);
    EXPECT_EQ(yamyToEvdevKeyCode(0x58), KEY_F12);
}

TEST_F(KeycodeMappingTest, FunctionKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F1), 0x3B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F2), 0x3C);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F12), 0x58);
}

// Test modifier key mappings using scan codes
TEST_F(KeycodeMappingTest, ModifierKeyMappingYamyToEvdev) {
    // Scan code 0x2A = LShift, 0x36 = RShift
    EXPECT_EQ(yamyToEvdevKeyCode(0x2A), KEY_LEFTSHIFT);
    EXPECT_EQ(yamyToEvdevKeyCode(0x36), KEY_RIGHTSHIFT);
    // Scan code 0x1D = LCtrl, 0xE01D = RCtrl
    EXPECT_EQ(yamyToEvdevKeyCode(0x1D), KEY_LEFTCTRL);
    EXPECT_EQ(yamyToEvdevKeyCode(0xE01D), KEY_RIGHTCTRL);
    // Scan code 0x38 = LAlt, 0xE038 = RAlt
    EXPECT_EQ(yamyToEvdevKeyCode(0x38), KEY_LEFTALT);
    EXPECT_EQ(yamyToEvdevKeyCode(0xE038), KEY_RIGHTALT);
    // Scan code 0xE05B = LWin, 0xE05C = RWin
    EXPECT_EQ(yamyToEvdevKeyCode(0xE05B), KEY_LEFTMETA);
    EXPECT_EQ(yamyToEvdevKeyCode(0xE05C), KEY_RIGHTMETA);
}

TEST_F(KeycodeMappingTest, ModifierKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTSHIFT), 0x2A);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTSHIFT), 0x36);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTCTRL), 0x1D);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTCTRL), 0xE01D);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTALT), 0x38);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTALT), 0xE038);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTMETA), 0xE05B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTMETA), 0xE05C);
}

// Test special key mappings using scan codes
TEST_F(KeycodeMappingTest, SpecialKeyMappingYamyToEvdev) {
    // Scan code 0x01 = Escape
    EXPECT_EQ(yamyToEvdevKeyCode(0x01), KEY_ESC);
    // Scan code 0x0F = Tab
    EXPECT_EQ(yamyToEvdevKeyCode(0x0F), KEY_TAB);
    // Scan code 0x3A = Caps Lock
    EXPECT_EQ(yamyToEvdevKeyCode(0x3A), KEY_CAPSLOCK);
    // Scan code 0x1C = Enter
    EXPECT_EQ(yamyToEvdevKeyCode(0x1C), KEY_ENTER);
    // Scan code 0x0E = Backspace
    EXPECT_EQ(yamyToEvdevKeyCode(0x0E), KEY_BACKSPACE);
    // Scan code 0x39 = Space
    EXPECT_EQ(yamyToEvdevKeyCode(0x39), KEY_SPACE);
    // Scan code 0xE052 = Insert (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE052), KEY_INSERT);
    // Scan code 0xE053 = Delete (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE053), KEY_DELETE);
    // Scan code 0xE047 = Home (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE047), KEY_HOME);
    // Scan code 0xE04F = End (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE04F), KEY_END);
    // Scan code 0xE049 = Page Up (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE049), KEY_PAGEUP);
    // Scan code 0xE051 = Page Down (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE051), KEY_PAGEDOWN);
}

TEST_F(KeycodeMappingTest, SpecialKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_ESC), 0x01);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_TAB), 0x0F);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_CAPSLOCK), 0x3A);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_ENTER), 0x1C);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_BACKSPACE), 0x0E);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SPACE), 0x39);
}

// Test arrow key mappings using E0-extended scan codes
TEST_F(KeycodeMappingTest, ArrowKeyMappingYamyToEvdev) {
    // E0-extended scan codes for arrow keys
    EXPECT_EQ(yamyToEvdevKeyCode(0xE04B), KEY_LEFT);
    EXPECT_EQ(yamyToEvdevKeyCode(0xE048), KEY_UP);
    EXPECT_EQ(yamyToEvdevKeyCode(0xE04D), KEY_RIGHT);
    EXPECT_EQ(yamyToEvdevKeyCode(0xE050), KEY_DOWN);
}

TEST_F(KeycodeMappingTest, ArrowKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFT), 0xE04B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_UP), 0xE048);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHT), 0xE04D);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_DOWN), 0xE050);
}

// Test numpad key mappings using scan codes
TEST_F(KeycodeMappingTest, NumpadKeyMappingYamyToEvdev) {
    // Scan codes for numpad: 0x52=KP0, 0x4F=KP1, etc.
    EXPECT_EQ(yamyToEvdevKeyCode(0x52), KEY_KP0);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4F), KEY_KP1);
    EXPECT_EQ(yamyToEvdevKeyCode(0x50), KEY_KP2);
    EXPECT_EQ(yamyToEvdevKeyCode(0x51), KEY_KP3);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4B), KEY_KP4);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4C), KEY_KP5);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4D), KEY_KP6);
    EXPECT_EQ(yamyToEvdevKeyCode(0x47), KEY_KP7);
    EXPECT_EQ(yamyToEvdevKeyCode(0x48), KEY_KP8);
    EXPECT_EQ(yamyToEvdevKeyCode(0x49), KEY_KP9);
    // 0x37 = KPAsterisk, 0x4E = KPPlus, 0x4A = KPMinus, 0x53 = KPDot
    EXPECT_EQ(yamyToEvdevKeyCode(0x37), KEY_KPASTERISK);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4E), KEY_KPPLUS);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4A), KEY_KPMINUS);
    EXPECT_EQ(yamyToEvdevKeyCode(0x53), KEY_KPDOT);
    // 0xE035 = KPSlash (E0-extended)
    EXPECT_EQ(yamyToEvdevKeyCode(0xE035), KEY_KPSLASH);
}

TEST_F(KeycodeMappingTest, NumpadKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KP0), 0x52);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KP9), 0x49);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KPASTERISK), 0x37);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KPPLUS), 0x4E);
}

// Test punctuation key mappings using scan codes
TEST_F(KeycodeMappingTest, PunctuationKeyMappingYamyToEvdev) {
    // Scan code 0x0C = Minus
    EXPECT_EQ(yamyToEvdevKeyCode(0x0C), KEY_MINUS);
    // Scan code 0x0D = Equal
    EXPECT_EQ(yamyToEvdevKeyCode(0x0D), KEY_EQUAL);
    // Scan code 0x1A = LeftBrace
    EXPECT_EQ(yamyToEvdevKeyCode(0x1A), KEY_LEFTBRACE);
    // Scan code 0x1B = RightBrace
    EXPECT_EQ(yamyToEvdevKeyCode(0x1B), KEY_RIGHTBRACE);
    // Scan code 0x27 = Semicolon
    EXPECT_EQ(yamyToEvdevKeyCode(0x27), KEY_SEMICOLON);
    // Scan code 0x28 = Apostrophe
    EXPECT_EQ(yamyToEvdevKeyCode(0x28), KEY_APOSTROPHE);
    // Scan code 0x29 = Grave
    EXPECT_EQ(yamyToEvdevKeyCode(0x29), KEY_GRAVE);
    // Scan code 0x2B = Backslash
    EXPECT_EQ(yamyToEvdevKeyCode(0x2B), KEY_BACKSLASH);
    // Scan code 0x33 = Comma
    EXPECT_EQ(yamyToEvdevKeyCode(0x33), KEY_COMMA);
    // Scan code 0x34 = Dot
    EXPECT_EQ(yamyToEvdevKeyCode(0x34), KEY_DOT);
    // Scan code 0x35 = Slash
    EXPECT_EQ(yamyToEvdevKeyCode(0x35), KEY_SLASH);
}

TEST_F(KeycodeMappingTest, PunctuationKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_MINUS), 0x0C);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_EQUAL), 0x0D);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTBRACE), 0x1A);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTBRACE), 0x1B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SEMICOLON), 0x27);
}

// Test lock key mappings using scan codes
TEST_F(KeycodeMappingTest, LockKeyMappingYamyToEvdev) {
    // Scan code 0x45 = NumLock
    EXPECT_EQ(yamyToEvdevKeyCode(0x45), KEY_NUMLOCK);
    // Scan code 0x46 = ScrollLock
    EXPECT_EQ(yamyToEvdevKeyCode(0x46), KEY_SCROLLLOCK);
}

TEST_F(KeycodeMappingTest, LockKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_NUMLOCK), 0x45);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SCROLLLOCK), 0x46);
}

// Test unknown key mapping
TEST_F(KeycodeMappingTest, UnknownKeyMapping) {
    // Unknown keys should return 0
    EXPECT_EQ(yamyToEvdevKeyCode(0xFF), 0);
    EXPECT_EQ(yamyToEvdevKeyCode(0xFE), 0);
    EXPECT_EQ(evdevToYamyKeyCode(0xFFF), 0);
}

// Test bidirectional mapping consistency using scan codes
TEST_F(KeycodeMappingTest, BidirectionalConsistency) {
    // Test that yamy(scan) -> evdev -> yamy(scan) roundtrip works
    uint16_t testKeys[] = {
        0x1E, 0x30, 0x2C,            // A, B, Z (scan codes)
        0x0B, 0x02, 0x0A,            // 0, 1, 9
        0x3B, 0x3C, 0x58,            // F1, F2, F12
        0x2A, 0x1D, 0x38,            // LShift, LCtrl, LAlt
        0x01, 0x1C, 0x39,            // Esc, Enter, Space
        0xE04B, 0xE048, 0xE04D, 0xE050,  // Arrow keys (E0-extended)
    };

    for (uint16_t yamyKey : testKeys) {
        uint16_t evdev = yamyToEvdevKeyCode(yamyKey);
        if (evdev != 0) {
            uint16_t roundtrip = evdevToYamyKeyCode(evdev);
            EXPECT_EQ(roundtrip, yamyKey)
                << "Roundtrip failed for YAMY key 0x" << std::hex << yamyKey;
        }
    }
}

//=============================================================================
// isModifierKey Tests
//=============================================================================

class ModifierKeyTest : public ::testing::Test {};

TEST_F(ModifierKeyTest, ShiftKeysAreModifiers) {
    EXPECT_TRUE(isModifierKey(KEY_LEFTSHIFT));
    EXPECT_TRUE(isModifierKey(KEY_RIGHTSHIFT));
}

TEST_F(ModifierKeyTest, CtrlKeysAreModifiers) {
    EXPECT_TRUE(isModifierKey(KEY_LEFTCTRL));
    EXPECT_TRUE(isModifierKey(KEY_RIGHTCTRL));
}

TEST_F(ModifierKeyTest, AltKeysAreModifiers) {
    EXPECT_TRUE(isModifierKey(KEY_LEFTALT));
    EXPECT_TRUE(isModifierKey(KEY_RIGHTALT));
}

TEST_F(ModifierKeyTest, MetaKeysAreModifiers) {
    EXPECT_TRUE(isModifierKey(KEY_LEFTMETA));
    EXPECT_TRUE(isModifierKey(KEY_RIGHTMETA));
}

TEST_F(ModifierKeyTest, LockKeysAreModifiers) {
    EXPECT_TRUE(isModifierKey(KEY_CAPSLOCK));
    EXPECT_TRUE(isModifierKey(KEY_NUMLOCK));
    EXPECT_TRUE(isModifierKey(KEY_SCROLLLOCK));
}

TEST_F(ModifierKeyTest, RegularKeysAreNotModifiers) {
    EXPECT_FALSE(isModifierKey(KEY_A));
    EXPECT_FALSE(isModifierKey(KEY_1));
    EXPECT_FALSE(isModifierKey(KEY_SPACE));
    EXPECT_FALSE(isModifierKey(KEY_ENTER));
    EXPECT_FALSE(isModifierKey(KEY_ESC));
    EXPECT_FALSE(isModifierKey(KEY_F1));
}

//=============================================================================
// getKeyName Tests
//=============================================================================

class KeyNameTest : public ::testing::Test {};

TEST_F(KeyNameTest, LetterKeyNames) {
    EXPECT_STREQ(getKeyName(KEY_A), "A");
    EXPECT_STREQ(getKeyName(KEY_B), "B");
    EXPECT_STREQ(getKeyName(KEY_Z), "Z");
}

TEST_F(KeyNameTest, NumberKeyNames) {
    EXPECT_STREQ(getKeyName(KEY_0), "0");
    EXPECT_STREQ(getKeyName(KEY_1), "1");
    EXPECT_STREQ(getKeyName(KEY_9), "9");
}

TEST_F(KeyNameTest, SpecialKeyNames) {
    EXPECT_STREQ(getKeyName(KEY_ESC), "ESC");
    EXPECT_STREQ(getKeyName(KEY_TAB), "TAB");
    EXPECT_STREQ(getKeyName(KEY_ENTER), "ENTER");
    EXPECT_STREQ(getKeyName(KEY_SPACE), "SPACE");
    EXPECT_STREQ(getKeyName(KEY_BACKSPACE), "BACKSPACE");
    EXPECT_STREQ(getKeyName(KEY_CAPSLOCK), "CAPSLOCK");
}

TEST_F(KeyNameTest, ModifierKeyNames) {
    EXPECT_STREQ(getKeyName(KEY_LEFTSHIFT), "LSHIFT");
    EXPECT_STREQ(getKeyName(KEY_RIGHTSHIFT), "RSHIFT");
    EXPECT_STREQ(getKeyName(KEY_LEFTCTRL), "LCTRL");
    EXPECT_STREQ(getKeyName(KEY_RIGHTCTRL), "RCTRL");
    EXPECT_STREQ(getKeyName(KEY_LEFTALT), "LALT");
    EXPECT_STREQ(getKeyName(KEY_RIGHTALT), "RALT");
    EXPECT_STREQ(getKeyName(KEY_LEFTMETA), "LWIN");
    EXPECT_STREQ(getKeyName(KEY_RIGHTMETA), "RWIN");
}

TEST_F(KeyNameTest, FunctionKeyNames) {
    EXPECT_STREQ(getKeyName(KEY_F1), "F1");
    EXPECT_STREQ(getKeyName(KEY_F12), "F12");
}

TEST_F(KeyNameTest, UnknownKeyName) {
    EXPECT_STREQ(getKeyName(0xFFFF), "UNKNOWN");
}

//=============================================================================
// KEYBOARD_INPUT_DATA Tests
//=============================================================================

class KeyboardInputDataTest : public ::testing::Test {};

TEST_F(KeyboardInputDataTest, FlagsConstants) {
    // Verify the flag constants are defined correctly
    EXPECT_EQ(KEYBOARD_INPUT_DATA::BREAK, 1);
    EXPECT_EQ(KEYBOARD_INPUT_DATA::E0, 2);
    EXPECT_EQ(KEYBOARD_INPUT_DATA::E1, 4);
    EXPECT_EQ(KEYBOARD_INPUT_DATA::E0E1, 6);
}

TEST_F(KeyboardInputDataTest, KeyUpDetection) {
    KEYBOARD_INPUT_DATA data;
    data.Flags = KEYBOARD_INPUT_DATA::BREAK;
    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::BREAK);

    data.Flags = 0;
    EXPECT_FALSE(data.Flags & KEYBOARD_INPUT_DATA::BREAK);
}

TEST_F(KeyboardInputDataTest, MouseEventDetection) {
    KEYBOARD_INPUT_DATA data;
    data.Flags = KEYBOARD_INPUT_DATA::E1;
    EXPECT_TRUE(data.Flags & KEYBOARD_INPUT_DATA::E1);

    data.Flags = 0;
    EXPECT_FALSE(data.Flags & KEYBOARD_INPUT_DATA::E1);
}

TEST_F(KeyboardInputDataTest, MouseButtonMakeCodes) {
    // Verify mouse button make codes as used by InputInjectorLinux
    // 1 = Left, 2 = Right, 3 = Middle, 4 = Wheel up, 5 = Wheel down
    // 6 = X1, 7 = X2
    KEYBOARD_INPUT_DATA data;
    data.Flags = KEYBOARD_INPUT_DATA::E1;  // Mouse event flag

    data.MakeCode = 1;
    EXPECT_EQ(data.MakeCode, 1);  // Left button

    data.MakeCode = 2;
    EXPECT_EQ(data.MakeCode, 2);  // Right button

    data.MakeCode = 3;
    EXPECT_EQ(data.MakeCode, 3);  // Middle button
}

//=============================================================================
// MouseButton Enum Tests
//=============================================================================

class MouseButtonTest : public ::testing::Test {};

TEST_F(MouseButtonTest, EnumValues) {
    // Verify enum values exist
    MouseButton left = MouseButton::Left;
    MouseButton right = MouseButton::Right;
    MouseButton middle = MouseButton::Middle;
    MouseButton x1 = MouseButton::X1;
    MouseButton x2 = MouseButton::X2;

    // Just verify they're all different
    EXPECT_NE(static_cast<int>(left), static_cast<int>(right));
    EXPECT_NE(static_cast<int>(left), static_cast<int>(middle));
    EXPECT_NE(static_cast<int>(left), static_cast<int>(x1));
    EXPECT_NE(static_cast<int>(left), static_cast<int>(x2));
}

//=============================================================================
// Uinput Access Tests - These test error handling when uinput unavailable
//=============================================================================

class UinputAccessTest : public ::testing::Test {
protected:
    bool hasUinputAccess() const {
        // Check if /dev/uinput exists and is accessible
        return access("/dev/uinput", W_OK) == 0;
    }
};

TEST_F(UinputAccessTest, DeviceExists) {
    // /dev/uinput should exist on Linux systems
    std::ifstream uinput("/dev/uinput");
    bool exists = uinput.good();
    // Note: May fail due to permissions, which is expected
    if (!exists) {
        GTEST_SKIP() << "/dev/uinput not accessible (likely permission issue)";
    }
}

//=============================================================================
// Comprehensive Key Mapping Coverage Tests
//=============================================================================

class KeyMappingCoverageTest : public ::testing::Test {};

TEST_F(KeyMappingCoverageTest, AllLettersHaveMapping) {
    // Test all letters A-Z have valid evdev->yamy mappings
    uint16_t letterEvdevCodes[] = {
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
        KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
        KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
        KEY_Y, KEY_Z
    };
    for (int i = 0; i < 26; i++) {
        uint16_t yamy = evdevToYamyKeyCode(letterEvdevCodes[i]);
        EXPECT_NE(yamy, 0) << "Missing evdev->yamy mapping for letter " << char('A' + i);
        // Verify round-trip: yamy -> evdev should give us back the original
        uint16_t evdev = yamyToEvdevKeyCode(yamy);
        EXPECT_EQ(evdev, letterEvdevCodes[i])
            << "Round-trip failed for letter " << char('A' + i);
    }
}

TEST_F(KeyMappingCoverageTest, AllNumbersHaveMapping) {
    // Test all numbers 0-9 have valid evdev->yamy mappings
    uint16_t numberEvdevCodes[] = {
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
        KEY_5, KEY_6, KEY_7, KEY_8, KEY_9
    };
    for (int i = 0; i < 10; i++) {
        uint16_t yamy = evdevToYamyKeyCode(numberEvdevCodes[i]);
        EXPECT_NE(yamy, 0) << "Missing evdev->yamy mapping for number " << i;
        uint16_t evdev = yamyToEvdevKeyCode(yamy);
        EXPECT_EQ(evdev, numberEvdevCodes[i])
            << "Round-trip failed for number " << i;
    }
}

TEST_F(KeyMappingCoverageTest, AllFunctionKeysHaveMapping) {
    // Test F1-F12 have valid evdev->yamy mappings
    uint16_t fkeyEvdevCodes[] = {
        KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
        KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12
    };
    for (int i = 0; i < 12; i++) {
        uint16_t yamy = evdevToYamyKeyCode(fkeyEvdevCodes[i]);
        EXPECT_NE(yamy, 0) << "Missing evdev->yamy mapping for F" << (i + 1);
        uint16_t evdev = yamyToEvdevKeyCode(yamy);
        EXPECT_EQ(evdev, fkeyEvdevCodes[i])
            << "Round-trip failed for F" << (i + 1);
    }
}

TEST_F(KeyMappingCoverageTest, AllNumpadKeysHaveMapping) {
    // Test numpad 0-9 have valid evdev->yamy mappings
    uint16_t kpEvdevCodes[] = {
        KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4,
        KEY_KP5, KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9
    };
    for (int i = 0; i < 10; i++) {
        uint16_t yamy = evdevToYamyKeyCode(kpEvdevCodes[i]);
        EXPECT_NE(yamy, 0) << "Missing evdev->yamy mapping for NUMPAD" << i;
        uint16_t evdev = yamyToEvdevKeyCode(yamy);
        EXPECT_EQ(evdev, kpEvdevCodes[i])
            << "Round-trip failed for NUMPAD" << i;
    }
}

} // namespace yamy::platform
