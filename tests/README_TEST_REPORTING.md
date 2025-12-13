# Test Reporting Guide

This guide explains how to use the automated testing framework and generate HTML reports.

## Overview

The testing framework consists of two components:

1. **automated_keymap_test.py** - Autonomous test framework that tests all key substitutions
2. **generate_test_report.py** - HTML report generator with color-coded results

## Quick Start

### 1. Run Tests and Export Results

```bash
# Run tests and export to JSON
python3 tests/automated_keymap_test.py \
    --config keymaps/config_clean.mayu \
    --json test_results.json
```

### 2. Generate HTML Report

```bash
# Generate HTML report from JSON results
python3 tests/generate_test_report.py \
    --input test_results.json \
    --output test_report.html \
    --config keymaps/config_clean.mayu \
    --baseline 50.0
```

### 3. View Report

Open `test_report.html` in your web browser to see:
- 📊 Test statistics (total, passed, failed, pass rate)
- 📈 Comparison to baseline (50% from task 1.6)
- ❌ Detailed failure analysis
- 🎨 Color-coded visualization

## Detailed Usage

### Automated Testing Framework

**automated_keymap_test.py** autonomously tests all key substitutions:

```bash
python3 tests/automated_keymap_test.py [OPTIONS]

Options:
  --config PATH       Path to .mayu config file (default: keymaps/config_clean.mayu)
  --yamy-test PATH    Path to yamy-test binary (auto-detected if not specified)
  --log PATH          Path to YAMY log file (default: /tmp/yamy_test.log)
  --json PATH         Export results to JSON file
```

**Prerequisites:**
- YAMY must be running
- Set `YAMY_DEBUG_KEYCODE=1` environment variable for logging
- `yamy-test` utility must be available

**Example:**
```bash
# Start YAMY with debug logging
export YAMY_DEBUG_KEYCODE=1
./build/bin/yamy &

# Run tests
python3 tests/automated_keymap_test.py \
    --config keymaps/config_clean.mayu \
    --json results.json
```

### Report Generator

**generate_test_report.py** creates HTML reports from test results:

```bash
python3 tests/generate_test_report.py [OPTIONS]

Options:
  --input PATH        Path to test results JSON file
  --output PATH       Path to output HTML file (default: test_report.html)
  --baseline FLOAT    Baseline pass rate for comparison (default: 50.0)
  --config PATH       Path to .mayu config file (for report header)
  --stats-only        Generate report from stats only (no detailed results)
  --total N           Total number of tests (stats-only mode)
  --passed N          Number of passed tests (stats-only mode)
  --failed N          Number of failed tests (stats-only mode)
```

**Example with JSON:**
```bash
python3 tests/generate_test_report.py \
    --input test_results.json \
    --output report.html \
    --baseline 50.0 \
    --config keymaps/config_clean.mayu
```

**Example with stats-only:**
```bash
python3 tests/generate_test_report.py \
    --stats-only \
    --total 174 \
    --passed 150 \
    --failed 24 \
    --output report.html
```

## Understanding the Report

### Report Sections

1. **Test Summary**
   - Total tests: Number of tests (substitutions × 2 for PRESS/RELEASE)
   - Passed: Number of successful tests
   - Failed: Number of failed tests
   - Pass Rate: Percentage of tests passed

2. **Baseline Comparison**
   - Shows improvement vs baseline (50% from task 1.6)
   - Color-coded: Green for improvement, Red for regression
   - Includes improvement percentage

3. **Test Failures** (if any)
   - **No Output Detected**: Events not reaching Layer 3 (YAMY may not be running)
   - **Wrong Output**: Incorrect evdev codes produced (layer processing errors)
   - Each failure shows:
     - Input key → Output key (event type)
     - Input/Expected/Actual evdev codes
     - Error message

### Color Coding

- 🟢 **Green**: Passed tests, improvements
- 🔴 **Red**: Failed tests, regressions
- 🟡 **Yellow**: Pass rate indicator

## Complete Workflow

Here's a complete workflow for testing and reporting:

```bash
# 1. Ensure YAMY is built
make build

# 2. Start YAMY with debug logging
export YAMY_DEBUG_KEYCODE=1
./build/bin/yamy &

# 3. Run automated tests
python3 tests/automated_keymap_test.py \
    --config keymaps/config_clean.mayu \
    --json test_results.json

# 4. Generate HTML report
python3 tests/generate_test_report.py \
    --input test_results.json \
    --output test_report_$(date +%Y%m%d_%H%M%S).html \
    --config keymaps/config_clean.mayu \
    --baseline 50.0

# 5. View report
xdg-open test_report_*.html
```

## Troubleshooting

### No YAMY Output

**Problem**: Tests report "No output detected in logs"

**Solutions:**
1. Check YAMY is running: `pgrep yamy`
2. Verify debug logging enabled: `echo $YAMY_DEBUG_KEYCODE` should show "1"
3. Check log access: Try `journalctl -u yamy -n 10 --no-pager`

### Wrong Output Evdev Codes

**Problem**: Tests show incorrect evdev codes

**Solutions:**
1. Check EventProcessor integration (task 2.6)
2. Verify layer functions (tasks 2.2, 2.3, 2.4)
3. Review logs for layer flow: `[LAYER1:IN] → [LAYER2:SUBST] → [LAYER3:OUT]`

### JSON Export Fails

**Problem**: Cannot export results to JSON

**Solutions:**
1. Check write permissions for output directory
2. Verify tests completed successfully
3. Ensure `--json` argument has a valid path

## Integration with CI/CD

The test framework and report generator are designed for CI/CD integration:

```bash
#!/bin/bash
# Example CI script

set -e

# Start YAMY
export YAMY_DEBUG_KEYCODE=1
./build/bin/yamy &
YAMY_PID=$!

# Wait for startup
sleep 2

# Run tests
python3 tests/automated_keymap_test.py \
    --config keymaps/config_clean.mayu \
    --json test_results.json

# Stop YAMY
kill $YAMY_PID

# Generate report
python3 tests/generate_test_report.py \
    --input test_results.json \
    --output test_report.html \
    --baseline 50.0

# Check results
PASS_RATE=$(jq '.stats.pass_rate' test_results.json)
if (( $(echo "$PASS_RATE >= 90.0" | bc -l) )); then
    echo "✓ Tests passed with $PASS_RATE% pass rate"
    exit 0
else
    echo "✗ Tests failed with $PASS_RATE% pass rate (minimum: 90%)"
    exit 1
fi
```

## Files

- `tests/automated_keymap_test.py` - Autonomous test framework
- `tests/generate_test_report.py` - HTML report generator
- `tests/README_TEST_REPORTING.md` - This guide
- `tests/README_AUTOMATED_TESTING.md` - Detailed test framework documentation

## Next Steps

See **tasks.md** for the complete testing roadmap:
- Task 3.6: ✓ Create test report generator (this task)
- Task 3.7: Create CI test runner script
- Task 3.8: Validate 100% pass rate for all substitutions

## References

- Design: `.spec-workflow/specs/key-remapping-consistency/design.md`
- Requirements: `.spec-workflow/specs/key-remapping-consistency/requirements.md`
- Baseline: `docs/INVESTIGATION_FINDINGS.md` (50% pass rate)
