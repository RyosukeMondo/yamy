# YAMY E2E Testing Framework Design

## Overview

Comprehensive testing framework for yamy daemon that enables automated end-to-end testing without user interaction.

## Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    E2E Testing Framework                          │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────┐    ┌──────────────┐    ┌──────────────────┐   │
│  │yamy-inject  │    │yamy-capture  │    │yamy-test-runner  │   │
│  │             │    │              │    │                  │   │
│  │ Generates   │    │ Monitors     │    │ Orchestrates     │   │
│  │ synthetic   │    │ /dev/input/  │    │ test execution   │   │
│  │ keyboard    │───>│ eventX for   │<───│ and validates    │   │
│  │ events via  │    │ YAMY virtual │    │ results          │   │
│  │ uinput      │    │ keyboard     │    │                  │   │
│  └─────────────┘    └──────────────┘    └──────────────────┘   │
│       │                    │                     │               │
└───────┼────────────────────┼─────────────────────┼───────────────┘
        │                    │                     │
        v                    v                     v
   [Test Input]         [Test Output]        [Test Report]
        │                    │                     │
        └────────────────────┴─────────────────────┘
                             │
                             v
┌──────────────────────────────────────────────────────────────────┐
│                   YAMY Daemon (System Under Test)                 │
│                                                                   │
│  InputHookLinux → Queue → EventProcessor → InputInjectorLinux    │
│     (evdev)              (3 layers)            (uinput)          │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

## Components

### 1. yamy-inject (Input Generator)

**Purpose**: Inject synthetic keyboard events into the system for testing

**Features**:
- Inject single key press/release
- Inject key sequences with configurable timing
- Load test scenarios from JSON files
- Support for extended keys (E0 prefix)
- Configurable delays between events
- Output injection log with timestamps

**CLI Interface**:
```bash
# Single key injection
yamy-inject --key A --press
yamy-inject --key A --release

# Key sequence (press + release)
yamy-inject --key A --sequence

# From file
yamy-inject --scenario test_basic_remap.json

# With timing
yamy-inject --key A --sequence --hold-ms 50 --delay-ms 100

# Multiple keys
yamy-inject --keys "A,B,C" --sequence
```

**Implementation Details**:
- Creates uinput device "YAMY Test Injector"
- Sends `struct input_event` with:
  - `type`: EV_KEY
  - `code`: evdev code (e.g., KEY_A = 30)
  - `value`: 0 (release), 1 (press), 2 (repeat)
- Follows each key event with EV_SYN/SYN_REPORT
- Timestamps for correlation with output

**Output Format** (JSON):
```json
{
  "injected_events": [
    {
      "timestamp_us": 1234567890123,
      "evdev_code": 30,
      "type": "press",
      "key_name": "KEY_A"
    },
    {
      "timestamp_us": 1234567890223,
      "evdev_code": 30,
      "type": "release",
      "key_name": "KEY_A"
    }
  ]
}
```

---

### 2. yamy-capture (Output Verifier)

**Purpose**: Monitor yamy virtual keyboard output and verify remapping

**Features**:
- Find and monitor YAMY virtual keyboard device automatically
- Capture events with high-resolution timestamps
- Filter by event type (press/release/all)
- Timeout support for test completion
- JSON output for automated verification
- Real-time event streaming mode

**CLI Interface**:
```bash
# Capture all events (timeout after 1s of inactivity)
yamy-capture --timeout 1000

# Capture N events then exit
yamy-capture --count 10

# Capture until specific key
yamy-capture --until-key ESC

# Output format
yamy-capture --format json
yamy-capture --format human

# Real-time streaming
yamy-capture --stream

# Match against expected output
yamy-capture --expect expected_output.json
```

**Implementation Details**:
- Scans `/dev/input/event*` for device named "Yamy Virtual Keyboard"
- Opens device with `O_RDONLY | O_NONBLOCK`
- Polls with configurable timeout
- Parses `struct input_event`
- Resolves evdev codes to key names via keycode_mapping

**Output Format** (JSON):
```json
{
  "captured_events": [
    {
      "timestamp_us": 1234567890150,
      "evdev_code": 15,
      "type": "press",
      "key_name": "KEY_TAB",
      "latency_us": 27
    },
    {
      "timestamp_us": 1234567890250,
      "evdev_code": 15,
      "type": "release",
      "key_name": "KEY_TAB",
      "latency_us": 27
    }
  ],
  "summary": {
    "total_events": 2,
    "duration_ms": 100,
    "average_latency_us": 27
  }
}
```

