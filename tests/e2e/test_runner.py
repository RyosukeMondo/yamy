#!/usr/bin/env python3
"""
E2E Test Runner for YAMY
========================

Orchestrates the new E2E test framework components to run automated tests.
"""

import argparse
import sys
import time
import os
import subprocess

# Add framework to the Python path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from framework.parser import MayuParser, KeyCodeMapper
from framework.injector import EventInjector
from framework.monitor import LogMonitor
from framework.reporter import ReportGenerator, TestResult
from framework.defs import KeyMapping

class E2EOrchestrator:
    def __init__(self, config_path: str, yamy_path: str, yamy_ctl_path: str, yamy_test_path: str, report_path: str):
        self.config_path = config_path
        self.yamy_path = yamy_path
        self.yamy_ctl_path = yamy_ctl_path
        self.yamy_test_path = yamy_test_path
        self.report_path = report_path
        
        self.parser = MayuParser(self.config_path)
        self.reporter = ReportGenerator(self.report_path)
        self.yamy_process = None

    def setup_yamy(self):
        """Kills any existing YAMY instance and starts a new one."""
        print("[Orchestrator] Setting up YAMY environment...")
        sys.stdout.flush()
        
        # Forcefully stop any running yamy process
        # Use basename and -x (exact match) to avoid killing self (the test runner)
        proc_name = os.path.basename(self.yamy_path)
        subprocess.run(['pkill', '-9', '-x', proc_name], stderr=subprocess.DEVNULL)
        time.sleep(1)

        # Start yamy in the background
        print(f"[Orchestrator] Starting '{self.yamy_path}' daemon...")
        sys.stdout.flush()
        # Use .out extension to avoid gitignore
        log_file = open('yamy_e2e_runner.out', 'w')
        env = os.environ.copy()
        env['YAMY_DEBUG_KEYCODE'] = '1' # Enable keycode logging
        self.yamy_process = subprocess.Popen(
            [self.yamy_path],
            stdout=log_file,
            stderr=log_file,
            env=env
        )
        
        # Allow time for the daemon to initialize
        time.sleep(3) 

        # Load the specified configuration
        print(f"[Orchestrator] Loading config: '{self.config_path}'")
        sys.stdout.flush()
        try:
            subprocess.run([self.yamy_ctl_path, 'reload', '--config', self.config_path], check=True)
            subprocess.run([self.yamy_ctl_path, 'start'], check=True)
        except subprocess.CalledProcessError as e:
            print(f"[Orchestrator] Failed to load config. Exit code: {e.returncode}")
            raise
        time.sleep(1) # Allow time for config to be applied

    def cleanup_yamy(self):
        """Cleans up the YAMY process."""
        if self.yamy_process:
            print("[Orchestrator] Cleaning up YAMY process...")
            self.yamy_process.terminate()
            try:
                self.yamy_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("[Orchestrator] YAMY did not terminate gracefully, sending SIGKILL.")
                self.yamy_process.kill()
            finally:
                self.yamy_process = None
        
        # Final cleanup
        proc_name = os.path.basename(self.yamy_path)
        subprocess.run(['pkill', '-9', '-x', proc_name], stderr=subprocess.DEVNULL)

    def run_single_test(self, mapping: KeyMapping, injector: EventInjector, monitor: LogMonitor):
        """Injects one key and checks for the correct output."""
        print(f"  Testing {mapping.input_key} -> {mapping.output_key}... ", end="", flush=True) 
        
        monitor.drain_events()
        
        # TAP the input key
        injector.tap(mapping.input_evdev)

        # Capture the output event
        # We look for a PRESS event (value=1)
        evt = monitor.capture_next_key_event(timeout=0.5)

        passed = False
        actual_code = None
        error_msg = None

        if evt:
            actual_code, value = evt
            if value == 1 and actual_code == mapping.output_evdev:
                passed = True
            elif value != 1:
                error_msg = f"Expected PRESS (1), got value {value}"
            else:
                error_msg = f"Expected {mapping.output_key} ({mapping.output_evdev}), got {KeyCodeMapper.get_key_name(actual_code)} ({actual_code})"
        else:
            error_msg = "No output event received"

        # Create and add the result
        result = TestResult(
            mapping=mapping,
            event_type="TAP",
            passed=passed,
            expected_evdev=mapping.output_evdev,
            actual_evdev=actual_code,
            error_message=error_msg
        )
        self.reporter.add_result(result)

        print("PASS" if passed else f"FAIL ({error_msg})")


    def run(self, quick_mode: bool = False):
        """Executes the full E2E test suite."""
        print("======== YAMY E2E Test Runner ========")
        sys.stdout.flush()
        
        # 1. Parse the mappings from the config file
        mappings = self.parser.parse()
        if not mappings:
            print("No 'def subst' mappings found to test. Exiting.")
            return True
            
        if quick_mode:
            print("[Orchestrator] Quick mode enabled. Selecting representative subset...")
            # Pick representative keys for quick testing
            representative_keys = {'A', '_0', 'F1', 'LCtrl', 'Space'}
            mappings = [m for m in mappings if m.input_key in representative_keys]
            
        print(f"Found {len(mappings)} key substitutions to test.")
        sys.stdout.flush()

        try:
            # 1. Start EventInjector (creates virtual keyboard)
            with EventInjector(self.yamy_test_path) as injector:
                # 2. Set up YAMY (now YAMY can see the virtual keyboard)
                self.setup_yamy()
                
                # 3. Use context manager for monitor
                with LogMonitor() as monitor:
                    print("\n=== Running Tests ===")
                    for mapping in mappings:
                        self.run_single_test(mapping, injector, monitor)
                        time.sleep(0.05) # Small delay between tests

        except Exception as e:
            print(f"\nFATAL ERROR during test execution: {e}")
            return False
        finally:
            # 4. Always clean up
            self.cleanup_yamy()
            
            # 5. Generate and save report
            self.reporter.generate_summary()
            self.reporter.save_json()

        # Return success if all tests passed
        return sum(1 for r in self.reporter.results if not r.passed) == 0


