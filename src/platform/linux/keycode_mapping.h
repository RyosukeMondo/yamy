#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keycode_mapping.h - Key code translation (Track 11)

#include <cstdint>
#include <string>

namespace yamy::platform {

/// Track 11: evdev keycode <-> YAMY keycode translation

/// Convert evdev keycode to YAMY key code
/// @param evdev_code Linux evdev keycode (from linux/input-event-codes.h)
/// @param event_type Event type: 0=RELEASE, 1=PRESS, 2=REPEAT
/// @return YAMY key code
uint16_t evdevToYamyKeyCode(uint16_t evdev_code, int event_type = -1);

/// Convert YAMY key code to evdev keycode
/// @param yamy_code YAMY key code
/// @return Linux evdev keycode
uint16_t yamyToEvdevKeyCode(uint16_t yamy_code);

/// Check if key is a modifier
/// @param evdev_code Linux evdev keycode
/// @return true if modifier key
bool isModifierKey(uint16_t evdev_code);

/// Get key name for debugging
/// @param evdev_code Linux evdev keycode
/// @return Key name string
const char* getKeyName(uint16_t evdev_code);

/// Set keyboard layout override from config file
/// @param layout Layout string ("us", "jp", etc.)
void setLayoutOverride(const std::string& layout);

/// Clear layout override (use auto-detection)
void clearLayoutOverride();

/// Detect current keyboard layout
/// @return Layout string ("us", "jp", etc.)
std::string detectKeyboardLayout();

/// Get hardware modifier evdev code for a number key
/// Used by ModifierKeyHandler for number-to-modifier feature (Task 4.3)
/// @param yamy_scancode YAMY scan code for number key (e.g., 0x0002 for _1)
/// @return Hardware modifier evdev code (e.g., KEY_LEFTSHIFT), or 0 if not registered
uint16_t getModifierForNumberKey(uint16_t yamy_scancode);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Virtual Key System (Spec: virtual-key-system)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Virtual regular keys (V_*): Range 0xD000-0xDFFF (4096 keys)
/// These are virtual keys that don't correspond to physical keys.
/// Used for intermediate mappings in substitution layer.
/// Example: def subst *A = *V_B creates virtual key V_B
constexpr uint16_t YAMY_VIRTUAL_KEY_BASE = 0xD000;
constexpr uint16_t YAMY_VIRTUAL_KEY_MAX = 0xDFFF;

/// Modal modifiers (M00-MFF): Range 0xF000-0xF0FF (256 modifiers)
/// These are user-defined modal modifiers that can be activated/deactivated.
/// Support tap/hold detection with configurable tap actions.
/// Example: M00, M01, ..., MFF
constexpr uint16_t YAMY_MOD_00 = 0xF000;
constexpr uint16_t YAMY_MOD_FF = 0xF0FF;

/// Lock keys (L00-LFF): Range 0xF100-0xF1FF (256 locks)
/// These are toggleable state keys similar to CapsLock.
/// Press once to activate, press again to deactivate.
/// Example: L00, L01, ..., LFF
constexpr uint16_t YAMY_LOCK_00 = 0xF100;
constexpr uint16_t YAMY_LOCK_FF = 0xF1FF;

/// Check if a keycode is a virtual key (V_*)
/// @param code YAMY key code
/// @return true if code is in virtual key range (0xE000-0xEFFF)
inline bool isVirtualKey(uint16_t code) {
    return code >= YAMY_VIRTUAL_KEY_BASE && code <= YAMY_VIRTUAL_KEY_MAX;
}

/// Check if a keycode is a modal modifier (M00-MFF)
/// @param code YAMY key code
/// @return true if code is in modifier range (0xF000-0xF0FF)
inline bool isModifier(uint16_t code) {
    return code >= YAMY_MOD_00 && code <= YAMY_MOD_FF;
}

/// Check if a keycode is a lock key (L00-LFF)
/// @param code YAMY key code
/// @return true if code is in lock range (0xF100-0xF1FF)
inline bool isLock(uint16_t code) {
    return code >= YAMY_LOCK_00 && code <= YAMY_LOCK_FF;
}

/// Get modifier number from modifier keycode
/// @param code YAMY key code (must be in range YAMY_MOD_00-YAMY_MOD_FF)
/// @return Modifier number (0x00-0xFF)
inline uint8_t getModifierNumber(uint16_t code) {
    return static_cast<uint8_t>(code - YAMY_MOD_00);
}

/// Get lock number from lock keycode
/// @param code YAMY key code (must be in range YAMY_LOCK_00-YAMY_LOCK_FF)
/// @return Lock number (0x00-0xFF)
inline uint8_t getLockNumber(uint16_t code) {
    return static_cast<uint8_t>(code - YAMY_LOCK_00);
}

} // namespace yamy::platform
