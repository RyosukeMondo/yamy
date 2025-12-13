//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keycode_mapping.cpp - evdev ↔ YAMY keycode translation (Track 11)
// Optimized with std::unordered_map for O(1) constant-time lookups

#include "keycode_mapping.h"
#include "../../utils/platform_logger.h"
#include <linux/input-event-codes.h>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstdio>

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
    // Letters (A-Z) - using SCAN CODES from 109.mayu, not VK codes
    {KEY_A, 0x1E}, {KEY_B, 0x30}, {KEY_C, 0x2E}, {KEY_D, 0x20},
    {KEY_E, 0x12}, {KEY_F, 0x21}, {KEY_G, 0x22}, {KEY_H, 0x23},
    {KEY_I, 0x17}, {KEY_J, 0x24}, {KEY_K, 0x25}, {KEY_L, 0x26},
    {KEY_M, 0x32}, {KEY_N, 0x31}, {KEY_O, 0x18}, {KEY_P, 0x19},
    {KEY_Q, 0x10}, {KEY_R, 0x13}, {KEY_S, 0x1F}, {KEY_T, 0x14},
    {KEY_U, 0x16}, {KEY_V, 0x2F}, {KEY_W, 0x11}, {KEY_X, 0x2D},
    {KEY_Y, 0x15}, {KEY_Z, 0x2C},

    // Numbers (0-9) - using SCAN CODES from 109.mayu
    {KEY_0, 0x0B}, {KEY_1, 0x02}, {KEY_2, 0x03}, {KEY_3, 0x04},
    {KEY_4, 0x05}, {KEY_5, 0x06}, {KEY_6, 0x07}, {KEY_7, 0x08},
    {KEY_8, 0x09}, {KEY_9, 0x0A},

    // Function keys (F1-F12) - using SCAN CODES from 109.mayu
    {KEY_F1, 0x3B}, {KEY_F2, 0x3C}, {KEY_F3, 0x3D}, {KEY_F4, 0x3E},
    {KEY_F5, 0x3F}, {KEY_F6, 0x40}, {KEY_F7, 0x41}, {KEY_F8, 0x42},
    {KEY_F9, 0x43}, {KEY_F10, 0x44}, {KEY_F11, 0x57}, {KEY_F12, 0x58},
    {KEY_F13, 0x64}, {KEY_F14, 0x65}, {KEY_F15, 0x66}, {KEY_F16, 0x67},
    {KEY_F17, 0x68}, {KEY_F18, 0x69}, {KEY_F19, 0x6A}, {KEY_F20, 0x6B},
    {KEY_F21, 0x6C}, {KEY_F22, 0x6D}, {KEY_F23, 0x6E}, {KEY_F24, 0x76},

    // Modifiers - using SCAN CODES from 109.mayu
    {KEY_LEFTSHIFT, 0x2A}, {KEY_RIGHTSHIFT, 0x36},
    {KEY_LEFTCTRL, 0x1D}, {KEY_RIGHTCTRL, 0xE01D},  // E0-extended
    {KEY_LEFTALT, 0x38}, {KEY_RIGHTALT, 0xE038},     // E0-extended
    {KEY_LEFTMETA, 0xE05B}, {KEY_RIGHTMETA, 0xE05C}, // E0-extended

    // Special keys - using SCAN CODES from 109.mayu
    {KEY_ESC, 0x01}, {KEY_TAB, 0x0F}, {KEY_CAPSLOCK, 0x3A},
    {KEY_ENTER, 0x1C}, {KEY_BACKSPACE, 0x0E}, {KEY_SPACE, 0x39},
    {KEY_INSERT, 0xE052}, {KEY_DELETE, 0xE053},  // E0-extended
    {KEY_HOME, 0xE047}, {KEY_END, 0xE04F},       // E0-extended
    {KEY_PAGEUP, 0xE049}, {KEY_PAGEDOWN, 0xE051},  // E0-extended

    // Arrow keys - using SCAN CODES from 109.mayu (E0-extended)
    {KEY_LEFT, 0xE04B}, {KEY_RIGHT, 0xE04D},
    {KEY_UP, 0xE048}, {KEY_DOWN, 0xE050},

    // Lock keys - using SCAN CODES from 109.mayu
    {KEY_NUMLOCK, 0x45}, {KEY_SCROLLLOCK, 0x46},

    // Numpad - using SCAN CODES from 109.mayu
    {KEY_KP0, 0x52}, {KEY_KP1, 0x4F}, {KEY_KP2, 0x50},
    {KEY_KP3, 0x51}, {KEY_KP4, 0x4B}, {KEY_KP5, 0x4C},
    {KEY_KP6, 0x4D}, {KEY_KP7, 0x47}, {KEY_KP8, 0x48},
    {KEY_KP9, 0x49}, {KEY_KPASTERISK, 0x37}, {KEY_KPPLUS, 0x4E},
    {KEY_KPMINUS, 0x4A}, {KEY_KPDOT, 0x53},
    {KEY_KPSLASH, 0xE035}, {KEY_KPENTER, 0xE01C},  // E0-extended

    // Punctuation - using SCAN CODES from 109.mayu
    {KEY_MINUS, 0x0C}, {KEY_EQUAL, 0x0D},  // Minus, CircumflexAccent (Caret) on JP
    {KEY_LEFTBRACE, 0x1A}, {KEY_RIGHTBRACE, 0x1B},  // @`, [{
    {KEY_SEMICOLON, 0x27}, {KEY_APOSTROPHE, 0x28},  // ;+, :*
    {KEY_GRAVE, 0x29}, {KEY_BACKSLASH, 0x2B},  // Kanji, ]}
    {KEY_COMMA, 0x33}, {KEY_DOT, 0x34},  // ,<, .>
    {KEY_SLASH, 0x35}, {KEY_102ND, 0x56},  // /?, Extra key

    // Other special keys
    {KEY_SYSRQ, 0xE037}, {KEY_PAUSE, 0xE11D}, {KEY_MENU, 0xE05D}
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