def main():
    parser = argparse.ArgumentParser(description="YAMY E2E Test Runner")
    parser.add_argument(
        '--config',
        type=str,
        default='keymaps/master.mayu',
        help="Path to the .mayu config file to test."
    )
    parser.add_argument(
        '--report-path',
        type=str,
        default='test_report.json',
        help="Path to save the JSON test report."
    )
    parser.add_argument(
        '--yamy-path',
        type=str,
        default='./build/bin/yamy',
        help="Path to the main 'yamy' executable."
    )
    parser.add_argument(
        '--yamy-ctl-path',
        type=str,
        default='./build/bin/yamy-ctl',
        help="Path to the 'yamy-ctl' control tool."
    )
    parser.add_argument(
        '--yamy-test-path',
        type=str,
        default='./build/bin/yamy-test',
        help="Path to the 'yamy-test' utility."
    )
    parser.add_argument(
        '--quick',
        action='store_true',
        help="Run a quick subset of tests (representative keys only)."
    )
    args = parser.parse_args()

    # Basic validation
    if not os.path.exists(args.config):
        print(f"Error: Config file not found at '{args.config}'")
        sys.exit(1)
    if not os.path.exists(args.yamy_path):
        print(f"Error: yamy executable not found at '{args.yamy_path}'")
        sys.exit(1)
    if not os.path.exists(args.yamy_ctl_path):
        print(f"Error: yamy-ctl executable not found at '{args.yamy_ctl_path}'")
        sys.exit(1)
    if not os.path.exists(args.yamy_test_path):
        print(f"Error: yamy-test executable not found at '{args.yamy_test_path}'")
        sys.exit(1)
        
    orchestrator = E2EOrchestrator(
        config_path=args.config,
        yamy_path=args.yamy_path,
        yamy_ctl_path=args.yamy_ctl_path,
        yamy_test_path=args.yamy_test_path,
        report_path=args.report_path
    )
    
    success = orchestrator.run(quick_mode=args.quick)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()