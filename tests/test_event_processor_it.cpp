//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_event_processor_it.cpp - Integration tests for EventProcessor
//
// Tests complete Layer 1→2→3 composition with real substitution table:
// - W→A: evdev 17 → 0x0011 → 0x001E → evdev 30
// - N→LShift: evdev 49 → 0x0031 → VK_LSHIFT → evdev 42
// - Event type preservation: PRESS in → PRESS out, RELEASE in → RELEASE out
// - Passthrough for unmapped keys
//
// Uses real EventProcessor with actual substitution table from config_clean.mayu
// to verify end-to-end event transformation pipeline.
//
// Part of task 3.4 in key-remapping-consistency spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include "../../src/platform/linux/keycode_mapping.h"
#include "../../src/core/engine/engine_event_processor.h"

namespace yamy::test {

//=============================================================================
// Integration Test Fixture
//=============================================================================

class EventProcessorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set US layout for consistent testing
        yamy::platform::setLayoutOverride("us");

        // Create substitution table matching config_clean.mayu
        // This is a REAL substitution table, not a mock
        createRealSubstitutionTable();

        // Create EventProcessor with real substitution table
        processor = new yamy::EventProcessor(substitutions);

        // Disable debug logging for cleaner test output
        processor->setDebugLogging(false);
    }

    void TearDown() override {
        delete processor;
        yamy::platform::clearLayoutOverride();
    }

    void createRealSubstitutionTable() {
        // Letter substitutions from config_clean.mayu
        // Format: *X = *Y means: X scan code → Y scan code

        // W→A: 0x0011 → 0x001E
        substitutions[0x0011] = 0x001E;

        // R→E: 0x0013 → 0x0012
        substitutions[0x0013] = 0x0012;

        // T→U: 0x0014 → 0x0016
        substitutions[0x0014] = 0x0016;

        // N→LShift: 0x0031 → 0x002A (LShift scan code)
        // NOTE: config_clean.mayu uses *LShift which is the scan code 0x002A
        substitutions[0x0031] = 0x002A;

        // D→Q: 0x0020 → 0x0010
        substitutions[0x0020] = 0x0010;

        // E→O: 0x0012 → 0x0018
        substitutions[0x0012] = 0x0018;

        // A→Tab: 0x001E → 0x000F
        substitutions[0x001E] = 0x000F;

        // B→Enter: 0x0030 → 0x001C
        substitutions[0x0030] = 0x001C;

        // Number substitutions
        // 1→LShift: 0x0002 → 0x002A
        substitutions[0x0002] = 0x002A;

        // 0→R: 0x000B → 0x0013
        substitutions[0x000B] = 0x0013;

        // Z→Z (identity/passthrough - technically not needed in table)
        // substitutions[0x002C] = 0x002C;  // Not needed, passthrough is default
    }

    yamy::SubstitutionTable substitutions;
    yamy::EventProcessor* processor;
};

//=============================================================================
// Integration Tests: Complete Layer 1→2→3 Flow
//=============================================================================

// Test: W→A complete transformation
// Layer 1: evdev 17 (KEY_W) → 0x0011 (W scan)
// Layer 2: 0x0011 → 0x001E (W→A substitution)
// Layer 3: 0x001E → evdev 30 (KEY_A)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_W_to_A) {
    auto result = processor->processEvent(KEY_W, yamy::EventType::PRESS);

    // Verify complete transformation
    EXPECT_EQ(result.output_evdev, KEY_A) << "Output should be KEY_A (evdev 30)";
    EXPECT_EQ(result.output_yamy, 0x001E) << "After Layer 2, should be A scan code";
    EXPECT_TRUE(result.valid) << "Event should be valid";
    EXPECT_EQ(result.type, yamy::EventType::PRESS) << "Event type should be preserved";
}

