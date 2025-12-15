# YAMY Testing Framework - User Guide

## Overview

The YAMY testing framework provides comprehensive automated end-to-end testing capabilities for the YAMY keyboard remapper daemon. It enables testing all features without user interaction through synthetic event injection and output verification.

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                  Testing Framework Components                 │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  yamy-inject         yamy-capture       yamy-test-runner     │
│  (Input Generator)   (Output Verifier)  (Orchestrator)       │
│       │                    │                    │             │
│       ├─> uinput ──────────┤                    │             │
│       │   device           │                    │             │
│       v                    v                    v             │
│  [Synthetic Events]   [Captured Events]   [Test Results]     │
│                                                               │
└──────────────────────────────────────────────────────────────┘
                             │
                             v
┌──────────────────────────────────────────────────────────────┐
│                  YAMY Daemon (Under Test)                     │
│                                                               │
│  InputHook → EventProcessor → InputInjector                   │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

## Installation

### Build the Tools

```bash
# Install dependencies (adds nlohmann_json)
conan install . --output-folder=build --build=missing

# Configure and build
cmake --preset linux-gcc-release
cmake --build build/linux-gcc-release

# Tools will be installed to build/linux-gcc-release/bin/
```

### Verify Installation

```bash
build/linux-gcc-release/bin/yamy-inject --help
build/linux-gcc-release/bin/yamy-capture --help
build/linux-gcc-release/bin/yamy-test-runner --help
```

### Permissions

The testing tools require access to `/dev/uinput` to create virtual keyboard devices:

```bash
# Option 1: Add user to input group (recommended)
sudo usermod -a -G input $USER
newgrp input

# Option 2: Run with sudo (for testing only)
sudo ./yamy-inject --help
```

## Tools

### 1. yamy-inject

**Purpose**: Inject synthetic keyboard events into the system

**Usage**:
```bash
# Single key (press + release)
yamy-inject --key 30

# Single key event
yamy-inject --key 30 --press
yamy-inject --key 30 --release

# Multiple keys
yamy-inject --keys 30,48,46

# From JSON scenario
yamy-inject --scenario tests/scenarios/basic_remap.json
```

**Options**:
- `--key <code>` - Inject single key (evdev code)
- `--keys <codes>` - Inject multiple keys (comma-separated)
- `--press` - Press event only
- `--release` - Release event only
- `--sequence` - Press + release for each key (default)
- `--delay <ms>` - Delay between events (default: 50ms)
- `--hold <ms>` - Hold time before release (default: 50ms)
- `--scenario <file>` - Load from JSON scenario file
- `--test-case <name>` - Run specific test case
- `--quiet` - Suppress output

**Common Evdev Codes**:
```
KEY_A=30, KEY_B=48, KEY_C=46, KEY_TAB=15
KEY_ESC=1, KEY_ENTER=28, KEY_SPACE=57
KEY_LEFTSHIFT=42, KEY_LEFTCTRL=29, KEY_LEFTALT=56
```

**Example**:
```bash
# Inject A → B → C sequence
yamy-inject --keys 30,48,46 --delay 100

# Run specific test case
yamy-inject --scenario tests/scenarios/modal_modifiers.json --test-case m0_activation
```

---

### 2. yamy-capture

**Purpose**: Monitor and capture YAMY virtual keyboard output

**Usage**:
```bash
# Capture for 2 seconds
yamy-capture --timeout 2000

# Capture 10 events
yamy-capture --count 10

# Capture until ESC key
yamy-capture --until-key 1

# Human-readable output
yamy-capture --format human

# Real-time streaming
yamy-capture --stream
```

**Options**:
- `--timeout <ms>` - Capture timeout (default: 1000ms)
- `--count <n>` - Stop after N events
- `--until-key <code>` - Stop when specific key captured
- `--format <type>` - Output format: `json` (default) or `human`
- `--stream` - Real-time event streaming
- `--quiet` - Suppress informational output

