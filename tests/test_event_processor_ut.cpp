//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_event_processor_ut.cpp - Unit tests for EventProcessor Layers 1 & 2
//
// Tests Layer 1 (evdevToYamyKeyCode) keycode mapping function:
// - Known evdev codes map to correct YAMY scan codes
// - Unmapped evdev codes return 0
// - Both US and JP keyboard layouts
//
// Tests Layer 2 (applySubstitution) substitution logic:
// - Substitution lookup with mock substitution table
// - Keys WITH substitution return transformed code
// - Keys WITHOUT substitution return original code (passthrough)
// - Modifier key substitutions work identically to regular keys
//
// Part of tasks 3.1 and 3.2 in key-remapping-consistency spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include "../../src/platform/linux/keycode_mapping.h"
#include "../../src/core/engine/engine_event_processor.h"

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

//=============================================================================
// Layer 2 Unit Tests: applySubstitution
//=============================================================================

class EventProcessorLayer2Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock substitution table for testing
        // Based on config_clean.mayu substitutions

        // Regular letter substitutions (e.g., W→A)
        mockSubstitutions[0x0011] = 0x001E;  // W → A
        mockSubstitutions[0x0013] = 0x0012;  // R → E
        mockSubstitutions[0x0014] = 0x0016;  // T → U

        // Number key substitution
        mockSubstitutions[0x0002] = 0x0003;  // 1 → 2

        // Modifier key substitutions (critical: must work identically to regular keys)
        // N → LShift (0x0031 → VK_LSHIFT which is 0xA0)
        mockSubstitutions[0x0031] = 0xA0;    // N → LShift (VK_LSHIFT)

        // Arrow key substitution (E0-extended scan codes)
        mockSubstitutions[0xE048] = 0xE050;  // Up → Down (for testing)

        // Create EventProcessor with mock substitution table
        processor = new yamy::EventProcessor(mockSubstitutions);

        // Disable debug logging for unit tests (cleaner output)
        processor->setDebugLogging(false);
    }

    void TearDown() override {
        delete processor;
    }

    yamy::SubstitutionTable mockSubstitutions;
    yamy::EventProcessor* processor;
};

