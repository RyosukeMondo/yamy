//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_state.cpp - Unified modifier and lock state tracking

#include "modifier_state.h"
#include "input_event.h"
#include "../../utils/misc.h"  // For VK_* constants
#include <cstring>
#include <cstdio>

namespace {
    // Windows scancodes for modifier keys
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
{
    m_state.reset();
}

void ModifierState::reset()
{
    m_state.reset();
    notifyGUILocks();
}

bool ModifierState::updateFromKeyEvent(const yamy::platform::KeyEvent& event)
{
    StdModifier mod = scancodeToStdModifier(static_cast<uint16_t>(event.scanCode), event.isExtended ? KEYBOARD_INPUT_DATA::E0 : 0);
    if (mod == RCTRL && !event.isExtended) mod = LCTRL; // Disambiguate
    if (mod == RALT && !event.isExtended) mod = LALT;

    if (mod == static_cast<StdModifier>(-1)) { // a bit of a hack
        mod = keycodeToStdModifier(event.scanCode);
    }
    
    if (mod != static_cast<StdModifier>(-1)) {
        setStdFlag(mod, event.isKeyDown);
        return true;
    }
    return false;
}

bool ModifierState::updateFromKID(const KEYBOARD_INPUT_DATA& kid)
{
    bool isKeyDown = !(kid.Flags & KEYBOARD_INPUT_DATA::BREAK);
    StdModifier mod = scancodeToStdModifier(kid.MakeCode, kid.Flags);
    if (mod != static_cast<StdModifier>(-1)) {
        setStdFlag(mod, isKeyDown);
        return true;
    }
    return false;
}

bool ModifierState::isShiftPressed() const { return m_state[LSHIFT] || m_state[RSHIFT]; }
bool ModifierState::isCtrlPressed() const { return m_state[LCTRL] || m_state[RCTRL]; }
bool ModifierState::isAltPressed() const { return m_state[LALT] || m_state[RALT]; }
bool ModifierState::isWinPressed() const { return m_state[LWIN] || m_state[RWIN]; }

void ModifierState::activateModifier(uint8_t mod_num) {
    m_state[VIRTUAL_OFFSET + mod_num] = true;
}

void ModifierState::deactivateModifier(uint8_t mod_num) {
    m_state[VIRTUAL_OFFSET + mod_num] = false;
}

bool ModifierState::isModifierActive(uint8_t mod_num) const {
    return m_state[VIRTUAL_OFFSET + mod_num];
}

void ModifierState::toggleLock(uint8_t lock_num) {
    size_t bit = LOCK_OFFSET + lock_num;
    m_state.flip(bit);
    fprintf(stderr, "[LockState] Lock L%02X toggled to %s\n", lock_num, m_state[bit] ? "ACTIVE" : "INACTIVE");
    fflush(stderr);
    notifyGUILocks();
}

bool ModifierState::isLockActive(uint8_t lock_num) const {
    return m_state[LOCK_OFFSET + lock_num];
}

void ModifierState::notifyGUILocks() {
    if (m_notifyCallback) {
        uint32_t lock_bits[8] = {0};
        for (int i = 0; i < 256; ++i) {
            if (isLockActive(i)) {
                lock_bits[i / 32] |= (1u << (i % 32));
            }
        }
        m_notifyCallback(lock_bits);
    }
}

Modifier ModifierState::toModifier() const {
    Modifier mod;
    if (isShiftPressed()) mod.press(Modifier::Type_Shift);
    if (isCtrlPressed()) mod.press(Modifier::Type_Control);
    if (isAltPressed()) mod.press(Modifier::Type_Alt);
    if (isWinPressed()) mod.press(Modifier::Type_Windows);
    if (m_state[CAPSLOCK]) mod.press(Modifier::Type_CapsLock);
    if (m_state[NUMLOCK]) mod.press(Modifier::Type_NumLock);
    if (m_state[SCROLLLOCK]) mod.press(Modifier::Type_ScrollLock);
    return mod;
}

void ModifierState::setStdFlag(StdModifier flag, bool pressed) {
    m_state[STD_OFFSET + flag] = pressed;
}

// Static helper functions for mapping
ModifierState::StdModifier ModifierState::scancodeToStdModifier(uint16_t scancode, uint16_t flags) {
    bool isExtended = (flags & KEYBOARD_INPUT_DATA::E0) != 0;

    switch (scancode) {
        case 0x2A: return LSHIFT;
        case 0x36: return RSHIFT;
        case 0x1D: return isExtended ? RCTRL : LCTRL;
        case 0x38: return isExtended ? RALT : LALT;
        case 0x5B: if (isExtended) return LWIN; break;
        case 0x5C: if (isExtended) return RWIN; break;
        case 0x3A: return CAPSLOCK;
        case 0x45: return NUMLOCK;
        case 0x46: return SCROLLLOCK;
    }
    return static_cast<StdModifier>(-1);
}

ModifierState::StdModifier ModifierState::keycodeToStdModifier(uint32_t keycode) {
    switch (keycode) {
        case 42: return LSHIFT;
        case 54: return RSHIFT;
        case 29: return LCTRL;
        case 97: return RCTRL;
        case 56: return LALT;
        case 100: return RALT;
        case 125: return LWIN;
        case 126: return RWIN;
        case 58: return CAPSLOCK;
        case 69: return NUMLOCK;
        case 70: return SCROLLLOCK;
    }
    return static_cast<StdModifier>(-1);
}

bool ModifierState::isModifierScancode(uint16_t scancode, uint16_t flags) {
    return scancodeToStdModifier(scancode, flags) != static_cast<StdModifier>(-1);
}

bool ModifierState::isModifierKeycode(uint32_t keycode) {
    return keycodeToStdModifier(keycode) != static_cast<StdModifier>(-1);
}

} // namespace yamy::input