**Output Format (JSON)**:
```json
{
  "captured_events": [
    {
      "evdev_code": 15,
      "key_name": "KEY_TAB",
      "type": "press",
      "timestamp_us": 1234567890123,
      "latency_us": 27
    }
  ],
  "summary": {
    "total_events": 2,
    "duration_us": 100000,
    "average_latency_us": 27
  }
}
```

**Example**:
```bash
# Capture and save to file
yamy-capture --count 10 --format json > output.json

# Monitor in real-time
yamy-capture --stream --format human
```

---

### 3. yamy-test-runner

**Purpose**: Execute complete E2E test scenarios with verification

**Usage**:
```bash
# Run single scenario
yamy-test-runner --scenario tests/scenarios/basic_remap.json

# Run test suite
yamy-test-runner --suite tests/suites/smoke_tests.json

# Generate report
yamy-test-runner --suite tests/suites/full_suite.json --report results.json
```

**Options**:
- `--scenario <file>` - Run single test scenario
- `--suite <file>` - Run test suite (multiple scenarios)
- `--test-case <name>` - Run specific test case
- `--report <file>` - Save JSON report to file
- `--quiet` - Suppress detailed output

**Example**:
```bash
# Run smoke tests
yamy-test-runner --suite tests/suites/smoke_tests.json

# Run specific test case with report
yamy-test-runner --scenario tests/scenarios/modal_modifiers.json \
  --test-case m0_layer_key_mapping \
  --report result.json
```

---

## Test Scenarios

### Scenario Format (JSON)

```json
{
  "name": "basic_key_remapping",
  "description": "Test simple key remapping",
  "config": "keymaps/test-config.mayu",
  "setup": {
    "daemon_args": ["--debug"],
    "env": {
      "YAMY_DEBUG_KEYCODE": "1"
    }
  },
  "test_cases": [
    {
      "name": "press_release_a_key",
      "description": "Press and release A, expect Tab",
      "input": [
        {
          "evdev_code": 30,
          "key_name": "KEY_A",
          "type": "press",
          "delay_before_ms": 0
        },
        {
          "evdev_code": 30,
          "key_name": "KEY_A",
          "type": "release",
          "delay_before_ms": 50
        }
      ],
      "expected_output": [
        {
          "evdev_code": 15,
          "key_name": "KEY_TAB",
          "type": "press"
        },
        {
          "evdev_code": 15,
          "key_name": "KEY_TAB",
          "type": "release"
        }
      ],
      "timeout_ms": 1000,
      "max_latency_us": 1000
    }
  ]
}
```

### Test Suite Format (JSON)

```json
{
  "name": "YAMY Smoke Test Suite",
  "description": "Quick verification tests",
  "test_scenarios": [
    "tests/scenarios/basic_remap.json",
    "tests/scenarios/modifier_keys.json"
  ],
  "global_setup": {
    "build_daemon": false,
    "clean_state": true
  },
  "global_teardown": {
    "collect_logs": true
  }
}
```

---

## Workflows

### 1. Quick Manual Test

Test a specific key mapping:

```bash
# 1. Start YAMY daemon
./build/linux-gcc-release/bin/yamy &

# 2. Start engine
./build/linux-gcc-release/bin/yamy-ctl start

# 3. Inject key and capture output in separate terminals
# Terminal 1:
yamy-capture --count 2

# Terminal 2:
yamy-inject --key 30

# 4. Verify output
```

### 2. Automated Test Scenario

Run a predefined test scenario:

```bash
# 1. Ensure YAMY is running with the correct config
./build/linux-gcc-release/bin/yamy --config keymaps/test-basic.mayu &
./build/linux-gcc-release/bin/yamy-ctl start

# 2. Run test
yamy-test-runner --scenario tests/scenarios/basic_remap.json

# 3. Check results
echo $?  # 0 = passed, 1 = failed
```

### 3. Full Test Suite with Report

Run complete test suite and generate report:

```bash
# 1. Start daemon
./build/linux-gcc-release/bin/yamy &
./build/linux-gcc-release/bin/yamy-ctl start

# 2. Run suite
yamy-test-runner --suite tests/suites/full_suite.json --report results.json

# 3. View report
cat results.json | jq '.summary'
```

