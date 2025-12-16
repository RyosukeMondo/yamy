#!/usr/bin/env python3
"""
Automated Keymap Testing Framework for YAMY (Refactored)
========================================================

Uses yamy-test interactive mode for reliable injection.
"""

import subprocess
import re
import sys
import time
import os
import json
import threading
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from pathlib import Path

# Try to import evdev for reading output
try:
    import evdev
    from evdev import ecodes, InputDevice, list_devices
except ImportError:
    print("ERROR: python3-evdev is required for output capture.")
    sys.exit(1)


@dataclass
class KeyMapping:
    input_key: str
    output_key: str
    input_evdev: int
    output_evdev: int


@dataclass
class TestResult:
    mapping: KeyMapping
    event_type: str
    passed: bool
    expected_evdev: int
    actual_evdev: Optional[int]
    error_message: Optional[str] = None


class KeyCodeMapper:
    # Use evdev.ecodes if possible, simplified here
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
        'Hiragana': ecodes.KEY_HIRAGANA, 'Kanji': ecodes.KEY_ZENKAKUHANKAKU, 'Eisuu': ecodes.KEY_KATAKANAHIRAGANA,
        'NumLock': ecodes.KEY_NUMLOCK, 'ScrollLock': ecodes.KEY_SCROLLLOCK, 'CapsLock': ecodes.KEY_CAPSLOCK,
    }

    @classmethod
    def get_evdev_code(cls, key_name: str) -> Optional[int]:
        return cls.KEY_MAP.get(key_name)

    @classmethod
    def get_key_name(cls, evdev_code: int) -> str:
        try:
            return evdev.ecodes.keys[evdev_code]
        except:
            for name, code in cls.KEY_MAP.items():
                if code == evdev_code:
                    return name
            return f"UNKNOWN_{evdev_code}"


class MayuParser:
    SUBST_PATTERN = re.compile(r'def\s+subst\s+\*(\S+)\s*=\s*\*(\S+)')

    def __init__(self, config_path: str):
        self.config_path = config_path

    def parse(self) -> List[KeyMapping]:
        mappings = []
        try:
            with open(self.config_path, 'r', encoding='utf-8') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.split('#')[0].strip()
                    if not line: continue
                    match = self.SUBST_PATTERN.search(line)
                    if match:
                        input_key = match.group(1)
                        output_key = match.group(2)
                        input_evdev = KeyCodeMapper.get_evdev_code(input_key)
                        output_evdev = KeyCodeMapper.get_evdev_code(output_key)
                        if input_evdev is not None and output_evdev is not None:
                            mappings.append(KeyMapping(input_key, output_key, input_evdev, output_evdev))
        except Exception as e:
            print(f"Error parsing config: {e}")
        return mappings