---

### 3. yamy-test-runner (E2E Orchestrator)

**Purpose**: Execute complete E2E test scenarios and validate results

**Features**:
- Load test scenarios from JSON/YAML
- Manage daemon lifecycle (start, configure, stop)
- Execute inject → capture → verify pipeline
- Compare actual vs expected output
- Generate test reports (pass/fail, diffs)
- Support for test suites (batch execution)
- Parallel test execution
- Regression testing mode

**CLI Interface**:
```bash
# Run single test scenario
yamy-test-runner --scenario tests/scenarios/basic_remap.json

# Run test suite
yamy-test-runner --suite tests/suites/all_features.yaml

# With custom daemon binary
yamy-test-runner --daemon ./build/yamy --scenario test.json

# Verbose output
yamy-test-runner --scenario test.json --verbose

# Generate report
yamy-test-runner --suite all.yaml --report report.html

# Parallel execution
yamy-test-runner --suite all.yaml --jobs 4
```

**Test Scenario Format** (JSON):
```json
{
  "name": "basic_key_remapping",
  "description": "Test simple A→Tab remapping",
  "config": "keymaps/test-basic.mayu",
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
          "delay_before_ms": 100
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

**Test Suite Format** (YAML):
```yaml
name: "Complete Feature Test Suite"
description: "Tests all yamy remapping features"

test_scenarios:
  - tests/scenarios/basic_remap.json
  - tests/scenarios/modifier_keys.json
  - tests/scenarios/modal_modifiers.json
  - tests/scenarios/number_modifiers.json
  - tests/scenarios/hotkey_combinations.json
  - tests/scenarios/config_reload.json

global_setup:
  build_daemon: true
  clean_state: true

global_teardown:
  collect_logs: true
  cleanup_devices: true
```

**Test Report Format** (JSON):
```json
{
  "suite": "Complete Feature Test Suite",
  "timestamp": "2025-12-15T00:30:00Z",
  "summary": {
    "total_scenarios": 6,
    "total_test_cases": 42,
    "passed": 40,
    "failed": 2,
    "duration_ms": 5234
  },
  "results": [
    {
      "scenario": "basic_key_remapping",
      "status": "PASSED",
      "test_cases": [
        {
          "name": "press_release_a_key",
          "status": "PASSED",
          "duration_ms": 102,
          "latency_us": 27,
          "diff": null
        }
      ]
    },
    {
      "scenario": "modal_modifiers",
      "status": "FAILED",
      "test_cases": [
        {
          "name": "m0_activation",
          "status": "FAILED",
          "duration_ms": 1050,
          "error": "Timeout waiting for output",
          "diff": {
            "expected": [{"evdev": 15, "type": "press"}],
            "actual": []
          }
        }
      ]
    }
  ]
}
```

---

## Test Scenario Library

### Core Features to Test

1. **Basic Key Remapping** (tests/scenarios/basic_remap.json)
   - Single key press/release
   - Multiple independent keys
   - Extended keys (E0 prefix)
   - Key repeat events

2. **Modifier Keys** (tests/scenarios/modifier_keys.json)
   - Shift, Ctrl, Alt combinations
   - Modifier state tracking
   - Both hardware modifiers

3. **Modal Modifiers** (tests/scenarios/modal_modifiers.json)
   - M0-M7 activation/deactivation
   - Modifier layers
   - Nested modifiers
   - Modifier timeout behavior

4. **Number Modifiers** (tests/scenarios/number_modifiers.json)
   - Number keys → Shift mapping
   - Combination with other modifiers
   - Edge cases (0-9 range)

5. **Hotkey Combinations** (tests/scenarios/hotkey_combinations.json)
   - Ctrl+C, Ctrl+V, etc.
   - Multi-key chords
   - Order-dependent sequences

6. **Config Hot Reload** (tests/scenarios/config_reload.json)
   - Switch config while events flowing
   - Verify new mappings applied
   - No dropped events during reload

7. **Performance & Latency** (tests/scenarios/performance.json)
   - Sustained input load (100+ keys/sec)
   - Latency measurements (p50, p99)
   - Queue depth under load

8. **Edge Cases** (tests/scenarios/edge_cases.json)
   - Unmapped keys pass through
   - Invalid evdev codes
   - Simultaneous press/release
   - Rapid key repeat

---

## Implementation Plan

### Phase 1: Core CLI Tools
1. Implement `yamy-inject` with uinput support
2. Implement `yamy-capture` with event monitoring
3. Add JSON serialization/deserialization
4. Test tools independently

### Phase 2: Test Runner
1. Implement scenario parser (JSON)
2. Implement daemon lifecycle management
3. Implement inject → capture → verify pipeline
4. Add result comparison and reporting

### Phase 3: Test Scenarios
1. Create comprehensive scenario library
2. Write test cases for all features
3. Document scenario format and examples

### Phase 4: Integration
1. Add CMake targets for test tools
2. Create CI/CD workflow for automated testing
3. Integrate with existing test infrastructure
4. Document usage and best practices

---

## Directory Structure

```
yamy/
├── src/
│   └── test/
│       ├── yamy_inject_main.cpp      # yamy-inject CLI
│       ├── yamy_capture_main.cpp     # yamy-capture CLI
│       ├── yamy_test_runner_main.cpp # yamy-test-runner CLI
│       ├── test_scenario.h           # Scenario data structures
│       ├── test_scenario_parser.cpp  # JSON/YAML parsing
│       ├── output_verifier.cpp       # Compare actual vs expected
│       └── daemon_manager.cpp        # Daemon lifecycle control
├── tests/
│   ├── scenarios/
│   │   ├── basic_remap.json
│   │   ├── modifier_keys.json
│   │   ├── modal_modifiers.json
│   │   ├── number_modifiers.json
│   │   ├── hotkey_combinations.json
│   │   ├── config_reload.json
│   │   ├── performance.json
│   │   └── edge_cases.json
│   └── suites/
│       ├── all_features.yaml
│       ├── regression.yaml
│       └── smoke.yaml
└── docs/
    └── TESTING_FRAMEWORK.md          # User guide