### 4. Continuous Integration

Example CI workflow:

```bash
#!/bin/bash
set -e

# Build
cmake --preset linux-gcc-release
cmake --build build/linux-gcc-release

# Start daemon in background
./build/linux-gcc-release/bin/yamy &
YAMY_PID=$!
sleep 2

# Start engine
./build/linux-gcc-release/bin/yamy-ctl start
sleep 1

# Run tests
./build/linux-gcc-release/bin/yamy-test-runner \
  --suite tests/suites/smoke_tests.json \
  --report test-results.json

# Save exit code
TEST_EXIT=$?

# Cleanup
kill $YAMY_PID

exit $TEST_EXIT
```

---

## Creating Test Scenarios

### Step 1: Identify Test Case

Determine what you want to test:
- Basic key remapping
- Modifier combinations
- Modal modifier activation
- Number-to-modifier feature
- Config hot reload

### Step 2: Create Scenario File

Create `tests/scenarios/my_test.json`:

```json
{
  "name": "my_feature_test",
  "description": "Test my specific feature",
  "config": "keymaps/my-config.mayu",
  "setup": {
    "env": {
      "YAMY_DEBUG_KEYCODE": "1"
    }
  },
  "test_cases": [
    {
      "name": "test_case_1",
      "description": "Description of what this tests",
      "input": [
        {
          "evdev_code": 30,
          "key_name": "KEY_A",
          "type": "press",
          "delay_before_ms": 0
        },
        {
          "evdev_code": 30,
          "key_name": "KEY_A",
          "type": "release",
          "delay_before_ms": 50
        }
      ],
      "expected_output": [
        {
          "evdev_code": 15,
          "key_name": "KEY_TAB",
          "type": "press"
        },
        {
          "evdev_code": 15,
          "key_name": "KEY_TAB",
          "type": "release"
        }
      ],
      "timeout_ms": 1000,
      "max_latency_us": 1000
    }
  ]
}
```

### Step 3: Test Manually

Before automating, verify with manual tools:

```bash
# Start capture
yamy-capture --count 2 &

# Inject input
yamy-inject --scenario tests/scenarios/my_test.json

# Check output matches expected
```

### Step 4: Add to Test Suite

Add to `tests/suites/my_suite.json`:

```json
{
  "name": "My Test Suite",
  "test_scenarios": [
    "tests/scenarios/my_test.json"
  ],
  "global_setup": {
    "clean_state": true
  }
}
```

### Step 5: Run Automated Tests

```bash
yamy-test-runner --suite tests/suites/my_suite.json --report results.json
```

---

## Debugging Test Failures

### 1. Enable Debug Logging

```bash
YAMY_DEBUG_KEYCODE=1 ./build/linux-gcc-release/bin/yamy
```

### 2. Run Individual Tools

Test injection and capture separately:

```bash
# Terminal 1: Capture with human output
yamy-capture --stream --format human

# Terminal 2: Inject events
yamy-inject --key 30 --delay 100
```

### 3. Check YAMY Logs

```bash
# View daemon logs
tail -f /tmp/yamy.log

# Check for error messages
grep ERROR /tmp/yamy.log
```

### 4. Verify Event Flow

Use existing debug tools:

```bash
# Check event processing
grep "LAYER" /tmp/yamy.log

# Verify substitution
grep "SUBST" /tmp/yamy.log
```

### 5. Test with Dry Run

Before injecting, check what would be sent:

```bash
# The existing yamy-test tool has dry-run
./build/linux-gcc-release/bin/yamy-test dry-run 30,48,46
```

---

## Best Practices

### 1. Test Isolation

- Each test case should be independent
- Clear state between test cases
- Don't rely on previous test state

### 2. Timing

- Use appropriate delays (`delay_before_ms`)
- Set realistic timeouts (`timeout_ms`)
- Consider system load in CI

### 3. Error Handling

- Test both success and failure cases
- Verify error conditions
- Check edge cases (rapid input, simultaneous keys)

### 4. Coverage

- Test all key remapping features
- Test all modifier combinations
- Test config hot reload
- Test performance under load