// Test: Key WITH substitution returns transformed code
TEST_F(EventProcessorLayer2Test, SubstitutionApplied) {
    // W→A substitution: 0x0011 → 0x001E
    // Process event through all layers (we only care about Layer 2 for this test)
    // Input: KEY_W (evdev 17) which maps to 0x0011 in Layer 1
    auto result = processor->processEvent(KEY_W, yamy::EventType::PRESS);

    // Verify Layer 2 applied the substitution
    // Layer 1: evdev 17 → yamy 0x0011 (W)
    // Layer 2: yamy 0x0011 → yamy 0x001E (A)
    // Layer 3: yamy 0x001E → evdev 30 (KEY_A)
    EXPECT_EQ(result.output_yamy, 0x001E);  // After Layer 2: should be A scan code
    EXPECT_EQ(result.output_evdev, KEY_A);  // After Layer 3: should be evdev for A
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

// Test: Key WITHOUT substitution returns original code (passthrough)
TEST_F(EventProcessorLayer2Test, PassthroughWhenNoSubstitution) {
    // Test a key that has NO substitution in our mock table
    // KEY_S (evdev 31) → yamy 0x001F (not in mockSubstitutions)
    auto result = processor->processEvent(KEY_S, yamy::EventType::PRESS);

    // Verify Layer 2 passed through unchanged
    // Layer 1: evdev 31 → yamy 0x001F (S)
    // Layer 2: yamy 0x001F → yamy 0x001F (passthrough, no substitution)
    // Layer 3: yamy 0x001F → evdev 31 (KEY_S)
    EXPECT_EQ(result.output_yamy, 0x001F);  // After Layer 2: unchanged
    EXPECT_EQ(result.output_evdev, KEY_S);  // After Layer 3: same key
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

// Test: Multiple substitutions work correctly
TEST_F(EventProcessorLayer2Test, MultipleSubstitutions) {
    // R→E: 0x0013 → 0x0012
    auto result_r = processor->processEvent(KEY_R, yamy::EventType::PRESS);
    EXPECT_EQ(result_r.output_yamy, 0x0012);  // Should be E scan code
    EXPECT_EQ(result_r.output_evdev, KEY_E);

    // T→U: 0x0014 → 0x0016
    auto result_t = processor->processEvent(KEY_T, yamy::EventType::PRESS);
    EXPECT_EQ(result_t.output_yamy, 0x0016);  // Should be U scan code
    EXPECT_EQ(result_t.output_evdev, KEY_U);
}

// Test: Modifier key substitution works identically to regular keys (CRITICAL)
TEST_F(EventProcessorLayer2Test, ModifierSubstitutionIdenticalToRegular) {
    // This is THE critical test for requirement 7 (Code Consistency)
    // N→LShift must use IDENTICAL logic to W→A, with NO special cases

    // Test N→LShift: 0x0031 → 0xA0 (VK_LSHIFT)
    auto result = processor->processEvent(KEY_N, yamy::EventType::PRESS);

    // Verify Layer 2 applied substitution identically to regular keys
    // Layer 1: evdev 49 → yamy 0x0031 (N)
    // Layer 2: yamy 0x0031 → yamy 0xA0 (LShift VK) - SAME LOOKUP AS W→A
    // Layer 3: yamy 0xA0 → evdev for LShift
    EXPECT_EQ(result.output_yamy, 0xA0);  // After Layer 2: should be VK_LSHIFT
    EXPECT_TRUE(result.valid);

    // The key point: This test verifies that modifier substitutions
    // go through the SAME code path as regular substitutions.
    // There should be NO branching like "if (isModifier) { special case }"
}

// Test: Number key substitution works
TEST_F(EventProcessorLayer2Test, NumberKeySubstitution) {
    // 1→2: 0x0002 → 0x0003
    auto result = processor->processEvent(KEY_1, yamy::EventType::PRESS);

    // Layer 1: evdev 2 → yamy 0x0002 (1)
    // Layer 2: yamy 0x0002 → yamy 0x0003 (2)
    // Layer 3: yamy 0x0003 → evdev 3 (KEY_2)
    EXPECT_EQ(result.output_yamy, 0x0003);  // After Layer 2: should be 2 scan code
    EXPECT_EQ(result.output_evdev, KEY_2);
    EXPECT_TRUE(result.valid);
}

// Test: Extended scan code (E0-prefixed) substitution works
TEST_F(EventProcessorLayer2Test, ExtendedScanCodeSubstitution) {
    // Up→Down: 0xE048 → 0xE050
    auto result = processor->processEvent(KEY_UP, yamy::EventType::PRESS);

    // Layer 1: evdev 103 → yamy 0xE048 (Up Arrow)
    // Layer 2: yamy 0xE048 → yamy 0xE050 (Down Arrow)
    // Layer 3: yamy 0xE050 → evdev 108 (KEY_DOWN)
    EXPECT_EQ(result.output_yamy, 0xE050);  // After Layer 2: should be Down scan code
    EXPECT_EQ(result.output_evdev, KEY_DOWN);
    EXPECT_TRUE(result.valid);
}

// Test: Event type preservation through Layer 2 (PRESS)
TEST_F(EventProcessorLayer2Test, EventTypePreservationPress) {
    // Test that PRESS in → PRESS out, even with substitution
    auto result = processor->processEvent(KEY_W, yamy::EventType::PRESS);

    // Event type must be preserved regardless of substitution
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

// Test: Event type preservation through Layer 2 (RELEASE)
TEST_F(EventProcessorLayer2Test, EventTypePreservationRelease) {
    // Test that RELEASE in → RELEASE out, even with substitution
    auto result = processor->processEvent(KEY_W, yamy::EventType::RELEASE);

    // Event type must be preserved regardless of substitution
    EXPECT_EQ(result.type, yamy::EventType::RELEASE);
}

// Test: Both PRESS and RELEASE work with same substitution
TEST_F(EventProcessorLayer2Test, PressAndReleaseSymmetry) {
    // Test W→A for both PRESS and RELEASE
    auto press_result = processor->processEvent(KEY_W, yamy::EventType::PRESS);
    auto release_result = processor->processEvent(KEY_W, yamy::EventType::RELEASE);

    // Both should produce same output key (A), just different event types
    EXPECT_EQ(press_result.output_yamy, 0x001E);
    EXPECT_EQ(release_result.output_yamy, 0x001E);
    EXPECT_EQ(press_result.output_evdev, KEY_A);
    EXPECT_EQ(release_result.output_evdev, KEY_A);

    // But event types should be preserved
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);

    // This test verifies the fix for the R→E, T→U "only works on RELEASE" bug
}

// Test: Passthrough also works for both PRESS and RELEASE
TEST_F(EventProcessorLayer2Test, PassthroughPressAndReleaseSymmetry) {
    // Test S (no substitution) for both PRESS and RELEASE
    auto press_result = processor->processEvent(KEY_S, yamy::EventType::PRESS);
    auto release_result = processor->processEvent(KEY_S, yamy::EventType::RELEASE);

    // Both should passthrough as S, with different event types
    EXPECT_EQ(press_result.output_yamy, 0x001F);
    EXPECT_EQ(release_result.output_yamy, 0x001F);
    EXPECT_EQ(press_result.output_evdev, KEY_S);
    EXPECT_EQ(release_result.output_evdev, KEY_S);

    // Event types preserved
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);
}

// Test: Empty substitution table (all keys passthrough)
TEST_F(EventProcessorLayer2Test, EmptySubstitutionTable) {
    // Create processor with empty substitution table
    yamy::SubstitutionTable emptyTable;
    yamy::EventProcessor emptyProcessor(emptyTable);
    emptyProcessor.setDebugLogging(false);

    // All keys should passthrough unchanged
    auto result_w = emptyProcessor.processEvent(KEY_W, yamy::EventType::PRESS, nullptr);
    EXPECT_EQ(result_w.output_yamy, 0x0011);  // W unchanged
    EXPECT_EQ(result_w.output_evdev, KEY_W);

    auto result_a = emptyProcessor.processEvent(KEY_A, yamy::EventType::PRESS, nullptr);
    EXPECT_EQ(result_a.output_yamy, 0x001E);  // A unchanged
    EXPECT_EQ(result_a.output_evdev, KEY_A);
}

// Test: Substitution chain stops at Layer 2 (no double substitution)
TEST_F(EventProcessorLayer2Test, NoDoubleSubstitution) {
    // Create a chain: A→B, B→C
    // When we input A, we should get B, NOT C (no recursive substitution)
    yamy::SubstitutionTable chainTable;
    chainTable[0x001E] = 0x0030;  // A → B
    chainTable[0x0030] = 0x002E;  // B → C

    yamy::EventProcessor chainProcessor(chainTable);
    chainProcessor.setDebugLogging(false);

    // Input A should output B (single substitution lookup only)
    auto result = chainProcessor.processEvent(KEY_A, yamy::EventType::PRESS, nullptr);
    EXPECT_EQ(result.output_yamy, 0x0030);  // Should be B, not C
    EXPECT_EQ(result.output_evdev, KEY_B);  // After Layer 3: KEY_B

    // This verifies Layer 2 does exactly ONE lookup, not recursive
}

// Test: Identity substitution (key maps to itself)
TEST_F(EventProcessorLayer2Test, IdentitySubstitution) {
    // Add identity mapping: A → A
    yamy::SubstitutionTable identityTable;
    identityTable[0x001E] = 0x001E;  // A → A

    yamy::EventProcessor identityProcessor(identityTable);
    identityProcessor.setDebugLogging(false);

    auto result = identityProcessor.processEvent(KEY_A, yamy::EventType::PRESS, nullptr);
    EXPECT_EQ(result.output_yamy, 0x001E);  // A → A
    EXPECT_EQ(result.output_evdev, KEY_A);
    EXPECT_TRUE(result.valid);
}

//=============================================================================
// Layer 3 Unit Tests: yamyToEvdevKeyCode
//=============================================================================

class EventProcessorLayer3Test : public ::testing::Test {
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

// Test: YAMY scan codes map to correct evdev output codes (letters)
TEST_F(EventProcessorLayer3Test, ScanCodeLetterMapping) {
    // Test letter scan codes map correctly
    // These should be found in g_scanToEvdevMap_US

    // 0x0011 (W scan code) → KEY_W (evdev 17)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0011), KEY_W);

    // 0x001E (A scan code) → KEY_A (evdev 30)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001E), KEY_A);

    // 0x001F (S scan code) → KEY_S (evdev 31)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001F), KEY_S);

    // 0x0020 (D scan code) → KEY_D (evdev 32)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0020), KEY_D);

    // 0x0012 (E scan code) → KEY_E (evdev 18)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0012), KEY_E);

    // 0x0013 (R scan code) → KEY_R (evdev 19)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0013), KEY_R);

    // 0x0014 (T scan code) → KEY_T (evdev 20) - CRITICAL TEST
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0014), KEY_T);

    // 0x0016 (U scan code) → KEY_U (evdev 22)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0016), KEY_U);

    // 0x0031 (N scan code) → KEY_N (evdev 49)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0031), KEY_N);
}

