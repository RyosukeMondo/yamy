//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// keycode_mapping.cpp - evdev ↔ YAMY keycode translation (Track 11)

#include "keycode_mapping.h"
#include <linux/input-event-codes.h>

namespace yamy::platform {

// Windows Virtual Key codes (VK_*) - define locally
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04
#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12  // ALT key
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14  // CAPS LOCK
#define VK_ESCAPE         0x1B
#define VK_SPACE          0x20
#define VK_PRIOR          0x21  // PAGE UP
#define VK_NEXT           0x22  // PAGE DOWN
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C  // PRINT SCREEN
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

// 0-9 keys
#define VK_0              0x30
#define VK_1              0x31
#define VK_2              0x32
#define VK_3              0x33
#define VK_4              0x34
#define VK_5              0x35
#define VK_6              0x36
#define VK_7              0x37
#define VK_8              0x38
#define VK_9              0x39

// A-Z keys
#define VK_A              0x41
#define VK_B              0x42
#define VK_C              0x43
#define VK_D              0x44
#define VK_E              0x45
#define VK_F              0x46
#define VK_G              0x47
#define VK_H              0x48
#define VK_I              0x49
#define VK_J              0x4A
#define VK_K              0x4B
#define VK_L              0x4C
#define VK_M              0x4D
#define VK_N              0x4E
#define VK_O              0x4F
#define VK_P              0x50
#define VK_Q              0x51
#define VK_R              0x52
#define VK_S              0x53
#define VK_T              0x54
#define VK_U              0x55
#define VK_V              0x56
#define VK_W              0x57
#define VK_X              0x58
#define VK_Y              0x59
#define VK_Z              0x5A

// Windows keys
#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D  // Context menu

// Numpad
#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F

// Function keys
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

// Lock keys
#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91  // SCROLL LOCK

// Modifier keys (left/right)
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4  // Left ALT
#define VK_RMENU          0xA5  // Right ALT

// Punctuation
#define VK_OEM_1          0xBA  // ';:' for US
#define VK_OEM_PLUS       0xBB  // '=+' for any country
#define VK_OEM_COMMA      0xBC  // ',<' for any country
#define VK_OEM_MINUS      0xBD  // '-_' for any country
#define VK_OEM_PERIOD     0xBE  // '.>' for any country
#define VK_OEM_2          0xBF  // '/?' for US
#define VK_OEM_3          0xC0  // '`~' for US
#define VK_OEM_4          0xDB  // '[{' for US
#define VK_OEM_5          0xDC  // '\|' for US
#define VK_OEM_6          0xDD  // ']}' for US
#define VK_OEM_7          0xDE  // ''"' for US
#define VK_OEM_8          0xDF
#define VK_OEM_102        0xE2  // "<>" or "\|" on RT 102-key kbd

uint16_t evdevToYamyKeyCode(uint16_t evdev_code) {
    switch (evdev_code) {
        // Letters
        case KEY_A: return VK_A;
        case KEY_B: return VK_B;
        case KEY_C: return VK_C;
        case KEY_D: return VK_D;
        case KEY_E: return VK_E;
        case KEY_F: return VK_F;
        case KEY_G: return VK_G;
        case KEY_H: return VK_H;
        case KEY_I: return VK_I;
        case KEY_J: return VK_J;
        case KEY_K: return VK_K;
        case KEY_L: return VK_L;
        case KEY_M: return VK_M;
        case KEY_N: return VK_N;
        case KEY_O: return VK_O;
        case KEY_P: return VK_P;
        case KEY_Q: return VK_Q;
        case KEY_R: return VK_R;
        case KEY_S: return VK_S;
        case KEY_T: return VK_T;
        case KEY_U: return VK_U;
        case KEY_V: return VK_V;
        case KEY_W: return VK_W;
        case KEY_X: return VK_X;
        case KEY_Y: return VK_Y;
        case KEY_Z: return VK_Z;

        // Numbers
        case KEY_1: return VK_1;
        case KEY_2: return VK_2;
        case KEY_3: return VK_3;
        case KEY_4: return VK_4;
        case KEY_5: return VK_5;
        case KEY_6: return VK_6;
        case KEY_7: return VK_7;
        case KEY_8: return VK_8;
        case KEY_9: return VK_9;
        case KEY_0: return VK_0;

        // Function keys
        case KEY_F1: return VK_F1;
        case KEY_F2: return VK_F2;
        case KEY_F3: return VK_F3;
        case KEY_F4: return VK_F4;
        case KEY_F5: return VK_F5;
        case KEY_F6: return VK_F6;
        case KEY_F7: return VK_F7;
        case KEY_F8: return VK_F8;
        case KEY_F9: return VK_F9;
        case KEY_F10: return VK_F10;
        case KEY_F11: return VK_F11;
        case KEY_F12: return VK_F12;

        // Modifiers
        case KEY_LEFTSHIFT: return VK_LSHIFT;
        case KEY_RIGHTSHIFT: return VK_RSHIFT;
        case KEY_LEFTCTRL: return VK_LCONTROL;
        case KEY_RIGHTCTRL: return VK_RCONTROL;
        case KEY_LEFTALT: return VK_LMENU;
        case KEY_RIGHTALT: return VK_RMENU;
        case KEY_LEFTMETA: return VK_LWIN;
        case KEY_RIGHTMETA: return VK_RWIN;

        // Special keys
        case KEY_ESC: return VK_ESCAPE;
        case KEY_TAB: return VK_TAB;
        case KEY_CAPSLOCK: return VK_CAPITAL;
        case KEY_ENTER: return VK_RETURN;
        case KEY_BACKSPACE: return VK_BACK;
        case KEY_SPACE: return VK_SPACE;
        case KEY_INSERT: return VK_INSERT;
        case KEY_DELETE: return VK_DELETE;
        case KEY_HOME: return VK_HOME;
        case KEY_END: return VK_END;
        case KEY_PAGEUP: return VK_PRIOR;
        case KEY_PAGEDOWN: return VK_NEXT;

        // Arrow keys
        case KEY_LEFT: return VK_LEFT;
        case KEY_RIGHT: return VK_RIGHT;
        case KEY_UP: return VK_UP;
        case KEY_DOWN: return VK_DOWN;

        // Lock keys
        case KEY_NUMLOCK: return VK_NUMLOCK;
        case KEY_SCROLLLOCK: return VK_SCROLL;

        // Numpad
        case KEY_KP0: return VK_NUMPAD0;
        case KEY_KP1: return VK_NUMPAD1;
        case KEY_KP2: return VK_NUMPAD2;
        case KEY_KP3: return VK_NUMPAD3;
        case KEY_KP4: return VK_NUMPAD4;
        case KEY_KP5: return VK_NUMPAD5;
        case KEY_KP6: return VK_NUMPAD6;
        case KEY_KP7: return VK_NUMPAD7;
        case KEY_KP8: return VK_NUMPAD8;
        case KEY_KP9: return VK_NUMPAD9;
        case KEY_KPASTERISK: return VK_MULTIPLY;
        case KEY_KPPLUS: return VK_ADD;
        case KEY_KPMINUS: return VK_SUBTRACT;
        case KEY_KPDOT: return VK_DECIMAL;
        case KEY_KPSLASH: return VK_DIVIDE;
        case KEY_KPENTER: return VK_RETURN;

        // Punctuation
        case KEY_MINUS: return VK_OEM_MINUS;
        case KEY_EQUAL: return VK_OEM_PLUS;
        case KEY_LEFTBRACE: return VK_OEM_4;
        case KEY_RIGHTBRACE: return VK_OEM_6;
        case KEY_SEMICOLON: return VK_OEM_1;
        case KEY_APOSTROPHE: return VK_OEM_7;
        case KEY_GRAVE: return VK_OEM_3;
        case KEY_BACKSLASH: return VK_OEM_5;
        case KEY_COMMA: return VK_OEM_COMMA;
        case KEY_DOT: return VK_OEM_PERIOD;
        case KEY_SLASH: return VK_OEM_2;
        case KEY_102ND: return VK_OEM_102;

        // Other
        case KEY_SYSRQ: return VK_SNAPSHOT;
        case KEY_PAUSE: return VK_PAUSE;
        case KEY_MENU: return VK_APPS;

        default: return 0;  // Unknown
    }
}

uint16_t yamyToEvdevKeyCode(uint16_t yamy_code) {
    // Reverse mapping
    switch (yamy_code) {
        // Letters
        case VK_A: return KEY_A;
        case VK_B: return KEY_B;
        case VK_C: return KEY_C;
        case VK_D: return KEY_D;
        case VK_E: return KEY_E;
        case VK_F: return KEY_F;
        case VK_G: return KEY_G;
        case VK_H: return KEY_H;
        case VK_I: return KEY_I;
        case VK_J: return KEY_J;
        case VK_K: return KEY_K;
        case VK_L: return KEY_L;
        case VK_M: return KEY_M;
        case VK_N: return KEY_N;
        case VK_O: return KEY_O;
        case VK_P: return KEY_P;
        case VK_Q: return KEY_Q;
        case VK_R: return KEY_R;
        case VK_S: return KEY_S;
        case VK_T: return KEY_T;
        case VK_U: return KEY_U;
        case VK_V: return KEY_V;
        case VK_W: return KEY_W;
        case VK_X: return KEY_X;
        case VK_Y: return KEY_Y;
        case VK_Z: return KEY_Z;

        // Numbers
        case VK_1: return KEY_1;
        case VK_2: return KEY_2;
        case VK_3: return KEY_3;
        case VK_4: return KEY_4;
        case VK_5: return KEY_5;
        case VK_6: return KEY_6;
        case VK_7: return KEY_7;
        case VK_8: return KEY_8;
        case VK_9: return KEY_9;
        case VK_0: return KEY_0;

        // Function keys
        case VK_F1: return KEY_F1;
        case VK_F2: return KEY_F2;
        case VK_F3: return KEY_F3;
        case VK_F4: return KEY_F4;
        case VK_F5: return KEY_F5;
        case VK_F6: return KEY_F6;
        case VK_F7: return KEY_F7;
        case VK_F8: return KEY_F8;
        case VK_F9: return KEY_F9;
        case VK_F10: return KEY_F10;
        case VK_F11: return KEY_F11;
        case VK_F12: return KEY_F12;

        // Modifiers
        case VK_LSHIFT: return KEY_LEFTSHIFT;
        case VK_RSHIFT: return KEY_RIGHTSHIFT;
        case VK_LCONTROL: return KEY_LEFTCTRL;
        case VK_RCONTROL: return KEY_RIGHTCTRL;
        case VK_LMENU: return KEY_LEFTALT;
        case VK_RMENU: return KEY_RIGHTALT;
        case VK_LWIN: return KEY_LEFTMETA;
        case VK_RWIN: return KEY_RIGHTMETA;

        // Special keys
        case VK_ESCAPE: return KEY_ESC;
        case VK_TAB: return KEY_TAB;
        case VK_CAPITAL: return KEY_CAPSLOCK;
        case VK_RETURN: return KEY_ENTER;
        case VK_BACK: return KEY_BACKSPACE;
        case VK_SPACE: return KEY_SPACE;
        case VK_INSERT: return KEY_INSERT;
        case VK_DELETE: return KEY_DELETE;
        case VK_HOME: return KEY_HOME;
        case VK_END: return KEY_END;
        case VK_PRIOR: return KEY_PAGEUP;
        case VK_NEXT: return KEY_PAGEDOWN;

        // Arrow keys
        case VK_LEFT: return KEY_LEFT;
        case VK_RIGHT: return KEY_RIGHT;
        case VK_UP: return KEY_UP;
        case VK_DOWN: return KEY_DOWN;

        // Lock keys
        case VK_NUMLOCK: return KEY_NUMLOCK;
        case VK_SCROLL: return KEY_SCROLLLOCK;

        // Numpad
        case VK_NUMPAD0: return KEY_KP0;
        case VK_NUMPAD1: return KEY_KP1;
        case VK_NUMPAD2: return KEY_KP2;
        case VK_NUMPAD3: return KEY_KP3;
        case VK_NUMPAD4: return KEY_KP4;
        case VK_NUMPAD5: return KEY_KP5;
        case VK_NUMPAD6: return KEY_KP6;
        case VK_NUMPAD7: return KEY_KP7;
        case VK_NUMPAD8: return KEY_KP8;
        case VK_NUMPAD9: return KEY_KP9;
        case VK_MULTIPLY: return KEY_KPASTERISK;
        case VK_ADD: return KEY_KPPLUS;
        case VK_SUBTRACT: return KEY_KPMINUS;
        case VK_DECIMAL: return KEY_KPDOT;
        case VK_DIVIDE: return KEY_KPSLASH;

        // Punctuation
        case VK_OEM_MINUS: return KEY_MINUS;
        case VK_OEM_PLUS: return KEY_EQUAL;
        case VK_OEM_4: return KEY_LEFTBRACE;
        case VK_OEM_6: return KEY_RIGHTBRACE;
        case VK_OEM_1: return KEY_SEMICOLON;
        case VK_OEM_7: return KEY_APOSTROPHE;
        case VK_OEM_3: return KEY_GRAVE;
        case VK_OEM_5: return KEY_BACKSLASH;
        case VK_OEM_COMMA: return KEY_COMMA;
        case VK_OEM_PERIOD: return KEY_DOT;
        case VK_OEM_2: return KEY_SLASH;
        case VK_OEM_102: return KEY_102ND;

        // Other
        case VK_SNAPSHOT: return KEY_SYSRQ;
        case VK_PAUSE: return KEY_PAUSE;
        case VK_APPS: return KEY_MENU;

        default: return 0;  // Unknown
    }
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
