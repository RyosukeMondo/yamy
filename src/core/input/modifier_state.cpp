//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_state.cpp - Platform-agnostic modifier state tracking

#include "modifier_state.h"
#include "input_event.h"

// Windows virtual key codes for modifier detection
// These match the VK_* constants from WinUser.h
namespace {
    constexpr uint16_t VK_LSHIFT    = 0xA0;
    constexpr uint16_t VK_RSHIFT    = 0xA1;
    constexpr uint16_t VK_LCONTROL  = 0xA2;
    constexpr uint16_t VK_RCONTROL  = 0xA3;
    constexpr uint16_t VK_LMENU     = 0xA4;  // Left Alt
    constexpr uint16_t VK_RMENU     = 0xA5;  // Right Alt
    constexpr uint16_t VK_LWIN      = 0x5B;
    constexpr uint16_t VK_RWIN      = 0x5C;
    constexpr uint16_t VK_CAPITAL   = 0x14;  // Caps Lock
    constexpr uint16_t VK_NUMLOCK   = 0x90;
    constexpr uint16_t VK_SCROLL    = 0x91;  // Scroll Lock

    // Windows scancodes for modifier keys
    // These are hardware scan codes from keyboard
    constexpr uint16_t SC_LSHIFT    = 0x2A;
    constexpr uint16_t SC_RSHIFT    = 0x36;
    constexpr uint16_t SC_LCTRL     = 0x1D;
    constexpr uint16_t SC_RCTRL     = 0x1D;  // Same as left, distinguished by E0 flag
    constexpr uint16_t SC_LALT      = 0x38;
    constexpr uint16_t SC_RALT      = 0x38;  // Same as left, distinguished by E0 flag
    constexpr uint16_t SC_LWIN      = 0x5B;  // E0 extended
    constexpr uint16_t SC_RWIN      = 0x5C;  // E0 extended
    constexpr uint16_t SC_CAPSLOCK  = 0x3A;
    constexpr uint16_t SC_NUMLOCK   = 0x45;
    constexpr uint16_t SC_SCROLLLOCK = 0x46;

    // Linux evdev keycodes for modifiers (from input-event-codes.h)
    // These values match linux/input-event-codes.h
    constexpr uint32_t KEY_LEFTSHIFT   = 42;
    constexpr uint32_t KEY_RIGHTSHIFT  = 54;
    constexpr uint32_t KEY_LEFTCTRL    = 29;
    constexpr uint32_t KEY_RIGHTCTRL   = 97;
    constexpr uint32_t KEY_LEFTALT     = 56;
    constexpr uint32_t KEY_RIGHTALT    = 100;
    constexpr uint32_t KEY_LEFTMETA    = 125;
    constexpr uint32_t KEY_RIGHTMETA   = 126;
    constexpr uint32_t KEY_CAPSLOCK    = 58;
    constexpr uint32_t KEY_NUMLOCK     = 69;
    constexpr uint32_t KEY_SCROLLLOCK  = 70;
}

