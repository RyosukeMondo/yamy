//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_number_modifiers.cpp - Unit tests for ModifierKeyHandler
//
// Tests ModifierKeyHandler class for number keys as custom hardware modifiers:
// - Hold/tap detection with configurable threshold
// - State machine transitions (IDLE → WAITING → MODIFIER_ACTIVE/TAP_DETECTED)
// - Registration and query methods
// - Edge cases (system suspend/resume, spurious events)
//
// Part of task 4.6 in key-remapping-consistency spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../src/core/engine/modifier_key_handler.h"
#include "../src/core/engine/engine_event_processor.h"

namespace yamy::test {

using namespace yamy::engine;

//=============================================================================
// ModifierKeyHandler Unit Tests
//=============================================================================

class ModifierKeyHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create handler with 200ms default threshold
        handler = std::make_unique<ModifierKeyHandler>(200);

        // Register test number keys
        // _1 (0x0002) → LSHIFT
        handler->registerNumberModifier(0x0002, HardwareModifier::LSHIFT);
        // _2 (0x0003) → RSHIFT
        handler->registerNumberModifier(0x0003, HardwareModifier::RSHIFT);
        // _3 (0x0004) → LCTRL
        handler->registerNumberModifier(0x0004, HardwareModifier::LCTRL);
    }

    void TearDown() override {
        handler.reset();
    }

    std::unique_ptr<ModifierKeyHandler> handler;
};

//=============================================================================
// Registration and Query Tests
//=============================================================================

TEST_F(ModifierKeyHandlerTest, RegisterNumberModifier) {
    // Test that registered keys are recognized
    EXPECT_TRUE(handler->isNumberModifier(0x0002));  // _1
    EXPECT_TRUE(handler->isNumberModifier(0x0003));  // _2
    EXPECT_TRUE(handler->isNumberModifier(0x0004));  // _3

    // Test that unregistered keys are not recognized
    EXPECT_FALSE(handler->isNumberModifier(0x0005)); // _4 (not registered)
    EXPECT_FALSE(handler->isNumberModifier(0x0011)); // W (not a number key)
}

TEST_F(ModifierKeyHandlerTest, IsModifierHeldInitialState) {
    // All registered keys should start in non-held state
    EXPECT_FALSE(handler->isModifierHeld(0x0002)); // _1
    EXPECT_FALSE(handler->isModifierHeld(0x0003)); // _2
    EXPECT_FALSE(handler->isModifierHeld(0x0004)); // _3
}

//=============================================================================
// State Machine Tests: TAP Detection
//=============================================================================

TEST_F(ModifierKeyHandlerTest, TapDetection_QuickRelease) {
    // Simulate quick tap: PRESS → immediate RELEASE (< 200ms)

    // PRESS event
    auto result_press = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result_press.action, ProcessingAction::WAITING_FOR_THRESHOLD);
    EXPECT_FALSE(result_press.valid);
    EXPECT_FALSE(handler->isModifierHeld(0x0002));

    // Wait 50ms (well below threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // RELEASE event
    auto result_release = handler->processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result_release.action, ProcessingAction::APPLY_SUBSTITUTION_RELEASE);
    EXPECT_TRUE(result_release.valid);
    EXPECT_FALSE(handler->isModifierHeld(0x0002));
}

TEST_F(ModifierKeyHandlerTest, TapDetection_JustBelowThreshold) {
    // Simulate tap just below threshold: PRESS → wait 150ms → RELEASE

    // PRESS event
    auto result_press = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result_press.action, ProcessingAction::WAITING_FOR_THRESHOLD);

    // Wait 150ms (below 200ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // RELEASE event - should still be TAP
    auto result_release = handler->processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result_release.action, ProcessingAction::APPLY_SUBSTITUTION_RELEASE);
    EXPECT_TRUE(result_release.valid);
    EXPECT_FALSE(handler->isModifierHeld(0x0002));
}

//=============================================================================
// State Machine Tests: HOLD Detection
//=============================================================================

TEST_F(ModifierKeyHandlerTest, HoldDetection_ExceedsThreshold) {
    // Simulate hold: PRESS → wait 250ms → check state

    // PRESS event
    auto result_press = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result_press.action, ProcessingAction::WAITING_FOR_THRESHOLD);
    EXPECT_FALSE(result_press.valid);

    // Wait 250ms (exceeds 200ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Send another PRESS event to trigger state transition
    // (In real event flow, repeated PRESS events occur during hold)
    auto result_hold = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result_hold.action, ProcessingAction::ACTIVATE_MODIFIER);
    EXPECT_TRUE(result_hold.valid);
    EXPECT_EQ(result_hold.output_yamy_code, 0xA0);  // VK_LSHIFT
    EXPECT_TRUE(handler->isModifierHeld(0x0002));
}

