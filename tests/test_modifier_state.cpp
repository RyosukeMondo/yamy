//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_modifier_state.cpp - Unit tests for ModifierState modal modifier support
//
// Tests ModifierState class for modal modifier functionality (mod0-mod19):
// - activate/deactivate methods for modal modifiers
// - isActive query method
// - Bitmask manipulation (concurrent modifiers)
// - Edge cases (activate twice, deactivate inactive, etc.)
//
// Part of task 1 in modal-modifier-remapping spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include "../src/core/input/modifier_state.h"
#include "../src/core/input/keyboard.h"

namespace yamy::test {

using namespace yamy::input;

//=============================================================================
// ModifierState Modal Modifier Tests
//=============================================================================

class ModifierStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        state = std::make_unique<ModifierState>();
    }

    void TearDown() override {
        state.reset();
    }

    std::unique_ptr<ModifierState> state;
};

//=============================================================================
// Activation Tests
//=============================================================================

TEST_F(ModifierStateTest, ActivateSingleModifier_Mod0) {
    // Test activating mod0
    state->activate(Modifier::Type_Mod0);

    EXPECT_TRUE(state->isActive(Modifier::Type_Mod0));
    EXPECT_EQ(state->getActiveBitmask(), 1u << 0);
}

TEST_F(ModifierStateTest, ActivateSingleModifier_Mod9) {
    // Test activating mod9
    state->activate(Modifier::Type_Mod9);

    EXPECT_TRUE(state->isActive(Modifier::Type_Mod9));
    EXPECT_EQ(state->getActiveBitmask(), 1u << 9);
}

TEST_F(ModifierStateTest, ActivateSingleModifier_Mod19) {
    // Test activating mod19 (highest modal modifier)
    state->activate(Modifier::Type_Mod19);

    EXPECT_TRUE(state->isActive(Modifier::Type_Mod19));
    EXPECT_EQ(state->getActiveBitmask(), 1u << 19);
}

//=============================================================================
// Deactivation Tests
//=============================================================================

TEST_F(ModifierStateTest, DeactivateSingleModifier) {
    // Activate mod5, then deactivate it
    state->activate(Modifier::Type_Mod5);
    EXPECT_TRUE(state->isActive(Modifier::Type_Mod5));

    state->deactivate(Modifier::Type_Mod5);
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod5));
    EXPECT_EQ(state->getActiveBitmask(), 0u);
}

TEST_F(ModifierStateTest, DeactivateInactive_SafeOperation) {
    // Deactivating an inactive modifier should be safe (no-op)
    state->deactivate(Modifier::Type_Mod7);

    EXPECT_FALSE(state->isActive(Modifier::Type_Mod7));
    EXPECT_EQ(state->getActiveBitmask(), 0u);
}

//=============================================================================
// Multiple Modifiers Tests
//=============================================================================

TEST_F(ModifierStateTest, ActivateMultipleConcurrent) {
    // Activate mod0, mod9, and mod19 simultaneously
    state->activate(Modifier::Type_Mod0);
    state->activate(Modifier::Type_Mod9);
    state->activate(Modifier::Type_Mod19);

    EXPECT_TRUE(state->isActive(Modifier::Type_Mod0));
    EXPECT_TRUE(state->isActive(Modifier::Type_Mod9));
    EXPECT_TRUE(state->isActive(Modifier::Type_Mod19));

    uint32_t expected = (1u << 0) | (1u << 9) | (1u << 19);
    EXPECT_EQ(state->getActiveBitmask(), expected);
}

TEST_F(ModifierStateTest, All20ModifiersConcurrent_StressTest) {
    // Activate all 20 modal modifiers simultaneously
    for (int i = 0; i < 20; ++i) {
        Modifier::Type type = static_cast<Modifier::Type>(Modifier::Type_Mod0 + i);
        state->activate(type);
    }

    // Verify all are active
    for (int i = 0; i < 20; ++i) {
        Modifier::Type type = static_cast<Modifier::Type>(Modifier::Type_Mod0 + i);
        EXPECT_TRUE(state->isActive(type));
    }

    // Bitmask should have bits 0-19 set
    uint32_t expected = (1u << 20) - 1;  // 0x000FFFFF
    EXPECT_EQ(state->getActiveBitmask(), expected);
}

//=============================================================================
// Edge Cases
//=============================================================================

TEST_F(ModifierStateTest, ActivateTwice_Idempotent) {
    // Activating the same modifier twice should be idempotent
    state->activate(Modifier::Type_Mod3);
    state->activate(Modifier::Type_Mod3);

    EXPECT_TRUE(state->isActive(Modifier::Type_Mod3));
    EXPECT_EQ(state->getActiveBitmask(), 1u << 3);
}

TEST_F(ModifierStateTest, IsActiveReturnsFalseForInactive) {
    // Initially, all modifiers should be inactive
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod0));
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod9));
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod19));
}

TEST_F(ModifierStateTest, GetActiveBitmaskInitiallyZero) {
    // Initially, bitmask should be zero
    EXPECT_EQ(state->getActiveBitmask(), 0u);
}

TEST_F(ModifierStateTest, ClearResetsAllModifiers) {
    // Activate several modifiers
    state->activate(Modifier::Type_Mod0);
    state->activate(Modifier::Type_Mod9);
    state->activate(Modifier::Type_Mod19);

    // Clear should reset all
    state->clear();

    EXPECT_FALSE(state->isActive(Modifier::Type_Mod0));
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod9));
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod19));
    EXPECT_EQ(state->getActiveBitmask(), 0u);
}

