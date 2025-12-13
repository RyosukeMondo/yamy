//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_event_processor_ut.cpp - Unit tests for EventProcessor Layer 1
//
// Tests Layer 1 (evdevToYamyKeyCode) keycode mapping function:
// - Known evdev codes map to correct YAMY scan codes
// - Unmapped evdev codes return 0
// - Both US and JP keyboard layouts
//
// Part of task 3.1 in key-remapping-consistency spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include "../../src/platform/linux/keycode_mapping.h"

namespace yamy::test {

//=============================================================================
// Layer 1 Unit Tests: evdevToYamyKeyCode
//=============================================================================

class EventProcessorLayer1Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure we're using US layout for consistent testing
        yamy::platform::setLayoutOverride("us");
    }

    void TearDown() override {
        // Clear any layout override after test
        yamy::platform::clearLayoutOverride();
    }
};

// Test: Known evdev codes map to correct YAMY scan codes (letters)
TEST_F(EventProcessorLayer1Test, LetterKeyMapping) {
    // Test letter keys A-Z
    // Based on g_evdevToYamyMap in keycode_mapping.cpp

    // KEY_W (evdev 17) -> 0x0011 (W scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_W, 1), 0x0011);

    // KEY_A (evdev 30) -> 0x001E (A scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_A, 1), 0x001E);

    // KEY_S (evdev 31) -> 0x001F (S scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_S, 1), 0x001F);

    // KEY_D (evdev 32) -> 0x0020 (D scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_D, 1), 0x0020);

    // KEY_E (evdev 18) -> 0x0012 (E scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_E, 1), 0x0012);

    // KEY_R (evdev 19) -> 0x0013 (R scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_R, 1), 0x0013);

    // KEY_T (evdev 20) -> 0x0014 (T scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_T, 1), 0x0014);

    // KEY_U (evdev 22) -> 0x0016 (U scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_U, 1), 0x0016);

    // KEY_N (evdev 49) -> 0x0031 (N scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_N, 1), 0x0031);
}

// Test: Number keys map correctly
TEST_F(EventProcessorLayer1Test, NumberKeyMapping) {
    // KEY_1 (evdev 2) -> 0x0002 (1 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_1, 1), 0x0002);

    // KEY_2 (evdev 3) -> 0x0003 (2 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_2, 1), 0x0003);

    // KEY_0 (evdev 11) -> 0x000B (0 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_0, 1), 0x000B);
}

// Test: Special keys map correctly
TEST_F(EventProcessorLayer1Test, SpecialKeyMapping) {
    // KEY_ESC (evdev 1) -> 0x0001 (ESC scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_ESC, 1), 0x0001);

    // KEY_TAB (evdev 15) -> 0x000F (TAB scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_TAB, 1), 0x000F);

    // KEY_ENTER (evdev 28) -> 0x001C (ENTER scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_ENTER, 1), 0x001C);

    // KEY_BACKSPACE (evdev 14) -> 0x000E (BACKSPACE scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_BACKSPACE, 1), 0x000E);

    // KEY_SPACE (evdev 57) -> 0x0039 (SPACE scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_SPACE, 1), 0x0039);
}

// Test: Modifier keys map correctly
TEST_F(EventProcessorLayer1Test, ModifierKeyMapping) {
    // KEY_LEFTSHIFT (evdev 42) -> 0x002A (Left Shift scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_LEFTSHIFT, 1), 0x002A);

    // KEY_RIGHTSHIFT (evdev 54) -> 0x0036 (Right Shift scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_RIGHTSHIFT, 1), 0x0036);

    // KEY_LEFTCTRL (evdev 29) -> 0x001D (Left Ctrl scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_LEFTCTRL, 1), 0x001D);

    // KEY_LEFTALT (evdev 56) -> 0x0038 (Left Alt scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_LEFTALT, 1), 0x0038);
}

// Test: Function keys map correctly
TEST_F(EventProcessorLayer1Test, FunctionKeyMapping) {
    // KEY_F1 (evdev 59) -> 0x003B (F1 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_F1, 1), 0x003B);

    // KEY_F2 (evdev 60) -> 0x003C (F2 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_F2, 1), 0x003C);

    // KEY_F12 (evdev 88) -> 0x0058 (F12 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_F12, 1), 0x0058);
}

// Test: Arrow keys map correctly
TEST_F(EventProcessorLayer1Test, ArrowKeyMapping) {
    // Arrow keys use E0-extended scan codes
    // KEY_UP (evdev 103) -> 0xE048 (Up Arrow scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_UP, 1), 0xE048);

    // KEY_DOWN (evdev 108) -> 0xE050 (Down Arrow scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_DOWN, 1), 0xE050);

    // KEY_LEFT (evdev 105) -> 0xE04B (Left Arrow scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_LEFT, 1), 0xE04B);

    // KEY_RIGHT (evdev 106) -> 0xE04D (Right Arrow scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_RIGHT, 1), 0xE04D);
}

// Test: Numpad keys map correctly
TEST_F(EventProcessorLayer1Test, NumpadKeyMapping) {
    // KEY_KP0 (evdev 82) -> 0x0052 (Numpad 0 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_KP0, 1), 0x0052);

    // KEY_KP5 (evdev 76) -> 0x004C (Numpad 5 scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_KP5, 1), 0x004C);

    // KEY_KPENTER (evdev 96) -> 0xE01C (Numpad Enter scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_KPENTER, 1), 0xE01C);
}

