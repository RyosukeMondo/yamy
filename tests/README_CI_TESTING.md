# CI Test Runner Documentation

## Overview

The `run_all_tests.sh` script is a comprehensive, CI-ready test runner that executes all test phases for the YAMY key remapping system. It implements **Task 3.7** of the Key Remapping Consistency specification.

## Features

- **Fully Autonomous**: No user interaction required
- **Complete Test Coverage**: Runs unit, integration, and E2E tests
- **YAMY Lifecycle Management**: Automatically starts and stops YAMY for testing
- **Robust Error Handling**: Proper cleanup on failure or interruption
- **CI/CD Ready**: Exit codes reflect test status (0 = pass, non-zero = fail)
- **Test Reporting**: Generates HTML report with comparison to baseline

## Test Phases

The script executes tests in the following order:

### Phase 1: Unit Tests (C++ GoogleTest)
- **Binary**: `build/bin/yamy_event_processor_ut`
- **Tests**: Layer 1, Layer 2, Layer 3 functions in isolation
- **Coverage**:
  - Layer 1: evdev → YAMY scan code mapping
  - Layer 2: Substitution table lookup
  - Layer 3: YAMY scan code → evdev output mapping

### Phase 2: Integration Tests (C++ GoogleTest)
- **Binary**: `build/bin/yamy_event_processor_it`
- **Tests**: Complete Layer 1→2→3 composition
- **Coverage**:
  - End-to-end event transformations (e.g., W→A, N→LShift)
  - Event type preservation (PRESS→PRESS, RELEASE→RELEASE)
  - Passthrough for unmapped keys

### Phase 3: E2E Tests (Python Autonomous Framework)
- **Script**: `tests/automated_keymap_test.py`
- **Tests**: All 87 substitutions with live YAMY instance
- **Coverage**:
  - 87 substitutions × 2 event types = 174 total tests
  - Synthetic key injection via `yamy-test` utility
  - Output verification via debug log parsing

### Phase 4: Test Report Generation
- **Script**: `tests/generate_test_report.py`
- **Output**: HTML report at `/tmp/test_report.html`
- **Features**:
  - Color-coded pass/fail visualization
  - Comparison to baseline (50% from task 1.6)
  - Detailed failure information for debugging

## Usage

### Basic Usage

```bash
cd /home/rmondo/repos/yamy
./tests/run_all_tests.sh
```

### Custom Log Directory

```bash
LOG_DIR=/custom/path ./tests/run_all_tests.sh
```

### View Test Report

After running tests, open the HTML report:

```bash
xdg-open /tmp/test_report.html
# or
firefox /tmp/test_report.html
```

## Exit Codes

- **0**: All tests passed
- **1**: One or more test phases failed

Use in CI/CD:

```bash
if ./tests/run_all_tests.sh; then
    echo "Tests passed - safe to merge"
else
    echo "Tests failed - check /tmp/test_report.html"
    exit 1
fi
```

## Prerequisites

### Required Binaries
- `build/bin/yamy` - YAMY daemon
- `build/bin/yamy-test` - Test injection utility
- `build/bin/yamy_event_processor_ut` - Unit tests
- `build/bin/yamy_event_processor_it` - Integration tests

Build these with:

```bash
cd build
make  # or ninja
```

### Required Files
- `tests/automated_keymap_test.py` - E2E test framework
- `tests/generate_test_report.py` - Report generator
- `keymaps/config_clean.mayu` - Test configuration

### Runtime Requirements
- Python 3 (no external dependencies)
- Linux (uses evdev for input events)

## Script Behavior

### Startup Sequence

1. **Pre-flight Checks**
   - Verify all binaries exist and are executable
   - Check Python scripts are present
   - Verify config file exists
   - Warn if YAMY is already running

2. **YAMY Startup**
   - Clear old log files
   - Start YAMY with `YAMY_DEBUG_KEYCODE=1` for debug logging
   - Wait up to 5 seconds for initialization
   - Verify process is running

3. **Test Execution**
   - Run unit tests (fail-fast on error)
   - Run integration tests (fail-fast on error)
   - Run E2E tests (fail-fast on error)
   - Generate HTML report (non-fatal if fails)

4. **Cleanup**
   - Stop YAMY gracefully (SIGTERM)
   - Wait up to 5 seconds for shutdown
   - Force kill if necessary (SIGKILL)
   - Kill any stray YAMY processes

### Cleanup Guarantees

The script uses `trap` to ensure cleanup runs on:
- Normal exit
- Error exit (`set -e`)
- User interrupt (Ctrl+C)
- Termination signal

YAMY will **always** be stopped, even if tests fail or script is interrupted.

## Configuration

Edit these variables at the top of `run_all_tests.sh`:

```bash
# Timeouts (seconds)
YAMY_STARTUP_TIMEOUT=5    # Wait for YAMY to start
YAMY_SHUTDOWN_TIMEOUT=5   # Wait for graceful shutdown
TEST_TIMEOUT=60           # Maximum test duration

# Baseline for comparison
BASELINE_PASS_RATE=50.0   # From task 1.6 investigation

# Log locations (can override with env var)
LOG_DIR="${LOG_DIR:-/tmp}"
```

## Troubleshooting

### "YAMY process died during startup"

Check YAMY log for errors:
```bash
cat /tmp/yamy_test.log
```

Common causes:
- Missing dependencies
- Invalid config file
- Permissions issues with uinput

### "Missing or not executable: [binary]"

Rebuild the project:
```bash
cd build
make clean
make
```

### "E2E tests FAILED"

