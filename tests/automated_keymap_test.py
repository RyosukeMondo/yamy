#!/usr/bin/env python3
"""
Automated Keymap Testing Framework for YAMY
============================================

This framework provides autonomous testing of all key substitutions without
user interaction. It follows requirements 5 and 6 from the specification.

Design: Following design.md Component 4 - AutomatedKeymapTest

Usage:
    python3 automated_keymap_test.py [--config CONFIG] [--log LOG]

Features:
- Parses .mayu config files to extract all substitutions
- Injects synthetic key events via yamy-test utility
- Verifies output by parsing debug logs
- Tests all 87 substitutions × 2 event types (PRESS/RELEASE)
- Zero user interaction required
- Generates clear pass/fail reports
"""

import subprocess
import re
import sys
import time
import os
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from pathlib import Path


@dataclass
class KeyMapping:
    """Represents a single key substitution."""
    input_key: str      # Input key name (e.g., "W", "Tab", "F1")
    output_key: str     # Output key name (e.g., "A", "Space", "LWin")
    input_evdev: int    # Input evdev code
    output_evdev: int   # Output evdev code


@dataclass
class TestResult:
    """Result of a single substitution test."""
    mapping: KeyMapping
    event_type: str     # "PRESS" or "RELEASE"
    passed: bool
    expected_evdev: int
    actual_evdev: Optional[int]
    error_message: Optional[str] = None


class KeyCodeMapper:
    """Maps YAMY key names to evdev codes."""

    # YAMY key name to evdev code mapping
    # Based on Linux input-event-codes.h and YAMY's keycode_mapping.cpp
    KEY_MAP = {
        # Letters
        'A': 30, 'B': 48, 'C': 46, 'D': 32, 'E': 18, 'F': 33, 'G': 34, 'H': 35,
        'I': 23, 'J': 36, 'K': 37, 'L': 38, 'M': 50, 'N': 49, 'O': 24, 'P': 25,
        'Q': 16, 'R': 19, 'S': 31, 'T': 20, 'U': 22, 'V': 47, 'W': 17, 'X': 45,
        'Y': 21, 'Z': 44,

        # Numbers
        '_0': 11, '_1': 2, '_2': 3, '_3': 4, '_4': 5, '_5': 6, '_6': 7, '_7': 8,
        '_8': 9, '_9': 10,

        # Function keys
        'F1': 59, 'F2': 60, 'F3': 61, 'F4': 62, 'F5': 63, 'F6': 64, 'F7': 65,
        'F8': 66, 'F9': 67, 'F10': 68, 'F11': 87, 'F12': 88,

        # Special keys
        'Tab': 15, 'Enter': 28, 'Esc': 1, 'Space': 57, 'BackSpace': 14,
        'Delete': 111, 'Insert': 110,

        # Navigation
        'Up': 103, 'Down': 108, 'Left': 105, 'Right': 106,
        'Home': 102, 'End': 107, 'PageUp': 104, 'PageDown': 109,

        # Modifiers
        'LShift': 42, 'RShift': 54, 'LCtrl': 29, 'RCtrl': 97, 'LAlt': 56,
        'RAlt': 100, 'LWin': 125, 'RWin': 126, 'Apps': 127,

        # JP-specific keys
        'Atmark': 40,           # @ key (JP: Shift+2 position)
        'Semicolon': 39,        # ; key
        'Colon': 39,            # : key (same as semicolon on JP keyboard)
        'Minus': 12,            # - key
        'Comma': 51,            # , key
        'Period': 52,           # . key
        'Slash': 53,            # / key
        'ReverseSolidus': 43,   # \ key (backslash)
        'Yen': 124,             # Yen key (JP-specific)
        'NonConvert': 94,       # 無変換 (Muhenkan)
        'Convert': 92,          # 変換 (Henkan)
        'Hiragana': 93,         # ひらがな (Hiragana/Katakana)
        'Kanji': 85,            # 半角/全角 (Hankaku/Zenkaku)
        'Eisuu': 90,            # 英数 (Eisu)

        # Lock keys
        'NumLock': 69, 'ScrollLock': 70, 'CapsLock': 58,
    }

    @classmethod
    def get_evdev_code(cls, key_name: str) -> Optional[int]:
        """Get evdev code for a YAMY key name."""
        return cls.KEY_MAP.get(key_name)

    @classmethod
    def get_key_name(cls, evdev_code: int) -> str:
        """Get key name for an evdev code (reverse lookup)."""
        for name, code in cls.KEY_MAP.items():
            if code == evdev_code:
                return name
        return f"UNKNOWN_{evdev_code}"


