//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keycode_stub.cpp - Minimal keycode support for test tools (no logger dependency)

#include "../../platform/linux/keycode_mapping.h"
#include <linux/input-event-codes.h>
#include <unordered_map>
#include <string>

namespace yamy::platform {

// Minimal key name mapping for test tools
static const std::unordered_map<uint16_t, const char*> evdev_names = {
    {KEY_A, "KEY_A"}, {KEY_B, "KEY_B"}, {KEY_C, "KEY_C"}, {KEY_D, "KEY_D"},
    {KEY_E, "KEY_E"}, {KEY_F, "KEY_F"}, {KEY_G, "KEY_G"}, {KEY_H, "KEY_H"},
    {KEY_I, "KEY_I"}, {KEY_J, "KEY_J"}, {KEY_K, "KEY_K"}, {KEY_L, "KEY_L"},
    {KEY_M, "KEY_M"}, {KEY_N, "KEY_N"}, {KEY_O, "KEY_O"}, {KEY_P, "KEY_P"},
    {KEY_Q, "KEY_Q"}, {KEY_R, "KEY_R"}, {KEY_S, "KEY_S"}, {KEY_T, "KEY_T"},
    {KEY_U, "KEY_U"}, {KEY_V, "KEY_V"}, {KEY_W, "KEY_W"}, {KEY_X, "KEY_X"},
    {KEY_Y, "KEY_Y"}, {KEY_Z, "KEY_Z"},
    {KEY_1, "KEY_1"}, {KEY_2, "KEY_2"}, {KEY_3, "KEY_3"}, {KEY_4, "KEY_4"},
    {KEY_5, "KEY_5"}, {KEY_6, "KEY_6"}, {KEY_7, "KEY_7"}, {KEY_8, "KEY_8"},
    {KEY_9, "KEY_9"}, {KEY_0, "KEY_0"},
    {KEY_TAB, "KEY_TAB"}, {KEY_ENTER, "KEY_ENTER"}, {KEY_ESC, "KEY_ESC"},
    {KEY_SPACE, "KEY_SPACE"}, {KEY_BACKSPACE, "KEY_BACKSPACE"},
    {KEY_DELETE, "KEY_DELETE"}, {KEY_INSERT, "KEY_INSERT"},
    {KEY_HOME, "KEY_HOME"}, {KEY_END, "KEY_END"},
    {KEY_PAGEUP, "KEY_PAGEUP"}, {KEY_PAGEDOWN, "KEY_PAGEDOWN"},
    {KEY_UP, "KEY_UP"}, {KEY_DOWN, "KEY_DOWN"},
    {KEY_LEFT, "KEY_LEFT"}, {KEY_RIGHT, "KEY_RIGHT"},
    {KEY_LEFTSHIFT, "KEY_LEFTSHIFT"}, {KEY_RIGHTSHIFT, "KEY_RIGHTSHIFT"},
    {KEY_LEFTCTRL, "KEY_LEFTCTRL"}, {KEY_RIGHTCTRL, "KEY_RIGHTCTRL"},
    {KEY_LEFTALT, "KEY_LEFTALT"}, {KEY_RIGHTALT, "KEY_RIGHTALT"},
    {KEY_LEFTMETA, "KEY_LEFTMETA"}, {KEY_RIGHTMETA, "KEY_RIGHTMETA"},
    {KEY_SEMICOLON, "KEY_SEMICOLON"}, {KEY_MINUS, "KEY_MINUS"},
    {KEY_F1, "KEY_F1"}, {KEY_F2, "KEY_F2"}, {KEY_F3, "KEY_F3"},
    {KEY_F4, "KEY_F4"}, {KEY_F5, "KEY_F5"}, {KEY_F6, "KEY_F6"},
    {KEY_F7, "KEY_F7"}, {KEY_F8, "KEY_F8"}, {KEY_F9, "KEY_F9"},
    {KEY_F10, "KEY_F10"}, {KEY_F11, "KEY_F11"}, {KEY_F12, "KEY_F12"}
};

const char* getKeyName(uint16_t evdev_code) {
    auto it = evdev_names.find(evdev_code);
    if (it != evdev_names.end()) {
        return it->second;
    }
    return "KEY_UNKNOWN";
}

} // namespace yamy::platform
