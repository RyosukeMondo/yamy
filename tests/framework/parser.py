
import re
import os
from typing import List, Optional
from dataclasses import dataclass

# Try to import evdev for reading output
try:
    import evdev
    from evdev import ecodes, InputDevice, list_devices
except ImportError:
    print("ERROR: python3-evdev is required for output capture.")
    # This is a library, so we should not exit here.
    # The user of the library should handle this case.
    ecodes = None


@dataclass
class KeyMapping:
    input_key: str
    output_key: str
    input_evdev: int
    output_evdev: int


class KeyCodeMapper:
    # Use evdev.ecodes if possible, simplified here
    KEY_MAP = {}
    if ecodes:
        KEY_MAP = {
            'A': ecodes.KEY_A, 'B': ecodes.KEY_B, 'C': ecodes.KEY_C, 'D': ecodes.KEY_D,
            'E': ecodes.KEY_E, 'F': ecodes.KEY_F, 'G': ecodes.KEY_G, 'H': ecodes.KEY_H,
            'I': ecodes.KEY_I, 'J': ecodes.KEY_J, 'K': ecodes.KEY_K, 'L': ecodes.KEY_L,
            'M': ecodes.KEY_M, 'N': ecodes.KEY_N, 'O': ecodes.KEY_O, 'P': ecodes.KEY_P,
            'Q': ecodes.KEY_Q, 'R': ecodes.KEY_R, 'S': ecodes.KEY_S, 'T': ecodes.KEY_T,
            'U': ecodes.KEY_U, 'V': ecodes.KEY_V, 'W': ecodes.KEY_W, 'X': ecodes.KEY_X,
            'Y': ecodes.KEY_Y, 'Z': ecodes.KEY_Z,
            '_0': ecodes.KEY_0, '_1': ecodes.KEY_1, '_2': ecodes.KEY_2, '_3': ecodes.KEY_3,
            '_4': ecodes.KEY_4, '_5': ecodes.KEY_5, '_6': ecodes.KEY_6, '_7': ecodes.KEY_7,
            '_8': ecodes.KEY_8, '_9': ecodes.KEY_9,
            'F1': ecodes.KEY_F1, 'F2': ecodes.KEY_F2, 'F3': ecodes.KEY_F3, 'F4': ecodes.KEY_F4,
            'F5': ecodes.KEY_F5, 'F6': ecodes.KEY_F6, 'F7': ecodes.KEY_F7, 'F8': ecodes.KEY_F8,
            'F9': ecodes.KEY_F9, 'F10': ecodes.KEY_F10, 'F11': ecodes.KEY_F11, 'F12': ecodes.KEY_F12,
            'Tab': ecodes.KEY_TAB, 'Enter': ecodes.KEY_ENTER, 'Esc': ecodes.KEY_ESC,
            'Space': ecodes.KEY_SPACE, 'BackSpace': ecodes.KEY_BACKSPACE,
            'Delete': ecodes.KEY_DELETE, 'Insert': ecodes.KEY_INSERT,
            'Up': ecodes.KEY_UP, 'Down': ecodes.KEY_DOWN, 'Left': ecodes.KEY_LEFT, 'Right': ecodes.KEY_RIGHT,
            'Home': ecodes.KEY_HOME, 'End': ecodes.KEY_END, 'PageUp': ecodes.KEY_PAGEUP, 'PageDown': ecodes.KEY_PAGEDOWN,
            'LShift': ecodes.KEY_LEFTSHIFT, 'RShift': ecodes.KEY_RIGHTSHIFT,
            'LCtrl': ecodes.KEY_LEFTCTRL, 'RCtrl': ecodes.KEY_RIGHTCTRL,
            'LAlt': ecodes.KEY_LEFTALT, 'RAlt': ecodes.KEY_RIGHTALT,
            'LWin': ecodes.KEY_LEFTMETA, 'RWin': ecodes.KEY_RIGHTMETA, 'Apps': ecodes.KEY_COMPOSE,
            'Atmark': ecodes.KEY_APOSTROPHE, 'Semicolon': ecodes.KEY_SEMICOLON, 'Colon': ecodes.KEY_SEMICOLON,
            'Minus': ecodes.KEY_MINUS, 'Comma': ecodes.KEY_COMMA, 'Period': ecodes.KEY_DOT,
            'Slash': ecodes.KEY_SLASH, 'ReverseSolidus': ecodes.KEY_BACKSLASH,
            'Yen': ecodes.KEY_YEN, 'NonConvert': ecodes.KEY_MUHENKAN, 'Convert': ecodes.KEY_HENKAN,
            'Hiragana': ecodes.KEY_HIRAGANA, 'Kanji': ecodes.KEY_GRAVE, 'Eisuu': ecodes.KEY_KATAKANAHIRAGANA,
            'NumLock': ecodes.KEY_NUMLOCK, 'ScrollLock': ecodes.KEY_SCROLLLOCK, 'CapsLock': ecodes.KEY_CAPSLOCK,
        }

    @classmethod
    def get_evdev_code(cls, key_name: str) -> Optional[int]:
        return cls.KEY_MAP.get(key_name)

    @classmethod
    def get_key_name(cls, evdev_code: int) -> str:
        if not ecodes:
            return f"UNKNOWN_{evdev_code}"
        try:
            return evdev.ecodes.keys[evdev_code]
        except:
            for name, code in cls.KEY_MAP.items():
                if code == evdev_code:
                    return name
            return f"UNKNOWN_{evdev_code}"


class MayuParser:
    SUBST_PATTERN = re.compile(r'def\s+subst\s+\*(\S+)\s*=\s*\*(\S+)')
    INCLUDE_PATTERN = re.compile(r'^\s*include\s+"([^"]+)"')

    def __init__(self, config_path: str):
        self.config_path = os.path.abspath(config_path)
        self.base_dir = os.path.dirname(self.config_path)

    def parse(self) -> List[KeyMapping]:
        return self._parse_file(self.config_path)

    def _parse_file(self, file_path: str) -> List[KeyMapping]:
        mappings = []
        try:
            # Handle relative paths
            if not os.path.isabs(file_path):
                 file_path = os.path.join(self.base_dir, file_path)

            with open(file_path, 'r', encoding='utf-8') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.split('#')[0].strip()
                    if not line: continue

                    # Check for include
                    include_match = self.INCLUDE_PATTERN.search(line)
                    if include_match:
                        included_file = include_match.group(1)
                        print(f"  [MayuParser] Including {included_file}")
                        mappings.extend(self._parse_file(included_file))
                        continue

                    # Check for subst
                    match = self.SUBST_PATTERN.search(line)
                    if match:
                        input_key = match.group(1)
                        output_key = match.group(2)
                        input_evdev = KeyCodeMapper.get_evdev_code(input_key)
                        output_evdev = KeyCodeMapper.get_evdev_code(output_key)
                        if input_evdev is not None and output_evdev is not None:
                            mappings.append(KeyMapping(input_key, output_key, input_evdev, output_evdev))
        except Exception as e:
            print(f"Error parsing config {file_path}: {e}")
        return mappings

