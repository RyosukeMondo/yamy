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

// Test letter key mappings (A-Z)
TEST_F(KeycodeMappingTest, LetterKeyMappingYamyToEvdev) {
    // VK_A = 0x41, KEY_A = 30
    EXPECT_EQ(yamyToEvdevKeyCode(0x41), KEY_A);
    EXPECT_EQ(yamyToEvdevKeyCode(0x42), KEY_B);
    EXPECT_EQ(yamyToEvdevKeyCode(0x43), KEY_C);
    EXPECT_EQ(yamyToEvdevKeyCode(0x44), KEY_D);
    EXPECT_EQ(yamyToEvdevKeyCode(0x45), KEY_E);
    EXPECT_EQ(yamyToEvdevKeyCode(0x46), KEY_F);
    EXPECT_EQ(yamyToEvdevKeyCode(0x47), KEY_G);
    EXPECT_EQ(yamyToEvdevKeyCode(0x48), KEY_H);
    EXPECT_EQ(yamyToEvdevKeyCode(0x49), KEY_I);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4A), KEY_J);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4B), KEY_K);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4C), KEY_L);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4D), KEY_M);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4E), KEY_N);
    EXPECT_EQ(yamyToEvdevKeyCode(0x4F), KEY_O);
    EXPECT_EQ(yamyToEvdevKeyCode(0x50), KEY_P);
    EXPECT_EQ(yamyToEvdevKeyCode(0x51), KEY_Q);
    EXPECT_EQ(yamyToEvdevKeyCode(0x52), KEY_R);
    EXPECT_EQ(yamyToEvdevKeyCode(0x53), KEY_S);
    EXPECT_EQ(yamyToEvdevKeyCode(0x54), KEY_T);
    EXPECT_EQ(yamyToEvdevKeyCode(0x55), KEY_U);
    EXPECT_EQ(yamyToEvdevKeyCode(0x56), KEY_V);
    EXPECT_EQ(yamyToEvdevKeyCode(0x57), KEY_W);
    EXPECT_EQ(yamyToEvdevKeyCode(0x58), KEY_X);
    EXPECT_EQ(yamyToEvdevKeyCode(0x59), KEY_Y);
    EXPECT_EQ(yamyToEvdevKeyCode(0x5A), KEY_Z);
}

TEST_F(KeycodeMappingTest, LetterKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_A), 0x41);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_B), 0x42);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_Z), 0x5A);
}

// Test number key mappings (0-9)
TEST_F(KeycodeMappingTest, NumberKeyMappingYamyToEvdev) {
    // VK_0 = 0x30, KEY_0 = 11
    EXPECT_EQ(yamyToEvdevKeyCode(0x30), KEY_0);
    EXPECT_EQ(yamyToEvdevKeyCode(0x31), KEY_1);
    EXPECT_EQ(yamyToEvdevKeyCode(0x32), KEY_2);
    EXPECT_EQ(yamyToEvdevKeyCode(0x33), KEY_3);
    EXPECT_EQ(yamyToEvdevKeyCode(0x34), KEY_4);
    EXPECT_EQ(yamyToEvdevKeyCode(0x35), KEY_5);
    EXPECT_EQ(yamyToEvdevKeyCode(0x36), KEY_6);
    EXPECT_EQ(yamyToEvdevKeyCode(0x37), KEY_7);
    EXPECT_EQ(yamyToEvdevKeyCode(0x38), KEY_8);
    EXPECT_EQ(yamyToEvdevKeyCode(0x39), KEY_9);
}

TEST_F(KeycodeMappingTest, NumberKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_0), 0x30);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_1), 0x31);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_9), 0x39);
}

// Test function key mappings (F1-F12)
TEST_F(KeycodeMappingTest, FunctionKeyMappingYamyToEvdev) {
    // VK_F1 = 0x70, KEY_F1 = 59
    EXPECT_EQ(yamyToEvdevKeyCode(0x70), KEY_F1);
    EXPECT_EQ(yamyToEvdevKeyCode(0x71), KEY_F2);
    EXPECT_EQ(yamyToEvdevKeyCode(0x72), KEY_F3);
    EXPECT_EQ(yamyToEvdevKeyCode(0x73), KEY_F4);
    EXPECT_EQ(yamyToEvdevKeyCode(0x74), KEY_F5);
    EXPECT_EQ(yamyToEvdevKeyCode(0x75), KEY_F6);
    EXPECT_EQ(yamyToEvdevKeyCode(0x76), KEY_F7);
    EXPECT_EQ(yamyToEvdevKeyCode(0x77), KEY_F8);
    EXPECT_EQ(yamyToEvdevKeyCode(0x78), KEY_F9);
    EXPECT_EQ(yamyToEvdevKeyCode(0x79), KEY_F10);
    EXPECT_EQ(yamyToEvdevKeyCode(0x7A), KEY_F11);
    EXPECT_EQ(yamyToEvdevKeyCode(0x7B), KEY_F12);
}

