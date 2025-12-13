#!/usr/bin/env python3
"""
Verify scan code mappings between keycode_mapping.cpp and 109.mayu
"""

import re

# Read 109.mayu and extract scan codes
mayu_scancodes = {}
with open('../keymaps/109.mayu', 'r', encoding='utf-8') as f:
    for line in f:
        # Match lines like: def key A = 0x1e
        match = re.match(r'def key (\w+).*=\s+(0x[0-9a-fA-F]+|E0-0x[0-9a-fA-F]+)', line)
        if match:
            keyname = match.group(1)
            scancode = match.group(2)
            mayu_scancodes[keyname] = scancode

# Read keycode_mapping.cpp and extract our mappings
our_mappings = {}
with open('../src/platform/linux/keycode_mapping.cpp', 'r') as f:
    content = f.read()

    # Find g_evdevToYamyMap section
    start = content.find('const std::unordered_map<uint16_t, uint16_t> g_evdevToYamyMap = {')
    end = content.find('};', start)
    mapping_section = content[start:end]

    # Extract mappings like {KEY_A, 0x1E}
    for match in re.finditer(r'\{KEY_(\w+),\s*(0x[0-9a-fA-F]+)\}', mapping_section):
        keyname = match.group(1)
        scancode = match.group(2).upper()
        our_mappings[keyname] = scancode

# Map evdev key names to 109.mayu key names
evdev_to_mayu = {
    # Letters stay the same
    **{chr(i): chr(i) for i in range(ord('A'), ord('Z')+1)},
    # Numbers
    **{str(i): f'_{i}' for i in range(10)},
    # Special keys
    'TAB': 'Tab',
    'ENTER': 'Enter',
    'BACKSPACE': 'BS',
    'ESC': 'Esc',
    'SPACE': 'Space',
    'LEFTSHIFT': 'LShift',
    'RIGHTSHIFT': 'RShift',
    'LEFTCTRL': 'LControl',
    'LEFTALT': 'LAlt',
    'CAPSLOCK': 'Eisuu',  # CapsLock position on 109
    **{f'F{i}': f'F{i}' for i in range(1, 25)},
}

# Compare and report
print("=" * 80)
print("Scan Code Mapping Verification")
print("=" * 80)
print()
print(f"{'Key':<15} {'evdev':<15} {'Our Scan':<12} {'109 Scan':<12} {'Match'}")
print("-" * 80)

total = 0
matches = 0
mismatches = 0

for evdev_key, mayu_key in sorted(evdev_to_mayu.items()):
    if evdev_key in our_mappings and mayu_key in mayu_scancodes:
        our_scan = our_mappings[evdev_key]
        expected_scan = mayu_scancodes[mayu_key].upper()

        # Normalize E0-extended codes
        if expected_scan.startswith('E0-'):
            expected_scan = '0x' + expected_scan[3:]
            expected_scan = f"0xE0{expected_scan[2:]}"

        match = "✓" if our_scan == expected_scan else "✗"
        if our_scan == expected_scan:
            matches += 1
        else:
            mismatches += 1

        total += 1

        print(f"{mayu_key:<15} {f'KEY_{evdev_key}':<15} {our_scan:<12} {expected_scan:<12} {match}")

print()
print("=" * 80)
print(f"Total: {total}, Matches: {matches}, Mismatches: {mismatches}")
print("=" * 80)

if mismatches > 0:
    print()
    print("MISMATCHES FOUND!")
    print("These keys have incorrect scan codes in g_evdevToYamyMap")
