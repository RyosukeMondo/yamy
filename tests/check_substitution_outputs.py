#!/usr/bin/env python3
"""
Check if all output scan codes from config.mayu substitutions are in g_scanToEvdevMap
"""

import re

# Read 109.mayu to get scan code definitions
mayu_scancodes = {}
with open('../keymaps/109.mayu', 'r', encoding='utf-8') as f:
    for line in f:
        match = re.match(r'def key (\w+).*=\s+((?:E0-)?0x[0-9a-fA-F]+)', line)
        if match:
            keyname = match.group(1)
            scancode = match.group(2)
            # Normalize E0-extended codes to 0xE0XX format
            if scancode.startswith('E0-'):
                scancode = '0xE0' + scancode[5:]
            mayu_scancodes[keyname] = scancode.upper()

# Read config.mayu to get substitutions
substitutions = []
with open('../keymaps/config.mayu', 'r', encoding='utf-8') as f:
    for line in f:
        # Match: def subst *A = *Tab
        match = re.match(r'def subst \*(\w+)\s*=\s*\*(\w+)', line)
        if match:
            from_key = match.group(1)
            to_key = match.group(2)
            substitutions.append((from_key, to_key))

# Read g_scanToEvdevMap_US to see what scan codes we can output
scan_map = {}
with open('../src/platform/linux/keycode_mapping.cpp', 'r') as f:
    content = f.read()

    # Find g_scanToEvdevMap_US section
    start = content.find('const std::unordered_map<uint16_t, uint16_t> g_scanToEvdevMap_US = {')
    end = content.find('};', start)
    mapping_section = content[start:end]

    # Extract scan codes like {0x1E, KEY_A}
    for match in re.finditer(r'\{(0x[0-9a-fA-F]+),\s*KEY_(\w+)\}', mapping_section):
        scancode = match.group(1).upper()
        evdev_key = match.group(2)
        scan_map[scancode] = evdev_key

print("=" * 100)
print("Substitution Output Verification - Check if output scan codes are in g_scanToEvdevMap_US")
print("=" * 100)
print()
print(f"{'From Key':<12} {'From Scan':<12} {'To Key':<12} {'To Scan':<12} {'In Map?':<8} {'Maps To':<20} {'Status'}")
print("-" * 100)

total = 0
in_map = 0
not_in_map = 0

for from_key, to_key in substitutions:
    # Get scan codes from 109.mayu
    from_scan = mayu_scancodes.get(from_key, '???')
    to_scan = mayu_scancodes.get(to_key, '???')

    # Check if output scan code is in g_scanToEvdevMap_US
    if to_scan in scan_map:
        in_map_status = "✓"
        maps_to = f"KEY_{scan_map[to_scan]}"
        status = "OK"
        in_map += 1
    else:
        in_map_status = "✗"
        maps_to = "NOT FOUND"
        status = "FAIL"
        not_in_map += 1

    total += 1

    print(f"{from_key:<12} {from_scan:<12} {to_key:<12} {to_scan:<12} {in_map_status:<8} {maps_to:<20} {status}")

print()
print("=" * 100)
print(f"Total substitutions: {total}")
print(f"Output scan codes in map: {in_map}")
print(f"Output scan codes NOT in map: {not_in_map}")
print("=" * 100)

if not_in_map > 0:
    print()
    print("⚠️  WARNING: Some substitutions output scan codes that aren't in g_scanToEvdevMap_US!")
    print("These substitutions will FAIL because yamyToEvdevKeyCode() can't map them to evdev codes.")
    print()
    print("Missing scan codes:")
    for from_key, to_key in substitutions:
        to_scan = mayu_scancodes.get(to_key, '???')
        if to_scan not in scan_map and to_scan != '???':
            print(f"  {to_scan} (from {to_key})")