// Scan code to evdev mapping for US keyboard layout
// Standard PC/AT scan codes used by YAMY internally - COMPLETE mapping
const std::unordered_map<uint16_t, uint16_t> g_scanToEvdevMap_US = {
    // Row 1
    {0x01, KEY_ESC}, {0x02, KEY_1}, {0x03, KEY_2}, {0x04, KEY_3},
    {0x05, KEY_4}, {0x06, KEY_5}, {0x07, KEY_6}, {0x08, KEY_7},
    {0x09, KEY_8}, {0x0A, KEY_9}, {0x0B, KEY_0}, {0x0C, KEY_MINUS},
    {0x0D, KEY_EQUAL}, {0x0E, KEY_BACKSPACE},

    // Row 2
    {0x0F, KEY_TAB}, {0x10, KEY_Q}, {0x11, KEY_W}, {0x12, KEY_E},
    {0x13, KEY_R}, {0x14, KEY_T}, {0x15, KEY_Y}, {0x16, KEY_U},
    {0x17, KEY_I}, {0x18, KEY_O}, {0x19, KEY_P}, {0x1A, KEY_LEFTBRACE},
    {0x1B, KEY_RIGHTBRACE}, {0x1C, KEY_ENTER},

    // Row 3 (Modifiers + letters)
    {0x1D, KEY_LEFTCTRL}, {0x1E, KEY_A}, {0x1F, KEY_S}, {0x20, KEY_D},
    {0x21, KEY_F}, {0x22, KEY_G}, {0x23, KEY_H}, {0x24, KEY_J},
    {0x25, KEY_K}, {0x26, KEY_L}, {0x27, KEY_SEMICOLON}, {0x28, KEY_APOSTROPHE},
    {0x29, KEY_GRAVE},

    // Row 4
    {0x2A, KEY_LEFTSHIFT}, {0x2B, KEY_BACKSLASH}, {0x2C, KEY_Z}, {0x2D, KEY_X},
    {0x2E, KEY_C}, {0x2F, KEY_V}, {0x30, KEY_B}, {0x31, KEY_N},
    {0x32, KEY_M}, {0x33, KEY_COMMA}, {0x34, KEY_DOT}, {0x35, KEY_SLASH},
    {0x36, KEY_RIGHTSHIFT},

    // Row 5
    {0x37, KEY_KPASTERISK},  // Numpad * (shared with PrintScreen on some keyboards)
    {0x38, KEY_LEFTALT}, {0x39, KEY_SPACE}, {0x3A, KEY_CAPSLOCK},

    // Function keys
    {0x3B, KEY_F1}, {0x3C, KEY_F2}, {0x3D, KEY_F3}, {0x3E, KEY_F4},
    {0x3F, KEY_F5}, {0x40, KEY_F6}, {0x41, KEY_F7}, {0x42, KEY_F8},
    {0x43, KEY_F9}, {0x44, KEY_F10},

    // Lock keys
    {0x45, KEY_NUMLOCK}, {0x46, KEY_SCROLLLOCK},

    // Numpad
    {0x47, KEY_KP7}, {0x48, KEY_KP8}, {0x49, KEY_KP9}, {0x4A, KEY_KPMINUS},
    {0x4B, KEY_KP4}, {0x4C, KEY_KP5}, {0x4D, KEY_KP6}, {0x4E, KEY_KPPLUS},
    {0x4F, KEY_KP1}, {0x50, KEY_KP2}, {0x51, KEY_KP3},
    {0x52, KEY_KP0}, {0x53, KEY_KPDOT},

    // Extended function keys
    {0x57, KEY_F11}, {0x58, KEY_F12},

    // E0-extended keys (0xE0XX format)
    {0xE01C, KEY_KPENTER},     // Numpad Enter
    {0xE01D, KEY_RIGHTCTRL},   // Right Ctrl
    {0xE035, KEY_KPSLASH},     // Numpad /
    {0xE037, KEY_SYSRQ},       // PrintScreen
    {0xE038, KEY_RIGHTALT},    // Right Alt
    {0xE047, KEY_HOME},        // Home
    {0xE048, KEY_UP},          // Up Arrow
    {0xE049, KEY_PAGEUP},      // Page Up
    {0xE04B, KEY_LEFT},        // Left Arrow
    {0xE04D, KEY_RIGHT},       // Right Arrow
    {0xE04F, KEY_END},         // End
    {0xE050, KEY_DOWN},        // Down Arrow
    {0xE051, KEY_PAGEDOWN},    // Page Down
    {0xE052, KEY_INSERT},      // Insert
    {0xE053, KEY_DELETE},      // Delete
    {0xE05B, KEY_LEFTMETA},    // Left Windows/Super
    {0xE05C, KEY_RIGHTMETA},   // Right Windows/Super
    {0xE05D, KEY_MENU},        // Menu/Apps key
    {0xE05F, KEY_SLEEP}        // Sleep
};