// Test: CRITICAL - Scan map is checked BEFORE VK map
// This is THE most important test for Layer 3 priority fix
TEST_F(EventProcessorLayer3Test, ScanMapPriorityOverVKMap) {
    // CRITICAL TEST: 0x0014 has two possible meanings:
    // - In scan map: 0x0014 = T key (KEY_T)
    // - In VK map: 0x14 = VK_CAPITAL = CAPS LOCK (KEY_CAPSLOCK)
    //
    // The scan map MUST be checked FIRST, so 0x0014 → KEY_T not KEY_CAPSLOCK
    // This fixes the T→U substitution bug where T was incorrectly mapped to CAPSLOCK

    uint16_t scan_code_t = 0x0014;
    uint16_t expected_evdev = KEY_T;      // Should be T (evdev 20)
    uint16_t wrong_evdev = KEY_CAPSLOCK;  // Should NOT be CAPSLOCK (evdev 58)

    uint16_t result = yamy::platform::yamyToEvdevKeyCode(scan_code_t);

    // Verify scan map wins over VK map
    EXPECT_EQ(result, expected_evdev)
        << "CRITICAL: 0x0014 should map to KEY_T (scan map), not KEY_CAPSLOCK (VK map)";
    EXPECT_NE(result, wrong_evdev)
        << "CRITICAL: 0x0014 must NOT map to KEY_CAPSLOCK (VK map lookup)";

    // Additional verification: Ensure we're getting T, not CAPSLOCK
    EXPECT_EQ(result, 20) << "Expected evdev code 20 for KEY_T";
    EXPECT_NE(result, 58) << "Must NOT be evdev code 58 for KEY_CAPSLOCK";
}

