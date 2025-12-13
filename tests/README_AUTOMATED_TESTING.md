# YAMY Automated Testing Framework

## Overview

The automated testing framework enables **zero-user-interaction** testing of all key substitutions defined in YAMY configuration files. This implements requirements 5 and 6 from the key-remapping-consistency specification.

## Features

- ✓ Autonomous operation - no manual key presses required
- ✓ Parses `.mayu` config files automatically
- ✓ Injects synthetic key events via `yamy-test` utility
- ✓ Verifies output by parsing YAMY debug logs
- ✓ Tests both PRESS and RELEASE events for every substitution
- ✓ Generates comprehensive pass/fail reports

## Quick Start

### Prerequisites

1. Build YAMY with the test tools:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```

2. Start YAMY with debug logging enabled:
   ```bash
   export YAMY_DEBUG_KEYCODE=1
   ./build/bin/yamy
   ```

### Running Tests

```bash
# Run all tests with default config
python3 tests/automated_keymap_test.py

# Specify custom config
python3 tests/automated_keymap_test.py --config path/to/custom.mayu

# Specify custom yamy-test location
python3 tests/automated_keymap_test.py --yamy-test ./build/bin/yamy-test
```

## How It Works

### 1. Config Parsing (MayuParser)

The framework parses `.mayu` config files to extract all `def subst` entries:

```python
parser = MayuParser('keymaps/config_clean.mayu')
mappings = parser.parse()
# Returns list of KeyMapping objects with input/output evdev codes
```

### 2. Event Injection (inject_key)

Uses the `yamy-test` utility to inject synthetic key events:

```python
test.inject_key(evdev_code=30, event_type="PRESS")
test.inject_key(evdev_code=30, event_type="RELEASE")
```

### 3. Output Verification (verify_output)

Parses YAMY debug logs to verify the output evdev code matches expected:

- Looks for `[LAYER3:OUT]` log entries
- Extracts actual evdev code from logs
- Compares with expected substitution output

### 4. Test Execution (test_all_substitutions)

Runs comprehensive test suite:
- For each substitution in config:
  - Test PRESS event
  - Test RELEASE event
- Generate statistics and report

## Architecture

```
AutomatedKeymapTest
├── MayuParser          # Parses .mayu config files
├── KeyCodeMapper       # Maps YAMY key names ↔ evdev codes
├── inject_key()        # Injects events via yamy-test
├── verify_output()     # Verifies from debug logs
├── test_substitution() # Tests one key + event type
└── test_all_substitutions() # Tests all 82+ substitutions × 2 events
```

## Sample Output

```
================================================================================
YAMY AUTOMATED KEYMAP TESTING FRAMEWORK
================================================================================

[AutomatedKeymapTest] Loading config: keymaps/config_clean.mayu
[AutomatedKeymapTest] Loaded 82 substitutions
[AutomatedKeymapTest] Testing 82 substitutions × 2 event types
[AutomatedKeymapTest] Total tests: 164
================================================================================

[1/82] Testing: A → Tab
           evdev 30 → 15
  Testing PRESS... ✓ PASS
  Testing RELEASE... ✓ PASS

[2/82] Testing: B → Enter
           evdev 48 → 28
  Testing PRESS... ✓ PASS
  Testing RELEASE... ✓ PASS

...

================================================================================
AUTOMATED KEYMAP TEST REPORT
================================================================================

Config: keymaps/config_clean.mayu
Total Substitutions: 82
Total Tests: 164 (PRESS + RELEASE for each substitution)

Results:
  PASSED: 164
  FAILED: 0
  PASS RATE: 100.0%

✓ ALL TESTS PASSED!

================================================================================
```

## Test Results

Each test produces a `TestResult` containing:

- `mapping`: The KeyMapping being tested
- `event_type`: "PRESS" or "RELEASE"
- `passed`: Boolean success status
- `expected_evdev`: Expected output evdev code
- `actual_evdev`: Actual output evdev code (from logs)
- `error_message`: Error description if test failed

## Integration with CI/CD

The framework returns proper exit codes:
- `0`: All tests passed
- `1`: One or more tests failed

Use in CI:
```bash
#!/bin/bash
# Start YAMY with debug logging
export YAMY_DEBUG_KEYCODE=1
./build/bin/yamy &
YAMY_PID=$!

# Wait for YAMY to initialize
sleep 2

# Run tests
python3 tests/automated_keymap_test.py --config keymaps/config_clean.mayu

# Capture exit code
TEST_RESULT=$?

# Cleanup
kill $YAMY_PID

exit $TEST_RESULT
```

## Debugging

### No output detected

If tests fail with "No output detected in logs":

1. Verify YAMY is running:
   ```bash
   pgrep yamy
   ```

2. Verify debug logging is enabled:
   ```bash
   export YAMY_DEBUG_KEYCODE=1
   ```

3. Check logs manually:
   ```bash
   journalctl -u yamy -f
   # or
   tail -f /tmp/yamy_test.log
   ```

### Injection failures

If events fail to inject:

1. Check permissions:
   ```bash
   # Add user to input group
   sudo usermod -a -G input $USER
   # Or run with sudo
   sudo python3 tests/automated_keymap_test.py
   ```

2. Verify uinput module loaded:
   ```bash
   lsmod | grep uinput
   # Load if needed
   sudo modprobe uinput
   ```

## Requirements

- Python 3.6+
- YAMY compiled with test utilities
- Linux with uinput support
- Permissions to create virtual input devices

## Related Files

- `src/test/yamy_test_main.cpp` - C++ test utility for event injection
- `tests/analyze_event_flow.py` - Log analysis tool (task 1.4)
- `.spec-workflow/specs/key-remapping-consistency/` - Specification docs

## Design Reference

Implementation follows design from:
- `.spec-workflow/specs/key-remapping-consistency/design.md` Component 4
- Tasks 3.5 in `.spec-workflow/specs/key-remapping-consistency/tasks.md`