Check if YAMY is processing events:
```bash
tail -f /tmp/yamy_test.log
# In another terminal:
./build/bin/yamy-test inject 17 PRESS  # Test W key
```

Look for `[LAYER1:IN]`, `[LAYER2:SUBST]`, `[LAYER3:OUT]` log entries.

### Tests hang or timeout

Kill stray YAMY processes:
```bash
pkill -9 yamy
```

Then retry:
```bash
./tests/run_all_tests.sh
```

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: YAMY Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake ninja-build

    - name: Build
      run: |
        mkdir -p build
        cd build
        cmake .. -GNinja
        ninja

    - name: Run Tests
      run: ./tests/run_all_tests.sh

    - name: Upload Test Report
      if: always()
      uses: actions/upload-artifact@v2
      with:
        name: test-report
        path: /tmp/test_report.html
```

### GitLab CI Example

```yaml
test:
  stage: test
  script:
    - mkdir -p build && cd build
    - cmake .. && make
    - cd .. && ./tests/run_all_tests.sh
  artifacts:
    when: always
    paths:
      - /tmp/test_report.html
    expire_in: 1 week
```

## Output Example

```
================================================================================
YAMY Comprehensive Test Runner
================================================================================

[i] Project: /home/rmondo/repos/yamy
[i] Build: /home/rmondo/repos/yamy/build
[i] Logs: /tmp

================================================================================
Pre-flight Checks
================================================================================

[✓] Found YAMY binary
[✓] Found yamy-test utility
[✓] Found Unit test binary
[✓] Found Integration test binary
[✓] Found Automated test framework
[✓] Found Report generator
[✓] Found Python 3: Python 3.10.12
[✓] Found config file: keymaps/config_clean.mayu
[✓] All pre-flight checks passed

================================================================================
Starting YAMY in Test Mode
================================================================================

[i] Starting YAMY with debug logging enabled...
[i] Log file: /tmp/yamy_test.log
[i] YAMY PID: 12345
[i] Waiting for YAMY to initialize...
[✓] YAMY started successfully

================================================================================
Phase 1: Unit Tests (C++ GoogleTest)
================================================================================

[i] Running unit tests for EventProcessor layers...
[i] Tests: Layer 1 (evdevToYamy), Layer 2 (substitution), Layer 3 (yamyToEvdev)
[==========] Running 18 tests from 3 test suites.
[----------] 6 tests from Layer1Tests
[----------] 6 tests from Layer2Tests
[----------] 6 tests from Layer3Tests
[==========] 18 tests from 3 test suites ran. (42 ms total)
[  PASSED  ] 18 tests.
[✓] Unit tests PASSED

================================================================================
Phase 2: Integration Tests (C++ GoogleTest)
================================================================================

[i] Running integration tests for Layer 1→2→3 composition...
[i] Tests: Complete event flow, event type preservation, known substitutions
[==========] Running 12 tests from 1 test suite.
[----------] 12 tests from EventProcessorIntegrationTests
[==========] 12 tests from 1 test suite ran. (68 ms total)
[  PASSED  ] 12 tests.
[✓] Integration tests PASSED

================================================================================
Phase 3: E2E Tests (Python Autonomous Framework)
================================================================================

[i] Running end-to-end tests with live YAMY instance...
[i] Tests: All 87 substitutions × 2 event types (PRESS/RELEASE) = 174 tests
[i] Config: keymaps/config_clean.mayu
[i] Results will be exported to: /tmp/test_results.json

Testing substitutions: 100%|████████████████████| 174/174 [00:45<00:00, 3.84 tests/s]

Pass rate: 100.0% (174/174 tests passed)
[✓] E2E tests PASSED

================================================================================
Phase 4: Test Report Generation
================================================================================

[i] Generating HTML test report...
[i] Input: /tmp/test_results.json
[i] Output: /tmp/test_report.html
[i] Baseline: 50.0%
[✓] Test report generated: /tmp/test_report.html

================================================================================
Test Summary
================================================================================

[✓] All test phases PASSED
[i] Test report: /tmp/test_report.html

================================================================================
Test Run Complete
================================================================================

[✓] All tests passed successfully!
[i] View detailed report: /tmp/test_report.html
```

## Next Steps

After running tests successfully:

1. **Review Test Report**: Open `/tmp/test_report.html` to see detailed results
2. **Verify Pass Rate**: Ensure 100% pass rate for production readiness
3. **Check Logs**: Review `/tmp/yamy_test.log` for event flow analysis
4. **Compare to Baseline**: Report should show improvement from 50% to 100%

## Related Documentation

- **Automated Testing**: `tests/README_AUTOMATED_TESTING.md`
- **Test Reporting**: `tests/README_TEST_REPORTING.md`
- **Investigation Findings**: `docs/INVESTIGATION_FINDINGS.md` (baseline)
- **Refactoring Validation**: `docs/REFACTORING_VALIDATION.md`

## Task Reference

This script implements **Task 3.7** of the Key Remapping Consistency specification:

> Create CI test runner script that orchestrates all test phases:
> - Start YAMY in test mode with debug logging
> - Run unit tests (C++ GoogleTest)
> - Run integration tests (C++ GoogleTest)
> - Run E2E tests (Python autonomous framework)
> - Generate test report
> - Stop YAMY
> - Exit with status 0 for all pass, non-zero for any failures

**Requirements**: 5 (Automated Testing), 6 (Test Coverage)

**Success Criteria**:
- ✓ Script successfully runs all test phases
- ✓ Starts/stops YAMY cleanly
- ✓ Generates comprehensive report
- ✓ Exits with correct status
- ✓ Handles failures gracefully
- ✓ Ready for CI/CD integration
- ✓ Runs completely autonomously