// Scan code to evdev mapping for Japanese 106/109 keyboard layout
// Based on 109.mayu definitions - key positions are different from US
const std::unordered_map<uint16_t, uint16_t> g_scanToEvdevMap_JP = {
    // Row 1 - Number row
    {0x01, KEY_ESC},
    {0x02, KEY_1},      // 1!
    {0x03, KEY_2},      // 2"
    {0x04, KEY_3},      // 3#
    {0x05, KEY_4},      // 4$
    {0x06, KEY_5},      // 5%
    {0x07, KEY_6},      // 6&
    {0x08, KEY_7},      // 7'
    {0x09, KEY_8},      // 8(
    {0x0A, KEY_9},      // 9)
    {0x0B, KEY_0},      // 0
    {0x0C, KEY_MINUS},  // -=
    {0x0D, KEY_EQUAL},  // ^~ (Circumflex on JP, but evdev code is same position)
    {0x0E, KEY_BACKSPACE},

    // Row 2 - QWERTY row
    {0x0F, KEY_TAB},
    {0x10, KEY_Q},
    {0x11, KEY_W},
    {0x12, KEY_E},
    {0x13, KEY_R},
    {0x14, KEY_T},
    {0x15, KEY_Y},
    {0x16, KEY_U},
    {0x17, KEY_I},
    {0x18, KEY_O},
    {0x19, KEY_P},
    {0x1A, KEY_LEFTBRACE},   // @` on JP keyboard (CommercialAt)
    {0x1B, KEY_RIGHTBRACE},  // [{ on JP keyboard
    {0x1C, KEY_ENTER},

    // Row 3 - ASDF row
    {0x1D, KEY_LEFTCTRL},
    {0x1E, KEY_A},
    {0x1F, KEY_S},
    {0x20, KEY_D},
    {0x21, KEY_F},
    {0x22, KEY_G},
    {0x23, KEY_H},
    {0x24, KEY_J},
    {0x25, KEY_K},
    {0x26, KEY_L},
    {0x27, KEY_SEMICOLON},   // ;+ on JP
    {0x28, KEY_APOSTROPHE},  // :* on JP (Colon)
    {0x29, KEY_GRAVE},       // 半角/全角 (Kanji) - using grave position

    // Row 4 - ZXCV row
    {0x2A, KEY_LEFTSHIFT},
    {0x2B, KEY_BACKSLASH},   // ]} on JP keyboard (RightSquareBracket)
    {0x2C, KEY_Z},
    {0x2D, KEY_X},
    {0x2E, KEY_C},
    {0x2F, KEY_V},
    {0x30, KEY_B},
    {0x31, KEY_N},
    {0x32, KEY_M},
    {0x33, KEY_COMMA},       // ,<
    {0x34, KEY_DOT},         // .>
    {0x35, KEY_SLASH},       // /?
    {0x36, KEY_RIGHTSHIFT},

    // Row 5
    {0x37, KEY_KPASTERISK},  // Numpad *
    {0x38, KEY_LEFTALT},
    {0x39, KEY_SPACE},
    {0x3A, KEY_CAPSLOCK},    // 英数 (Eisuu) - using capslock position

    // Function keys
    {0x3B, KEY_F1}, {0x3C, KEY_F2}, {0x3D, KEY_F3}, {0x3E, KEY_F4},
    {0x3F, KEY_F5}, {0x40, KEY_F6}, {0x41, KEY_F7}, {0x42, KEY_F8},
    {0x43, KEY_F9}, {0x44, KEY_F10},

    // Lock keys
    {0x45, KEY_NUMLOCK}, {0x46, KEY_SCROLLLOCK},

    // Numpad
    {0x47, KEY_KP7}, {0x48, KEY_KP8}, {0x49, KEY_KP9}, {0x4A, KEY_KPMINUS},
    {0x4B, KEY_KP4}, {0x4C, KEY_KP5}, {0x4D, KEY_KP6}, {0x4E, KEY_KPPLUS},
    {0x4F, KEY_KP1}, {0x50, KEY_KP2}, {0x51, KEY_KP3},
    {0x52, KEY_KP0}, {0x53, KEY_KPDOT},

    // Extended function keys
    {0x57, KEY_F11}, {0x58, KEY_F12},

    // Japanese-specific keys
    {0x70, KEY_KATAKANAHIRAGANA},  // ひらがな (Hiragana)
    {0x73, KEY_RO},                 // ＼_ (ReverseSolidus/BackSlash)
    {0x79, KEY_HENKAN},             // 変換 (Convert)
    {0x7B, KEY_MUHENKAN},           // 無変換 (NonConvert)
    {0x7D, KEY_YEN},                // \| (YenSign)

    // E0-extended keys (same as US layout)
    {0xE01C, KEY_KPENTER},     // Numpad Enter
    {0xE01D, KEY_RIGHTCTRL},   // Right Ctrl
    {0xE035, KEY_KPSLASH},     // Numpad /
    {0xE037, KEY_SYSRQ},       // PrintScreen
    {0xE038, KEY_RIGHTALT},    // Right Alt
    {0xE047, KEY_HOME},        // Home
    {0xE048, KEY_UP},          // Up Arrow
    {0xE049, KEY_PAGEUP},      // Page Up
    {0xE04B, KEY_LEFT},        // Left Arrow
    {0xE04D, KEY_RIGHT},       // Right Arrow
    {0xE04F, KEY_END},         // End
    {0xE050, KEY_DOWN},        // Down Arrow
    {0xE051, KEY_PAGEDOWN},    // Page Down
    {0xE052, KEY_INSERT},      // Insert
    {0xE053, KEY_DELETE},      // Delete
    {0xE05B, KEY_LEFTMETA},    // Left Windows/Super
    {0xE05C, KEY_RIGHTMETA},   // Right Windows/Super
    {0xE05D, KEY_MENU},        // Menu/Apps key
    {0xE05F, KEY_SLEEP}        // Sleep
};

} // anonymous namespace

