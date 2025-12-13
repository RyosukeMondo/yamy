#!/usr/bin/env python3
"""
Real-time Keyboard Monitor - Shows what keys you press and what YAMY outputs
Run this in a terminal and type to see the transformations!
"""

import sys
import os
import subprocess
import time
from collections import deque

# ANSI colors
GREEN = '\033[92m'
RED = '\033[91m'
BLUE = '\033[94m'
YELLOW = '\033[93m'
CYAN = '\033[96m'
RESET = '\033[0m'
BOLD = '\033[1m'
CLEAR = '\033[2J\033[H'

# Key code to name mapping (common keys)
KEY_NAMES = {
    16: 'Q', 17: 'W', 18: 'E', 19: 'R', 20: 'T', 21: 'Y', 22: 'U', 23: 'I', 24: 'O', 25: 'P',
    30: 'A', 31: 'S', 32: 'D', 33: 'F', 34: 'G', 35: 'H', 36: 'J', 37: 'K', 38: 'L',
    44: 'Z', 45: 'X', 46: 'C', 47: 'V', 48: 'B', 49: 'N', 50: 'M',
    2: '1', 3: '2', 4: '3', 5: '4', 6: '5', 7: '6', 8: '7', 9: '8', 10: '9', 11: '0',
    15: 'Tab', 28: 'Enter', 14: 'Backspace', 57: 'Space', 1: 'Esc',
    12: 'Minus', 13: 'Equal', 26: 'LeftBrace', 27: 'RightBrace', 39: 'Semicolon',
    40: 'Apostrophe', 41: 'Grave', 43: 'Backslash', 51: 'Comma', 52: 'Period', 53: 'Slash',
}

# Expected mappings from config_clean.mayu (QWERTY → Output)
EXPECTED_MAPPINGS = {
    17: 'A',      # W → A
    18: 'O',      # E → O
    19: 'E',      # R → E
    20: 'U',      # T → U
    21: 'I',      # Y → I
    30: 'Tab',    # A → Tab
    48: 'Enter',  # B → Enter
    47: 'Backspace',  # V → Backspace
    16: 'Minus',  # Q → Minus
    22: 'D',      # U → D
    23: 'H',      # I → H
    24: 'T',      # O → T
    25: 'N',      # P → N
    31: 'Semicolon',  # S → Semicolon
    32: 'Q',      # D → Q
    33: 'J',      # F → J
    34: 'K',      # G → K
    35: 'X',      # H → X
    36: 'B',      # J → B
    37: 'M',      # K → M
    38: 'W',      # L → W
}

class KeyboardMonitor:
    def __init__(self):
        self.history = deque(maxlen=15)
        self.log_file = '/tmp/yamy_clean.log'

    def get_key_name(self, code):
        return KEY_NAMES.get(code, f'Key{code}')

    def tail_log(self):
        """Get last few lines from YAMY debug log"""
        try:
            result = subprocess.run(['tail', '-20', self.log_file],
                                  capture_output=True, text=True, timeout=0.5)
            return result.stdout
        except:
            return ""

    def parse_layer_output(self, log_text, input_code):
        """Parse YAMY debug log to find what was output"""
        lines = log_text.strip().split('\n')

        # Find Layer 3 output
        for line in reversed(lines):
            if '[LAYER3:OUT]' in line and 'evdev' in line:
                # Extract evdev code from line like: "[LAYER3:OUT] Found in US scan map → evdev 30 (KEY_A)"
                if '→ evdev' in line:
                    parts = line.split('→ evdev')[1].split()
                    if parts:
                        try:
                            output_code = int(parts[0])
                            key_name = self.get_key_name(output_code)
                            return output_code, key_name
                        except:
                            pass
        return None, None

    def display(self):
        """Display the monitoring interface"""
        print(CLEAR)
        print(f"{BOLD}{CYAN}╔════════════════════════════════════════════════════════════════════════════╗{RESET}")
        print(f"{BOLD}{CYAN}║              YAMY Real-Time Keyboard Monitor - Type to Test!              ║{RESET}")
        print(f"{BOLD}{CYAN}╚════════════════════════════════════════════════════════════════════════════╝{RESET}")
        print()
        print(f"{YELLOW}Instructions: Open a text editor and type keys. Watch the transformations!{RESET}")
        print(f"{YELLOW}Press Ctrl+C to exit.{RESET}")
        print()
        print(f"{BOLD}┌──────────────┬─────────────────┬─────────────────┬──────────────┐{RESET}")
        print(f"{BOLD}│ You Pressed  │  Expected Map   │  Actual Output  │    Status    │{RESET}")
        print(f"{BOLD}├──────────────┼─────────────────┼─────────────────┼──────────────┤{RESET}")

        for entry in self.history:
            print(entry)

        print(f"{BOLD}└──────────────┴─────────────────┴─────────────────┴──────────────┘{RESET}")
        print()
        print(f"{BLUE}Monitoring YAMY debug log: {self.log_file}{RESET}")

    def monitor(self):
        """Monitor keyboard events from YAMY debug log"""
        print(f"{CYAN}Starting keyboard monitor...{RESET}")
        print(f"{YELLOW}Watching: {self.log_file}{RESET}")
        print()

        # Follow the log file
        last_size = 0

        while True:
            try:
                # Check if log file has new content
                if os.path.exists(self.log_file):
                    current_size = os.path.getsize(self.log_file)

                    if current_size > last_size:
                        log_content = self.tail_log()

                        # Look for LAYER1:IN (input)
                        for line in log_content.split('\n'):
                            if '[LAYER1:IN] Input evdev code' in line:
                                # Parse input
                                try:
                                    parts = line.split('=')[1].split()
                                    input_code = int(parts[0])
                                    input_name = self.get_key_name(input_code)

                                    # Get expected mapping
                                    expected = EXPECTED_MAPPINGS.get(input_code, input_name)

                                    # Parse actual output
                                    output_code, output_name = self.parse_layer_output(log_content, input_code)

                                    # Determine status
                                    if output_name:
                                        if output_name == expected:
                                            status = f"{GREEN}✓ PASS{RESET}"
                                        else:
                                            status = f"{RED}✗ FAIL{RESET}"
                                    else:
                                        output_name = "???"
                                        status = f"{RED}✗ NO OUTPUT{RESET}"

                                    # Add to history
                                    entry = f"│ {input_name:<12} │ {expected:<15} │ {output_name:<15} │ {status:<12} │"
                                    self.history.append(entry)

                                    # Update display
                                    self.display()

                                except Exception as e:
                                    pass

                        last_size = current_size

                time.sleep(0.1)

            except KeyboardInterrupt:
                print(f"\n\n{CYAN}Monitoring stopped.{RESET}")
                break
            except Exception as e:
                print(f"{RED}Error: {e}{RESET}")
                time.sleep(1)

if __name__ == '__main__':
    monitor = KeyboardMonitor()
    monitor.display()
    monitor.monitor()
