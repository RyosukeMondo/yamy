//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keycode_mapping.cpp - evdev ↔ YAMY keycode translation (Track 11)
// Optimized with std::unordered_map for O(1) constant-time lookups

#include "keycode_mapping.h"
#include <linux/input-event-codes.h>
#include <unordered_map>

namespace yamy::platform {

// Windows Virtual Key codes (VK_*) - define locally
constexpr uint16_t VK_BACK           = 0x08;
constexpr uint16_t VK_TAB            = 0x09;
constexpr uint16_t VK_RETURN         = 0x0D;
constexpr uint16_t VK_PAUSE          = 0x13;
constexpr uint16_t VK_CAPITAL        = 0x14;  // CAPS LOCK
constexpr uint16_t VK_ESCAPE         = 0x1B;
constexpr uint16_t VK_SPACE          = 0x20;
constexpr uint16_t VK_PRIOR          = 0x21;  // PAGE UP
constexpr uint16_t VK_NEXT           = 0x22;  // PAGE DOWN
constexpr uint16_t VK_END            = 0x23;
constexpr uint16_t VK_HOME           = 0x24;
constexpr uint16_t VK_LEFT           = 0x25;
constexpr uint16_t VK_UP             = 0x26;
constexpr uint16_t VK_RIGHT          = 0x27;
constexpr uint16_t VK_DOWN           = 0x28;
constexpr uint16_t VK_SNAPSHOT       = 0x2C;  // PRINT SCREEN
constexpr uint16_t VK_INSERT         = 0x2D;
constexpr uint16_t VK_DELETE         = 0x2E;

// 0-9 keys
constexpr uint16_t VK_0              = 0x30;
constexpr uint16_t VK_1              = 0x31;
constexpr uint16_t VK_2              = 0x32;
constexpr uint16_t VK_3              = 0x33;
constexpr uint16_t VK_4              = 0x34;
constexpr uint16_t VK_5              = 0x35;
constexpr uint16_t VK_6              = 0x36;
constexpr uint16_t VK_7              = 0x37;
constexpr uint16_t VK_8              = 0x38;
constexpr uint16_t VK_9              = 0x39;

// A-Z keys
constexpr uint16_t VK_A              = 0x41;
constexpr uint16_t VK_B              = 0x42;
constexpr uint16_t VK_C              = 0x43;
constexpr uint16_t VK_D              = 0x44;
constexpr uint16_t VK_E              = 0x45;
constexpr uint16_t VK_F              = 0x46;
constexpr uint16_t VK_G              = 0x47;
constexpr uint16_t VK_H              = 0x48;
constexpr uint16_t VK_I              = 0x49;
constexpr uint16_t VK_J              = 0x4A;
constexpr uint16_t VK_K              = 0x4B;
constexpr uint16_t VK_L              = 0x4C;
constexpr uint16_t VK_M              = 0x4D;
constexpr uint16_t VK_N              = 0x4E;
constexpr uint16_t VK_O              = 0x4F;
constexpr uint16_t VK_P              = 0x50;
constexpr uint16_t VK_Q              = 0x51;
constexpr uint16_t VK_R              = 0x52;
constexpr uint16_t VK_S              = 0x53;
constexpr uint16_t VK_T              = 0x54;
constexpr uint16_t VK_U              = 0x55;
constexpr uint16_t VK_V              = 0x56;
constexpr uint16_t VK_W              = 0x57;
constexpr uint16_t VK_X              = 0x58;
constexpr uint16_t VK_Y              = 0x59;
constexpr uint16_t VK_Z              = 0x5A;

// Windows keys
constexpr uint16_t VK_LWIN           = 0x5B;
constexpr uint16_t VK_RWIN           = 0x5C;
constexpr uint16_t VK_APPS           = 0x5D;  // Context menu

// Numpad
constexpr uint16_t VK_NUMPAD0        = 0x60;
constexpr uint16_t VK_NUMPAD1        = 0x61;
constexpr uint16_t VK_NUMPAD2        = 0x62;
constexpr uint16_t VK_NUMPAD3        = 0x63;
constexpr uint16_t VK_NUMPAD4        = 0x64;
constexpr uint16_t VK_NUMPAD5        = 0x65;
constexpr uint16_t VK_NUMPAD6        = 0x66;
constexpr uint16_t VK_NUMPAD7        = 0x67;
constexpr uint16_t VK_NUMPAD8        = 0x68;
constexpr uint16_t VK_NUMPAD9        = 0x69;
constexpr uint16_t VK_MULTIPLY       = 0x6A;
constexpr uint16_t VK_ADD            = 0x6B;
constexpr uint16_t VK_SUBTRACT       = 0x6D;
constexpr uint16_t VK_DECIMAL        = 0x6E;
constexpr uint16_t VK_DIVIDE         = 0x6F;

// Function keys
constexpr uint16_t VK_F1             = 0x70;
constexpr uint16_t VK_F2             = 0x71;
constexpr uint16_t VK_F3             = 0x72;
constexpr uint16_t VK_F4             = 0x73;
constexpr uint16_t VK_F5             = 0x74;
constexpr uint16_t VK_F6             = 0x75;
constexpr uint16_t VK_F7             = 0x76;
constexpr uint16_t VK_F8             = 0x77;
constexpr uint16_t VK_F9             = 0x78;
constexpr uint16_t VK_F10            = 0x79;
constexpr uint16_t VK_F11            = 0x7A;
constexpr uint16_t VK_F12            = 0x7B;
constexpr uint16_t VK_F13            = 0x7C;
constexpr uint16_t VK_F14            = 0x7D;
constexpr uint16_t VK_F15            = 0x7E;
constexpr uint16_t VK_F16            = 0x7F;
constexpr uint16_t VK_F17            = 0x80;
constexpr uint16_t VK_F18            = 0x81;
constexpr uint16_t VK_F19            = 0x82;
constexpr uint16_t VK_F20            = 0x83;
constexpr uint16_t VK_F21            = 0x84;
constexpr uint16_t VK_F22            = 0x85;
constexpr uint16_t VK_F23            = 0x86;
constexpr uint16_t VK_F24            = 0x87;

// Lock keys
constexpr uint16_t VK_NUMLOCK        = 0x90;
constexpr uint16_t VK_SCROLL         = 0x91;  // SCROLL LOCK

// Modifier keys (left/right)
constexpr uint16_t VK_LSHIFT         = 0xA0;
constexpr uint16_t VK_RSHIFT         = 0xA1;
constexpr uint16_t VK_LCONTROL       = 0xA2;
constexpr uint16_t VK_RCONTROL       = 0xA3;
constexpr uint16_t VK_LMENU          = 0xA4;  // Left ALT
constexpr uint16_t VK_RMENU          = 0xA5;  // Right ALT

// Punctuation
constexpr uint16_t VK_OEM_1          = 0xBA;  // ';:' for US
constexpr uint16_t VK_OEM_PLUS       = 0xBB;  // '=+' for any country
constexpr uint16_t VK_OEM_COMMA      = 0xBC;  // ',<' for any country
constexpr uint16_t VK_OEM_MINUS      = 0xBD;  // '-_' for any country
constexpr uint16_t VK_OEM_PERIOD     = 0xBE;  // '.>' for any country
constexpr uint16_t VK_OEM_2          = 0xBF;  // '/?' for US
constexpr uint16_t VK_OEM_3          = 0xC0;  // '`~' for US
constexpr uint16_t VK_OEM_4          = 0xDB;  // '[{' for US
constexpr uint16_t VK_OEM_5          = 0xDC;  // '\|' for US
constexpr uint16_t VK_OEM_6          = 0xDD;  // ']}' for US
constexpr uint16_t VK_OEM_7          = 0xDE;  // ''"' for US
constexpr uint16_t VK_OEM_102        = 0xE2;  // "<>" or "\|" on RT 102-key kbd

namespace {

// Pre-built hash map for evdev -> YAMY conversion (O(1) lookup)
const std::unordered_map<uint16_t, uint16_t> g_evdevToYamyMap = {
    // Letters (A-Z)
    {KEY_A, VK_A}, {KEY_B, VK_B}, {KEY_C, VK_C}, {KEY_D, VK_D},
    {KEY_E, VK_E}, {KEY_F, VK_F}, {KEY_G, VK_G}, {KEY_H, VK_H},
    {KEY_I, VK_I}, {KEY_J, VK_J}, {KEY_K, VK_K}, {KEY_L, VK_L},
    {KEY_M, VK_M}, {KEY_N, VK_N}, {KEY_O, VK_O}, {KEY_P, VK_P},
    {KEY_Q, VK_Q}, {KEY_R, VK_R}, {KEY_S, VK_S}, {KEY_T, VK_T},
    {KEY_U, VK_U}, {KEY_V, VK_V}, {KEY_W, VK_W}, {KEY_X, VK_X},
    {KEY_Y, VK_Y}, {KEY_Z, VK_Z},

    // Numbers (0-9)
    {KEY_0, VK_0}, {KEY_1, VK_1}, {KEY_2, VK_2}, {KEY_3, VK_3},
    {KEY_4, VK_4}, {KEY_5, VK_5}, {KEY_6, VK_6}, {KEY_7, VK_7},
    {KEY_8, VK_8}, {KEY_9, VK_9},

    // Function keys (F1-F24)
    {KEY_F1, VK_F1}, {KEY_F2, VK_F2}, {KEY_F3, VK_F3}, {KEY_F4, VK_F4},
    {KEY_F5, VK_F5}, {KEY_F6, VK_F6}, {KEY_F7, VK_F7}, {KEY_F8, VK_F8},
    {KEY_F9, VK_F9}, {KEY_F10, VK_F10}, {KEY_F11, VK_F11}, {KEY_F12, VK_F12},
    {KEY_F13, VK_F13}, {KEY_F14, VK_F14}, {KEY_F15, VK_F15}, {KEY_F16, VK_F16},
    {KEY_F17, VK_F17}, {KEY_F18, VK_F18}, {KEY_F19, VK_F19}, {KEY_F20, VK_F20},
    {KEY_F21, VK_F21}, {KEY_F22, VK_F22}, {KEY_F23, VK_F23}, {KEY_F24, VK_F24},

    // Modifiers
    {KEY_LEFTSHIFT, VK_LSHIFT}, {KEY_RIGHTSHIFT, VK_RSHIFT},
    {KEY_LEFTCTRL, VK_LCONTROL}, {KEY_RIGHTCTRL, VK_RCONTROL},
    {KEY_LEFTALT, VK_LMENU}, {KEY_RIGHTALT, VK_RMENU},
    {KEY_LEFTMETA, VK_LWIN}, {KEY_RIGHTMETA, VK_RWIN},

    // Special keys
    {KEY_ESC, VK_ESCAPE}, {KEY_TAB, VK_TAB}, {KEY_CAPSLOCK, VK_CAPITAL},
    {KEY_ENTER, VK_RETURN}, {KEY_BACKSPACE, VK_BACK}, {KEY_SPACE, VK_SPACE},
    {KEY_INSERT, VK_INSERT}, {KEY_DELETE, VK_DELETE},
    {KEY_HOME, VK_HOME}, {KEY_END, VK_END},
    {KEY_PAGEUP, VK_PRIOR}, {KEY_PAGEDOWN, VK_NEXT},

    // Arrow keys
    {KEY_LEFT, VK_LEFT}, {KEY_RIGHT, VK_RIGHT},
    {KEY_UP, VK_UP}, {KEY_DOWN, VK_DOWN},

    // Lock keys
    {KEY_NUMLOCK, VK_NUMLOCK}, {KEY_SCROLLLOCK, VK_SCROLL},

    // Numpad
    {KEY_KP0, VK_NUMPAD0}, {KEY_KP1, VK_NUMPAD1}, {KEY_KP2, VK_NUMPAD2},
    {KEY_KP3, VK_NUMPAD3}, {KEY_KP4, VK_NUMPAD4}, {KEY_KP5, VK_NUMPAD5},
    {KEY_KP6, VK_NUMPAD6}, {KEY_KP7, VK_NUMPAD7}, {KEY_KP8, VK_NUMPAD8},
    {KEY_KP9, VK_NUMPAD9}, {KEY_KPASTERISK, VK_MULTIPLY}, {KEY_KPPLUS, VK_ADD},
    {KEY_KPMINUS, VK_SUBTRACT}, {KEY_KPDOT, VK_DECIMAL},
    {KEY_KPSLASH, VK_DIVIDE}, {KEY_KPENTER, VK_RETURN},

    // Punctuation
    {KEY_MINUS, VK_OEM_MINUS}, {KEY_EQUAL, VK_OEM_PLUS},
    {KEY_LEFTBRACE, VK_OEM_4}, {KEY_RIGHTBRACE, VK_OEM_6},
    {KEY_SEMICOLON, VK_OEM_1}, {KEY_APOSTROPHE, VK_OEM_7},
    {KEY_GRAVE, VK_OEM_3}, {KEY_BACKSLASH, VK_OEM_5},
    {KEY_COMMA, VK_OEM_COMMA}, {KEY_DOT, VK_OEM_PERIOD},
    {KEY_SLASH, VK_OEM_2}, {KEY_102ND, VK_OEM_102},

    // Other
    {KEY_SYSRQ, VK_SNAPSHOT}, {KEY_PAUSE, VK_PAUSE}, {KEY_MENU, VK_APPS}
};

// Pre-built hash map for YAMY -> evdev conversion (O(1) lookup)
const std::unordered_map<uint16_t, uint16_t> g_yamyToEvdevMap = {
    // Letters (A-Z)
    {VK_A, KEY_A}, {VK_B, KEY_B}, {VK_C, KEY_C}, {VK_D, KEY_D},
    {VK_E, KEY_E}, {VK_F, KEY_F}, {VK_G, KEY_G}, {VK_H, KEY_H},
    {VK_I, KEY_I}, {VK_J, KEY_J}, {VK_K, KEY_K}, {VK_L, KEY_L},
    {VK_M, KEY_M}, {VK_N, KEY_N}, {VK_O, KEY_O}, {VK_P, KEY_P},
    {VK_Q, KEY_Q}, {VK_R, KEY_R}, {VK_S, KEY_S}, {VK_T, KEY_T},
    {VK_U, KEY_U}, {VK_V, KEY_V}, {VK_W, KEY_W}, {VK_X, KEY_X},
    {VK_Y, KEY_Y}, {VK_Z, KEY_Z},

    // Numbers (0-9)
    {VK_0, KEY_0}, {VK_1, KEY_1}, {VK_2, KEY_2}, {VK_3, KEY_3},
    {VK_4, KEY_4}, {VK_5, KEY_5}, {VK_6, KEY_6}, {VK_7, KEY_7},
    {VK_8, KEY_8}, {VK_9, KEY_9},

    // Function keys (F1-F24)
    {VK_F1, KEY_F1}, {VK_F2, KEY_F2}, {VK_F3, KEY_F3}, {VK_F4, KEY_F4},
    {VK_F5, KEY_F5}, {VK_F6, KEY_F6}, {VK_F7, KEY_F7}, {VK_F8, KEY_F8},
    {VK_F9, KEY_F9}, {VK_F10, KEY_F10}, {VK_F11, KEY_F11}, {VK_F12, KEY_F12},
    {VK_F13, KEY_F13}, {VK_F14, KEY_F14}, {VK_F15, KEY_F15}, {VK_F16, KEY_F16},
    {VK_F17, KEY_F17}, {VK_F18, KEY_F18}, {VK_F19, KEY_F19}, {VK_F20, KEY_F20},
    {VK_F21, KEY_F21}, {VK_F22, KEY_F22}, {VK_F23, KEY_F23}, {VK_F24, KEY_F24},

    // Modifiers
    {VK_LSHIFT, KEY_LEFTSHIFT}, {VK_RSHIFT, KEY_RIGHTSHIFT},
    {VK_LCONTROL, KEY_LEFTCTRL}, {VK_RCONTROL, KEY_RIGHTCTRL},
    {VK_LMENU, KEY_LEFTALT}, {VK_RMENU, KEY_RIGHTALT},
    {VK_LWIN, KEY_LEFTMETA}, {VK_RWIN, KEY_RIGHTMETA},

    // Special keys
    {VK_ESCAPE, KEY_ESC}, {VK_TAB, KEY_TAB}, {VK_CAPITAL, KEY_CAPSLOCK},
    {VK_RETURN, KEY_ENTER}, {VK_BACK, KEY_BACKSPACE}, {VK_SPACE, KEY_SPACE},
    {VK_INSERT, KEY_INSERT}, {VK_DELETE, KEY_DELETE},
    {VK_HOME, KEY_HOME}, {VK_END, KEY_END},
    {VK_PRIOR, KEY_PAGEUP}, {VK_NEXT, KEY_PAGEDOWN},

    // Arrow keys
    {VK_LEFT, KEY_LEFT}, {VK_RIGHT, KEY_RIGHT},
    {VK_UP, KEY_UP}, {VK_DOWN, KEY_DOWN},

    // Lock keys
    {VK_NUMLOCK, KEY_NUMLOCK}, {VK_SCROLL, KEY_SCROLLLOCK},

    // Numpad
    {VK_NUMPAD0, KEY_KP0}, {VK_NUMPAD1, KEY_KP1}, {VK_NUMPAD2, KEY_KP2},
    {VK_NUMPAD3, KEY_KP3}, {VK_NUMPAD4, KEY_KP4}, {VK_NUMPAD5, KEY_KP5},
    {VK_NUMPAD6, KEY_KP6}, {VK_NUMPAD7, KEY_KP7}, {VK_NUMPAD8, KEY_KP8},
    {VK_NUMPAD9, KEY_KP9}, {VK_MULTIPLY, KEY_KPASTERISK}, {VK_ADD, KEY_KPPLUS},
    {VK_SUBTRACT, KEY_KPMINUS}, {VK_DECIMAL, KEY_KPDOT},
    {VK_DIVIDE, KEY_KPSLASH},

    // Punctuation
    {VK_OEM_MINUS, KEY_MINUS}, {VK_OEM_PLUS, KEY_EQUAL},
    {VK_OEM_4, KEY_LEFTBRACE}, {VK_OEM_6, KEY_RIGHTBRACE},
    {VK_OEM_1, KEY_SEMICOLON}, {VK_OEM_7, KEY_APOSTROPHE},
    {VK_OEM_3, KEY_GRAVE}, {VK_OEM_5, KEY_BACKSLASH},
    {VK_OEM_COMMA, KEY_COMMA}, {VK_OEM_PERIOD, KEY_DOT},
    {VK_OEM_2, KEY_SLASH}, {VK_OEM_102, KEY_102ND},

    // Other
    {VK_SNAPSHOT, KEY_SYSRQ}, {VK_PAUSE, KEY_PAUSE}, {VK_APPS, KEY_MENU}
};

} // anonymous namespace

uint16_t evdevToYamyKeyCode(uint16_t evdev_code) {
    auto it = g_evdevToYamyMap.find(evdev_code);
    return (it != g_evdevToYamyMap.end()) ? it->second : 0;
}

uint16_t yamyToEvdevKeyCode(uint16_t yamy_code) {
    auto it = g_yamyToEvdevMap.find(yamy_code);
    return (it != g_yamyToEvdevMap.end()) ? it->second : 0;
}

bool isModifierKey(uint16_t evdev_code) {
    switch (evdev_code) {
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL:
        case KEY_LEFTALT:
        case KEY_RIGHTALT:
        case KEY_LEFTMETA:
        case KEY_RIGHTMETA:
        case KEY_CAPSLOCK:
        case KEY_NUMLOCK:
        case KEY_SCROLLLOCK:
            return true;
        default:
            return false;
    }
}

const char* getKeyName(uint16_t evdev_code) {
    switch (evdev_code) {
        case KEY_A: return "A";
        case KEY_B: return "B";
        case KEY_C: return "C";
        case KEY_D: return "D";
        case KEY_E: return "E";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_I: return "I";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_M: return "M";
        case KEY_N: return "N";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_Q: return "Q";
        case KEY_R: return "R";
        case KEY_S: return "S";
        case KEY_T: return "T";
        case KEY_U: return "U";
        case KEY_V: return "V";
        case KEY_W: return "W";
        case KEY_X: return "X";
        case KEY_Y: return "Y";
        case KEY_Z: return "Z";
        case KEY_1: return "1";
        case KEY_2: return "2";
        case KEY_3: return "3";
        case KEY_4: return "4";
        case KEY_5: return "5";
        case KEY_6: return "6";
        case KEY_7: return "7";
        case KEY_8: return "8";
        case KEY_9: return "9";
        case KEY_0: return "0";
        case KEY_ESC: return "ESC";
        case KEY_TAB: return "TAB";
        case KEY_CAPSLOCK: return "CAPSLOCK";
        case KEY_ENTER: return "ENTER";
        case KEY_BACKSPACE: return "BACKSPACE";
        case KEY_SPACE: return "SPACE";
        case KEY_LEFTSHIFT: return "LSHIFT";
        case KEY_RIGHTSHIFT: return "RSHIFT";
        case KEY_LEFTCTRL: return "LCTRL";
        case KEY_RIGHTCTRL: return "RCTRL";
        case KEY_LEFTALT: return "LALT";
        case KEY_RIGHTALT: return "RALT";
        case KEY_LEFTMETA: return "LWIN";
        case KEY_RIGHTMETA: return "RWIN";
        case KEY_F1: return "F1";
        case KEY_F2: return "F2";
        case KEY_F3: return "F3";
        case KEY_F4: return "F4";
        case KEY_F5: return "F5";
        case KEY_F6: return "F6";
        case KEY_F7: return "F7";
        case KEY_F8: return "F8";
        case KEY_F9: return "F9";
        case KEY_F10: return "F10";
        case KEY_F11: return "F11";
        case KEY_F12: return "F12";
        default: return "UNKNOWN";
    }
}

} // namespace yamy::platform
