#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_state.h - Platform-agnostic modifier state tracking
//
// Provides detection of modifier keys from platform-neutral KeyEvent structure.
// Supports both Windows scancodes and Linux keycodes transparently.

#ifndef _MODIFIER_STATE_H
#define _MODIFIER_STATE_H

#include "../platform/types.h"
#include "keyboard.h"
#include <cstdint>

namespace yamy::input {

/// Modifier key flags (bitmask)
enum ModifierFlag : uint32_t {
    MOD_NONE       = 0,
    MOD_LSHIFT     = 1 << 0,
    MOD_RSHIFT     = 1 << 1,
    MOD_LCTRL      = 1 << 2,
    MOD_RCTRL      = 1 << 3,
    MOD_LALT       = 1 << 4,
    MOD_RALT       = 1 << 5,
    MOD_LWIN       = 1 << 6,   // Windows key / Super key on Linux
    MOD_RWIN       = 1 << 7,
    MOD_CAPSLOCK   = 1 << 8,
    MOD_NUMLOCK    = 1 << 9,
    MOD_SCROLLLOCK = 1 << 10,

    // Combined flags for convenience
    MOD_SHIFT      = MOD_LSHIFT | MOD_RSHIFT,
    MOD_CTRL       = MOD_LCTRL | MOD_RCTRL,
    MOD_ALT        = MOD_LALT | MOD_RALT,
    MOD_WIN        = MOD_LWIN | MOD_RWIN,
};

/// Platform-agnostic modifier state tracker
class ModifierState {
public:
    ModifierState();

    /// Reset all modifier states to not pressed
    void reset();

    /// Update modifier state from a KeyEvent
    /// @param event The key event to process
    /// @return true if the event was a modifier key, false otherwise
    bool updateFromKeyEvent(const yamy::platform::KeyEvent& event);

    /// Update modifier state from KEYBOARD_INPUT_DATA (legacy compatibility)
    /// @param kid The keyboard input data
    /// @return true if the event was a modifier key, false otherwise
    bool updateFromKID(const KEYBOARD_INPUT_DATA& kid);

    /// Get current modifier flags
    uint32_t getFlags() const { return m_flags; }

    /// Check if any shift key is pressed
    bool isShiftPressed() const { return (m_flags & MOD_SHIFT) != 0; }

    /// Check if any control key is pressed
    bool isCtrlPressed() const { return (m_flags & MOD_CTRL) != 0; }

    /// Check if any alt key is pressed
    bool isAltPressed() const { return (m_flags & MOD_ALT) != 0; }

    /// Check if any windows/super key is pressed
    bool isWinPressed() const { return (m_flags & MOD_WIN) != 0; }

    /// Check if left shift is pressed
    bool isLShiftPressed() const { return (m_flags & MOD_LSHIFT) != 0; }

    /// Check if right shift is pressed
    bool isRShiftPressed() const { return (m_flags & MOD_RSHIFT) != 0; }

    /// Check if left control is pressed
    bool isLCtrlPressed() const { return (m_flags & MOD_LCTRL) != 0; }

    /// Check if right control is pressed
    bool isRCtrlPressed() const { return (m_flags & MOD_RCTRL) != 0; }

    /// Check if left alt is pressed
    bool isLAltPressed() const { return (m_flags & MOD_LALT) != 0; }

    /// Check if right alt is pressed
    bool isRAltPressed() const { return (m_flags & MOD_RALT) != 0; }

    /// Check if left windows/super is pressed
    bool isLWinPressed() const { return (m_flags & MOD_LWIN) != 0; }

    /// Check if right windows/super is pressed
    bool isRWinPressed() const { return (m_flags & MOD_RWIN) != 0; }

    /// Check if caps lock is active (toggled on)
    bool isCapsLockOn() const { return (m_flags & MOD_CAPSLOCK) != 0; }

    /// Check if num lock is active (toggled on)
    bool isNumLockOn() const { return (m_flags & MOD_NUMLOCK) != 0; }

    /// Check if scroll lock is active (toggled on)
    bool isScrollLockOn() const { return (m_flags & MOD_SCROLLLOCK) != 0; }

    /// Set lock key state (e.g., from system query)
    void setLockState(bool capsLock, bool numLock, bool scrollLock);

    /// Convert internal modifier flags to Modifier object for engine compatibility
    Modifier toModifier() const;

    /// Check if a given scancode represents a modifier key (Windows)
    static bool isModifierScancode(uint16_t scancode, uint16_t flags);

    /// Check if a given keycode represents a modifier key (Linux)
    static bool isModifierKeycode(uint32_t keycode);

private:
    /// Set or clear a modifier flag based on key press state
    void setFlag(ModifierFlag flag, bool pressed);

    /// Detect modifier from Windows scancode
    /// @return The modifier flag, or MOD_NONE if not a modifier
    static ModifierFlag detectModifierFromScancode(uint16_t scancode, uint16_t flags);

    /// Detect modifier from Linux keycode
    /// @return The modifier flag, or MOD_NONE if not a modifier
    static ModifierFlag detectModifierFromKeycode(uint32_t keycode);

    uint32_t m_flags;
};

} // namespace yamy::input

#endif // !_MODIFIER_STATE_H