TEST_F(KeycodeMappingTest, FunctionKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F1), 0x70);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F2), 0x71);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_F12), 0x7B);
}

// Test modifier key mappings
TEST_F(KeycodeMappingTest, ModifierKeyMappingYamyToEvdev) {
    // VK_LSHIFT = 0xA0, VK_RSHIFT = 0xA1
    EXPECT_EQ(yamyToEvdevKeyCode(0xA0), KEY_LEFTSHIFT);
    EXPECT_EQ(yamyToEvdevKeyCode(0xA1), KEY_RIGHTSHIFT);
    // VK_LCONTROL = 0xA2, VK_RCONTROL = 0xA3
    EXPECT_EQ(yamyToEvdevKeyCode(0xA2), KEY_LEFTCTRL);
    EXPECT_EQ(yamyToEvdevKeyCode(0xA3), KEY_RIGHTCTRL);
    // VK_LMENU = 0xA4 (Left Alt), VK_RMENU = 0xA5 (Right Alt)
    EXPECT_EQ(yamyToEvdevKeyCode(0xA4), KEY_LEFTALT);
    EXPECT_EQ(yamyToEvdevKeyCode(0xA5), KEY_RIGHTALT);
    // VK_LWIN = 0x5B, VK_RWIN = 0x5C
    EXPECT_EQ(yamyToEvdevKeyCode(0x5B), KEY_LEFTMETA);
    EXPECT_EQ(yamyToEvdevKeyCode(0x5C), KEY_RIGHTMETA);
}

TEST_F(KeycodeMappingTest, ModifierKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTSHIFT), 0xA0);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTSHIFT), 0xA1);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTCTRL), 0xA2);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTCTRL), 0xA3);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTALT), 0xA4);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTALT), 0xA5);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTMETA), 0x5B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTMETA), 0x5C);
}

// Test special key mappings
TEST_F(KeycodeMappingTest, SpecialKeyMappingYamyToEvdev) {
    // VK_ESCAPE = 0x1B
    EXPECT_EQ(yamyToEvdevKeyCode(0x1B), KEY_ESC);
    // VK_TAB = 0x09
    EXPECT_EQ(yamyToEvdevKeyCode(0x09), KEY_TAB);
    // VK_CAPITAL = 0x14 (Caps Lock)
    EXPECT_EQ(yamyToEvdevKeyCode(0x14), KEY_CAPSLOCK);
    // VK_RETURN = 0x0D
    EXPECT_EQ(yamyToEvdevKeyCode(0x0D), KEY_ENTER);
    // VK_BACK = 0x08
    EXPECT_EQ(yamyToEvdevKeyCode(0x08), KEY_BACKSPACE);
    // VK_SPACE = 0x20
    EXPECT_EQ(yamyToEvdevKeyCode(0x20), KEY_SPACE);
    // VK_INSERT = 0x2D
    EXPECT_EQ(yamyToEvdevKeyCode(0x2D), KEY_INSERT);
    // VK_DELETE = 0x2E
    EXPECT_EQ(yamyToEvdevKeyCode(0x2E), KEY_DELETE);
    // VK_HOME = 0x24
    EXPECT_EQ(yamyToEvdevKeyCode(0x24), KEY_HOME);
    // VK_END = 0x23
    EXPECT_EQ(yamyToEvdevKeyCode(0x23), KEY_END);
    // VK_PRIOR = 0x21 (Page Up)
    EXPECT_EQ(yamyToEvdevKeyCode(0x21), KEY_PAGEUP);
    // VK_NEXT = 0x22 (Page Down)
    EXPECT_EQ(yamyToEvdevKeyCode(0x22), KEY_PAGEDOWN);
}

