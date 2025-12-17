#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_state.h - Unified modifier and lock state tracking

#ifndef _MODIFIER_STATE_H
#define _MODIFIER_STATE_H

#include "../platform/types.h"
#include "keyboard.h"
#include <cstdint>
#include <bitset>
#include <functional>

namespace yamy::input {

/// Callback type for GUI notification when lock state changes
/// The callback receives a pointer to the 8-element lock bits array
using LockStateChangeCallback = std::function<void(const uint32_t lockBits[8])>;

/// Unified, platform-agnostic modifier and lock state tracker
class ModifierState {
public:
    // Bitset layout constants
    static constexpr size_t STD_MOD_COUNT = 16;
    static constexpr size_t VIRTUAL_MOD_COUNT = 256;
    static constexpr size_t LOCK_COUNT = 256;

    static constexpr size_t STD_OFFSET = 0;
    static constexpr size_t VIRTUAL_OFFSET = STD_OFFSET + STD_MOD_COUNT;
    static constexpr size_t LOCK_OFFSET = VIRTUAL_OFFSET + VIRTUAL_MOD_COUNT;
    static constexpr size_t TOTAL_BITS = LOCK_OFFSET + LOCK_COUNT;

    // Standard modifier flags (for indexing into the bitset)
    enum StdModifier : size_t {
        LSHIFT = 0, RSHIFT, LCTRL, RCTRL, LALT, RALT, LWIN, RWIN,
        CAPSLOCK, NUMLOCK, SCROLLLOCK,
        // For engine compatibility
        UP, DOWN, REPEAT, IMELOCK, IMECOMP
    };

    ModifierState();

    /// Reset all modifier and lock states to not pressed/inactive
    void reset();

    /// Update modifier state from a KeyEvent
    bool updateFromKeyEvent(const yamy::platform::KeyEvent& event);

    /// Update modifier state from KEYBOARD_INPUT_DATA (legacy compatibility)
    bool updateFromKID(const KEYBOARD_INPUT_DATA& kid);

    // --- Standard Modifier Accessors ---
    bool isShiftPressed() const;
    bool isCtrlPressed() const;
    bool isAltPressed() const;
    bool isWinPressed() const;
    
    // --- Virtual Modifier (M00-MFF) Methods ---
    void activateModifier(uint8_t mod_num);
    void deactivateModifier(uint8_t mod_num);
    bool isModifierActive(uint8_t mod_num) const;
    const std::bitset<TOTAL_BITS>& getFullState() const { return m_state; }

    // --- Lock (L00-LFF) Methods ---
    void toggleLock(uint8_t lock_num);
    bool isLockActive(uint8_t lock_num) const;
    void setNotificationCallback(LockStateChangeCallback callback) { m_notifyCallback = callback; }

    /// Convert internal state to legacy Modifier object for engine compatibility
    Modifier toModifier() const;

    /// Check if a given scancode represents a modifier key (Windows)
    static bool isModifierScancode(uint16_t scancode, uint16_t flags);
    /// Check if a given keycode represents a modifier key (Linux)
    static bool isModifierKeycode(uint32_t keycode);

private:
    void setStdFlag(StdModifier flag, bool pressed);
    static StdModifier scancodeToStdModifier(uint16_t scancode, uint16_t flags);
    static StdModifier keycodeToStdModifier(uint32_t keycode);
    void notifyGUILocks();

    std::bitset<TOTAL_BITS> m_state;
    LockStateChangeCallback m_notifyCallback;
};

} // namespace yamy::input

#endif // !_MODIFIER_STATE_H