namespace yamy::input {

ModifierState::ModifierState()
    : m_flags(MOD_NONE)
{
}

void ModifierState::reset()
{
    m_flags = MOD_NONE;
}

bool ModifierState::updateFromKeyEvent(const yamy::platform::KeyEvent& event)
{
    // KeyEvent contains:
    // - scanCode: platform-specific scan code (Windows scancode or Linux evdev keycode)
    // - isKeyDown: true if key pressed, false if released
    // - isExtended: true for extended keys (E0 prefix on Windows)
    // - flags: additional flags

    // Try Windows scancode detection first
    uint16_t flags = event.isExtended ? KEYBOARD_INPUT_DATA::E0 : 0;
    ModifierFlag mod = detectModifierFromScancode(
        static_cast<uint16_t>(event.scanCode), flags);

    if (mod == MOD_NONE) {
        // Try Linux keycode detection
        mod = detectModifierFromKeycode(event.scanCode);
    }

    if (mod != MOD_NONE) {
        setFlag(mod, event.isKeyDown);
        return true;
    }

    return false;
}

bool ModifierState::updateFromKID(const KEYBOARD_INPUT_DATA& kid)
{
    bool isKeyDown = !(kid.Flags & KEYBOARD_INPUT_DATA::BREAK);

    ModifierFlag mod = detectModifierFromScancode(kid.MakeCode, kid.Flags);

    if (mod != MOD_NONE) {
        setFlag(mod, isKeyDown);
        return true;
    }

    return false;
}

void ModifierState::setLockState(bool capsLock, bool numLock, bool scrollLock)
{
    setFlag(MOD_CAPSLOCK, capsLock);
    setFlag(MOD_NUMLOCK, numLock);
    setFlag(MOD_SCROLLLOCK, scrollLock);
}

Modifier ModifierState::toModifier() const
{
    Modifier mod;

    // Map internal flags to Modifier type
    if (isShiftPressed()) {
        mod.press(Modifier::Type_Shift);
    }
    if (isCtrlPressed()) {
        mod.press(Modifier::Type_Control);
    }
    if (isAltPressed()) {
        mod.press(Modifier::Type_Alt);
    }
    if (isWinPressed()) {
        mod.press(Modifier::Type_Windows);
    }
    if (isCapsLockOn()) {
        mod.press(Modifier::Type_CapsLock);
    }
    if (isNumLockOn()) {
        mod.press(Modifier::Type_NumLock);
    }
    if (isScrollLockOn()) {
        mod.press(Modifier::Type_ScrollLock);
    }

    return mod;
}

bool ModifierState::isModifierScancode(uint16_t scancode, uint16_t flags)
{
    return detectModifierFromScancode(scancode, flags) != MOD_NONE;
}

bool ModifierState::isModifierKeycode(uint32_t keycode)
{
    return detectModifierFromKeycode(keycode) != MOD_NONE;
}

void ModifierState::setFlag(ModifierFlag flag, bool pressed)
{
    if (pressed) {
        m_flags |= flag;
    } else {
        m_flags &= ~flag;
    }
}

ModifierFlag ModifierState::detectModifierFromScancode(uint16_t scancode, uint16_t flags)
{
    bool isExtended = (flags & KEYBOARD_INPUT_DATA::E0) != 0;

    switch (scancode) {
        case SC_LSHIFT:
            return MOD_LSHIFT;

        case SC_RSHIFT:
            return MOD_RSHIFT;

        case SC_LCTRL:  // 0x1D
            // Left and Right Ctrl have same scancode, distinguished by E0
            return isExtended ? MOD_RCTRL : MOD_LCTRL;

        case SC_LALT:   // 0x38
            // Left and Right Alt have same scancode, distinguished by E0
            return isExtended ? MOD_RALT : MOD_LALT;

        case SC_LWIN:   // 0x5B with E0
            if (isExtended) return MOD_LWIN;
            break;

        case SC_RWIN:   // 0x5C with E0
            if (isExtended) return MOD_RWIN;
            break;

        case SC_CAPSLOCK:
            return MOD_CAPSLOCK;

        case SC_NUMLOCK:
            return MOD_NUMLOCK;

        case SC_SCROLLLOCK:
            return MOD_SCROLLLOCK;
    }

    return MOD_NONE;
}

ModifierFlag ModifierState::detectModifierFromKeycode(uint32_t keycode)
{
    switch (keycode) {
        case KEY_LEFTSHIFT:
            return MOD_LSHIFT;

        case KEY_RIGHTSHIFT:
            return MOD_RSHIFT;

        case KEY_LEFTCTRL:
            return MOD_LCTRL;

        case KEY_RIGHTCTRL:
            return MOD_RCTRL;

        case KEY_LEFTALT:
            return MOD_LALT;

        case KEY_RIGHTALT:
            return MOD_RALT;

        case KEY_LEFTMETA:
            return MOD_LWIN;

        case KEY_RIGHTMETA:
            return MOD_RWIN;

        case KEY_CAPSLOCK:
            return MOD_CAPSLOCK;

        case KEY_NUMLOCK:
            return MOD_NUMLOCK;

        case KEY_SCROLLLOCK:
            return MOD_SCROLLLOCK;
    }

    return MOD_NONE;
}

} // namespace yamy::input
