# M00 Integration Testing

## Overview

The M00 integration test suite provides **automated, reliable testing** of the M00 virtual modifier feature (modifier-on-first-key) with proper timing control and synchronization. These tests verify hold/tap detection, modifier activation, and key combination mappings.

## Features

- ✓ Automated testing with proper timing control
- ✓ Reliable hold/tap detection verification
- ✓ Synchronization between event injection and processing
- ✓ Comprehensive test coverage for M00 feature
- ✓ Debug logging support for troubleshooting
- ✓ Fast execution (<10 seconds for full suite)

## Test Architecture

### Components

```
M00 Integration Tests
├── EventSimulator        # Event injection with timing control
│   ├── injectKey()      # Single key press/release
│   ├── injectSequence() # Multiple events with delays
│   ├── waitForEngineReady() # Engine initialization sync
│   └── waitForOutput()  # Async processing sync
├── EngineTestFixture    # Test harness
│   ├── Engine           # Core event processing engine
│   ├── MockInputInjector # Captures output events
│   ├── MockWindowSystem  # Stub window system
│   └── MockInputHook    # Stub input hook
└── Test Methods         # Specific test scenarios
    ├── TapAShouldOutputB
    ├── HoldAPlusShouldOutputD
    ├── VimModeSemicolonPlusHOutputsLeft
    ├── VimModeSemicolonTapOutputsSemicolon
    └── VimModeAllArrowKeys
```

### EventSimulator

The `EventSimulator` utility (tests/test_utils/event_simulator.h) provides:

1. **Event Injection with Timing**
   - `injectKey()`: Inject single key press or release
   - `injectSequence()`: Inject multiple events with configurable delays
   - Uses evdev codes for Linux compatibility

2. **Synchronization Helpers**
   - `waitForEngineReady()`: Wait for Engine initialization (checks Engine::getState())
   - `waitForOutput()`: Wait for MockInputInjector to receive expected output count
   - Prevents race conditions in test execution

3. **Timing Control**
   - Configurable delays between events
   - Proper sleep intervals (not busy-wait loops)
   - Threshold-aware timing for hold/tap detection

### Test Patterns

All tests follow this pattern:

```cpp
TEST_F(EngineTestFixture, TestName) {
    // 1. Load configuration
    loadJsonConfig(TEST_CONFIG);

    // 2. Define event sequence with timing
    std::vector<EventSimulator::Event> events = {
        {30, EventSimulator::PRESS, 0},      // Key A press
        {30, EventSimulator::RELEASE, 250},  // Key A release after 250ms (hold)
        {31, EventSimulator::PRESS, 100},    // Key S press 100ms later
        {31, EventSimulator::RELEASE, 50}    // Key S release 50ms later
    };

    // 3. Inject events
    EventSimulator simulator;
    simulator.injectSequence(engine, events);

    // 4. Wait for processing to complete
    simulator.waitForOutput(mockInjector, 1000);

    // 5. Assert expected output
    EXPECT_EQ(mockInjector->getOutputCount(), 2);
    EXPECT_EQ(mockInjector->getOutput(0), 32); // Expect D key
}
```

## Test Scenarios

### 1. Basic Tap Detection
**Test**: `TapAShouldOutputB`

Tests that tapping the M00 trigger key (A) outputs the tap key (B).

- Press A, Release A within threshold (<200ms)
- Expected: Output B

### 2. Hold + Key Combination
**Test**: `HoldAPlusShouldOutputD`

Tests that holding M00 trigger (A) and pressing another key (S) activates the mapping.

- Press A, Hold 250ms (exceeds 200ms threshold)
- Press S while holding A
- Expected: Output D (M00-S mapping)

### 3. Vim Mode: Arrow Key Navigation
**Tests**: `VimModeSemicolonPlusHOutputsLeft`, `VimModeAllArrowKeys`

Tests Vim-style navigation with Semicolon as M00 trigger.

- Hold Semicolon + H → Left arrow
- Hold Semicolon + J → Down arrow
- Hold Semicolon + K → Up arrow
- Hold Semicolon + L → Right arrow

### 4. Vim Mode: Tap Detection
**Test**: `VimModeSemicolonTapOutputsSemicolon`

Tests that tapping Semicolon outputs Semicolon (not activating M00).

- Press Semicolon, Release within threshold
- Expected: Output Semicolon

## Key Mappings

### Evdev Codes (Linux)

The tests use evdev codes for Linux compatibility:

