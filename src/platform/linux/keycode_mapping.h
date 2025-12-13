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

} // namespace yamy::platform
