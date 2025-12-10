#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keycode_mapping.h - Key code translation (Track 11)

#include <cstdint>

namespace yamy::platform {

/// Track 11: evdev keycode <-> YAMY keycode translation

/// Convert evdev keycode to YAMY key code
/// @param evdev_code Linux evdev keycode (from linux/input-event-codes.h)
/// @return YAMY key code
uint16_t evdevToYamyKeyCode(uint16_t evdev_code);

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

} // namespace yamy::platform