| Key | YAMY Code | Evdev Code |
|-----|-----------|------------|
| A   | 0x1e      | 30         |
| B   | 0x30      | 48         |
| S   | 0x1f      | 31         |
| D   | 0x20      | 32         |
| H   | 0x23      | 35         |
| J   | 0x24      | 36         |
| K   | 0x25      | 37         |
| L   | 0x26      | 38         |
| Semicolon | 0x27 | 39        |
| Left | 0xE04B   | 105        |
| Down | 0xE050   | 108        |
| Up   | 0xE048   | 103        |
| Right | 0xE04D  | 106        |

## Running Tests Locally

### Prerequisites

1. **Build the tests:**
   ```bash
   mkdir -p build_ninja
   cd build_ninja
   cmake -G Ninja -DBUILD_LINUX_TESTING=ON ..
   ninja yamy_m00_integration_test
   ```

2. **Verify binary exists:**
   ```bash
   ls -la build_ninja/bin/yamy_m00_integration_test
   ```

### Basic Execution

```bash
# Run all M00 integration tests
./build_ninja/bin/yamy_m00_integration_test

# Run specific test
./build_ninja/bin/yamy_m00_integration_test --gtest_filter="*TapA*"

# Run with XML output
./build_ninja/bin/yamy_m00_integration_test --gtest_output=xml:test-results.xml
```

### Debugging Test Failures

#### Enable Debug Logging

```bash
# Set environment variable for detailed event flow logging
export YAMY_DEBUG_KEYCODE=1
./build_ninja/bin/yamy_m00_integration_test
```

This enables `[TEST]` prefixed logs showing:
- Input events (evdev codes in hex)
- Modifier activation/deactivation
- Rule matching
- Output generation
- Timing information

#### Understanding Test Logs

```
[TEST] EventProcessor::processEvent - Input: evdev 0x1e (30) type PRESS
[TEST] ModifierKeyHandler::checkActivation - M00 trigger detected
[TEST] ModifierKeyHandler::processNumberKey - M00 state: WAITING
[TEST] ModifierKeyHandler::checkAndActivateWaitingModifiers - Checking thresholds
[TEST] ModifierKeyHandler::processNumberKey - M00 activated (hold > 200ms)
[TEST] EventProcessor::processEvent - Input: evdev 0x1f (31) type PRESS
[TEST] EventProcessor::lookupRule - Found rule: M00-S → D
[TEST] EventProcessor::generateOutput - Output: evdev 0x20 (32) type PRESS
```

#### Common Issues

**Test fails: Output count mismatch**
- Check timing: Is hold duration exceeding threshold?
- Verify synchronization: Is `waitForOutput()` timeout sufficient?
- Review logs: Are events being processed in correct order?

**Test fails: Wrong output key**
- Check evdev code mapping: Is conversion correct?
- Verify configuration: Is JSON config loaded properly?
- Review rule matching: Is M00 activated before combo key?

**Test hangs or times out**
- Check Engine initialization: Is `waitForEngineReady()` called?
- Verify timing values: Are delays too long?
- Check for deadlocks: Are any locks held indefinitely?

### Verify Test Reliability

Run tests multiple times to ensure consistency:

```bash
# Run 10 consecutive times (should all pass)
for i in {1..10}; do
    echo "Run $i:"
    ./build_ninja/bin/yamy_m00_integration_test || exit 1
done
echo "All 10 runs passed!"
```

Expected execution time: <2 seconds per run

## Integration with CI/CD

The M00 integration tests are integrated into the CI pipeline to run automatically on:
- Pull requests
- Pushes to master/main branch

### CI Configuration

The tests are defined in `.github/workflows/ci.yml` under the `m00-integration-tests` job:

```yaml
m00-integration-tests:
  name: M00 Integration Tests
  runs-on: ubuntu-latest
  timeout-minutes: 5

  steps:
  - uses: actions/checkout@v3

  - name: Install dependencies
    run: |
      sudo apt-get update
      sudo apt-get install -y cmake ninja-build g++

  - name: Build M00 integration tests
    run: |
      cmake -B build -G Ninja -DBUILD_LINUX_TESTING=ON
      cmake --build build --target yamy_m00_integration_test

  - name: Run M00 integration tests
    run: ./build/bin/yamy_m00_integration_test --gtest_output=xml:m00-test-results.xml

  - name: Upload test results
    uses: actions/upload-artifact@v4
    if: always()
    with:
      name: m00-test-results
      path: m00-test-results.xml
```

### CI Behavior

- **On Success**: Tests pass, build continues
- **On Failure**:
  - Test results uploaded as artifact
  - Build fails, merge blocked
  - Review test output and logs to diagnose issue

### Viewing CI Test Results

1. Go to GitHub Actions tab
2. Click on the workflow run
3. Navigate to "M00 Integration Tests" job
4. View test output in step "Run M00 integration tests"
5. Download test results artifact for detailed analysis

## Test Development Guidelines

### Adding New Tests

1. **Define test scenario**
   - What M00 behavior are you testing?
   - What's the expected input/output?