// Test: N→LShift complete transformation (CRITICAL - previously broken)
// Layer 1: evdev 49 (KEY_N) → 0x0031 (N scan)
// Layer 2: 0x0031 → 0x002A (N→LShift substitution)
// Layer 3: 0x002A → evdev 42 (KEY_LEFTSHIFT)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_N_to_LShift) {
    auto result = processor->processEvent(KEY_N, yamy::EventType::PRESS);

    // Verify modifier substitution works identically to regular keys
    EXPECT_EQ(result.output_evdev, KEY_LEFTSHIFT) << "Output should be KEY_LEFTSHIFT (evdev 42)";
    EXPECT_EQ(result.output_yamy, 0x002A) << "After Layer 2, should be LShift scan code";
    EXPECT_TRUE(result.valid) << "Event should be valid";
    EXPECT_EQ(result.type, yamy::EventType::PRESS) << "Event type should be preserved";
}

// Test: R→E complete transformation (previously partial - only worked on RELEASE)
// Layer 1: evdev 19 (KEY_R) → 0x0013 (R scan)
// Layer 2: 0x0013 → 0x0012 (R→E substitution)
// Layer 3: 0x0012 → evdev 18 (KEY_E)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_R_to_E) {
    auto result = processor->processEvent(KEY_R, yamy::EventType::PRESS);

    // This test verifies the fix for "only works on RELEASE" bug
    EXPECT_EQ(result.output_evdev, KEY_E) << "Output should be KEY_E (evdev 18)";
    EXPECT_EQ(result.output_yamy, 0x0012) << "After Layer 2, should be E scan code";
    EXPECT_TRUE(result.valid) << "Event should be valid";
    EXPECT_EQ(result.type, yamy::EventType::PRESS) << "Event type should be preserved";
}

// Test: T→U complete transformation (previously partial - only worked on RELEASE)
// Layer 1: evdev 20 (KEY_T) → 0x0014 (T scan)
// Layer 2: 0x0014 → 0x0016 (T→U substitution)
// Layer 3: 0x0016 → evdev 22 (KEY_U)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_T_to_U) {
    auto result = processor->processEvent(KEY_T, yamy::EventType::PRESS);

    // This test verifies the fix for "only works on RELEASE" bug
    // Also verifies Layer 3 scan map priority fix (0x0014 → KEY_T not KEY_CAPSLOCK)
    EXPECT_EQ(result.output_evdev, KEY_U) << "Output should be KEY_U (evdev 22)";
    EXPECT_EQ(result.output_yamy, 0x0016) << "After Layer 2, should be U scan code";
    EXPECT_TRUE(result.valid) << "Event should be valid";
    EXPECT_EQ(result.type, yamy::EventType::PRESS) << "Event type should be preserved";
}

// Test: D→Q complete transformation
// Layer 1: evdev 32 (KEY_D) → 0x0020 (D scan)
// Layer 2: 0x0020 → 0x0010 (D→Q substitution)
// Layer 3: 0x0010 → evdev 16 (KEY_Q)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_D_to_Q) {
    auto result = processor->processEvent(KEY_D, yamy::EventType::PRESS);

    EXPECT_EQ(result.output_evdev, KEY_Q) << "Output should be KEY_Q (evdev 16)";
    EXPECT_EQ(result.output_yamy, 0x0010) << "After Layer 2, should be Q scan code";
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

// Test: Number key substitution (1→LShift)
// Layer 1: evdev 2 (KEY_1) → 0x0002 (1 scan)
// Layer 2: 0x0002 → 0x002A (1→LShift substitution)
// Layer 3: 0x002A → evdev 42 (KEY_LEFTSHIFT)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_1_to_LShift) {
    auto result = processor->processEvent(KEY_1, yamy::EventType::PRESS);

    EXPECT_EQ(result.output_evdev, KEY_LEFTSHIFT) << "Output should be KEY_LEFTSHIFT";
    EXPECT_EQ(result.output_yamy, 0x002A) << "After Layer 2, should be LShift scan code";
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

