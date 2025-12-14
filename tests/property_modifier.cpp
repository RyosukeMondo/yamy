//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// property_modifier.cpp - Property-based tests for modifier state tracking
//
// Tests modifier state properties using RapidCheck to explore state space:
// 1. Key-down/key-up pairing: all key-down events have matching key-up
// 2. Modifier state consistency: state transitions are valid
// 3. No stuck keys: after all events processed, all keys can be released
//
// Part of Phase 5 (Property-Based Testing) in modern-cpp-toolchain spec
//
// This tests the ModifierState class that tracks standard modifiers (shift,
// ctrl, alt, win) and modal modifiers (mod0-mod19).
//
// Usage:
//   Run with default iterations (100): ./yamy_property_modifier_test
//   Run with 1000 iterations: RC_PARAMS="max_success=1000" ./yamy_property_modifier_test
//   Run with verbose output: RC_PARAMS="verbose_progress=1" ./yamy_property_modifier_test
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include "../src/core/input/modifier_state.h"
#include "../src/core/platform/types.h"
#include <vector>
#include <set>
#include <map>

using namespace yamy::input;
using namespace yamy::platform;

//=============================================================================
// Test Data Structures
//=============================================================================

// Represents a modifier key that can be tested
struct ModifierKey {
    uint32_t scanCode;      // Scan code for the key
    bool isExtended;        // Extended key flag (E0)
    ModifierFlag flag;      // Expected modifier flag
    std::string name;       // Name for debugging

    bool operator==(const ModifierKey& other) const {
        return scanCode == other.scanCode && isExtended == other.isExtended;
    }

    bool operator<(const ModifierKey& other) const {
        return scanCode < other.scanCode ||
               (scanCode == other.scanCode && isExtended < other.isExtended);
    }
};

// All testable modifier keys (Windows scancodes)
static const std::vector<ModifierKey> ALL_MODIFIER_KEYS = {
    // Shift keys
    {0x2A, false, MOD_LSHIFT, "LShift"},
    {0x36, false, MOD_RSHIFT, "RShift"},
    // Control keys
    {0x1D, false, MOD_LCTRL, "LCtrl"},
    {0x1D, true,  MOD_RCTRL, "RCtrl"},
    // Alt keys
    {0x38, false, MOD_LALT, "LAlt"},
    {0x38, true,  MOD_RALT, "RAlt"},
    // Windows keys
    {0x5B, true,  MOD_LWIN, "LWin"},
    {0x5C, true,  MOD_RWIN, "RWin"},
    // Lock keys
    {0x3A, false, MOD_CAPSLOCK, "CapsLock"},
    {0x45, false, MOD_NUMLOCK, "NumLock"},
    {0x46, false, MOD_SCROLLLOCK, "ScrollLock"},
};

// Input event for testing
struct TestEvent {
    ModifierKey key;
    bool isKeyDown;

    KeyEvent toKeyEvent() const {
        KeyEvent evt;
        evt.scanCode = key.scanCode;
        evt.isKeyDown = isKeyDown;
        evt.isExtended = key.isExtended;
        evt.timestamp = 0;
        evt.flags = 0;
        evt.extraInfo = 0;
        evt.key = KeyCode::Unknown;
        return evt;
    }
};

//=============================================================================
// RapidCheck Generators
//=============================================================================

namespace rc {

// Generate a random modifier key
template<>
struct Arbitrary<ModifierKey> {
    static Gen<ModifierKey> arbitrary() {
        return gen::elementOf(ALL_MODIFIER_KEYS);
    }
};

// Generate a random test event
template<>
struct Arbitrary<TestEvent> {
    static Gen<TestEvent> arbitrary() {
        return gen::build<TestEvent>(
            gen::set(&TestEvent::key, gen::arbitrary<ModifierKey>()),
            gen::set(&TestEvent::isKeyDown, gen::arbitrary<bool>())
        );
    }
};

} // namespace rc