// Test: Another scan/VK conflict - verify scan map priority
TEST_F(EventProcessorLayer3Test, ScanMapPriorityMultipleCases) {
    // There may be other scan codes that conflict with VK codes
    // Test a few more to ensure scan map always wins

    // 0x0001 (ESC scan) should map to KEY_ESC from scan map
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0001), KEY_ESC);

    // 0x000F (TAB scan) should map to KEY_TAB from scan map
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x000F), KEY_TAB);

    // 0x001C (ENTER scan) should map to KEY_ENTER from scan map
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001C), KEY_ENTER);

    // 0x003A (CAPSLOCK scan) should map to KEY_CAPSLOCK from scan map
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003A), KEY_CAPSLOCK);
}

// Test: Number scan codes map correctly
TEST_F(EventProcessorLayer3Test, ScanCodeNumberMapping) {
    // 0x0002 (1 scan) → KEY_1 (evdev 2)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0002), KEY_1);

    // 0x0003 (2 scan) → KEY_2 (evdev 3)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0003), KEY_2);

    // 0x000B (0 scan) → KEY_0 (evdev 11)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x000B), KEY_0);
}

// Test: Modifier scan codes map correctly
TEST_F(EventProcessorLayer3Test, ScanCodeModifierMapping) {
    // 0x002A (Left Shift scan) → KEY_LEFTSHIFT (evdev 42)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x002A), KEY_LEFTSHIFT);

    // 0x0036 (Right Shift scan) → KEY_RIGHTSHIFT (evdev 54)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0036), KEY_RIGHTSHIFT);

    // 0x001D (Left Ctrl scan) → KEY_LEFTCTRL (evdev 29)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001D), KEY_LEFTCTRL);

    // 0x0038 (Left Alt scan) → KEY_LEFTALT (evdev 56)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0038), KEY_LEFTALT);
}