2. **Create test configuration**
   - Define JSON config with necessary keys and mappings
   - Set appropriate holdThresholdMs value

3. **Implement test method**
   ```cpp
   TEST_F(EngineTestFixture, YourTestName) {
       loadJsonConfig(YOUR_CONFIG);

       std::vector<EventSimulator::Event> events = {
           // Define event sequence with timing
       };

       EventSimulator simulator;
       simulator.injectSequence(engine, events);
       simulator.waitForOutput(mockInjector, 1000);

       // Assertions
       EXPECT_EQ(mockInjector->getOutputCount(), expected);
   }
   ```

4. **Verify timing**
   - Use debug logs to verify event timing
   - Adjust delays to reliably trigger hold/tap behavior

5. **Test reliability**
   - Run 10+ times to verify consistency
   - Check that execution time is reasonable (<10s total)

### Timing Guidelines

- **Tap detection**: Release within threshold (typically <200ms)
- **Hold detection**: Hold exceeds threshold (typically ≥200ms)
- **Combo timing**: Hold trigger, then press combo key
- **Inter-event delay**: 50-100ms between independent events
- **Processing wait**: 1000ms timeout for waitForOutput()

### Code Quality

- Keep tests focused and atomic (one behavior per test)
- Use descriptive test names (e.g., `HoldAShouldActivateM00`)
- Document complex timing scenarios with comments
- Follow existing test patterns for consistency

## Related Files

- `tests/test_m00_integration.cpp` - M00 integration test implementation
- `tests/test_utils/event_simulator.h` - EventSimulator utility interface
- `tests/test_utils/event_simulator.cpp` - EventSimulator utility implementation
- `src/core/engine/engine_event_processor.cpp` - EventProcessor with test logging
- `src/core/engine/modifier_key_handler.cpp` - ModifierKeyHandler with test logging
- `.github/workflows/ci.yml` - CI/CD configuration
- `CMakeLists.txt` - Build configuration (yamy_m00_integration_test target)

## Specification Reference

This test suite implements the requirements from:
- `.spec-workflow/specs/m00-integration-test-automation/`
  - `requirements.md` - User stories for automated integration testing
  - `design.md` - Test architecture and synchronization design
  - `tasks.md` - Implementation tasks and validation criteria

## Test Results History

| Date | Pass Rate | Execution Time | Notes |
|------|-----------|----------------|-------|
| 2025-12-18 | 100% | <2s | Initial implementation |
| 2025-12-19 | 100% | <2s | All 5 tests passing consistently |

## Troubleshooting Common Issues

### Issue: Tests fail intermittently

**Symptom**: Tests pass sometimes but fail other times

**Root Causes**:
- Race condition in event processing
- Insufficient wait time for processing
- Timing threshold boundary cases

**Solutions**:
1. Add debug logging: `YAMY_DEBUG_KEYCODE=1`
2. Check timing: Verify delays exceed/stay within thresholds by safe margin
3. Increase wait timeout: Adjust `waitForOutput()` timeout
4. Review synchronization: Ensure `waitForEngineReady()` is called

### Issue: Test execution is slow

**Symptom**: Tests take >10 seconds to complete

**Root Causes**:
- Excessive delays in event sequences
- Long timeout values
- Synchronous processing bottlenecks

**Solutions**:
1. Review event delays: Minimize unnecessary waits
2. Optimize threshold values: Use minimum necessary for reliable detection
3. Check for busy-wait loops: Use proper sleep/condition variables
4. Profile execution: Identify slow test methods

### Issue: Wrong output received

**Symptom**: Test expects key X but receives key Y

**Root Causes**:
- Incorrect evdev code mapping
- Configuration not loaded properly
- M00 not activating when expected

**Solutions**:
1. Verify evdev mapping: Check yamyToEvdev() conversion
2. Review JSON config: Ensure mappings are correct
3. Check logs: Verify M00 activation state
4. Validate timing: Ensure hold duration exceeds threshold

## Future Enhancements

Potential improvements for the test suite:

- **Coverage expansion**: Add tests for edge cases (rapid tap/hold, multi-key combos)
- **Performance benchmarks**: Measure event processing latency
- **Stress testing**: Test with high-frequency event injection
- **Platform variants**: Test on different Linux distributions
- **Vim mode coverage**: Expand Vim-style navigation tests
- **Error injection**: Test error handling and recovery

## Support

For issues or questions about M00 integration testing:

1. Check this documentation first
2. Review test logs with `YAMY_DEBUG_KEYCODE=1`
3. Check specification: `.spec-workflow/specs/m00-integration-test-automation/`
4. Review implementation: `tests/test_m00_integration.cpp`
5. File an issue with test output and logs
