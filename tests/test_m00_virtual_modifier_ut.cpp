#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <iostream>

#include "../src/core/engine/engine_event_processor.h"
#include "../src/core/engine/modifier_key_handler.h"
#include "../src/core/engine/lookup_table.h"
#include "../src/core/input/modifier_state.h"

using namespace yamy;

/**
 * @test M00VirtualModifierActivation
 * @brief CRITICAL: Tests that CapsLock → M00 virtual modifier activation works correctly
 *
 * This test verifies the fix for the bug where M00 virtual modifiers weren't
 * activating because EventProcessor::layer2_applySubstitution() wasn't calling
 * processNumberKey().
 *
 * Test Scenarios:
 * 1. Press CapsLock → should be suppressed (return evdev 0)
 * 2. Hold CapsLock >200ms + press H → should output LEFT arrow
 * 3. Release CapsLock → should deactivate M00
 * 4. Press H alone → should pass through as H
 * 5. Tap CapsLock <200ms → should output Escape
 */
class M00VirtualModifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<EventProcessor>();
        modState = std::make_unique<input::ModifierState>();

        // Register CapsLock (0x3A) as M00 trigger with Escape (0x01) as tap output
        processor->registerVirtualModifierTrigger(0x3A, 0, 0x01);

        // Add rule: M00+H (0x23) → LEFT (0xE04B)
        auto* lookupTable = processor->getLookupTable();
        ASSERT_NE(lookupTable, nullptr);

        engine::CompiledRule rule;
        rule.outputScanCode = 0xE04B; // LEFT arrow (extended scan code)
        rule.requiredOn.set(input::ModifierState::VIRTUAL_OFFSET + 0); // M00 must be ON
        lookupTable->addRule(0x23, rule); // H key
    }

    void TearDown() override {
        processor.reset();
        modState.reset();
    }

    std::unique_ptr<EventProcessor> processor;
    std::unique_ptr<input::ModifierState> modState;
};

/**
 * Test 1: CapsLock press should be suppressed (not output)
 * CRITICAL: This verifies that virtual modifier triggers are processed in layer2
 */
TEST_F(M00VirtualModifierTest, CapsLockPressShouldBeSuppressed) {
    // Press CapsLock (evdev 58 → YAMY 0x3A)
    auto result = processor->processEvent(58, EventType::PRESS, modState.get());

    // CRITICAL: CapsLock should be suppressed (output_evdev == 0)
    // If this fails, it means processNumberKey() isn't being called in layer2
    EXPECT_EQ(result.output_evdev, 0)
        << "CRITICAL BUG: CapsLock not suppressed! "
        << "EventProcessor::layer2_applySubstitution() must call processNumberKey() "
        << "BEFORE rule lookup to activate virtual modifiers.";

    EXPECT_TRUE(result.valid || result.output_evdev == 0)
        << "Suppressed events should either be invalid or have output_evdev=0";
}

/**
 * Test 2: Hold CapsLock >200ms + press H → should activate M00 and output LEFT
 * CRITICAL: This verifies the complete M00 activation and rule matching flow
 */
TEST_F(M00VirtualModifierTest, HoldCapsLockPlusHShouldOutputLeft) {
    // Press CapsLock
    auto result1 = processor->processEvent(58, EventType::PRESS, modState.get());
    EXPECT_EQ(result1.output_evdev, 0) << "CapsLock press should be suppressed";

    // Wait for threshold (200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Press H while CapsLock is held
    // The checkAndActivateWaitingModifiers() should be called at the start
    // of processEvent and activate M00 before processing H
    auto result2 = processor->processEvent(35, EventType::PRESS, modState.get()); // evdev 35 = H

    // CRITICAL: H should be remapped to LEFT (evdev 105)
    // If this fails, it means either:
    // 1. M00 didn't activate (processNumberKey not called)
    // 2. Rule lookup isn't checking M00 state
    // 3. checkAndActivateWaitingModifiers() not called
    EXPECT_EQ(result2.output_evdev, 105)
        << "CRITICAL BUG: M00+H should output LEFT (evdev 105)! "
        << "Got evdev " << result2.output_evdev << " instead. "
        << "This means M00 virtual modifier did not activate correctly.";

    EXPECT_EQ(result2.output_yamy, 0xE04B) << "Output YAMY should be LEFT (0xE04B)";
    EXPECT_TRUE(result2.valid) << "M00+H result should be valid";
}

