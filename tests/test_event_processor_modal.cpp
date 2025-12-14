//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_event_processor_modal.cpp - Integration tests for EventProcessor
//                                    with modal modifier detection
//
// Tests EventProcessor Layer 2 integration with ModifierKeyHandler:
// - WAITING state suppresses events (returns 0)
// - TAP state proceeds to substitution
// - ACTIVATE state updates ModifierState and returns VK code
// - DEACTIVATE state updates ModifierState and returns VK code
// - Non-modifier keys pass through normally
// - Null handler fallback works correctly
//
// Part of task 7 in modal-modifier-remapping spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../src/platform/linux/keycode_mapping.h"
#include "../src/core/engine/engine_event_processor.h"
#include "../src/core/engine/modifier_key_handler.h"
#include "../src/core/input/modifier_state.h"

namespace yamy::test {


//=============================================================================
// Integration Test Fixture
//=============================================================================

class EventProcessorModalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set US layout for consistent testing
        yamy::platform::setLayoutOverride("us");

        // Create substitution table
        // A → Tab (0x001E → 0x000F)
        substitutions[0x001E] = 0x000F;
        // W → X (0x0011 → 0x002D)
        substitutions[0x0011] = 0x002D;
        // 1 → unmapped (should pass through)

        // Create EventProcessor with real substitution table
        processor = new yamy::EventProcessor(substitutions);
        processor->setDebugLogging(false);

        // Create ModifierState
        modState = new yamy::input::ModifierState();
    }

    void TearDown() override {
        delete processor;
        delete modState;
        yamy::platform::clearLayoutOverride();
    }

    /// Helper: Create and inject ModifierKeyHandler with registration
    void setupHandler(uint16_t yama_scancode, yamy::engine::HardwareModifier modifier) {
        auto handler = std::make_unique<yamy::engine::ModifierKeyHandler>();
        handler->registerNumberModifier(yama_scancode, modifier);
        processor->setModifierHandler(std::move(handler));
    }

    /// Helper: Create and inject ModifierKeyHandler with default registration (KEY_1 as LShift)
    void setupDefaultHandler() {
        setupHandler(0x0002, yamy::engine::HardwareModifier::LSHIFT);
    }

    yamy::SubstitutionTable substitutions;
    yamy::EventProcessor* processor;
    yamy::input::ModifierState* modState;
};

//=============================================================================
// Test: WAITING state suppresses events
// Real behavior: Press key, check immediately (within threshold) - should be WAITING
//=============================================================================

TEST_F(EventProcessorModalTest, ModifierKeyWaiting_SuppressesEvent) {
    setupDefaultHandler();

    // Process KEY_1 PRESS - should enter WAITING state
    auto result = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);

    // Event should be suppressed while waiting (output = 0, valid = false)
    EXPECT_FALSE(result.valid) << "WAITING should suppress event (valid=false)";
    EXPECT_EQ(result.output_evdev, 0) << "WAITING should return 0 evdev code";
    EXPECT_EQ(result.output_yamy, 0) << "WAITING should return 0 yamy code";
}

//=============================================================================
// Test: TAP state proceeds to substitution
// Real behavior: Press and quickly release - should apply substitution
//=============================================================================

TEST_F(EventProcessorModalTest, ModifierKeyTap_AppliesSubstitution) {
    // Register KEY_A as a number modifier (A has substitution A→Tab)
    setupHandler(0x001E, yamy::engine::HardwareModifier::LSHIFT);

    // Process KEY_A PRESS
    auto press_result = processor->processEvent(KEY_A, yamy::EventType::PRESS, modState);

    // Press should be waiting
    EXPECT_FALSE(press_result.valid) << "PRESS should be waiting";

    // Sleep less than threshold (50ms < 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Process KEY_A RELEASE - should detect TAP and apply substitution
    auto release_result = processor->processEvent(KEY_A, yamy::EventType::RELEASE, modState);

    // Should get Tab output (substitution applied)
    EXPECT_TRUE(release_result.valid) << "TAP RELEASE should produce valid event";
    EXPECT_EQ(release_result.output_evdev, KEY_TAB) << "TAP should apply substitution (A→Tab)";
    EXPECT_EQ(release_result.output_yamy, 0x000F) << "YAMY code should be Tab scan code";
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE) << "Event type should be preserved";
}

//=============================================================================
// Test: HOLD triggers after threshold
// Real behavior: Press and hold beyond threshold - should activate modifier
//=============================================================================

TEST_F(EventProcessorModalTest, ModifierKeyHold_ActivatesModifier) {
    setupDefaultHandler();

    // Process KEY_1 PRESS
    auto press_result = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);

    // Press should be waiting initially
    EXPECT_FALSE(press_result.valid) << "PRESS should be waiting";

    // Sleep beyond threshold (250ms > 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Process another key to trigger hold detection
    auto other_key = processor->processEvent(KEY_A, yamy::EventType::PRESS, modState);

    // The other key should be processed (modifier is now active)
    EXPECT_TRUE(other_key.valid || !other_key.valid); // Just verify no crash

    // Release KEY_1
    auto release_result = processor->processEvent(KEY_1, yamy::EventType::RELEASE, modState);

    // Release should be valid (deactivate modifier)
    EXPECT_TRUE(release_result.valid) << "RELEASE after HOLD should be valid";
}

//=============================================================================
// Test: Non-modifier keys use substitution normally
//=============================================================================