```

---

## Dependencies

### Required Libraries
- **libudev**: Device enumeration for finding virtual keyboard
- **libevdev** (optional): Simplified evdev handling
- **nlohmann/json** or **RapidJSON**: JSON parsing
- **yaml-cpp**: YAML test suite format

### Platform Requirements
- Linux with `/dev/input/event*` support
- uinput kernel module loaded
- Permissions to create uinput devices (root or input group)

---

## Usage Examples

### Example 1: Test Basic Remapping

```bash
# 1. Start daemon with test config
yamy --config keymaps/test-basic.mayu &

# 2. Inject A key press/release
yamy-inject --key A --sequence > /tmp/inject.log &

# 3. Capture output
yamy-capture --count 2 --format json > /tmp/capture.log

# 4. Verify
diff <(jq '.expected' test.json) <(jq '.' /tmp/capture.log)
```

### Example 2: Automated E2E Test

```bash
# Run single scenario
yamy-test-runner --scenario tests/scenarios/basic_remap.json

# Run full suite
yamy-test-runner --suite tests/suites/all_features.yaml --report report.html
```

### Example 3: Performance Testing

```bash
# Generate sustained load
yamy-inject --scenario tests/scenarios/performance.json &

# Monitor latency
yamy-capture --stream --format json | jq '.latency_us'
```

---

## Validation Strategy

### Test Coverage Goals
- **Unit Tests**: 80%+ coverage (existing)
- **Integration Tests**: All EventProcessor layers
- **E2E Tests**: All user-facing features
- **Performance Tests**: Latency < 1ms p99

### Continuous Integration
```yaml
# .github/workflows/test.yml
jobs:
  e2e-tests:
    runs-on: ubuntu-latest
    steps:
      - name: Build daemon and test tools
        run: cmake --build build --target all

      - name: Load uinput module
        run: sudo modprobe uinput

      - name: Run E2E test suite
        run: build/yamy-test-runner --suite tests/suites/all_features.yaml

      - name: Upload test report
        uses: actions/upload-artifact@v3
        with:
          name: test-report
          path: report.html
```

---

## Future Enhancements

1. **GUI Test Inspector**: Visual tool to view test execution in real-time
2. **Fuzzing Integration**: Property-based testing with random key sequences
3. **Regression Detection**: Automatic comparison with baseline performance
4. **Multi-Layout Testing**: Test across different keyboard layouts (US, JP, DE, etc.)
5. **Remote Testing**: Run tests against daemon on different machine
6. **Coverage Integration**: Map test scenarios to code coverage

---

## References

- ARCHITECTURE_REFACTOR.md - Architecture deep-dive
- tests/README_AUTOMATED_TESTING.md - Existing test infrastructure
- src/platform/linux/keycode_mapping.cpp - evdev ↔ YAMY translation
- src/test/yamy_test_main.cpp - Existing test utility