//=============================================================================
// Helper Functions
//=============================================================================

// Apply a sequence of events to modifier state
void applyEvents(ModifierState& state, const std::vector<TestEvent>& events) {
    for (const auto& event : events) {
        KeyEvent ke = event.toKeyEvent();
        state.updateFromKeyEvent(ke);
    }
}

// Count how many times a key appears as pressed in event sequence
int countKeyDowns(const std::vector<TestEvent>& events, const ModifierKey& key) {
    int count = 0;
    for (const auto& event : events) {
        if (event.key == key && event.isKeyDown) {
            count++;
        }
    }
    return count;
}

// Count how many times a key appears as released in event sequence
int countKeyUps(const std::vector<TestEvent>& events, const ModifierKey& key) {
    int count = 0;
    for (const auto& event : events) {
        if (event.key == key && !event.isKeyDown) {
            count++;
        }
    }
    return count;
}

// Get the final expected state of a key after processing all events
bool getFinalKeyState(const std::vector<TestEvent>& events, const ModifierKey& key) {
    bool state = false;
    for (const auto& event : events) {
        if (event.key == key) {
            state = event.isKeyDown;
        }
    }
    return state;
}

//=============================================================================
// Property 1: Key-down/key-up Pairing
// All key-down events should have corresponding state tracking
//=============================================================================

TEST_CASE("ModifierState: Key events update state correctly",
          "[property][modifier][pairing]") {
    rc::check("each key event updates modifier state", []() {
        ModifierState state;

        // Generate a sequence of events
        const auto numEvents = *rc::gen::inRange(1, 20);
        std::vector<TestEvent> events;

        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }

        // Track expected final state for each unique key
        std::map<ModifierKey, bool> expectedState;
        for (const auto& event : events) {
            expectedState[event.key] = event.isKeyDown;
        }

        // Apply all events
        applyEvents(state, events);

        // Verify each key's final state matches expectations
        for (const auto& [key, expectedPressed] : expectedState) {
            bool actualPressed = (state.getFlags() & key.flag) != 0;
            RC_ASSERT(actualPressed == expectedPressed);
        }
    });
}

//=============================================================================
// Property 2: Modifier State Consistency
// State should be consistent with the sequence of events applied
//=============================================================================

TEST_CASE("ModifierState: State is consistent with event history",
          "[property][modifier][consistency]") {
    rc::check("final state matches last event for each key", []() {
        ModifierState state;

        // Generate random event sequence
        const auto numEvents = *rc::gen::inRange(0, 30);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }

        // Apply events
        applyEvents(state, events);

        // For each modifier key, verify final state
        for (const auto& modKey : ALL_MODIFIER_KEYS) {
            bool expectedPressed = getFinalKeyState(events, modKey);
            bool actualPressed = (state.getFlags() & modKey.flag) != 0;

            RC_ASSERT(actualPressed == expectedPressed);
        }
    });
}

TEST_CASE("ModifierState: Reset clears all flags",
          "[property][modifier][consistency]") {
    rc::check("reset() clears all modifier state", []() {
        ModifierState state;

        // Generate and apply random events
        const auto numEvents = *rc::gen::inRange(1, 20);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }
        applyEvents(state, events);

        // Reset state
        state.reset();

        // Verify all flags are clear
        RC_ASSERT(state.getFlags() == MOD_NONE);
        RC_ASSERT(state.getActiveBitmask() == 0);
    });
}

//=============================================================================
// Property 3: No Stuck Keys
// After processing all events, state should be valid (no impossible states)
//=============================================================================