// Test: E0-extended scan codes map correctly (arrow keys)
TEST_F(EventProcessorLayer3Test, ExtendedScanCodeMapping) {
    // E0-extended codes use the 0xE0XX format

    // 0xE048 (Up Arrow scan) → KEY_UP (evdev 103)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE048), KEY_UP);

    // 0xE050 (Down Arrow scan) → KEY_DOWN (evdev 108)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE050), KEY_DOWN);

    // 0xE04B (Left Arrow scan) → KEY_LEFT (evdev 105)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE04B), KEY_LEFT);

    // 0xE04D (Right Arrow scan) → KEY_RIGHT (evdev 106)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE04D), KEY_RIGHT);
}

// Test: E0-extended navigation keys
TEST_F(EventProcessorLayer3Test, ExtendedNavigationKeyMapping) {
    // 0xE047 (Home scan) → KEY_HOME (evdev 102)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE047), KEY_HOME);

    // 0xE04F (End scan) → KEY_END (evdev 107)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE04F), KEY_END);

    // 0xE049 (Page Up scan) → KEY_PAGEUP (evdev 104)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE049), KEY_PAGEUP);

    // 0xE051 (Page Down scan) → KEY_PAGEDOWN (evdev 109)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE051), KEY_PAGEDOWN);

    // 0xE052 (Insert scan) → KEY_INSERT (evdev 110)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE052), KEY_INSERT);

    // 0xE053 (Delete scan) → KEY_DELETE (evdev 111)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE053), KEY_DELETE);
}

// Test: VK map fallback for keys NOT in scan maps
TEST_F(EventProcessorLayer3Test, VKMapFallbackForSpecialKeys) {
    // Some keys are only in the VK map, not in scan maps
    // These should use VK map as fallback

    // VK_LSHIFT (0xA0) should fallback to VK map → KEY_LEFTSHIFT
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xA0), KEY_LEFTSHIFT);

    // VK_RSHIFT (0xA1) should fallback to VK map → KEY_RIGHTSHIFT
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xA1), KEY_RIGHTSHIFT);

    // VK_LCONTROL (0xA2) should fallback to VK map → KEY_LEFTCTRL
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xA2), KEY_LEFTCTRL);

    // VK_RCONTROL (0xA3) should fallback to VK map → KEY_RIGHTCTRL
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xA3), KEY_RIGHTCTRL);

    // VK_LMENU (0xA4) should fallback to VK map → KEY_LEFTALT
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xA4), KEY_LEFTALT);

    // VK_RMENU (0xA5) should fallback to VK map → KEY_RIGHTALT
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xA5), KEY_RIGHTALT);
}

// Test: VK codes that conflict with scan codes use scan map
TEST_F(EventProcessorLayer3Test, VKConflictWithScanCode) {
    // IMPORTANT: VK letter codes (0x41-0x5A) and number codes (0x30-0x39)
    // CONFLICT with scan codes for function keys and other keys.
    // The scan map takes priority, so these map to their scan code meanings, not VK meanings.

    // 0x41: VK_A in VK map, but also scan code for KEY_F7
    // Scan map wins → KEY_F7 (evdev 65)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x41), KEY_F7);

    // 0x57: VK_W in VK map, but also scan code for KEY_F11
    // Scan map wins → KEY_F11 (evdev 87)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x57), KEY_F11);

    // 0x30: VK_0 in VK map, but also scan code for KEY_B
    // Scan map wins → KEY_B (evdev 48)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x30), KEY_B);

    // This demonstrates the scan map priority is working correctly
}