//=============================================================================
// Interaction with Standard Modifiers
//=============================================================================

TEST_F(ModifierStateTest, StandardAndModalCombined) {
    // Test that modal modifiers work independently of standard modifiers
    // (Standard modifiers are tracked in m_flags, modal in m_modal)

    state->activate(Modifier::Type_Mod9);

    // Modal modifier should be active
    EXPECT_TRUE(state->isActive(Modifier::Type_Mod9));
    EXPECT_EQ(state->getActiveBitmask(), 1u << 9);

    // Standard modifiers should still be accessible (separate tracking)
    // Note: We can't set standard modifiers via activate() in current impl,
    // but we can verify modal doesn't interfere
    EXPECT_FALSE(state->isShiftPressed());
    EXPECT_FALSE(state->isCtrlPressed());
}

TEST_F(ModifierStateTest, IsActive_StandardModifiersSupported) {
    // Test that isActive() handles standard modifiers gracefully
    // (Returns false since we haven't pressed them)
    EXPECT_FALSE(state->isActive(Modifier::Type_Shift));
    EXPECT_FALSE(state->isActive(Modifier::Type_Control));
    EXPECT_FALSE(state->isActive(Modifier::Type_Alt));
    EXPECT_FALSE(state->isActive(Modifier::Type_Windows));
}

//=============================================================================
// Bitmask Manipulation
//=============================================================================

TEST_F(ModifierStateTest, BitmaskCorrectness_SingleBit) {
    // Verify bitmask has exactly one bit set for single activation
    state->activate(Modifier::Type_Mod7);

    uint32_t bitmask = state->getActiveBitmask();

    // Count set bits (should be exactly 1)
    int count = 0;
    for (int i = 0; i < 32; ++i) {
        if (bitmask & (1u << i)) count++;
    }

    EXPECT_EQ(count, 1);
    EXPECT_EQ(bitmask, 1u << 7);
}

TEST_F(ModifierStateTest, BitmaskCorrectness_MultipleBits) {
    // Verify bitmask has correct bits set for multiple activations
    state->activate(Modifier::Type_Mod2);
    state->activate(Modifier::Type_Mod5);
    state->activate(Modifier::Type_Mod11);

    uint32_t bitmask = state->getActiveBitmask();

    // Count set bits (should be exactly 3)
    int count = 0;
    for (int i = 0; i < 32; ++i) {
        if (bitmask & (1u << i)) count++;
    }

    EXPECT_EQ(count, 3);

    // Verify specific bits
    EXPECT_TRUE(bitmask & (1u << 2));
    EXPECT_TRUE(bitmask & (1u << 5));
    EXPECT_TRUE(bitmask & (1u << 11));
}

TEST_F(ModifierStateTest, PartialDeactivation) {
    // Activate multiple modifiers, then deactivate some
    state->activate(Modifier::Type_Mod1);
    state->activate(Modifier::Type_Mod4);
    state->activate(Modifier::Type_Mod8);

    // Deactivate mod4
    state->deactivate(Modifier::Type_Mod4);

    // mod1 and mod8 should still be active
    EXPECT_TRUE(state->isActive(Modifier::Type_Mod1));
    EXPECT_FALSE(state->isActive(Modifier::Type_Mod4));
    EXPECT_TRUE(state->isActive(Modifier::Type_Mod8));

    uint32_t expected = (1u << 1) | (1u << 8);
    EXPECT_EQ(state->getActiveBitmask(), expected);
}

//=============================================================================
// toModifier() Tests
//=============================================================================

TEST_F(ModifierStateTest, ToModifier_IncludesModalModifiers) {
    // Activate some modal modifiers
    state->activate(Modifier::Type_Mod0);
    state->activate(Modifier::Type_Mod9);
    state->activate(Modifier::Type_Mod19);

    // Convert to Modifier object
    Modifier mod = state->toModifier();

    // Verify modal modifiers are included
    EXPECT_TRUE(mod.isPressed(Modifier::Type_Mod0));
    EXPECT_TRUE(mod.isPressed(Modifier::Type_Mod9));
    EXPECT_TRUE(mod.isPressed(Modifier::Type_Mod19));

    // Verify other modal modifiers are not active
    EXPECT_FALSE(mod.isPressed(Modifier::Type_Mod1));
    EXPECT_FALSE(mod.isPressed(Modifier::Type_Mod5));
}

TEST_F(ModifierStateTest, ToModifier_CombinesStandardAndModal) {
    // This test verifies that toModifier() includes both standard and modal modifiers
    // Note: We can't easily set standard modifiers without a real key event,
    // so we just verify modal modifiers are included

    state->activate(Modifier::Type_Mod3);
    state->activate(Modifier::Type_Mod7);

    Modifier mod = state->toModifier();

    EXPECT_TRUE(mod.isPressed(Modifier::Type_Mod3));
    EXPECT_TRUE(mod.isPressed(Modifier::Type_Mod7));
}

TEST_F(ModifierStateTest, ToModifier_AllModalModifiersActive) {
    // Activate all 20 modal modifiers
    for (int i = 0; i < 20; ++i) {
        Modifier::Type modType = static_cast<Modifier::Type>(Modifier::Type_Mod0 + i);
        state->activate(modType);
    }

    // Convert to Modifier object
    Modifier mod = state->toModifier();

    // Verify all modal modifiers are included
    for (int i = 0; i < 20; ++i) {
        Modifier::Type modType = static_cast<Modifier::Type>(Modifier::Type_Mod0 + i);
        EXPECT_TRUE(mod.isPressed(modType)) << "Mod" << i << " should be active";
    }
}

} // namespace yamy::test
