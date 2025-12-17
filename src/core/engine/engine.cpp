//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.cpp - Main engine implementation (core logic split into separate files)

#include "misc.h"
#include "engine.h"
#include <algorithm>
#include <cstring>

// Implementation split across the following files:
// - engine_lifecycle.cpp: Constructor, destructor, start, stop
// - engine_keyboard_handler.cpp: Keyboard event handling thread
// - engine_ipc_handler.cpp: IPC message handling
// - engine_event_processor.cpp: Event processing logic
// - engine_generator.cpp: Key event generation
// - engine_modifier.cpp: Modifier key handling
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
    const yamy::input::ModifierState& mods
) const {
    const auto& state = mods.getFullState();

    // Check most specific entries first (vector is sorted by specificity DESC)
    for (const auto& entry : m_virtualKeymap) {
        // Quick check: does input key match?
        if (entry.input_key != key) {
            continue;
        }

        // Check Virtual Modifiers (M00-MFF)
        bool mods_match = true;
        for (int i = 0; i < 256; ++i) {
            // Check if this specific modifier bit is required by the entry
            bool required = (entry.required_mods[i / 32] >> (i % 32)) & 1;
            
            // Check if the modifier is active in the current state
            // VIRTUAL_OFFSET is where M00 starts
            bool active = state.test(yamy::input::ModifierState::VIRTUAL_OFFSET + i);

            // Strict match on REQUIRED bits: If required, must be active
            if (required && !active) {
                mods_match = false;
                break;
            }
            // Note: We ignore "required OFF" for now in this legacy structure, 
            // as m_virtualKeymap seems to only store required_mods mask (positive match).
        }
        if (!mods_match) {
            continue;
        }

        // Check Lock States (L00-LFF)
        bool locks_match = true;
        for (int i = 0; i < 256; ++i) {
            bool required = (entry.required_locks[i / 32] >> (i % 32)) & 1;
            bool active = state.test(yamy::input::ModifierState::LOCK_OFFSET + i);

            if (required && !active) {
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

std::vector<yamy::engine::CompiledRule> Engine::compileSubstitute(const Keyboard::Substitute& sub) {
    using namespace yamy::input;

    std::vector<yamy::engine::CompiledRule> rules;
    yamy::engine::CompiledRule base_rule;

    // --- Compile Output ---
    const Key* toKey = sub.m_mkeyTo.m_key;
    if (toKey && toKey->getScanCodesSize() > 0) {
        base_rule.outputScanCode = toKey->getScanCodes()[0].m_scan;
    } else {
        base_rule.outputScanCode = 0;
    }

    // --- Compile Input Conditions ---
    const Modifier& fromMod = sub.m_mkeyFrom.m_modifier;
    std::vector<std::pair<size_t, size_t>> generic_modifiers;

    // --- Handle Generic Modifiers (Shift, Ctrl, Alt, Win) ---
    if (fromMod.isOn(Modifier::Type_Shift)) {
        generic_modifiers.push_back({ModifierState::LSHIFT, ModifierState::RSHIFT});
    } else if (!fromMod.isDontcare(Modifier::Type_Shift)) {
        base_rule.requiredOff.set(ModifierState::LSHIFT);
        base_rule.requiredOff.set(ModifierState::RSHIFT);
    }

    if (fromMod.isOn(Modifier::Type_Control)) {
        generic_modifiers.push_back({ModifierState::LCTRL, ModifierState::RCTRL});
    } else if (!fromMod.isDontcare(Modifier::Type_Control)) {
        base_rule.requiredOff.set(ModifierState::LCTRL);
        base_rule.requiredOff.set(ModifierState::RCTRL);
    }

    if (fromMod.isOn(Modifier::Type_Alt)) {
        generic_modifiers.push_back({ModifierState::LALT, ModifierState::RALT});
    } else if (!fromMod.isDontcare(Modifier::Type_Alt)) {
        base_rule.requiredOff.set(ModifierState::LALT);
        base_rule.requiredOff.set(ModifierState::RALT);
    }

    if (fromMod.isOn(Modifier::Type_Windows)) {
        generic_modifiers.push_back({ModifierState::LWIN, ModifierState::RWIN});
    } else if (!fromMod.isDontcare(Modifier::Type_Windows)) {
        base_rule.requiredOff.set(ModifierState::LWIN);
        base_rule.requiredOff.set(ModifierState::RWIN);
    }

    // --- Handle Specific State Modifiers ---
    if (fromMod.isOn(Modifier::Type_CapsLock)) {
        base_rule.requiredOn.set(ModifierState::CAPSLOCK);
    } else if (!fromMod.isDontcare(Modifier::Type_CapsLock)) {
        base_rule.requiredOff.set(ModifierState::CAPSLOCK);
    }
    if (fromMod.isOn(Modifier::Type_NumLock)) {
        base_rule.requiredOn.set(ModifierState::NUMLOCK);
    } else if (!fromMod.isDontcare(Modifier::Type_NumLock)) {
        base_rule.requiredOff.set(ModifierState::NUMLOCK);
    }
    if (fromMod.isOn(Modifier::Type_ScrollLock)) {
        base_rule.requiredOn.set(ModifierState::SCROLLLOCK);
    } else if (!fromMod.isDontcare(Modifier::Type_ScrollLock)) {
        base_rule.requiredOff.set(ModifierState::SCROLLLOCK);
    }
     if (fromMod.isOn(Modifier::Type_Up)) {
        base_rule.requiredOn.set(ModifierState::UP);
    } else if (!fromMod.isDontcare(Modifier::Type_Up)) {
        base_rule.requiredOff.set(ModifierState::UP);
    }

    // --- Handle Virtual Modifiers (M00-MFF) ---
    for (int i = 0; i < 256; ++i) {
        if (sub.m_mkeyFrom.isVirtualModActive(i)) {
            base_rule.requiredOn.set(ModifierState::VIRTUAL_OFFSET + i);
        }
    }
    
    // --- Handle Lock Modifiers (L00-LFF) ---
    for (int i = 0; i < 10; ++i) {
        Modifier::Type lockType = static_cast<Modifier::Type>(Modifier::Type_Lock0 + i);
        if (fromMod.isOn(lockType)) {
            base_rule.requiredOn.set(ModifierState::LOCK_OFFSET + i);
        } else if (!fromMod.isDontcare(lockType)) {
            base_rule.requiredOff.set(ModifierState::LOCK_OFFSET + i);
        }
    }

    // --- Expand Generic Modifiers ---
    if (generic_modifiers.empty()) {
        rules.push_back(base_rule);
    } else {
        size_t num_expansions = 1 << generic_modifiers.size();
        for (size_t i = 0; i < num_expansions; ++i) {
            yamy::engine::CompiledRule new_rule = base_rule;
            for (size_t j = 0; j < generic_modifiers.size(); ++j) {
                if ((i >> j) & 1) {
                    new_rule.requiredOn.set(generic_modifiers[j].second); // e.g., RSHIFT
                } else {
                    new_rule.requiredOn.set(generic_modifiers[j].first);  // e.g., LSHIFT
                }
            }
            rules.push_back(new_rule);
        }
    }

    return rules;
}
