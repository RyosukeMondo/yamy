//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_state.cpp - Platform-agnostic modifier state tracking

#include "modifier_state.h"
#include "input_event.h"
#include "../../utils/misc.h"  // For VK_* constants

// Additional VK_* constants not in misc.h
namespace {
#ifndef VK_LWIN
    constexpr uint16_t VK_LWIN      = 0x5B;
#endif
#ifndef VK_RWIN
    constexpr uint16_t VK_RWIN      = 0x5C;
#endif
#ifndef VK_CAPITAL
    constexpr uint16_t VK_CAPITAL   = 0x14;  // Caps Lock
#endif
#ifndef VK_NUMLOCK
    constexpr uint16_t VK_NUMLOCK   = 0x90;
#endif
#ifndef VK_SCROLL
    constexpr uint16_t VK_SCROLL    = 0x91;  // Scroll Lock
#endif

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
    : m_flags(MOD_NONE), m_modal(0)
{
}

void ModifierState::reset()
{
    m_flags = MOD_NONE;
    m_modal = 0;
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

    // Map modal modifiers (mod0-mod19) to Modifier type
    // Each bit in m_modal corresponds to a modal modifier
    for (int i = 0; i < 20; ++i) {
        if (m_modal & (1u << i)) {
            Modifier::Type modalType = static_cast<Modifier::Type>(Modifier::Type_Mod0 + i);
            mod.press(modalType);
        }
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

// Modal modifier methods implementation

void ModifierState::activate(Modifier::Type type)
{
    // Handle standard modifiers (backward compatibility)
    if (type >= Modifier::Type_Shift && type < Modifier::Type_Mod0) {
        // This is a standard modifier, not a modal modifier
        // Could update m_flags here if needed, but for now we skip
        return;
    }

    // Handle modal modifiers (Type_Mod0 through Type_Mod19)
    if (type >= Modifier::Type_Mod0 && type <= Modifier::Type_Mod19) {
        int bit = type - Modifier::Type_Mod0;
        m_modal |= (1u << bit);
    }
}

void ModifierState::deactivate(Modifier::Type type)
{
    // Handle standard modifiers (backward compatibility)
    if (type >= Modifier::Type_Shift && type < Modifier::Type_Mod0) {
        // This is a standard modifier, not a modal modifier
        return;
    }

    // Handle modal modifiers (Type_Mod0 through Type_Mod19)
    if (type >= Modifier::Type_Mod0 && type <= Modifier::Type_Mod19) {
        int bit = type - Modifier::Type_Mod0;
        m_modal &= ~(1u << bit);
    }
}

bool ModifierState::isActive(Modifier::Type type) const
{
    // Handle standard modifiers
    if (type == Modifier::Type_Shift) {
        return isShiftPressed();
    }
    if (type == Modifier::Type_Control) {
        return isCtrlPressed();
    }
    if (type == Modifier::Type_Alt) {
        return isAltPressed();
    }
    if (type == Modifier::Type_Windows) {
        return isWinPressed();
    }

    // Handle modal modifiers (Type_Mod0 through Type_Mod19)
    if (type >= Modifier::Type_Mod0 && type <= Modifier::Type_Mod19) {
        int bit = type - Modifier::Type_Mod0;
        return (m_modal & (1u << bit)) != 0;
    }

    return false;
}

void ModifierState::clear()
{
    m_flags = MOD_NONE;
    m_modal = 0;
}

} // namespace yamy::input