TEST_CASE("ModifierState: No stuck keys after event sequence",
          "[property][modifier][stuck-keys]") {
    rc::check("keys can be released after arbitrary event sequence", []() {
        ModifierState state;

        // Generate random event sequence
        const auto numEvents = *rc::gen::inRange(0, 30);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }

        // Apply events
        applyEvents(state, events);

        // Now release all modifier keys explicitly
        std::vector<TestEvent> releaseEvents;
        for (const auto& modKey : ALL_MODIFIER_KEYS) {
            TestEvent release;
            release.key = modKey;
            release.isKeyDown = false;
            releaseEvents.push_back(release);
        }

        // Apply all release events
        applyEvents(state, releaseEvents);

        // After releasing all keys, all standard modifiers should be clear
        // (except lock keys which toggle, not follow press/release)
        uint32_t flags = state.getFlags();

        // Shift, Ctrl, Alt, Win should all be released
        RC_ASSERT((flags & MODFLAG_SHIFT) == 0);
        RC_ASSERT((flags & MODFLAG_CTRL) == 0);
        RC_ASSERT((flags & MODFLAG_ALT) == 0);
        RC_ASSERT((flags & MODFLAG_WIN) == 0);

        // Lock keys may still be set (they toggle), so we don't check them
    });
}

//=============================================================================
// Property 4: Combined Modifier State
// Multiple modifiers can be pressed simultaneously
//=============================================================================

TEST_CASE("ModifierState: Multiple simultaneous modifiers",
          "[property][modifier][simultaneous]") {
    rc::check("multiple keys can be pressed simultaneously", []() {
        ModifierState state;

        // Generate 2-4 distinct modifier keys
        const auto numKeys = *rc::gen::inRange(2, 5);
        std::set<ModifierKey> selectedKeys;

        while (selectedKeys.size() < static_cast<size_t>(numKeys)) {
            auto key = *rc::gen::arbitrary<ModifierKey>();
            selectedKeys.insert(key);
        }

        // Press all selected keys
        for (const auto& key : selectedKeys) {
            TestEvent event;
            event.key = key;
            event.isKeyDown = true;
            KeyEvent ke = event.toKeyEvent();
            state.updateFromKeyEvent(ke);
        }

        // Verify all keys are pressed
        for (const auto& key : selectedKeys) {
            bool isPressed = (state.getFlags() & key.flag) != 0;
            RC_ASSERT(isPressed);
        }

        // Count number of bits set (excluding lock keys which toggle)
        uint32_t flags = state.getFlags();
        uint32_t nonLockFlags = flags & ~(MOD_CAPSLOCK | MOD_NUMLOCK | MOD_SCROLLLOCK);

        // At least the number of non-lock keys should be pressed
        int pressedCount = __builtin_popcount(nonLockFlags);

        // Count non-lock keys we pressed
        int expectedNonLock = 0;
        for (const auto& key : selectedKeys) {
            if (key.flag != MOD_CAPSLOCK &&
                key.flag != MOD_NUMLOCK &&
                key.flag != MOD_SCROLLLOCK) {
                expectedNonLock++;
            }
        }

        RC_ASSERT(pressedCount >= expectedNonLock);
    });
}

//=============================================================================
// Property 5: Modal Modifier State
// Modal modifiers (mod0-mod19) work independently from standard modifiers
//=============================================================================

TEST_CASE("ModifierState: Modal modifiers are independent",
          "[property][modifier][modal]") {
    rc::check("modal modifiers don't interfere with standard modifiers", []() {
        ModifierState state;

        // Press some standard modifiers
        const auto numStdEvents = *rc::gen::inRange(0, 5);
        std::vector<TestEvent> standardEvents;
        for (int i = 0; i < numStdEvents; ++i) {
            standardEvents.push_back(*rc::gen::arbitrary<TestEvent>());
        }
        applyEvents(state, standardEvents);
        uint32_t standardFlags = state.getFlags();

        // Activate/deactivate some modal modifiers
        const auto numModalOps = *rc::gen::inRange(0, 10);
        for (int i = 0; i < numModalOps; ++i) {
            int modIndex = *rc::gen::inRange(0, 20);
            bool activate = *rc::gen::arbitrary<bool>();

            Modifier::Type modType = static_cast<Modifier::Type>(
                Modifier::Type_Mod0 + modIndex);

            if (activate) {
                state.activate(modType);
            } else {
                state.deactivate(modType);
            }
        }

        // Standard modifier flags should be unchanged
        RC_ASSERT(state.getFlags() == standardFlags);
    });
}