// Test: Number key substitution (0→R)
// Layer 1: evdev 11 (KEY_0) → 0x000B (0 scan)
// Layer 2: 0x000B → 0x0013 (0→R substitution)
// Layer 3: 0x0013 → evdev 19 (KEY_R)
TEST_F(EventProcessorIntegrationTest, CompleteTransformation_0_to_R) {
    auto result = processor->processEvent(KEY_0, yamy::EventType::PRESS);

    EXPECT_EQ(result.output_evdev, KEY_R) << "Output should be KEY_R";
    EXPECT_EQ(result.output_yamy, 0x0013) << "After Layer 2, should be R scan code";
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

//=============================================================================
// Event Type Preservation Tests
//=============================================================================

// Test: PRESS event type is preserved through all layers
TEST_F(EventProcessorIntegrationTest, EventTypePreservation_PRESS) {
    // Test W→A with PRESS
    auto result = processor->processEvent(KEY_W, yamy::EventType::PRESS);

    EXPECT_EQ(result.type, yamy::EventType::PRESS) << "PRESS in → PRESS out";
    EXPECT_EQ(result.output_evdev, KEY_A);
    EXPECT_TRUE(result.valid);
}

// Test: RELEASE event type is preserved through all layers
TEST_F(EventProcessorIntegrationTest, EventTypePreservation_RELEASE) {
    // Test W→A with RELEASE
    auto result = processor->processEvent(KEY_W, yamy::EventType::RELEASE);

    EXPECT_EQ(result.type, yamy::EventType::RELEASE) << "RELEASE in → RELEASE out";
    EXPECT_EQ(result.output_evdev, KEY_A);
    EXPECT_TRUE(result.valid);
}

// Test: PRESS and RELEASE produce same output key, different event types
TEST_F(EventProcessorIntegrationTest, PressReleaseSameTransformation) {
    // Test W→A for both PRESS and RELEASE
    auto press_result = processor->processEvent(KEY_W, yamy::EventType::PRESS);
    auto release_result = processor->processEvent(KEY_W, yamy::EventType::RELEASE);

    // Both should produce KEY_A
    EXPECT_EQ(press_result.output_evdev, KEY_A);
    EXPECT_EQ(release_result.output_evdev, KEY_A);
    EXPECT_EQ(press_result.output_yamy, 0x001E);
    EXPECT_EQ(release_result.output_yamy, 0x001E);

    // But event types should differ
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);

    // Both should be valid
    EXPECT_TRUE(press_result.valid);
    EXPECT_TRUE(release_result.valid);
}

// Test: Modifier substitution works for both PRESS and RELEASE (CRITICAL)
// This verifies the fix for "N→LShift only works on RELEASE"
TEST_F(EventProcessorIntegrationTest, ModifierSubstitution_PRESS_and_RELEASE) {
    // Test N→LShift for PRESS
    auto press_result = processor->processEvent(KEY_N, yamy::EventType::PRESS);
    EXPECT_EQ(press_result.output_evdev, KEY_LEFTSHIFT);
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);
    EXPECT_TRUE(press_result.valid);

    // Test N→LShift for RELEASE
    auto release_result = processor->processEvent(KEY_N, yamy::EventType::RELEASE);
    EXPECT_EQ(release_result.output_evdev, KEY_LEFTSHIFT);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);
    EXPECT_TRUE(release_result.valid);

    // This test proves modifier substitutions work IDENTICALLY to regular substitutions
    // No special-case code, no RELEASE-only behavior
}

// Test: R→E works for both PRESS and RELEASE (previously only RELEASE)
TEST_F(EventProcessorIntegrationTest, PreviouslyPartialKey_R_PRESS_and_RELEASE) {
    // Test R→E for PRESS (this was broken before)
    auto press_result = processor->processEvent(KEY_R, yamy::EventType::PRESS);
    EXPECT_EQ(press_result.output_evdev, KEY_E);
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);
    EXPECT_TRUE(press_result.valid);

    // Test R→E for RELEASE (this worked before)
    auto release_result = processor->processEvent(KEY_R, yamy::EventType::RELEASE);
    EXPECT_EQ(release_result.output_evdev, KEY_E);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);
    EXPECT_TRUE(release_result.valid);
}

// Test: T→U works for both PRESS and RELEASE (previously only RELEASE)
TEST_F(EventProcessorIntegrationTest, PreviouslyPartialKey_T_PRESS_and_RELEASE) {
    // Test T→U for PRESS (this was broken before)
    auto press_result = processor->processEvent(KEY_T, yamy::EventType::PRESS);
    EXPECT_EQ(press_result.output_evdev, KEY_U);
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);
    EXPECT_TRUE(press_result.valid);

    // Test T→U for RELEASE (this worked before)
    auto release_result = processor->processEvent(KEY_T, yamy::EventType::RELEASE);
    EXPECT_EQ(release_result.output_evdev, KEY_U);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);
    EXPECT_TRUE(release_result.valid);
}