### 5. Documentation

- Document test purpose in `description`
- Use meaningful test case names
- Comment complex test sequences

---

## Troubleshooting

### Permission Denied: /dev/uinput

```bash
# Check permissions
ls -l /dev/uinput

# Add user to input group
sudo usermod -a -G input $USER
newgrp input

# Or run with sudo (not recommended for CI)
sudo ./yamy-inject --key 30
```

### YAMY Virtual Keyboard Not Found

```bash
# Check if YAMY is running
ps aux | grep yamy

# Check if engine is started
./build/linux-gcc-release/bin/yamy-ctl status

# Start engine if stopped
./build/linux-gcc-release/bin/yamy-ctl start

# Verify virtual device exists
ls /dev/input/by-id/ | grep -i yamy
```

### Test Timeout

Increase timeout in scenario:

```json
{
  "timeout_ms": 5000,
  "max_latency_us": 5000
}
```

Or check if YAMY is processing events:

```bash
YAMY_DEBUG_KEYCODE=1 ./yamy
# Watch for event processing in logs
```

### Event Count Mismatch

Check if:
- YAMY is consuming test input (not another process)
- Virtual keyboard is being captured
- No key repeat interfering

```bash
# List all input devices
cat /proc/bus/input/devices

# Monitor specific device
evtest /dev/input/eventX
```

---

## Reference

### Evdev Key Codes

Full list in `/usr/include/linux/input-event-codes.h`

Common codes:
```
Letters:
  KEY_A=30, KEY_B=48, KEY_C=46, KEY_D=32, KEY_E=18
  KEY_F=33, KEY_G=34, KEY_H=35, KEY_I=23, KEY_J=36
  KEY_K=37, KEY_L=38, KEY_M=50, KEY_N=49, KEY_O=24
  KEY_P=25, KEY_Q=16, KEY_R=19, KEY_S=31, KEY_T=20
  KEY_U=22, KEY_V=47, KEY_W=17, KEY_X=45, KEY_Y=21
  KEY_Z=44

Numbers:
  KEY_1=2, KEY_2=3, KEY_3=4, KEY_4=5, KEY_5=6
  KEY_6=7, KEY_7=8, KEY_8=9, KEY_9=10, KEY_0=11

Special:
  KEY_ESC=1, KEY_TAB=15, KEY_ENTER=28, KEY_SPACE=57
  KEY_BACKSPACE=14, KEY_CAPSLOCK=58

Modifiers:
  KEY_LEFTSHIFT=42, KEY_RIGHTSHIFT=54
  KEY_LEFTCTRL=29, KEY_RIGHTCTRL=97
  KEY_LEFTALT=56, KEY_RIGHTALT=100
  KEY_LEFTMETA=125, KEY_RIGHTMETA=126

Arrow keys:
  KEY_UP=103, KEY_DOWN=108, KEY_LEFT=105, KEY_RIGHT=106

Function keys:
  KEY_F1=59, KEY_F2=60, ..., KEY_F12=88
```

### Test Result Codes

```
Exit codes:
  0 - All tests passed
  1 - One or more tests failed

Test status:
  PASSED - Test completed successfully
  FAILED - Output did not match expected
  TIMEOUT - Test did not complete in time
  ERROR - Test execution error
```

---

## Examples

See `tests/scenarios/` for complete examples:
- `basic_remap.json` - Simple key remapping
- `modifier_keys.json` - Hardware modifiers
- `modal_modifiers.json` - M0-M7 modal modifiers

See `tests/suites/` for test suites:
- `smoke_tests.json` - Quick verification
- `full_suite.json` - Comprehensive testing

---

## Contributing

When adding new features to YAMY:

1. **Create test scenarios** for the new feature
2. **Add to test suite** (usually `full_suite.json`)
3. **Verify tests pass** before committing
4. **Update documentation** if adding new test types

---

## Support

For issues with the testing framework:
- Check `/tmp/yamy.log` for daemon errors
- Run with `--verbose` flag for detailed output
- Consult existing test scenarios as examples
- Report issues at https://github.com/your-org/yamy/issues