class MayuParser:
    """Parses .mayu configuration files to extract key substitutions."""

    SUBST_PATTERN = re.compile(r'def\s+subst\s+\*(\S+)\s*=\s*\*(\S+)')

    def __init__(self, config_path: str):
        self.config_path = config_path

    def parse(self) -> List[KeyMapping]:
        """Parse .mayu file and return list of key mappings."""
        mappings = []

        with open(self.config_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                # Remove comments
                line = line.split('#')[0].strip()
                if not line:
                    continue

                # Match substitution definition
                match = self.SUBST_PATTERN.search(line)
                if match:
                    input_key = match.group(1)
                    output_key = match.group(2)

                    # Get evdev codes
                    input_evdev = KeyCodeMapper.get_evdev_code(input_key)
                    output_evdev = KeyCodeMapper.get_evdev_code(output_key)

                    if input_evdev is None:
                        print(f"Warning: Unknown input key '{input_key}' at line {line_num}")
                        continue

                    if output_evdev is None:
                        print(f"Warning: Unknown output key '{output_key}' at line {line_num}")
                        continue

                    mappings.append(KeyMapping(
                        input_key=input_key,
                        output_key=output_key,
                        input_evdev=input_evdev,
                        output_evdev=output_evdev
                    ))

        return mappings


class AutomatedKeymapTest:
    """
    Autonomous keymap testing framework.

    Tests all key substitutions without user interaction by:
    1. Parsing .mayu config to extract substitutions
    2. Injecting synthetic events via yamy-test
    3. Verifying output from debug logs
    4. Generating comprehensive test reports
    """

    def __init__(self, config_path: str, yamy_test_path: str = None, log_file: str = None):
        self.config_path = config_path
        self.yamy_test_path = yamy_test_path or self._find_yamy_test()
        self.log_file = log_file or "/tmp/yamy_test.log"
        self.mappings: List[KeyMapping] = []
        self.results: List[TestResult] = []

    def _find_yamy_test(self) -> str:
        """Find yamy-test binary."""
        possible_paths = [
            "./build/bin/yamy-test",
            "../build/bin/yamy-test",
            "./bin/yamy-test",
            "yamy-test"
        ]

        for path in possible_paths:
            if os.path.exists(path):
                return path

            # Try which command
            try:
                result = subprocess.run(['which', path],
                                       capture_output=True, text=True)
                if result.returncode == 0:
                    return result.stdout.strip()
            except:
                pass

        return "yamy-test"  # Hope it's in PATH

    def load_config(self) -> bool:
        """Load and parse .mayu config file."""
        print(f"\n[AutomatedKeymapTest] Loading config: {self.config_path}")

        if not os.path.exists(self.config_path):
            print(f"ERROR: Config file not found: {self.config_path}")
            return False

        parser = MayuParser(self.config_path)
        self.mappings = parser.parse()

        print(f"[AutomatedKeymapTest] Loaded {len(self.mappings)} substitutions")
        return len(self.mappings) > 0

    def inject_key(self, evdev_code: int, event_type: str) -> bool:
        """
        Inject a single key event using yamy-test utility.

        Args:
            evdev_code: Linux evdev code for the key
            event_type: "PRESS" or "RELEASE"

        Returns:
            True if injection succeeded, False otherwise
        """
        try:
            cmd = [self.yamy_test_path, "inject", str(evdev_code), event_type]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            return result.returncode == 0
        except Exception as e:
            print(f"ERROR: Failed to inject key {evdev_code} {event_type}: {e}")
            return False

    def verify_output(self, expected_evdev: int, event_type: str,
                     timeout: float = 1.0) -> Tuple[bool, Optional[int]]:
        """
        Verify output from debug logs.

        Parses YAMY debug logs to find [LAYER3:OUT] entries and verify
        the output evdev code matches expected.

        Args:
            expected_evdev: Expected output evdev code
            event_type: "PRESS" or "RELEASE"
            timeout: Max time to wait for log entry (seconds)

        Returns:
            Tuple of (success: bool, actual_evdev: Optional[int])
        """
        # Wait a bit for YAMY to process
        time.sleep(0.1)

        # Pattern to match [LAYER3:OUT] log entries
        # Format: [LAYER3:OUT] yamy 0xYYYY → evdev ZZ (KEY_NAME)
        pattern = re.compile(r'\[LAYER3:OUT\]\s+yamy\s+0x[0-9A-F]+\s+→\s+evdev\s+(\d+)')

        start_time = time.time()

        # Try to read from systemd journal or log file
        # First try getting recent YAMY logs
        try:
            # Try journalctl for yamy logs (last 10 lines)
            result = subprocess.run(
                ['journalctl', '-u', 'yamy', '-n', '20', '--no-pager'],
                capture_output=True, text=True, timeout=2
            )
            if result.returncode == 0:
                log_content = result.stdout
            else:
                # Fall back to log file if exists
                if os.path.exists(self.log_file):
                    with open(self.log_file, 'r') as f:
                        # Read last 50 lines
                        lines = f.readlines()
                        log_content = ''.join(lines[-50:])
                else:
                    # No logs available
                    return False, None
        except:
            # If all else fails, check if log file exists
            if os.path.exists(self.log_file):
                with open(self.log_file, 'r') as f:
                    lines = f.readlines()
                    log_content = ''.join(lines[-50:])
            else:
                return False, None

        # Search for LAYER3:OUT in recent logs
        matches = pattern.findall(log_content)
        if matches:
            # Get the most recent match
            actual_evdev = int(matches[-1])
            success = (actual_evdev == expected_evdev)
            return success, actual_evdev

        # No LAYER3:OUT found in logs
        return False, None

    def test_substitution(self, mapping: KeyMapping, event_type: str) -> TestResult:
        """
        Test a single substitution for one event type.

        Args:
            mapping: The key mapping to test
            event_type: "PRESS" or "RELEASE"

        Returns:
            TestResult with pass/fail status
        """
        # Inject input key
        success = self.inject_key(mapping.input_evdev, event_type)

        if not success:
            return TestResult(
                mapping=mapping,
                event_type=event_type,
                passed=False,
                expected_evdev=mapping.output_evdev,
                actual_evdev=None,
                error_message="Failed to inject key event"
            )

        # Verify output
        passed, actual_evdev = self.verify_output(mapping.output_evdev, event_type)

        if actual_evdev is None:
            error_msg = "No output detected in logs (YAMY may not be running or YAMY_DEBUG_KEYCODE not set)"
        elif not passed:
            error_msg = f"Expected evdev {mapping.output_evdev}, got {actual_evdev}"
        else:
            error_msg = None

        return TestResult(
            mapping=mapping,
            event_type=event_type,
            passed=passed,
            expected_evdev=mapping.output_evdev,
            actual_evdev=actual_evdev,
            error_message=error_msg
        )

    def test_all_substitutions(self) -> Dict[str, any]:
        """
        Test all substitutions for both PRESS and RELEASE events.

        Returns:
            Dictionary with test statistics and results
        """
        print(f"\n[AutomatedKeymapTest] Testing {len(self.mappings)} substitutions × 2 event types")
        print(f"[AutomatedKeymapTest] Total tests: {len(self.mappings) * 2}")
        print("=" * 80)

        self.results = []
        total_tests = len(self.mappings) * 2
        passed_count = 0

        for i, mapping in enumerate(self.mappings, 1):
            print(f"\n[{i}/{len(self.mappings)}] Testing: {mapping.input_key} → {mapping.output_key}")
            print(f"           evdev {mapping.input_evdev} → {mapping.output_evdev}")

            # Test PRESS
            print(f"  Testing PRESS...", end=" ")
            result_press = self.test_substitution(mapping, "PRESS")
            self.results.append(result_press)

            if result_press.passed:
                print("✓ PASS")
                passed_count += 1
            else:
                print(f"✗ FAIL: {result_press.error_message}")

            # Small delay between tests
            time.sleep(0.05)

            # Test RELEASE
            print(f"  Testing RELEASE...", end=" ")
            result_release = self.test_substitution(mapping, "RELEASE")
            self.results.append(result_release)

            if result_release.passed:
                print("✓ PASS")
                passed_count += 1
            else:
                print(f"✗ FAIL: {result_release.error_message}")

            # Small delay between tests
            time.sleep(0.05)

        # Calculate statistics
        failed_count = total_tests - passed_count
        pass_rate = (passed_count / total_tests * 100) if total_tests > 0 else 0

        stats = {
            'total_tests': total_tests,
            'passed': passed_count,
            'failed': failed_count,
            'pass_rate': pass_rate,
            'substitutions_tested': len(self.mappings)
        }

        return stats

    def generate_report(self, stats: Dict[str, any]) -> str:
        """
        Generate comprehensive test report.

        Args:
            stats: Test statistics dictionary

        Returns:
            Report as formatted string
        """
        report = []
        report.append("\n" + "=" * 80)
        report.append("AUTOMATED KEYMAP TEST REPORT")
        report.append("=" * 80)
        report.append(f"\nConfig: {self.config_path}")
        report.append(f"Total Substitutions: {stats['substitutions_tested']}")
        report.append(f"Total Tests: {stats['total_tests']} (PRESS + RELEASE for each substitution)")
        report.append(f"\nResults:")
        report.append(f"  PASSED: {stats['passed']}")
        report.append(f"  FAILED: {stats['failed']}")
        report.append(f"  PASS RATE: {stats['pass_rate']:.1f}%")

        # List failures if any
        failures = [r for r in self.results if not r.passed]
        if failures:
            report.append(f"\n\nFailed Tests ({len(failures)}):")
            report.append("-" * 80)

            for result in failures:
                report.append(f"\n  {result.mapping.input_key} → {result.mapping.output_key} ({result.event_type})")
                report.append(f"    Input evdev:    {result.mapping.input_evdev}")
                report.append(f"    Expected evdev: {result.expected_evdev}")
                report.append(f"    Actual evdev:   {result.actual_evdev or 'N/A'}")
                report.append(f"    Error:          {result.error_message}")
        else:
            report.append("\n\n✓ ALL TESTS PASSED!")

        report.append("\n" + "=" * 80)

        return "\n".join(report)

    def run(self) -> bool:
        """
        Run complete test suite.

        Returns:
            True if all tests passed, False otherwise
        """
        print("\n" + "=" * 80)
        print("YAMY AUTOMATED KEYMAP TESTING FRAMEWORK")
        print("=" * 80)

        # Check if YAMY is running
        try:
            result = subprocess.run(['pgrep', 'yamy'], capture_output=True)
            if result.returncode != 0:
                print("\nWARNING: YAMY does not appear to be running!")
                print("Please start YAMY with YAMY_DEBUG_KEYCODE=1 for logging")
                print("\nExample:")
                print("  export YAMY_DEBUG_KEYCODE=1")
                print("  ./build/bin/yamy")
                print("")
                response = input("Continue anyway? [y/N]: ")
                if response.lower() != 'y':
                    return False
        except:
            pass

        # Load config
        if not self.load_config():
            return False

        # Run all tests
        stats = self.test_all_substitutions()

        # Generate and print report
        report = self.generate_report(stats)
        print(report)

        # Return True only if all tests passed
        return stats['failed'] == 0


def main():
    """Main entry point."""
    import argparse

    parser = argparse.ArgumentParser(
        description="Automated keymap testing framework for YAMY",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        '--config',
        default='keymaps/config_clean.mayu',
        help='Path to .mayu config file (default: keymaps/config_clean.mayu)'
    )

    parser.add_argument(
        '--yamy-test',
        help='Path to yamy-test binary (auto-detected if not specified)'
    )

    parser.add_argument(
        '--log',
        default='/tmp/yamy_test.log',
        help='Path to YAMY log file (default: /tmp/yamy_test.log)'
    )

    args = parser.parse_args()

    # Create test framework
    test = AutomatedKeymapTest(
        config_path=args.config,
        yamy_test_path=args.yamy_test,
        log_file=args.log
    )

    # Run tests
    success = test.run()

    # Exit with appropriate status code
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