TEST_CASE("ModifierState: Modal modifier activate/deactivate",
          "[property][modifier][modal]") {
    rc::check("activated modal modifiers can be queried", []() {
        ModifierState state;

        // Pick a random subset of modal modifiers to activate
        std::set<int> activeModifiers;
        const auto numActive = *rc::gen::inRange(0, 10);

        for (int i = 0; i < numActive; ++i) {
            activeModifiers.insert(*rc::gen::inRange(0, 20));
        }

        // Activate selected modifiers
        for (int modIndex : activeModifiers) {
            Modifier::Type modType = static_cast<Modifier::Type>(
                Modifier::Type_Mod0 + modIndex);
            state.activate(modType);
        }

        // Verify activated modifiers are active
        for (int modIndex : activeModifiers) {
            Modifier::Type modType = static_cast<Modifier::Type>(
                Modifier::Type_Mod0 + modIndex);
            RC_ASSERT(state.isActive(modType));
        }

        // Verify non-activated modifiers are not active
        for (int i = 0; i < 20; ++i) {
            if (activeModifiers.find(i) == activeModifiers.end()) {
                Modifier::Type modType = static_cast<Modifier::Type>(
                    Modifier::Type_Mod0 + i);
                RC_ASSERT(!state.isActive(modType));
            }
        }
    });
}

//=============================================================================
// Property 6: Clear Operation
// Clear should reset both standard and modal modifiers
//=============================================================================

TEST_CASE("ModifierState: Clear resets all state",
          "[property][modifier][clear]") {
    rc::check("clear() resets standard and modal modifiers", []() {
        ModifierState state;

        // Apply random standard modifier events
        const auto numEvents = *rc::gen::inRange(0, 10);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }
        applyEvents(state, events);

        // Activate random modal modifiers
        const auto numModal = *rc::gen::inRange(0, 10);
        for (int i = 0; i < numModal; ++i) {
            int modIndex = *rc::gen::inRange(0, 20);
            Modifier::Type modType = static_cast<Modifier::Type>(
                Modifier::Type_Mod0 + modIndex);
            state.activate(modType);
        }

        // Clear all state
        state.clear();

        // Verify everything is clear
        RC_ASSERT(state.getFlags() == MOD_NONE);
        RC_ASSERT(state.getActiveBitmask() == 0);

        // Verify no modal modifiers are active
        for (int i = 0; i < 20; ++i) {
            Modifier::Type modType = static_cast<Modifier::Type>(
                Modifier::Type_Mod0 + i);
            RC_ASSERT(!state.isActive(modType));
        }
    });
}

//=============================================================================
// Shrinking Example Test
// Demonstrates RapidCheck's shrinking capability
//=============================================================================

TEST_CASE("ModifierState: Shrinking example for documentation",
          "[property][modifier][shrinking]") {
    // This test demonstrates shrinking by documenting what happens when
    // a property fails. RapidCheck will minimize the failing input.
    //
    // Example: If we had a bug where pressing LShift twice causes issues,
    // RapidCheck would shrink a 100-event sequence down to just:
    // [LShift down, LShift down]
    //
    // This makes debugging much easier than analyzing the full sequence.

    rc::check("example: state consistency is maintained (always passes)", []() {
        ModifierState state;

        const auto numEvents = *rc::gen::inRange(0, 50);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }

        applyEvents(state, events);

        // This property always passes, just demonstrating the concept
        // If it failed, RapidCheck would shrink the event sequence
        RC_ASSERT(true);
    });
}