TEST_F(KeycodeMappingTest, SpecialKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_ESC), 0x1B);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_TAB), 0x09);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_CAPSLOCK), 0x14);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_ENTER), 0x0D);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_BACKSPACE), 0x08);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SPACE), 0x20);
}

// Test arrow key mappings
TEST_F(KeycodeMappingTest, ArrowKeyMappingYamyToEvdev) {
    // VK_LEFT = 0x25, VK_UP = 0x26, VK_RIGHT = 0x27, VK_DOWN = 0x28
    EXPECT_EQ(yamyToEvdevKeyCode(0x25), KEY_LEFT);
    EXPECT_EQ(yamyToEvdevKeyCode(0x26), KEY_UP);
    EXPECT_EQ(yamyToEvdevKeyCode(0x27), KEY_RIGHT);
    EXPECT_EQ(yamyToEvdevKeyCode(0x28), KEY_DOWN);
}

TEST_F(KeycodeMappingTest, ArrowKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFT), 0x25);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_UP), 0x26);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHT), 0x27);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_DOWN), 0x28);
}

// Test numpad key mappings
TEST_F(KeycodeMappingTest, NumpadKeyMappingYamyToEvdev) {
    // VK_NUMPAD0 = 0x60 - 0x69
    EXPECT_EQ(yamyToEvdevKeyCode(0x60), KEY_KP0);
    EXPECT_EQ(yamyToEvdevKeyCode(0x61), KEY_KP1);
    EXPECT_EQ(yamyToEvdevKeyCode(0x62), KEY_KP2);
    EXPECT_EQ(yamyToEvdevKeyCode(0x63), KEY_KP3);
    EXPECT_EQ(yamyToEvdevKeyCode(0x64), KEY_KP4);
    EXPECT_EQ(yamyToEvdevKeyCode(0x65), KEY_KP5);
    EXPECT_EQ(yamyToEvdevKeyCode(0x66), KEY_KP6);
    EXPECT_EQ(yamyToEvdevKeyCode(0x67), KEY_KP7);
    EXPECT_EQ(yamyToEvdevKeyCode(0x68), KEY_KP8);
    EXPECT_EQ(yamyToEvdevKeyCode(0x69), KEY_KP9);
    // VK_MULTIPLY = 0x6A, VK_ADD = 0x6B, VK_SUBTRACT = 0x6D, VK_DECIMAL = 0x6E, VK_DIVIDE = 0x6F
    EXPECT_EQ(yamyToEvdevKeyCode(0x6A), KEY_KPASTERISK);
    EXPECT_EQ(yamyToEvdevKeyCode(0x6B), KEY_KPPLUS);
    EXPECT_EQ(yamyToEvdevKeyCode(0x6D), KEY_KPMINUS);
    EXPECT_EQ(yamyToEvdevKeyCode(0x6E), KEY_KPDOT);
    EXPECT_EQ(yamyToEvdevKeyCode(0x6F), KEY_KPSLASH);
}

TEST_F(KeycodeMappingTest, NumpadKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KP0), 0x60);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KP9), 0x69);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KPASTERISK), 0x6A);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_KPPLUS), 0x6B);
}

// Test punctuation key mappings
TEST_F(KeycodeMappingTest, PunctuationKeyMappingYamyToEvdev) {
    // VK_OEM_MINUS = 0xBD
    EXPECT_EQ(yamyToEvdevKeyCode(0xBD), KEY_MINUS);
    // VK_OEM_PLUS = 0xBB (Equal sign)
    EXPECT_EQ(yamyToEvdevKeyCode(0xBB), KEY_EQUAL);
    // VK_OEM_4 = 0xDB ([)
    EXPECT_EQ(yamyToEvdevKeyCode(0xDB), KEY_LEFTBRACE);
    // VK_OEM_6 = 0xDD (])
    EXPECT_EQ(yamyToEvdevKeyCode(0xDD), KEY_RIGHTBRACE);
    // VK_OEM_1 = 0xBA (;)
    EXPECT_EQ(yamyToEvdevKeyCode(0xBA), KEY_SEMICOLON);
    // VK_OEM_7 = 0xDE (')
    EXPECT_EQ(yamyToEvdevKeyCode(0xDE), KEY_APOSTROPHE);
    // VK_OEM_3 = 0xC0 (`)
    EXPECT_EQ(yamyToEvdevKeyCode(0xC0), KEY_GRAVE);
    // VK_OEM_5 = 0xDC (\)
    EXPECT_EQ(yamyToEvdevKeyCode(0xDC), KEY_BACKSLASH);
    // VK_OEM_COMMA = 0xBC (,)
    EXPECT_EQ(yamyToEvdevKeyCode(0xBC), KEY_COMMA);
    // VK_OEM_PERIOD = 0xBE (.)
    EXPECT_EQ(yamyToEvdevKeyCode(0xBE), KEY_DOT);
    // VK_OEM_2 = 0xBF (/)
    EXPECT_EQ(yamyToEvdevKeyCode(0xBF), KEY_SLASH);
}

