//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.cpp - Main engine implementation (core logic split into separate files)

#include "misc.h"
#include "engine.h"
#include "../input/lock_state.h"
#include <algorithm>
#include <cstring>

// Implementation split across the following files:
// - engine_lifecycle.cpp: Constructor, destructor, start, stop
// - engine_keyboard_handler.cpp: Keyboard event handling thread
// - engine_ipc_handler.cpp: IPC message handling
// - engine_event_processor.cpp: Event processing logic
// - engine_generator.cpp: Key event generation
// - engine_modifier.cpp: Modifier key handling
// - engine_focus.cpp: Window focus management
// - engine_window.cpp: Window management
// - engine_setting.cpp: Settings management
// - engine_input.cpp: Input injection
// - engine_log.cpp: Logging

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Virtual Keymap Implementation (Task 3.4)

/// Count number of set bits in a bitmask array
static uint8_t popcount_array(const uint32_t* bits, size_t count) {
    uint8_t total = 0;
    for (size_t i = 0; i < count; ++i) {
        // Use __builtin_popcount if available (GCC/Clang)
        #if defined(__GNUC__) || defined(__clang__)
        total += static_cast<uint8_t>(__builtin_popcount(bits[i]));
        #else
        // Fallback implementation
        uint32_t v = bits[i];
        while (v) {
            total += v & 1;
            v >>= 1;
        }
        #endif
    }
    return total;
}

uint16_t Engine::lookupKeymap(
    uint16_t key,
    const yamy::input::ModifierState& mods,
    const yamy::input::LockState& locks
) const {
    const uint32_t* active_mods = mods.getModifierBits();
    const uint32_t* active_locks = locks.getLockBits();

    // Check most specific entries first (vector is sorted by specificity DESC)
    for (const auto& entry : m_virtualKeymap) {
        // Quick check: does input key match?
        if (entry.input_key != key) {
            continue;
        }

        // Check if all required modifiers are active
        // For each word in the bitmask array, verify that all required bits are set
        // Formula: (active_mods[i] & required_mods[i]) == required_mods[i]
        bool mods_match = true;
        for (int i = 0; i < 8; ++i) {
            if ((active_mods[i] & entry.required_mods[i]) != entry.required_mods[i]) {
                mods_match = false;
                break;
            }
        }
        if (!mods_match) {
            continue;
        }

        // Check if all required locks are active
        bool locks_match = true;
        for (int i = 0; i < 8; ++i) {
            if ((active_locks[i] & entry.required_locks[i]) != entry.required_locks[i]) {
                locks_match = false;
                break;
            }
        }
        if (!locks_match) {
            continue;
        }

        // Found match! Return output key
        return entry.output_key;
    }

    // No match found - return 0 (no mapping)
    return 0;
}

void Engine::sortKeymapBySpecificity() {
    // Sort by specificity in descending order (highest first)
    // This ensures that more specific matches are checked before less specific ones
    std::sort(m_virtualKeymap.begin(), m_virtualKeymap.end(),
        [](const KeymapEntry& a, const KeymapEntry& b) {
            return a.specificity > b.specificity;  // DESC order
        });
}

void Engine::addKeymapEntry(
    uint16_t input_key,
    uint16_t output_key,
    const uint32_t required_mods[8],
    const uint32_t required_locks[8]
) {
    KeymapEntry entry;
    entry.input_key = input_key;
    entry.output_key = output_key;

    // Copy modifier and lock requirements
    std::memcpy(entry.required_mods, required_mods, sizeof(entry.required_mods));
    std::memcpy(entry.required_locks, required_locks, sizeof(entry.required_locks));

    // Calculate specificity: count of set bits in modifiers + locks
    uint8_t mod_count = popcount_array(required_mods, 8);
    uint8_t lock_count = popcount_array(required_locks, 8);
    entry.specificity = mod_count + lock_count;

    // Add to keymap (will be sorted later by sortKeymapBySpecificity)
    m_virtualKeymap.push_back(entry);
}