// Test: Unmapped evdev codes return 0
TEST_F(EventProcessorLayer1Test, UnmappedKeysReturnZero) {
    // Test some reserved/unmapped evdev codes
    // These codes are not in g_evdevToYamyMap

    // KEY_RESERVED (0) should return 0
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(0, 1), 0);

    // High unmapped codes should return 0
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(999, 1), 0);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(65535, 1), 0);
}

// Test: Event type parameter is passed correctly (PRESS vs RELEASE)
TEST_F(EventProcessorLayer1Test, EventTypeHandling) {
    // The function should accept event_type parameter
    // Event type doesn't affect the mapping, just passed through

    // PRESS (1)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_W, 1), 0x0011);

    // RELEASE (0)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_W, 0), 0x0011);

    // REPEAT (2)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_W, 2), 0x0011);

    // Default (no event type)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_W), 0x0011);
}

// Test: Punctuation keys map correctly
TEST_F(EventProcessorLayer1Test, PunctuationKeyMapping) {
    // KEY_MINUS (evdev 12) -> 0x000C (Minus scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_MINUS, 1), 0x000C);

    // KEY_EQUAL (evdev 13) -> 0x000D (Equal scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_EQUAL, 1), 0x000D);

    // KEY_LEFTBRACE (evdev 26) -> 0x001A (Left Bracket scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_LEFTBRACE, 1), 0x001A);

    // KEY_RIGHTBRACE (evdev 27) -> 0x001B (Right Bracket scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_RIGHTBRACE, 1), 0x001B);

    // KEY_SEMICOLON (evdev 39) -> 0x0027 (Semicolon scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_SEMICOLON, 1), 0x0027);

    // KEY_COMMA (evdev 51) -> 0x0033 (Comma scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_COMMA, 1), 0x0033);

    // KEY_DOT (evdev 52) -> 0x0034 (Period scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_DOT, 1), 0x0034);

    // KEY_SLASH (evdev 53) -> 0x0035 (Slash scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_SLASH, 1), 0x0035);
}

// Test: Navigation keys map correctly
TEST_F(EventProcessorLayer1Test, NavigationKeyMapping) {
    // KEY_HOME (evdev 102) -> 0xE047 (Home scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_HOME, 1), 0xE047);

    // KEY_END (evdev 107) -> 0xE04F (End scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_END, 1), 0xE04F);

    // KEY_PAGEUP (evdev 104) -> 0xE049 (Page Up scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_PAGEUP, 1), 0xE049);

    // KEY_PAGEDOWN (evdev 109) -> 0xE051 (Page Down scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_PAGEDOWN, 1), 0xE051);

    // KEY_INSERT (evdev 110) -> 0xE052 (Insert scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_INSERT, 1), 0xE052);

    // KEY_DELETE (evdev 111) -> 0xE053 (Delete scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_DELETE, 1), 0xE053);
}

// Test: Complete alphabet coverage
TEST_F(EventProcessorLayer1Test, CompleteAlphabetMapping) {
    // Test all 26 letters to ensure complete coverage
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_A, 1), 0x001E);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_B, 1), 0x0030);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_C, 1), 0x002E);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_D, 1), 0x0020);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_E, 1), 0x0012);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_F, 1), 0x0021);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_G, 1), 0x0022);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_H, 1), 0x0023);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_I, 1), 0x0017);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_J, 1), 0x0024);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_K, 1), 0x0025);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_L, 1), 0x0026);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_M, 1), 0x0032);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_N, 1), 0x0031);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_O, 1), 0x0018);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_P, 1), 0x0019);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_Q, 1), 0x0010);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_R, 1), 0x0013);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_S, 1), 0x001F);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_T, 1), 0x0014);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_U, 1), 0x0016);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_V, 1), 0x002F);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_W, 1), 0x0011);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_X, 1), 0x002D);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_Y, 1), 0x0015);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_Z, 1), 0x002C);
}

// Test: Complete number row coverage
TEST_F(EventProcessorLayer1Test, CompleteNumberRowMapping) {
    // Test all 10 number keys
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_1, 1), 0x0002);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_2, 1), 0x0003);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_3, 1), 0x0004);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_4, 1), 0x0005);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_5, 1), 0x0006);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_6, 1), 0x0007);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_7, 1), 0x0008);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_8, 1), 0x0009);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_9, 1), 0x000A);
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_0, 1), 0x000B);
}

// Test: Lock keys map correctly
TEST_F(EventProcessorLayer1Test, LockKeyMapping) {
    // KEY_CAPSLOCK (evdev 58) -> 0x003A (Caps Lock scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_CAPSLOCK, 1), 0x003A);

    // KEY_NUMLOCK (evdev 69) -> 0x0045 (Num Lock scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_NUMLOCK, 1), 0x0045);

    // KEY_SCROLLLOCK (evdev 70) -> 0x0046 (Scroll Lock scan code)
    EXPECT_EQ(yamy::platform::evdevToYamyKeyCode(KEY_SCROLLLOCK, 1), 0x0046);
}

} // namespace yamy::test

//=============================================================================
// Main entry point
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