//=============================================================================
// Passthrough Tests (unmapped keys)
//=============================================================================

// Test: Unmapped key passes through unchanged
// S has no substitution in our table
// Layer 1: evdev 31 (KEY_S) → 0x001F (S scan)
// Layer 2: 0x001F → 0x001F (passthrough, no substitution)
// Layer 3: 0x001F → evdev 31 (KEY_S)
TEST_F(EventProcessorIntegrationTest, PassthroughUnmappedKey) {
    auto result = processor->processEvent(KEY_S, yamy::EventType::PRESS);

    EXPECT_EQ(result.output_evdev, KEY_S) << "Output should be KEY_S (unchanged)";
    EXPECT_EQ(result.output_yamy, 0x001F) << "YAMY code should be S scan (unchanged)";
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, yamy::EventType::PRESS);
}

// Test: Multiple unmapped keys passthrough
TEST_F(EventProcessorIntegrationTest, MultiplePassthroughKeys) {
    // Test F (no substitution)
    auto result_f = processor->processEvent(KEY_F, yamy::EventType::PRESS);
    EXPECT_EQ(result_f.output_evdev, KEY_F);
    EXPECT_EQ(result_f.output_yamy, 0x0021); // F scan code
    EXPECT_TRUE(result_f.valid);

    // Test G (no substitution)
    auto result_g = processor->processEvent(KEY_G, yamy::EventType::PRESS);
    EXPECT_EQ(result_g.output_evdev, KEY_G);
    EXPECT_EQ(result_g.output_yamy, 0x0022); // G scan code
    EXPECT_TRUE(result_g.valid);

    // Test H (no substitution)
    auto result_h = processor->processEvent(KEY_H, yamy::EventType::PRESS);
    EXPECT_EQ(result_h.output_evdev, KEY_H);
    EXPECT_EQ(result_h.output_yamy, 0x0023); // H scan code
    EXPECT_TRUE(result_h.valid);
}

// Test: Passthrough preserves event type
TEST_F(EventProcessorIntegrationTest, PassthroughPreservesEventType) {
    // Test S passthrough for PRESS
    auto press_result = processor->processEvent(KEY_S, yamy::EventType::PRESS);
    EXPECT_EQ(press_result.output_evdev, KEY_S);
    EXPECT_EQ(press_result.type, yamy::EventType::PRESS);

    // Test S passthrough for RELEASE
    auto release_result = processor->processEvent(KEY_S, yamy::EventType::RELEASE);
    EXPECT_EQ(release_result.output_evdev, KEY_S);
    EXPECT_EQ(release_result.type, yamy::EventType::RELEASE);
}

// Test: Unmapped evdev code (not in Layer 1 map)
// Should return invalid event
TEST_F(EventProcessorIntegrationTest, CompletelyUnmappedEvdevCode) {
    // Use a reserved/unmapped evdev code
    auto result = processor->processEvent(999, yamy::EventType::PRESS);

    // Layer 1 fails, so entire event is invalid
    EXPECT_FALSE(result.valid) << "Unmapped evdev code should produce invalid event";
    EXPECT_EQ(result.output_evdev, 0);
    EXPECT_EQ(result.output_yamy, 0);
}

//=============================================================================
// Special Cases and Edge Cases
//=============================================================================

// Test: Special key transformations (A→Tab, B→Enter)
TEST_F(EventProcessorIntegrationTest, SpecialKeyTransformations) {
    // A→Tab
    auto result_a = processor->processEvent(KEY_A, yamy::EventType::PRESS);
    EXPECT_EQ(result_a.output_evdev, KEY_TAB);
    EXPECT_EQ(result_a.output_yamy, 0x000F); // Tab scan code
    EXPECT_TRUE(result_a.valid);

    // B→Enter
    auto result_b = processor->processEvent(KEY_B, yamy::EventType::PRESS);
    EXPECT_EQ(result_b.output_evdev, KEY_ENTER);
    EXPECT_EQ(result_b.output_yamy, 0x001C); // Enter scan code
    EXPECT_TRUE(result_b.valid);
}