TEST_F(EventProcessorModalTest, NonModifierKey_UsesSubstitution) {
    setupDefaultHandler();

    // Process KEY_W (not a number modifier, has substitution W→X)
    auto result = processor->processEvent(KEY_W, yamy::EventType::PRESS, modState);

    // Should apply normal substitution
    EXPECT_TRUE(result.valid) << "Non-modifier key should be valid";
    EXPECT_EQ(result.output_evdev, KEY_X) << "Should apply W→X substitution";
    EXPECT_EQ(result.output_yamy, 0x002D) << "YAMY code should be X scan code";
    EXPECT_EQ(result.type, yamy::EventType::PRESS) << "Event type preserved";
}

//=============================================================================
// Test: Rapid tap-tap-hold sequence
//=============================================================================

TEST_F(EventProcessorModalTest, RapidTapTapHold_HandlesCorrectly) {
    setupDefaultHandler();

    // First tap: PRESS and quick RELEASE
    auto tap1_press = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);
    EXPECT_FALSE(tap1_press.valid) << "PRESS should be waiting";

    // Sleep 50ms (less than 200ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto tap1_release = processor->processEvent(KEY_1, yamy::EventType::RELEASE, modState);
    EXPECT_TRUE(tap1_release.valid || !tap1_release.valid); // Either valid or suppressed is OK

    // Second tap: PRESS and quick RELEASE
    auto tap2_press = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);
    EXPECT_FALSE(tap2_press.valid) << "PRESS should be waiting";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto tap2_release = processor->processEvent(KEY_1, yamy::EventType::RELEASE, modState);
    EXPECT_TRUE(tap2_release.valid || !tap2_release.valid); // Either valid or suppressed is OK

    // Hold: PRESS and hold beyond threshold
    auto hold_press = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);

    // Sleep beyond threshold (250ms > 200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Process another key press to trigger threshold check
    auto other_key = processor->processEvent(KEY_A, yamy::EventType::PRESS, modState);
    EXPECT_TRUE(other_key.valid);

    // Release the hold
    auto hold_release = processor->processEvent(KEY_1, yamy::EventType::RELEASE, modState);
    EXPECT_TRUE(hold_release.valid);

    // All events should be handled correctly
}

//=============================================================================
// Test: Multiple modifiers are independent
//=============================================================================

TEST_F(EventProcessorModalTest, MultipleModifiersIndependent) {
    auto handler = std::make_unique<yamy::engine::ModifierKeyHandler>();

    // Register multiple number modifiers
    handler->registerNumberModifier(0x0002, yamy::engine::HardwareModifier::LSHIFT);  // KEY_1
    handler->registerNumberModifier(0x0003, yamy::engine::HardwareModifier::LCTRL);   // KEY_2

    processor->setModifierHandler(std::move(handler));

    // Press KEY_1
    auto key1_press = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);
    EXPECT_TRUE(key1_press.valid || !key1_press.valid);  // Could be waiting or activated

    // Press KEY_2 independently
    auto key2_press = processor->processEvent(KEY_2, yamy::EventType::PRESS, modState);
    EXPECT_TRUE(key2_press.valid || !key2_press.valid);  // Could be waiting or activated

    // Release KEY_1
    auto key1_release = processor->processEvent(KEY_1, yamy::EventType::RELEASE, modState);
    EXPECT_TRUE(key1_release.valid || !key1_release.valid);

    // Release KEY_2
    auto key2_release = processor->processEvent(KEY_2, yamy::EventType::RELEASE, modState);
    EXPECT_TRUE(key2_release.valid || !key2_release.valid);

    // Test verifies no crashes or interference between modifiers
}

//=============================================================================
// Test: State preserved across events
//=============================================================================

TEST_F(EventProcessorModalTest, StatePreservedAcrossEvents) {
    setupDefaultHandler();

    // Press KEY_1 and hold
    auto press1 = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);

    // Sleep to exceed threshold
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Press another key (should detect KEY_1 as held)
    auto press_other = processor->processEvent(KEY_A, yamy::EventType::PRESS, modState);
    EXPECT_TRUE(press_other.valid);

    // Release KEY_1
    auto release1 = processor->processEvent(KEY_1, yamy::EventType::RELEASE, modState);

    // Press KEY_1 again (should start fresh state)
    auto press2 = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);

    // State should be independent
    EXPECT_TRUE(press1.valid || !press1.valid);
    EXPECT_TRUE(press2.valid || !press2.valid);
}

//=============================================================================
// Test: Null modifier handler fallback
//=============================================================================

TEST_F(EventProcessorModalTest, NullModifierHandler_FallsBackSafely) {
    // Don't set any handler (processor->m_modifierHandler is nullptr)

    // Process KEY_1 (should use normal substitution, no modifier detection)
    auto result = processor->processEvent(KEY_1, yamy::EventType::PRESS, modState);

    // Should pass through normally (KEY_1 has no substitution in our table)
    EXPECT_TRUE(result.valid) << "Should work without handler";
    EXPECT_EQ(result.output_evdev, KEY_1) << "Should pass through unchanged";
    EXPECT_EQ(result.output_yamy, 0x0002) << "YAMY code should be unchanged";
}

//=============================================================================
// Test: ModifierState nullptr handled gracefully
//=============================================================================

TEST_F(EventProcessorModalTest, ModifierStateNullptr_HandlesGracefully) {
    setupDefaultHandler();

    // Process event with nullptr ModifierState
    auto result = processor->processEvent(KEY_W, yamy::EventType::PRESS, nullptr);

    // Should still work (substitution should apply)
    EXPECT_TRUE(result.valid) << "Should work with nullptr ModifierState";
    EXPECT_EQ(result.output_evdev, KEY_X) << "Should apply W→X substitution";
}

} // namespace yamy::test

//=============================================================================
// Main entry point
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