class TestRunner:
    def __init__(self, config_path: str):
        self.config_path = config_path
        self.injector_proc = None
        self.output_device = None
        self.mappings = []
        self.results = []
        self.yamy_process = None

    def setup(self) -> bool:
        # 1. Start yamy-test interactive
        print("[Setup] Starting yamy-test interactive...")
        self.injector_proc = subprocess.Popen(
            ['./build/bin/yamy-test', 'interactive'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        # Wait for READY
        while True:
            line = self.injector_proc.stdout.readline().strip()
            if not line:
                print(f"[Setup] Failed to start yamy-test interactive (EOF).")
                return False
            if line == "READY":
                break
            print(f"[Setup] yamy-test: {line}")
        
        print("[Setup] Virtual keyboard created and ready.")

        # 2. Restart YAMY
        print("[Setup] Restarting YAMY daemon...")
        subprocess.run(['pkill', '-9', '-x', 'yamy'], stderr=subprocess.DEVNULL)
        time.sleep(1)

        log_file = open('/tmp/yamy_test_runner.log', 'w')
        self.yamy_process = subprocess.Popen(
            ['./build/bin/yamy'],
            stdout=log_file,
            stderr=log_file
        )
        print(f"[Setup] YAMY started with PID {self.yamy_process.pid}")
        
        time.sleep(3)

        # 3. Load Config
        print(f"[Setup] Loading config: {self.config_path}")
        subprocess.run(['./build/bin/yamy-ctl', 'reload', '--config', self.config_path], check=True)
        subprocess.run(['./build/bin/yamy-ctl', 'start'], check=True)
        time.sleep(1)

        # 4. Find YAMY Output Device
        print("[Setup] Finding YAMY output device...")
        found = False
        for path in list_devices():
            try:
                d = InputDevice(path)
                if "Yamy Virtual" in d.name:
                    self.output_device = d
                    print(f"[Setup] Found output device: {d.name} at {path}")
                    found = True
                    break
            except:
                pass
        
        if not found:
            print("[Setup] ERROR: Could not find YAMY output device.")
            return False

        try:
            self.output_device.grab()
        except:
            pass

        # 5. Parse Config
        parser = MayuParser(self.config_path)
        self.mappings = parser.parse()
        print(f"[Setup] Loaded {len(self.mappings)} legacy substitutions for testing.")

        return True

    def teardown(self):
        if self.injector_proc:
            try:
                self.injector_proc.stdin.write("EXIT\n")
                self.injector_proc.stdin.close()
                self.injector_proc.terminate()
            except: pass
        
        if self.output_device:
            try:
                self.output_device.ungrab()
                self.output_device.close()
            except: pass
            
        if self.yamy_process:
            self.yamy_process.terminate()
            self.yamy_process.wait()
        
        subprocess.run(['pkill', '-9', '-x', 'yamy'], stderr=subprocess.DEVNULL)
        print("[Teardown] Cleanup complete.")

    def inject_cmd(self, cmd: str):
        if not self.injector_proc: return
        self.injector_proc.stdin.write(cmd + "\n")
        # Consume OK
        self.injector_proc.stdout.readline()

    def capture_next_key_event(self, timeout=1.0) -> Optional[tuple]:
        if not self.output_device: return None
        from select import select
        
        start = time.time()
        while time.time() - start < timeout:
            r, w, x = select([self.output_device.fd], [], [], 0.1)
            if r:
                try:
                    for event in self.output_device.read():
                        if event.type == ecodes.EV_KEY:
                            return (event.code, event.value)
                except: return None
        return None

    def drain_events(self):
        if not self.output_device: return
        from select import select
        while True:
            r, _, _ = select([self.output_device.fd], [], [], 0)
            if r:
                try:
                    list(self.output_device.read())
                except: break
            else:
                break

    def run_test(self, input_code: int, expected_code: int, description: str) -> TestResult:
        self.drain_events()

        # PRESS
        self.inject_cmd(f"PRESS {input_code}")
        
        # Capture PRESS (might be None for modifiers)
        evt_press = self.capture_next_key_event(timeout=0.2)
        
        # RELEASE
        self.inject_cmd(f"RELEASE {input_code}")

        # Capture RELEASE/TAP output
        evt_release = self.capture_next_key_event(timeout=0.5)

        passed = False
        actual = None
        error = None

        # Determine which event is the meaningful output
        # For normal keys: PRESS output is immediate (evt_press)
        # For modifiers: PRESS is suppressed, output comes on RELEASE (evt_release)
        
        target_evt = evt_press if evt_press else evt_release

        if target_evt:
            actual_code, actual_val = target_evt
            if actual_val == 1: # PRESS
                actual = actual_code
                if actual_code == expected_code:
                    passed = True
                else:
                    error = f"Expected {KeyCodeMapper.get_key_name(expected_code)}, got {KeyCodeMapper.get_key_name(actual_code)}"
            else:
                error = "Received RELEASE event as first output"
        else:
            error = "No output event received"

        return TestResult(
            mapping=KeyMapping(KeyCodeMapper.get_key_name(input_code), KeyCodeMapper.get_key_name(expected_code), input_code, expected_code),
            event_type="TAP",
            passed=passed,
            expected_evdev=expected_code,
            actual_evdev=actual,
            error_message=error
        )

    def execute(self):
        if not self.setup():
            self.teardown()
            return False

        print("\n=== Running Tests ===")
        passes = 0
        fails = 0

        # Mappings
        for m in self.mappings:
            # Skip M20-M29 (B, V, etc) as they are modifiers and need tap logic
            if m.input_key in ['B', 'V', 'M', 'X', '_1', 'LCtrl', 'C', 'Tab', 'Q', 'A']:
                 continue

            print(f"Testing {m.input_key} -> {m.output_key} ... ", end="", flush=True)
            res = self.run_test(m.input_evdev, m.output_evdev, "")
            self.results.append(res)
            if res.passed:
                print("PASS")
                passes += 1
            else:
                print(f"FAIL ({res.error_message})")
                fails += 1
            time.sleep(0.05)

        # M20 TAP
        print("Testing M20(B) TAP -> Enter ... ", end="", flush=True)
        res = self.run_test(ecodes.KEY_B, ecodes.KEY_ENTER, "M20 TAP")
        self.results.append(res)
        if res.passed:
            print("PASS")
            passes += 1
        else:
            print(f"FAIL ({res.error_message})")
            fails += 1
        
        # M20 HOLD
        print("Testing M20(B) HOLD + W -> 1(LShift) ... ", end="", flush=True)
        self.drain_events()
        
        self.inject_cmd(f"PRESS {ecodes.KEY_B}")
        self.inject_cmd(f"WAIT 300") # Hold
        self.inject_cmd(f"PRESS {ecodes.KEY_W}")
        
        # Expect LShift (because _1 -> M24 -> LShift)
        evt = self.capture_next_key_event(timeout=0.5)
        
        self.inject_cmd(f"RELEASE {ecodes.KEY_W}")
        self.inject_cmd(f"RELEASE {ecodes.KEY_B}")
        
        # Drain
        self.capture_next_key_event(timeout=0.2)
        
        if evt and evt[0] == ecodes.KEY_LEFTSHIFT:
            print("PASS")
            passes += 1
        else:
            got = KeyCodeMapper.get_key_name(evt[0]) if evt else "None"
            print(f"FAIL (Expected KEY_LEFTSHIFT, got {got})")
            fails += 1

        print("\n=== Summary ===")
        print(f"Passed: {passes}")
        print(f"Failed: {fails}")
        
        self.teardown()
        return fails == 0

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', default='keymaps/master.mayu')
    args = parser.parse_args()

    runner = TestRunner(args.config)
    success = runner.execute()
    sys.exit(0 if success else 1)