// Test: Substitution chain doesn't happen (E→O, but O doesn't substitute to T)
// This verifies Layer 2 does ONE lookup only, not recursive
TEST_F(EventProcessorIntegrationTest, NoRecursiveSubstitution) {
    // E→O: 0x0012 → 0x0018
    // Our table has E→O, but O is not in the table as a source key
    // (or if it is, we test that we don't apply O's substitution)

    auto result = processor->processEvent(KEY_E, yamy::EventType::PRESS);

    // Should get O, not whatever O might map to
    EXPECT_EQ(result.output_evdev, KEY_O);
    EXPECT_EQ(result.output_yamy, 0x0018); // O scan code
    EXPECT_TRUE(result.valid);
}

// Test: Layer 3 scan map priority (critical for T→U fix)
// When T→U substitution happens: 0x0014 (T) → 0x0016 (U)
// Layer 3 must output KEY_U (evdev 22), not misinterpret 0x0016
TEST_F(EventProcessorIntegrationTest, Layer3ScanMapPriority) {
    // Process T→U
    auto result = processor->processEvent(KEY_T, yamy::EventType::PRESS);

    // Verify we get U (evdev 22), not any VK code conflict
    EXPECT_EQ(result.output_evdev, KEY_U);
    EXPECT_EQ(result.output_evdev, 22);
    EXPECT_NE(result.output_evdev, KEY_CAPSLOCK); // Should NOT be CAPSLOCK

    // This verifies Layer 3 yamyToEvdevKeyCode checks scan map BEFORE VK map
}

// Test: Modifier key output is correct (N→LShift)
TEST_F(EventProcessorIntegrationTest, ModifierKeyOutput) {
    auto result = processor->processEvent(KEY_N, yamy::EventType::PRESS);

    // Verify output is LEFTSHIFT
    EXPECT_EQ(result.output_evdev, KEY_LEFTSHIFT);
    EXPECT_EQ(result.output_evdev, 42); // evdev code for LEFTSHIFT
    EXPECT_EQ(result.output_yamy, 0x002A); // LShift scan code

    // This verifies the entire N→LShift pipeline works correctly
}

// Test: Multiple event sequences (simulating real key presses)
TEST_F(EventProcessorIntegrationTest, MultipleEventSequence) {
    // Simulate pressing W (should become A)
    auto w_press = processor->processEvent(KEY_W, yamy::EventType::PRESS);
    EXPECT_EQ(w_press.output_evdev, KEY_A);
    EXPECT_EQ(w_press.type, yamy::EventType::PRESS);

    // Simulate releasing W (should become A release)
    auto w_release = processor->processEvent(KEY_W, yamy::EventType::RELEASE);
    EXPECT_EQ(w_release.output_evdev, KEY_A);
    EXPECT_EQ(w_release.type, yamy::EventType::RELEASE);

    // Simulate pressing N (should become LShift)
    auto n_press = processor->processEvent(KEY_N, yamy::EventType::PRESS);
    EXPECT_EQ(n_press.output_evdev, KEY_LEFTSHIFT);
    EXPECT_EQ(n_press.type, yamy::EventType::PRESS);

    // Simulate releasing N (should become LShift release)
    auto n_release = processor->processEvent(KEY_N, yamy::EventType::RELEASE);
    EXPECT_EQ(n_release.output_evdev, KEY_LEFTSHIFT);
    EXPECT_EQ(n_release.type, yamy::EventType::RELEASE);

    // All events should be valid
    EXPECT_TRUE(w_press.valid);
    EXPECT_TRUE(w_release.valid);
    EXPECT_TRUE(n_press.valid);
    EXPECT_TRUE(n_release.valid);
}

// Test: REPEAT event type (if supported)
TEST_F(EventProcessorIntegrationTest, RepeatEventType) {
    // Test W→A with REPEAT
    auto result = processor->processEvent(KEY_W, yamy::EventType::REPEAT);

    EXPECT_EQ(result.output_evdev, KEY_A);
    EXPECT_EQ(result.type, yamy::EventType::REPEAT) << "REPEAT in → REPEAT out";
    EXPECT_TRUE(result.valid);
}

} // namespace yamy::test

//=============================================================================
// Main entry point
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