// Global layout override (set from config file)
static std::string g_layoutOverride = "";

// Set layout override from config
void setLayoutOverride(const std::string& layout) {
    g_layoutOverride = layout;
    PLATFORM_LOG_INFO("keycode", "Layout override set to: %s", layout.c_str());
}

// Clear layout override (use auto-detection)
void clearLayoutOverride() {
    g_layoutOverride = "";
    PLATFORM_LOG_INFO("keycode", "Layout override cleared, using auto-detection");
}

// Detect current keyboard layout
std::string detectKeyboardLayout() {
    static std::string cachedLayout;
    static bool layoutDetected = false;

    // Check for config file override first
    if (!g_layoutOverride.empty()) {
        PLATFORM_LOG_INFO("keycode", "Using layout from config: %s", g_layoutOverride.c_str());
        return g_layoutOverride;
    }

    // Return cached layout if already detected
    if (layoutDetected) {
        return cachedLayout;
    }

    // Query current layout using setxkbmap
    FILE* pipe = popen("setxkbmap -query 2>/dev/null | grep 'layout:' | awk '{print $2}'", "r");
    if (!pipe) {
        cachedLayout = "us";  // Default to US if detection fails
        layoutDetected = true;
        return cachedLayout;
    }

    char buffer[128];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
        // Remove newline
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    }
    pclose(pipe);

    // Parse layout (handle "jp,us" format - first one is active)
    if (result.empty()) {
        cachedLayout = "us";
    } else if (result.find("jp") == 0 || result.find(",jp") != std::string::npos) {
        cachedLayout = "jp";
    } else {
        cachedLayout = "us";
    }

    layoutDetected = true;

    // Log detected layout
    PLATFORM_LOG_INFO("keycode", "Detected keyboard layout: %s", cachedLayout.c_str());

    return cachedLayout;
}

