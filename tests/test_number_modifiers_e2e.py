#!/usr/bin/env python3
"""
test_number_modifiers_e2e.py - E2E tests for number keys as custom modifiers

Tests the complete number modifier feature:
- HOLD detection (≥200ms): Number key activates hardware modifier
- TAP detection (<200ms): Number key applies normal substitution
- Combinations: Hold number modifier + press other key

Part of task 4.6 in key-remapping-consistency spec
"""

import subprocess
import time
import os
import sys
import re
from pathlib import Path

class NumberModifierE2ETest:
    """E2E test framework for number modifier feature"""

    def __init__(self, yamy_test_path='build/bin/yamy-test', log_dir='/tmp'):
        self.yamy_test_path = yamy_test_path
        self.log_dir = log_dir
        self.log_file = os.path.join(log_dir, 'yamy_debug.log')

        # Number key mappings (YAMY scan codes)
        self.number_keys = {
            '_1': {'evdev': 2, 'yamy': 0x0002},   # KEY_1
            '_2': {'evdev': 3, 'yamy': 0x0003},   # KEY_2
            '_3': {'evdev': 4, 'yamy': 0x0004},   # KEY_3
            '_4': {'evdev': 5, 'yamy': 0x0005},   # KEY_4
            '_5': {'evdev': 6, 'yamy': 0x0006},   # KEY_5
            '_6': {'evdev': 7, 'yamy': 0x0007},   # KEY_6
            '_7': {'evdev': 8, 'yamy': 0x0008},   # KEY_7
            '_8': {'evdev': 9, 'yamy': 0x0009},   # KEY_8
            '_9': {'evdev': 10, 'yamy': 0x000A},  # KEY_9
            '_0': {'evdev': 11, 'yamy': 0x000B},  # KEY_0
        }

        # Modifier VK codes
        self.modifiers = {
            'LSHIFT': 0xA0,
            'RSHIFT': 0xA1,
            'LCTRL': 0xA2,
            'RCTRL': 0xA3,
            'LALT': 0xA4,
            'RALT': 0xA5,
            'LWIN': 0x5B,
            'RWIN': 0x5C,
        }

        # Test letter key (A)
        self.test_key_A = {'evdev': 30, 'yamy': 0x001E}

    def inject_key(self, evdev_code, event_type):
        """Inject a key event using yamy-test utility"""
        try:
            cmd = [self.yamy_test_path, 'inject', str(evdev_code), event_type]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=2)
            return result.returncode == 0
        except subprocess.TimeoutExpired:
            print(f"WARNING: inject timeout for evdev {evdev_code} {event_type}")
            return False
        except Exception as e:
            print(f"ERROR: inject failed: {e}")
            return False

    def clear_log(self):
        """Clear the debug log file"""
        if os.path.exists(self.log_file):
            open(self.log_file, 'w').close()

    def parse_logs(self):
        """Parse debug logs and extract events"""
        if not os.path.exists(self.log_file):
            return []

        events = []
        with open(self.log_file, 'r') as f:
            for line in f:
                # Parse [MODIFIER] entries
                if '[MODIFIER]' in line:
                    events.append(line.strip())
                # Parse [LAYER3:OUT] entries
                elif '[LAYER3:OUT]' in line:
                    events.append(line.strip())

        return events

    def test_tap_detection(self, number_key_name='_1'):
        """Test TAP detection: quick PRESS → RELEASE should apply substitution"""
        print(f"\nTest: TAP detection for {number_key_name}")

        number_key = self.number_keys[number_key_name]
        evdev = number_key['evdev']

        # Clear log
        self.clear_log()

        # Inject PRESS
        if not self.inject_key(evdev, 'PRESS'):
            print(f"  ❌ Failed to inject PRESS")
            return False

        # Wait 50ms (well below 200ms threshold)
        time.sleep(0.05)

        # Inject RELEASE
        if not self.inject_key(evdev, 'RELEASE'):
            print(f"  ❌ Failed to inject RELEASE")
            return False

        # Wait for processing
        time.sleep(0.1)

        # Parse logs
        events = self.parse_logs()

        # Look for TAP detection in logs
        tap_detected = any('Tap detected' in event for event in events)
        modifier_activated = any('Hold detected' in event for event in events)

        if tap_detected and not modifier_activated:
            print(f"  ✅ TAP detected correctly (substitution should apply)")
            return True
        elif modifier_activated:
            print(f"  ❌ Incorrectly detected as HOLD (should be TAP)")
            return False
        else:
            print(f"  ⚠️  No modifier processing detected (may not be registered)")
            print(f"  Log entries: {len(events)}")
            for event in events[:5]:  # Show first 5 events
                print(f"    {event}")
            return False

    def test_hold_detection(self, number_key_name='_1', modifier_name='LSHIFT'):
        """Test HOLD detection: PRESS → wait 250ms → check for modifier activation"""
        print(f"\nTest: HOLD detection for {number_key_name} → {modifier_name}")

        number_key = self.number_keys[number_key_name]
        evdev = number_key['evdev']
        expected_vk = self.modifiers[modifier_name]

        # Clear log
        self.clear_log()

        # Inject PRESS
        if not self.inject_key(evdev, 'PRESS'):
            print(f"  ❌ Failed to inject PRESS")
            return False

        # Wait 250ms (exceeds 200ms threshold)
        time.sleep(0.25)

        # Inject another PRESS (to trigger threshold check in real event flow)
        if not self.inject_key(evdev, 'PRESS'):
            print(f"  ❌ Failed to inject second PRESS")
            return False

        # Wait for processing
        time.sleep(0.1)

        # Parse logs
        events = self.parse_logs()

        # Look for HOLD detection in logs
        hold_detected = any('Hold detected' in event and f'VK 0x{expected_vk:04X}' in event.upper() for event in events)

        if hold_detected:
            print(f"  ✅ HOLD detected correctly (modifier VK 0x{expected_vk:04X} activated)")

            # Inject RELEASE to deactivate
            self.inject_key(evdev, 'RELEASE')
            time.sleep(0.1)

            return True
        else:
            print(f"  ❌ HOLD not detected")
            print(f"  Log entries: {len(events)}")
            for event in events[:5]:
                print(f"    {event}")
            return False

    def test_hold_and_release(self, number_key_name='_1', modifier_name='LSHIFT'):
        """Test HOLD → RELEASE: Modifier should activate and then deactivate"""
        print(f"\nTest: HOLD and RELEASE for {number_key_name} → {modifier_name}")

        number_key = self.number_keys[number_key_name]
        evdev = number_key['evdev']
        expected_vk = self.modifiers[modifier_name]

        # Clear log
        self.clear_log()

        # Inject PRESS and wait for HOLD
        self.inject_key(evdev, 'PRESS')
        time.sleep(0.25)
        self.inject_key(evdev, 'PRESS')  # Trigger threshold check
        time.sleep(0.1)

        # Inject RELEASE
        self.inject_key(evdev, 'RELEASE')
        time.sleep(0.1)

        # Parse logs
        events = self.parse_logs()

        # Look for both activation and deactivation
        activated = any('Hold detected' in event or 'ACTIVATE' in event.upper() for event in events)
        deactivated = any('Deactivating modifier' in event or 'DEACTIVATE' in event.upper() for event in events)

        if activated and deactivated:
            print(f"  ✅ Modifier activated and deactivated correctly")
            return True
        elif activated and not deactivated:
            print(f"  ❌ Modifier activated but not deactivated")
            return False
        else:
            print(f"  ❌ Modifier not activated")
            return False

    def test_combination(self, number_key_name='_1', test_key_name='A'):
        """Test combination: HOLD number + PRESS letter → modified letter output"""
        print(f"\nTest: Combination {number_key_name} + {test_key_name}")

        number_key = self.number_keys[number_key_name]
        num_evdev = number_key['evdev']

        test_key = self.test_key_A
        test_evdev = test_key['evdev']

        # Clear log
        self.clear_log()

        # Step 1: HOLD number key
        self.inject_key(num_evdev, 'PRESS')
        time.sleep(0.25)
        self.inject_key(num_evdev, 'PRESS')  # Trigger threshold
        time.sleep(0.1)

        # Step 2: PRESS test key (A)
        self.inject_key(test_evdev, 'PRESS')
        time.sleep(0.05)
        self.inject_key(test_evdev, 'RELEASE')
        time.sleep(0.1)

        # Step 3: RELEASE number key
        self.inject_key(num_evdev, 'RELEASE')
        time.sleep(0.1)

        # Parse logs
        events = self.parse_logs()

        # Look for modifier activation and test key output
        modifier_active = any('Hold detected' in event or 'ACTIVATE' in event.upper() for event in events)
        test_key_output = any('[LAYER3:OUT]' in event and test_key_name.upper() in event for event in events)

        if modifier_active and test_key_output:
            print(f"  ✅ Combination works (modifier active + test key output)")
            return True
        elif not modifier_active:
            print(f"  ❌ Modifier not activated")
            return False
        else:
            print(f"  ❌ Test key not output")
            return False

    def run_all_tests(self):
        """Run all E2E tests and report results"""
        print("="*60)
        print("Number Modifier E2E Tests")
        print("="*60)

        # Check if yamy-test exists
        if not os.path.exists(self.yamy_test_path):
            print(f"ERROR: yamy-test not found at {self.yamy_test_path}")
            print(f"Please build yamy-test first")
            return False

        # Check if YAMY is running
        try:
            result = subprocess.run(['pgrep', 'yamy'], capture_output=True)
            if result.returncode != 0:
                print("WARNING: YAMY does not appear to be running")
                print("Please start YAMY with: ./build/bin/yamy")
                print("Note: E2E tests require a running YAMY instance")
                return False
        except Exception:
            pass  # pgrep may not be available

        results = []

        # Test 1: TAP detection
        results.append(('TAP detection (_1)', self.test_tap_detection('_1')))

        # Test 2: HOLD detection
        results.append(('HOLD detection (_1 → LSHIFT)', self.test_hold_detection('_1', 'LSHIFT')))

        # Test 3: HOLD and RELEASE
        results.append(('HOLD and RELEASE (_1 → LSHIFT)', self.test_hold_and_release('_1', 'LSHIFT')))

        # Test 4: Combination
        results.append(('Combination (_1 + A)', self.test_combination('_1', 'A')))

        # Test 5: Multiple number keys
        results.append(('TAP detection (_2)', self.test_tap_detection('_2')))
        results.append(('HOLD detection (_2 → RSHIFT)', self.test_hold_detection('_2', 'RSHIFT')))

        # Print summary
        print("\n" + "="*60)
        print("Test Summary")
        print("="*60)

        passed = sum(1 for _, result in results if result)
        total = len(results)

        for test_name, result in results:
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"{status}: {test_name}")

        print(f"\nTotal: {passed}/{total} tests passed ({100*passed//total}%)")
        print("="*60)

        return passed == total


def main():
    """Main entry point"""
    # Parse command line arguments
    yamy_test_path = 'build/bin/yamy-test'
    log_dir = '/tmp'

    if len(sys.argv) > 1:
        yamy_test_path = sys.argv[1]
    if len(sys.argv) > 2:
        log_dir = sys.argv[2]

    # Create test instance
    tester = NumberModifierE2ETest(yamy_test_path, log_dir)

    # Run all tests
    success = tester.run_all_tests()

    # Exit with appropriate code
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