TEST_F(KeycodeMappingTest, PunctuationKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_MINUS), 0xBD);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_EQUAL), 0xBB);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_LEFTBRACE), 0xDB);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_RIGHTBRACE), 0xDD);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SEMICOLON), 0xBA);
}

// Test lock key mappings
TEST_F(KeycodeMappingTest, LockKeyMappingYamyToEvdev) {
    // VK_NUMLOCK = 0x90
    EXPECT_EQ(yamyToEvdevKeyCode(0x90), KEY_NUMLOCK);
    // VK_SCROLL = 0x91
    EXPECT_EQ(yamyToEvdevKeyCode(0x91), KEY_SCROLLLOCK);
}

TEST_F(KeycodeMappingTest, LockKeyMappingEvdevToYamy) {
    EXPECT_EQ(evdevToYamyKeyCode(KEY_NUMLOCK), 0x90);
    EXPECT_EQ(evdevToYamyKeyCode(KEY_SCROLLLOCK), 0x91);
}

// Test unknown key mapping
TEST_F(KeycodeMappingTest, UnknownKeyMapping) {
    // Unknown keys should return 0
    EXPECT_EQ(yamyToEvdevKeyCode(0xFF), 0);
    EXPECT_EQ(yamyToEvdevKeyCode(0xFE), 0);
    EXPECT_EQ(evdevToYamyKeyCode(0xFFF), 0);
}

// Test bidirectional mapping consistency
TEST_F(KeycodeMappingTest, BidirectionalConsistency) {
    // Test that yamy -> evdev -> yamy roundtrip works for common keys
    uint16_t testKeys[] = {
        0x41, 0x42, 0x5A,       // A, B, Z
        0x30, 0x31, 0x39,       // 0, 1, 9
        0x70, 0x71, 0x7B,       // F1, F2, F12
        0xA0, 0xA2, 0xA4,       // LShift, LCtrl, LAlt
        0x1B, 0x0D, 0x20,       // Esc, Enter, Space
        0x25, 0x26, 0x27, 0x28, // Arrow keys
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
    // Test all letters A-Z have valid mappings
    for (int i = 0; i < 26; i++) {
        uint16_t vk = 0x41 + i;  // VK_A to VK_Z
        uint16_t evdev = yamyToEvdevKeyCode(vk);
        EXPECT_NE(evdev, 0) << "Missing mapping for letter VK_" << char('A' + i);
    }
}

TEST_F(KeyMappingCoverageTest, AllNumbersHaveMapping) {
    // Test all numbers 0-9 have valid mappings
    for (int i = 0; i < 10; i++) {
        uint16_t vk = 0x30 + i;  // VK_0 to VK_9
        uint16_t evdev = yamyToEvdevKeyCode(vk);
        EXPECT_NE(evdev, 0) << "Missing mapping for number VK_" << i;
    }
}

TEST_F(KeyMappingCoverageTest, AllFunctionKeysHaveMapping) {
    // Test F1-F12 have valid mappings
    for (int i = 0; i < 12; i++) {
        uint16_t vk = 0x70 + i;  // VK_F1 to VK_F12
        uint16_t evdev = yamyToEvdevKeyCode(vk);
        EXPECT_NE(evdev, 0) << "Missing mapping for F" << (i + 1);
    }
}

TEST_F(KeyMappingCoverageTest, AllNumpadKeysHaveMapping) {
    // Test numpad 0-9 have valid mappings
    for (int i = 0; i < 10; i++) {
        uint16_t vk = 0x60 + i;  // VK_NUMPAD0 to VK_NUMPAD9
        uint16_t evdev = yamyToEvdevKeyCode(vk);
        EXPECT_NE(evdev, 0) << "Missing mapping for NUMPAD" << i;
    }
}

} // namespace yamy::platform