// Test: Unmapped codes return 0
TEST_F(EventProcessorLayer3Test, UnmappedCodesReturnZero) {
    // Test codes that are NOT in scan maps OR VK map

    // Arbitrary unmapped scan code
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x00FF), 0);

    // High unmapped code
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x9999), 0);

    // Extended unmapped code
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE0FF), 0);

    // Very high unmapped code
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xFFFF), 0);

    // Zero code
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0000), 0);
}

// Test: Complete alphabet coverage via scan codes
TEST_F(EventProcessorLayer3Test, CompleteAlphabetScanMapping) {
    // Test all 26 letter scan codes map correctly
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001E), KEY_A);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0030), KEY_B);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x002E), KEY_C);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0020), KEY_D);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0012), KEY_E);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0021), KEY_F);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0022), KEY_G);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0023), KEY_H);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0017), KEY_I);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0024), KEY_J);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0025), KEY_K);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0026), KEY_L);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0032), KEY_M);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0031), KEY_N);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0018), KEY_O);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0019), KEY_P);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0010), KEY_Q);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0013), KEY_R);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001F), KEY_S);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0014), KEY_T);  // CRITICAL
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0016), KEY_U);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x002F), KEY_V);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0011), KEY_W);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x002D), KEY_X);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0015), KEY_Y);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x002C), KEY_Z);
}

// Test: Complete number row via scan codes
TEST_F(EventProcessorLayer3Test, CompleteNumberRowScanMapping) {
    // Test all 10 number scan codes
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0002), KEY_1);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0003), KEY_2);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0004), KEY_3);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0005), KEY_4);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0006), KEY_5);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0007), KEY_6);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0008), KEY_7);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0009), KEY_8);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x000A), KEY_9);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x000B), KEY_0);
}

// Test: Function keys via scan codes
TEST_F(EventProcessorLayer3Test, FunctionKeyScanMapping) {
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003B), KEY_F1);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003C), KEY_F2);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003D), KEY_F3);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003E), KEY_F4);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003F), KEY_F5);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0040), KEY_F6);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0041), KEY_F7);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0042), KEY_F8);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0043), KEY_F9);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0044), KEY_F10);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0057), KEY_F11);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0058), KEY_F12);
}

// Test: Numpad keys via scan codes
TEST_F(EventProcessorLayer3Test, NumpadScanMapping) {
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0052), KEY_KP0);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x004F), KEY_KP1);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0050), KEY_KP2);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0051), KEY_KP3);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x004B), KEY_KP4);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x004C), KEY_KP5);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x004D), KEY_KP6);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0047), KEY_KP7);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0048), KEY_KP8);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0049), KEY_KP9);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE01C), KEY_KPENTER);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0037), KEY_KPASTERISK);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x004E), KEY_KPPLUS);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x004A), KEY_KPMINUS);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0053), KEY_KPDOT);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE035), KEY_KPSLASH);
}

// Test: Punctuation keys via scan codes
TEST_F(EventProcessorLayer3Test, PunctuationScanMapping) {
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x000C), KEY_MINUS);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x000D), KEY_EQUAL);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001A), KEY_LEFTBRACE);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x001B), KEY_RIGHTBRACE);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0027), KEY_SEMICOLON);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0028), KEY_APOSTROPHE);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0029), KEY_GRAVE);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x002B), KEY_BACKSLASH);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0033), KEY_COMMA);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0034), KEY_DOT);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0035), KEY_SLASH);
}

// Test: Lock keys via scan codes
TEST_F(EventProcessorLayer3Test, LockKeyScanMapping) {
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x003A), KEY_CAPSLOCK);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0045), KEY_NUMLOCK);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0x0046), KEY_SCROLLLOCK);
}

// Test: Extended Right-side modifiers
TEST_F(EventProcessorLayer3Test, ExtendedRightModifierMapping) {
    // Right Ctrl (E0-extended)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE01D), KEY_RIGHTCTRL);

    // Right Alt (E0-extended)
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE038), KEY_RIGHTALT);

    // Left/Right Windows keys
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE05B), KEY_LEFTMETA);
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE05C), KEY_RIGHTMETA);

    // Menu/Apps key
    EXPECT_EQ(yamy::platform::yamyToEvdevKeyCode(0xE05D), KEY_MENU);
}

} // namespace yamy::test

//=============================================================================
// Main entry point
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