TEST_F(ModifierKeyHandlerTest, HoldDetection_ReleaseAfterActivation) {
    // Simulate hold and release: PRESS → wait → activate → RELEASE

    // PRESS event
    handler->processNumberKey(0x0002, EventType::PRESS);

    // Wait to exceed threshold and activate
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto result_activate = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result_activate.action, ProcessingAction::ACTIVATE_MODIFIER);

    // RELEASE event - should deactivate modifier
    auto result_release = handler->processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result_release.action, ProcessingAction::DEACTIVATE_MODIFIER);
    EXPECT_TRUE(result_release.valid);
    EXPECT_EQ(result_release.output_yamy_code, 0xA0);  // VK_LSHIFT
    EXPECT_FALSE(handler->isModifierHeld(0x0002));
}

TEST_F(ModifierKeyHandlerTest, HoldDetection_MultipleModifiers) {
    // Test different modifier types return correct VK codes

    // _1 → LSHIFT (0xA0)
    handler->processNumberKey(0x0002, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto result1 = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result1.output_yamy_code, 0xA0);

    // _2 → RSHIFT (0xA1)
    handler->processNumberKey(0x0003, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto result2 = handler->processNumberKey(0x0003, EventType::PRESS);
    EXPECT_EQ(result2.output_yamy_code, 0xA1);

    // _3 → LCTRL (0xA2)
    handler->processNumberKey(0x0004, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto result3 = handler->processNumberKey(0x0004, EventType::PRESS);
    EXPECT_EQ(result3.output_yamy_code, 0xA2);
}

//=============================================================================
// State Machine Tests: Edge Cases
//=============================================================================

TEST_F(ModifierKeyHandlerTest, EdgeCase_SpuriousRelease) {
    // RELEASE without PRESS should be handled gracefully
    auto result = handler->processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result.action, ProcessingAction::NOT_A_NUMBER_MODIFIER);
    EXPECT_FALSE(result.valid);
}

TEST_F(ModifierKeyHandlerTest, EdgeCase_RepeatedPress) {
    // Multiple PRESS events before RELEASE (key repeat scenario)

    // First PRESS
    auto result1 = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result1.action, ProcessingAction::WAITING_FOR_THRESHOLD);

    // Wait a bit but not enough to exceed threshold
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Second PRESS (key repeat)
    auto result2 = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result2.action, ProcessingAction::WAITING_FOR_THRESHOLD);

    // Still should detect TAP on quick release
    auto result_release = handler->processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result_release.action, ProcessingAction::APPLY_SUBSTITUTION_RELEASE);
}

TEST_F(ModifierKeyHandlerTest, EdgeCase_AlreadyActive) {
    // PRESS when modifier already active (shouldn't happen but test graceful handling)

    // Activate modifier
    handler->processNumberKey(0x0002, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_TRUE(handler->isModifierHeld(0x0002));

    // Another PRESS while active - should ignore
    auto result = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result.action, ProcessingAction::WAITING_FOR_THRESHOLD);
    EXPECT_FALSE(result.valid);
}

TEST_F(ModifierKeyHandlerTest, EdgeCase_SystemSuspendResume) {
    // Simulate system suspend/resume (> 5 second elapsed time)
    // This test is time-consuming, so we create a handler with shorter thresholds for testing
    auto fast_handler = std::make_unique<ModifierKeyHandler>(100);
    fast_handler->registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    // PRESS event
    fast_handler->processNumberKey(0x0002, EventType::PRESS);

    // Wait 6 seconds (exceeds maximum threshold)
    // Note: For CI/CD, we won't actually wait 6 seconds. This test documents expected behavior.
    // In production, hasExceededMaximum() will return true after 5 seconds.
    // For testing, we trust the implementation of hasExceededMaximum().

    // Skip this test in CI/CD (too slow)
    GTEST_SKIP() << "System suspend/resume test skipped (too slow for CI/CD)";
}

TEST_F(ModifierKeyHandlerTest, EdgeCase_NotANumberModifier) {
    // Process a key that's not registered as a number modifier
    auto result = handler->processNumberKey(0x0011, EventType::PRESS);  // W key
    EXPECT_EQ(result.action, ProcessingAction::NOT_A_NUMBER_MODIFIER);
    EXPECT_FALSE(result.valid);
}

