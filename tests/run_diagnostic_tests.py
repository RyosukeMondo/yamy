#!/usr/bin/env python3
"""
Diagnostic Test Suite for Key Remapping Consistency
Tests all 87 substitutions from config_clean.mayu

This script:
1. Parses config_clean.mayu to extract all substitutions
2. Maps keys to their evdev codes
3. Injects PRESS and RELEASE events for each substitution
4. Analyzes logs to identify asymmetries and layer skipping
5. Generates comprehensive report
"""

import subprocess
import sys
import time
import os
import re
from pathlib import Path
from collections import defaultdict

# Evdev key code mapping (from linux/input-event-codes.h)
KEY_MAP = {
    # Letters
    'A': 30, 'B': 48, 'C': 46, 'D': 32, 'E': 18, 'F': 33, 'G': 34, 'H': 35,
    'I': 23, 'J': 36, 'K': 37, 'L': 38, 'M': 50, 'N': 49, 'O': 24, 'P': 25,
    'Q': 16, 'R': 19, 'S': 31, 'T': 20, 'U': 22, 'V': 47, 'W': 17, 'X': 45,
    'Y': 21, 'Z': 44,

    # Numbers
    '_0': 11, '_1': 2, '_2': 3, '_3': 4, '_4': 5, '_5': 6, '_6': 7, '_7': 8,
    '_8': 9, '_9': 10,

    # Special keys
    'Tab': 15, 'Enter': 28, 'Esc': 1, 'Space': 57, 'BackSpace': 14,
    'Delete': 111, 'Insert': 110,

    # JP-specific keys
    'Atmark': 26,       # @
    'Semicolon': 39,    # ;
    'Colon': 40,        # :
    'Minus': 12,        # -
    'Comma': 51,        # ,
    'Period': 52,       # .
    'Slash': 53,        # /
    'ReverseSolidus': 125,  # \
    'NonConvert': 94,   # 無変換
    'Hiragana': 92,     # ひらがな
    'Convert': 93,      # 変換
    'Kanji': 85,        # 半角/全角
    'Yen': 124,         # ¥
    'Eisuu': 90,        # 英数

    # Function keys
    'F1': 59, 'F2': 60, 'F3': 61, 'F4': 62, 'F5': 63, 'F6': 64,
    'F7': 65, 'F8': 66, 'F9': 67, 'F10': 68, 'F11': 87, 'F12': 88,

    # Navigation
    'Home': 102, 'End': 107, 'PageUp': 104, 'PageDown': 109,
    'Up': 103, 'Down': 108, 'Left': 105, 'Right': 106,

    # Modifiers
    'LShift': 42, 'RShift': 54, 'LCtrl': 29, 'RCtrl': 97,
    'LAlt': 56, 'RAlt': 100, 'LWin': 125, 'Apps': 127,

    # Lock keys
    'NumLock': 69, 'ScrollLock': 70, 'CapsLock': 58,
}