uint16_t evdevToYamyKeyCode(uint16_t evdev_code, int event_type) {
    static bool debug_logging = (std::getenv("YAMY_DEBUG_KEYCODE") != nullptr);

    // Convert event type to string
    const char* event_type_str = "UNKNOWN";
    if (event_type == 0) {
        event_type_str = "RELEASE";
    } else if (event_type == 1) {
        event_type_str = "PRESS";
    } else if (event_type == 2) {
        event_type_str = "REPEAT";
    }

    // Lookup in map
    auto it = g_evdevToYamyMap.find(evdev_code);
    uint16_t result = (it != g_evdevToYamyMap.end()) ? it->second : 0;

    // Log with event type
    if (debug_logging) {
        if (result != 0) {
            PLATFORM_LOG_INFO("keycode", "[LAYER1:IN] evdev %d (%s) %s → yamy 0x%04X",
                              evdev_code, getKeyName(evdev_code), event_type_str, result);
        } else {
            PLATFORM_LOG_INFO("keycode", "[LAYER1:IN] evdev %d (%s) %s → NOT FOUND",
                              evdev_code, getKeyName(evdev_code), event_type_str);
        }
    }

    return result;
}

uint16_t yamyToEvdevKeyCode(uint16_t yamy_code) {
    static bool debug_logging = (std::getenv("YAMY_DEBUG_KEYCODE") != nullptr);

    // First try scan code mapping based on detected keyboard layout
    // This is the PRIMARY mapping since .mayu files use scan codes
    std::string layout = detectKeyboardLayout();

    auto it = (layout == "jp") ? g_scanToEvdevMap_JP.find(yamy_code) : g_scanToEvdevMap_US.find(yamy_code);

    if (it != (layout == "jp" ? g_scanToEvdevMap_JP.end() : g_scanToEvdevMap_US.end())) {
        if (debug_logging) {
            PLATFORM_LOG_INFO("keycode", "[LAYER3:OUT] yamy 0x%04X → evdev %d (%s) - Found in %s scan map",
                              yamy_code, it->second, getKeyName(it->second), layout.c_str());
        }
        return it->second;
    }

    // If not found in scan map, try VK code mapping as fallback
    // VK codes are Windows virtual keys used for special cases
    it = g_yamyToEvdevMap.find(yamy_code);
    if (it != g_yamyToEvdevMap.end()) {
        if (debug_logging) {
            PLATFORM_LOG_INFO("keycode", "[LAYER3:OUT] yamy 0x%04X → evdev %d (%s) - Found in VK map",
                              yamy_code, it->second, getKeyName(it->second));
        }
        return it->second;
    }

    // Not found in either map
    if (debug_logging) {
        PLATFORM_LOG_INFO("keycode", "[LAYER3:OUT] yamy 0x%04X → NOT FOUND in any map", yamy_code);
    }
    return 0;
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