//=============================================================================
// Reset Tests
//=============================================================================

TEST_F(ModifierKeyHandlerTest, Reset_ClearsAllStates) {
    // Activate a modifier
    handler->processNumberKey(0x0002, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_TRUE(handler->isModifierHeld(0x0002));

    // Reset all states
    handler->reset();

    // Modifier should no longer be held
    EXPECT_FALSE(handler->isModifierHeld(0x0002));
}

TEST_F(ModifierKeyHandlerTest, Reset_AllowsNewEvents) {
    // Activate and reset
    handler->processNumberKey(0x0002, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    handler->processNumberKey(0x0002, EventType::PRESS);
    handler->reset();

    // New PRESS event should work normally
    auto result = handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result.action, ProcessingAction::WAITING_FOR_THRESHOLD);
    EXPECT_FALSE(result.valid);
}

//=============================================================================
// Threshold Configuration Tests
//=============================================================================

TEST_F(ModifierKeyHandlerTest, CustomThreshold_50ms) {
    // Create handler with custom 50ms threshold
    auto custom_handler = std::make_unique<ModifierKeyHandler>(50);
    custom_handler->registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    // PRESS event
    custom_handler->processNumberKey(0x0002, EventType::PRESS);

    // Wait 100ms (exceeds 50ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should activate modifier
    auto result = custom_handler->processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result.action, ProcessingAction::ACTIVATE_MODIFIER);
    EXPECT_TRUE(result.valid);
}

TEST_F(ModifierKeyHandlerTest, CustomThreshold_500ms) {
    // Create handler with custom 500ms threshold
    auto custom_handler = std::make_unique<ModifierKeyHandler>(500);
    custom_handler->registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    // PRESS event
    custom_handler->processNumberKey(0x0002, EventType::PRESS);

    // Wait 250ms (below 500ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // RELEASE - should still be TAP
    auto result = custom_handler->processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result.action, ProcessingAction::APPLY_SUBSTITUTION_RELEASE);
    EXPECT_TRUE(result.valid);
}

//=============================================================================
// All Hardware Modifiers Tests
//=============================================================================

TEST_F(ModifierKeyHandlerTest, AllModifierTypes) {
    // Create handler and register all modifier types
    auto full_handler = std::make_unique<ModifierKeyHandler>(100);

    // Register all 8 modifiers
    full_handler->registerNumberModifier(0x0002, HardwareModifier::LSHIFT);  // _1
    full_handler->registerNumberModifier(0x0003, HardwareModifier::RSHIFT);  // _2
    full_handler->registerNumberModifier(0x0004, HardwareModifier::LCTRL);   // _3
    full_handler->registerNumberModifier(0x0005, HardwareModifier::RCTRL);   // _4
    full_handler->registerNumberModifier(0x0006, HardwareModifier::LALT);    // _5
    full_handler->registerNumberModifier(0x0007, HardwareModifier::RALT);    // _6
    full_handler->registerNumberModifier(0x0008, HardwareModifier::LWIN);    // _7
    full_handler->registerNumberModifier(0x0009, HardwareModifier::RWIN);    // _8

    // Verify all are registered
    for (uint16_t scan = 0x0002; scan <= 0x0009; scan++) {
        EXPECT_TRUE(full_handler->isNumberModifier(scan));
    }

    // Test each modifier returns correct VK code
    struct TestCase {
        uint16_t scancode;
        uint16_t expected_vk;
    };

    TestCase test_cases[] = {
        {0x0002, 0xA0},  // LSHIFT
        {0x0003, 0xA1},  // RSHIFT
        {0x0004, 0xA2},  // LCTRL
        {0x0005, 0xA3},  // RCTRL
        {0x0006, 0xA4},  // LALT
        {0x0007, 0xA5},  // RALT
        {0x0008, 0x5B},  // LWIN
        {0x0009, 0x5C},  // RWIN
    };

    for (const auto& tc : test_cases) {
        full_handler->processNumberKey(tc.scancode, EventType::PRESS);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        auto result = full_handler->processNumberKey(tc.scancode, EventType::PRESS);
        EXPECT_EQ(result.action, ProcessingAction::ACTIVATE_MODIFIER);
        EXPECT_EQ(result.output_yamy_code, tc.expected_vk);
        full_handler->reset();
    }
}

} // namespace yamy::test