/**
 * Test 3: Release CapsLock → should deactivate M00
 * Verifies that modifier state cleanup works correctly
 */
TEST_F(M00VirtualModifierTest, ReleaseCapsLockShouldDeactivateM00) {
    // Press and hold CapsLock
    processor->processEvent(58, EventType::PRESS, modState.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Press H → should output LEFT
    auto result1 = processor->processEvent(35, EventType::PRESS, modState.get());
    EXPECT_EQ(result1.output_evdev, 105) << "M00+H should output LEFT";

    // Release CapsLock
    auto result2 = processor->processEvent(58, EventType::RELEASE, modState.get());
    EXPECT_EQ(result2.output_evdev, 0) << "CapsLock release should be suppressed";

    // Press H again (M00 should be deactivated now)
    auto result3 = processor->processEvent(35, EventType::PRESS, modState.get());

    // H should pass through as H (evdev 35), not LEFT
    EXPECT_EQ(result3.output_evdev, 35)
        << "After M00 deactivation, H should pass through unchanged";
}

/**
 * Test 4: Press H alone → should pass through unchanged
 * Verifies that keys work normally when M00 is not active
 */
TEST_F(M00VirtualModifierTest, HShouldPassThroughWithoutM00) {
    // Press H without activating M00
    auto result = processor->processEvent(35, EventType::PRESS, modState.get());

    // H should pass through as H (evdev 35)
    EXPECT_EQ(result.output_evdev, 35) << "H should pass through as evdev 35";
    EXPECT_EQ(result.output_yamy, 0x23) << "H YAMY code should be 0x23";
    EXPECT_TRUE(result.valid) << "H passthrough should be valid";
}

/**
 * Test 5: Tap CapsLock <200ms → should output Escape
 * CRITICAL: Verifies hold-vs-tap detection works correctly
 */
TEST_F(M00VirtualModifierTest, TapCapsLockShouldOutputEscape) {
    // Press CapsLock
    auto result1 = processor->processEvent(58, EventType::PRESS, modState.get());
    EXPECT_EQ(result1.output_evdev, 0) << "CapsLock press should be suppressed";

    // Release quickly (before 200ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto result2 = processor->processEvent(58, EventType::RELEASE, modState.get());

    // CRITICAL: Should output Escape (evdev 1)
    // The is_tap flag should be set, and output should be Escape
    EXPECT_TRUE(result2.is_tap)
        << "CRITICAL BUG: TAP not detected! "
        << "Quick CapsLock tap should set is_tap flag.";

    EXPECT_EQ(result2.output_evdev, 1)
        << "CRITICAL BUG: CapsLock tap should output Escape (evdev 1)! "
        << "Got evdev " << result2.output_evdev << " instead.";

    EXPECT_EQ(result2.output_yamy, 0x01) << "Tap output YAMY should be Escape (0x01)";
}

/**
 * Test 6: Multiple M00 activations in sequence
 * Verifies that M00 can be activated multiple times correctly
 */
TEST_F(M00VirtualModifierTest, MultipleM00ActivationsWork) {
    // First activation
    processor->processEvent(58, EventType::PRESS, modState.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto result1 = processor->processEvent(35, EventType::PRESS, modState.get());
    EXPECT_EQ(result1.output_evdev, 105) << "First M00+H should output LEFT";
    processor->processEvent(35, EventType::RELEASE, modState.get());
    processor->processEvent(58, EventType::RELEASE, modState.get());

    // Second activation
    processor->processEvent(58, EventType::PRESS, modState.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto result2 = processor->processEvent(35, EventType::PRESS, modState.get());
    EXPECT_EQ(result2.output_evdev, 105) << "Second M00+H should output LEFT";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