class DiagnosticTest:
    def __init__(self, config_path, yamy_test_bin):
        self.config_path = Path(config_path)
        self.yamy_test_bin = Path(yamy_test_bin)
        self.substitutions = []
        self.results = {
            'total': 0,
            'tested': 0,
            'full_working': [],      # Works on both PRESS and RELEASE
            'press_only': [],         # Only works on PRESS
            'release_only': [],       # Only works on RELEASE
            'layer_skipping': [],     # Shows Layer 1 but not Layer 2/3
            'failed': [],             # Doesn't work at all
            'passthrough': [],        # Passthrough keys (X→X)
        }

    def parse_config(self):
        """Parse config_clean.mayu to extract substitutions"""
        with open(self.config_path) as f:
            for line in f:
                line = line.strip()
                # Match: def subst *KEY = *VALUE
                match = re.match(r'def\s+subst\s+\*(\S+)\s+=\s+\*(\S+)', line)
                if match:
                    source, target = match.groups()
                    # Skip if key not in map
                    if source not in KEY_MAP:
                        print(f"Warning: {source} not in KEY_MAP, skipping")
                        continue
                    self.substitutions.append((source, target, KEY_MAP[source]))

        self.results['total'] = len(self.substitutions)
        print(f"Parsed {len(self.substitutions)} substitutions from {self.config_path}")
        return self.substitutions

    def inject_key(self, evdev_code, event_type):
        """Inject a key event using yamy-test"""
        cmd = [str(self.yamy_test_bin), 'inject', str(evdev_code), event_type]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=2)
            return result.returncode == 0
        except subprocess.TimeoutExpired:
            print(f"Timeout injecting {evdev_code} {event_type}")
            return False
        except Exception as e:
            print(f"Error injecting {evdev_code} {event_type}: {e}")
            return False

    def test_substitution(self, source, target, evdev_code):
        """Test a single substitution for PRESS and RELEASE"""
        print(f"\nTesting: {source} → {target} (evdev {evdev_code})")

        # Test PRESS
        press_success = self.inject_key(evdev_code, 'PRESS')
        time.sleep(0.05)  # Small delay between events

        # Test RELEASE
        release_success = self.inject_key(evdev_code, 'RELEASE')
        time.sleep(0.05)

        return {
            'source': source,
            'target': target,
            'evdev': evdev_code,
            'press_success': press_success,
            'release_success': release_success,
        }

    def analyze_log_file(self, log_path):
        """Analyze YAMY debug log to categorize substitutions"""
        if not os.path.exists(log_path):
            print(f"Warning: Log file {log_path} not found")
            return

        # Parse log using analyze_event_flow.py
        analyze_script = Path(__file__).parent / 'analyze_event_flow.py'
        if not analyze_script.exists():
            print(f"Warning: analyze_event_flow.py not found at {analyze_script}")
            return

        try:
            result = subprocess.run(
                ['python3', str(analyze_script), log_path],
                capture_output=True,
                text=True,
                timeout=30
            )

            if result.returncode == 0:
                print("\n=== Log Analysis Results ===")
                print(result.stdout)
                return result.stdout
            else:
                print(f"Log analysis failed: {result.stderr}")
        except Exception as e:
            print(f"Error analyzing log: {e}")

    def categorize_results(self, test_results):
        """Categorize test results based on PRESS/RELEASE behavior"""
        for result in test_results:
            source = result['source']
            target = result['target']
            press = result['press_success']
            release = result['release_success']

            # Check if it's a passthrough
            if source == target:
                self.results['passthrough'].append(result)
            elif press and release:
                self.results['full_working'].append(result)
            elif press and not release:
                self.results['press_only'].append(result)
            elif not press and release:
                self.results['release_only'].append(result)
            else:
                self.results['failed'].append(result)

            self.results['tested'] += 1

    def print_report(self):
        """Print comprehensive diagnostic report"""
        print("\n" + "="*80)
        print("DIAGNOSTIC TEST REPORT")
        print("="*80)

        print(f"\nTotal Substitutions: {self.results['total']}")
        print(f"Tested: {self.results['tested']}")

        # Calculate percentages
        total = self.results['tested']
        if total == 0:
            print("No tests were run!")
            return

        full = len(self.results['full_working']) + len(self.results['passthrough'])
        partial = len(self.results['press_only']) + len(self.results['release_only'])
        failed = len(self.results['failed'])

        print(f"\n--- Success Rate ---")
        print(f"Fully Working: {full}/{total} ({100*full//total}%)")
        print(f"Partially Working: {partial}/{total} ({100*partial//total}%)")
        print(f"Failed: {failed}/{total} ({100*failed//total}%)")

        # Details
        if self.results['full_working']:
            print(f"\n--- Fully Working ({len(self.results['full_working'])}) ---")
            for r in self.results['full_working'][:10]:  # Show first 10
                print(f"  ✓ {r['source']} → {r['target']} (evdev {r['evdev']})")
            if len(self.results['full_working']) > 10:
                print(f"  ... and {len(self.results['full_working']) - 10} more")

        if self.results['press_only']:
            print(f"\n--- PRESS Only ({len(self.results['press_only'])}) ---")
            for r in self.results['press_only']:
                print(f"  ⚠ {r['source']} → {r['target']} (evdev {r['evdev']}) - RELEASE BROKEN")

        if self.results['release_only']:
            print(f"\n--- RELEASE Only ({len(self.results['release_only'])}) ---")
            for r in self.results['release_only']:
                print(f"  ⚠ {r['source']} → {r['target']} (evdev {r['evdev']}) - PRESS BROKEN")

        if self.results['failed']:
            print(f"\n--- Failed ({len(self.results['failed'])}) ---")
            for r in self.results['failed']:
                print(f"  ✗ {r['source']} → {r['target']} (evdev {r['evdev']})")

        if self.results['passthrough']:
            print(f"\n--- Passthrough Keys ({len(self.results['passthrough'])}) ---")
            for r in self.results['passthrough'][:5]:  # Show first 5
                print(f"  → {r['source']} (evdev {r['evdev']})")

        print("\n" + "="*80)

        # Baseline metrics
        print("\nBASELINE METRICS:")
        print(f"  Total substitutions: {self.results['total']}")
        print(f"  Fully working: {full} ({100*full//total}%)")
        print(f"  PRESS/RELEASE asymmetry: {partial}")
        print(f"  Complete failures: {failed}")
        print("="*80)

    def save_results(self, output_path):
        """Save results to JSON file"""
        import json
        with open(output_path, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"\nResults saved to {output_path}")

def main():
    # Paths
    repo_root = Path(__file__).parent.parent
    config_path = repo_root / 'keymaps' / 'config_clean.mayu'
    yamy_test_bin = repo_root / 'build' / 'bin' / 'yamy-test'

    if not config_path.exists():
        print(f"Error: {config_path} not found")
        return 1

    if not yamy_test_bin.exists():
        print(f"Error: {yamy_test_bin} not found")
        print("Build yamy-test first: cd build && make yamy-test")
        return 1

    # Create test instance
    test = DiagnosticTest(config_path, yamy_test_bin)

    # Parse config
    substitutions = test.parse_config()
    if not substitutions:
        print("No substitutions found!")
        return 1

    print("\n" + "="*80)
    print("STARTING DIAGNOSTIC TESTS")
    print("="*80)
    print("\nNOTE: YAMY must be running with debug logging enabled:")
    print("  export YAMY_DEBUG_KEYCODE=1")
    print("  ./build/bin/yamy --config keymaps/config_clean.mayu")
    print("\nPress Ctrl+C to stop at any time")
    print("="*80)

    # Wait for user
    try:
        input("\nPress Enter when YAMY is ready...")
    except KeyboardInterrupt:
        print("\nAborted")
        return 1

    # Run tests
    test_results = []
    try:
        for source, target, evdev in substitutions:
            result = test.test_substitution(source, target, evdev)
            test_results.append(result)
            time.sleep(0.1)  # Delay between tests
    except KeyboardInterrupt:
        print("\n\nTests interrupted by user")

    # Categorize results
    test.categorize_results(test_results)

    # Print report
    test.print_report()

    # Save results
    output_path = repo_root / 'tests' / 'diagnostic_results.json'
    test.save_results(output_path)

    return 0

if __name__ == '__main__':
    sys.exit(main())
