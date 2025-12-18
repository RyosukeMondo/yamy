# Tasks: M00 Integration Test Automation

## Overview

This spec fixes the root cause of integration test failures and eliminates manual debugging. All tasks focus on making tests work reliably and automatically.

**Total Tasks**: 6
**Estimated Effort**: 8-12 hours
**Priority**: Critical - blocks automated verification

---

## Task 1: Create EventSimulator Utility

**Status**: [ ] Pending

**Description**: Create EventSimulator class to inject events with proper timing and synchronization

**Files**:
- `tests/test_utils/event_simulator.h` (new)
- `tests/test_utils/event_simulator.cpp` (new)
- `tests/CMakeLists.txt` (modify - add test_utils)

**Requirements**: US-4 (Reproducible Test Scenarios)

**Success Criteria**:
- EventSimulator can inject sequences with timing
- `waitForEngineReady()` polls engine status
- `waitForOutput()` waits for mock injector calls
- Compiles without errors
- Can be included in test files

**_Prompt**:
```
Role: Test Infrastructure Engineer

Task: Implement the EventSimulator utility class for M00 integration tests in spec m00-integration-test-automation. First run spec-workflow-guide to get the workflow guide then implement the task.

Create a new EventSimulator class that provides:
1. Event injection with configurable delays between events
2. Engine initialization synchronization (waitForEngineReady)
3. Output synchronization (waitForOutput for mock injector)
4. Proper evdev code handling

Restrictions:
- Do NOT modify existing test files yet (that's Task 3)
- Do NOT use busy-wait loops (use proper sleep/condition variables)
- Do NOT hardcode timeout values (make them configurable)

Leverage:
- Existing MockInputInjector interface
- Engine::getState() or similar status check
- std::chrono for timing

Requirements: US-4 (Reproducible Test Scenarios)

Success: EventSimulator compiles, provides timing control, and synchronization helpers work correctly

After implementation:
1. Edit tasks.md: Change this task from [ ] to [-] before starting
2. Implement the EventSimulator class
3. Log implementation with log-implementation tool including all artifacts (classes, functions, etc.)
4. Edit tasks.md: Change this task from [-] to [x] when complete
```

---

## Task 2: Add Integration Test Logging

**Status**: [ ] Pending

**Description**: Add detailed logging to EventProcessor and ModifierKeyHandler for test debugging

**Files**:
- `src/core/engine/engine_event_processor.cpp` (modify)
- `src/core/engine/modifier_key_handler.cpp` (modify)

**Requirements**: US-3 (Comprehensive Observability)

**Success Criteria**:
- Logs show: event input (evdev code), modifier activation, rule matching, output generation
- Logs include timestamps for timing verification
- Logs are conditional on YAMY_DEBUG_KEYCODE or similar flag
- Existing functionality unchanged

**_Prompt**:
```
Role: Observability Engineer

Task: Add comprehensive logging to M00 event processing for spec m00-integration-test-automation. First run spec-workflow-guide to get the workflow guide then implement the task.

Add DEBUG-level logging to:
1. EventProcessor::processEvent() - log input, modifier checks, rule lookups, outputs
2. ModifierKeyHandler::checkAndActivateWaitingModifiers() - log threshold checks, activations
3. ModifierKeyHandler::processNumberKey() - log state transitions (WAITING → ACTIVE)

Log format: [TEST] prefix for easy filtering, include hex codes for keys/scancodes

Restrictions:
- Do NOT change any logic, only add logging
- Do NOT log on every event (only when m_debugLogging is true or env var set)
- Do NOT add excessive logging that impacts performance

Leverage:
- Existing Logger infrastructure (LOG_DEBUG, LOG_INFO)
- Existing m_debugLogging flag in EventProcessor
- printf to stderr for fallback if logger unavailable

Requirements: US-3 (Comprehensive Observability)

Success: Running tests with YAMY_DEBUG_KEYCODE=1 shows detailed event flow, state transitions, and timing

After implementation:
1. Edit tasks.md: Change this task from [ ] to [-] before starting
2. Add logging to EventProcessor and ModifierKeyHandler
3. Log implementation with log-implementation tool
4. Edit tasks.md: Change this task from [-] to [x] when complete
```

---

## Task 3: Fix EngineTestFixture Initialization

**Status**: [ ] Pending

**Description**: Update test fixture to properly initialize Engine and wait for readiness before injecting events

**Files**:
- `tests/test_m00_integration.cpp` (modify)

**Requirements**: US-2 (Root Cause Investigation), US-4 (Reproducible Test Scenarios)

**Success Criteria**:
- Engine::start() completes fully before tests inject events
- Setting application waits for EventProcessor registration
- loadJsonConfig() uses proper synchronization
- Tests use evdev codes (not YAMY scan codes)
- No race conditions during initialization

**_Prompt**:
```
Role: Test Infrastructure Engineer

Task: Fix the EngineTestFixture initialization and event injection in spec m00-integration-test-automation. First run spec-workflow-guide to get the workflow guide then implement the task.

Fix the test fixture to:
1. Wait for Engine to be fully initialized after start()
2. Wait additional time after setSetting() for EventProcessor to register triggers
3. Use EventSimulator from Task 1 for event injection
4. Convert YAMY scan codes to evdev codes (0x1e → 30 for A key)

Key fixes:
- In loadJsonConfig(): Add waitForEngineReady() call
- In loadJsonConfig(): Increase wait from 100ms to 500ms after setSetting()
- Update injectKey() to use evdev codes properly
- Add helper: yamyToEvdev() conversion function

Restrictions:
- Do NOT re-enable tests yet (that's Task 5)
- Do NOT change test logic, only fix setup and timing
- Do NOT remove DISABLED_ prefix yet

Leverage:
- EventSimulator from Task 1
- Existing yamyToEvdevKeyCode() function if available
- std::this_thread::sleep_for for waits

Requirements: US-2 (Root Cause Investigation), US-4 (Reproducible Test Scenarios)

Success: Test fixture initializes properly, logs show M00 trigger registration before events are injected

After implementation:
1. Edit tasks.md: Change this task from [ ] to [-] before starting
2. Fix EngineTestFixture setup and timing
3. Log implementation with log-implementation tool
4. Edit tasks.md: Change this task from [-] to [x] when complete
```

---

## Task 4: Fix Event Timing and Synchronization

**Status**: [ ] Pending

**Description**: Update test methods to use EventSimulator with proper timing for hold/tap detection

**Files**:
- `tests/test_m00_integration.cpp` (modify)

**Requirements**: US-4 (Reproducible Test Scenarios)

**Success Criteria**:
- Tests inject events with proper delays (e.g., 250ms hold time)
- Tests wait for async processing to complete
- Tests verify output after synchronization
- No hardcoded sleeps - use EventSimulator

**_Prompt**:
```
Role: Test Engineer

Task: Update M00 integration test methods to use proper timing and synchronization for spec m00-integration-test-automation. First run spec-workflow-guide to get the workflow guide then implement the task.

Update each test method (still DISABLED for now):
1. TapAShouldOutputB - Use EventSimulator.injectSequence with 100ms delay
2. HoldAPlusShouldOutputD - Use 250ms delay to exceed threshold
3. VimModeSemicolonPlusHOutputsLeft - Proper hold + combo timing
4. VimModeSemicolonTapOutputsSemicolon - Tap timing <200ms
5. VimModeAllArrowKeys - Multiple hold+key combinations

Pattern for each test:
```cpp
std::vector<EventSimulator::Event> events = {
    {evdev_code, true, 0},        // Press
    {evdev_code, false, delay_ms} // Release after delay
};
simulator.injectSequence(engine, events);
ASSERT_TRUE(simulator.waitForOutput(mockInjector, 1000));
```

Restrictions:
- Do NOT remove DISABLED_ prefix yet (Task 5 does that)
- Do NOT change expected outputs - only fix timing/sync
- Do NOT add new test cases - fix existing 5 tests only

Leverage:
- EventSimulator from Task 1
- evdev code mapping: A=30, S=31, H=35, J=36, K=37, L=38, Semicolon=39
- waitForOutput() to sync with async processing

Requirements: US-4 (Reproducible Test Scenarios)

Success: All 5 test methods use EventSimulator, have proper timing, wait for processing before assertions

After implementation:
1. Edit tasks.md: Change this task from [ ] to [-] before starting
2. Update all 5 test methods with proper timing
3. Log implementation with log-implementation tool
4. Edit tasks.md: Change this task from [-] to [x] when complete
```

---

## Task 5: Re-enable and Verify All Tests

**Status**: [ ] Pending

**Description**: Remove DISABLED_ prefix from all 5 tests and verify they pass consistently

**Files**:
- `tests/test_m00_integration.cpp` (modify)
- `CMakeLists.txt` or test runner config (if needed)

**Requirements**: US-1 (Automated Integration Testing), US-2 (Root Cause Investigation)

**Success Criteria**:
- All 5 tests have DISABLED_ prefix removed
- All tests pass when run individually
- All tests pass when run together
- Tests pass 10 consecutive times (no flaky failures)
- Test execution time <10 seconds total

**_Prompt**:
```
Role: QA Engineer

Task: Re-enable all M00 integration tests and verify reliability for spec m00-integration-test-automation. First run spec-workflow-guide to get the workflow guide then implement the task.

Steps:
1. Remove DISABLED_ prefix from all 5 tests:
   - DISABLED_TapAShouldOutputB → TapAShouldOutputB
   - DISABLED_HoldAPlusShouldOutputD → HoldAPlusShouldOutputD
   - DISABLED_VimModeSemicolonPlusHOutputsLeft → VimModeSemicolonPlusHOutputsLeft
   - DISABLED_VimModeSemicolonTapOutputsSemicolon → VimModeSemicolonTapOutputsSemicolon
   - DISABLED_VimModeAllArrowKeys → VimModeAllArrowKeys

2. Remove the comment blocks explaining why tests were disabled

3. Build and run tests: `./build_ninja/bin/yamy_m00_integration_test`

4. If any test fails:
   - Check logs with YAMY_DEBUG_KEYCODE=1
   - Verify timing values (may need adjustment)
   - Check synchronization (waitForOutput timeout)
   - Fix and re-test

5. Run 10 times to verify: `for i in {1..10}; do ./build_ninja/bin/yamy_m00_integration_test || exit 1; done`

Restrictions:
- Do NOT disable tests again - fix the issue if they fail
- Do NOT skip verification - must pass 10 consecutive times
- Do NOT proceed if execution time >10 seconds

Leverage:
- Logging from Task 2 for debugging
- EventSimulator timing control from Task 1
- Build system (cmake)

Requirements: US-1 (Automated Integration Testing), US-2 (Root Cause Investigation)

Success: All 5 tests pass consistently, total execution <10s, test output shows clear pass/fail

After implementation:
1. Edit tasks.md: Change this task from [ ] to [-] before starting
2. Re-enable all tests and verify reliability
3. Log implementation with log-implementation tool including test results
4. Edit tasks.md: Change this task from [-] to [x] when complete
```

---

## Task 6: Document Testing Approach and Add CI

**Status**: [ ] Pending

**Description**: Document the automated testing approach and add tests to CI/CD pipeline

**Files**:
- `tests/README.md` or `docs/testing.md` (new/modify)
- `.github/workflows/tests.yml` or CI config (modify)

**Requirements**: US-5 (CI/CD Integration)

**Success Criteria**:
- Documentation explains M00 test architecture
- CI runs integration tests on every commit
- CI reports test results clearly
- Failed tests block merges

**_Prompt**:
```
Role: DevOps Engineer

Task: Document testing approach and integrate M00 tests into CI/CD for spec m00-integration-test-automation. First run spec-workflow-guide to get the workflow guide then implement the task.

Create/update documentation:
1. How M00 integration tests work (EventSimulator, timing, synchronization)
2. How to run tests locally
3. How to debug test failures (YAMY_DEBUG_KEYCODE=1)
4. Test architecture diagram (optional)

Update CI/CD:
1. Add M00 integration test job to GitHub Actions / CI
2. Run tests on pull requests
3. Report results (pass/fail + execution time)
4. Block merge if tests fail

Restrictions:
- Do NOT skip documentation - critical for maintenance
- Do NOT allow CI failures to be ignored
- Do NOT add flaky test detection - tests must be reliable

Leverage:
- Existing CI configuration (.github/workflows/ or similar)
- CMake test targets
- GitHub Actions or CI platform docs

Requirements: US-5 (CI/CD Integration)

Success: Tests run automatically in CI, documentation is clear, team can maintain tests without manual UAT

After implementation:
1. Edit tasks.md: Change this task from [ ] to [-] before starting
2. Write documentation and update CI config
3. Log implementation with log-implementation tool
4. Edit tasks.md: Change this task from [-] to [x] when complete
```

---

## Task Summary

| Task | Component | Effort | Dependencies |
|------|-----------|--------|--------------|
| 1 | EventSimulator | 2h | None |
| 2 | Logging | 1h | None |
| 3 | Test Fixture | 2h | Task 1 |
| 4 | Test Timing | 2h | Tasks 1, 3 |
| 5 | Re-enable Tests | 2h | Tasks 1-4 |
| 6 | Documentation/CI | 1h | Task 5 |

**Total**: ~10 hours

## Implementation Order

1. **Foundation** (Tasks 1-2) - Can be done in parallel
2. **Fix Infrastructure** (Task 3) - Requires Task 1
3. **Fix Tests** (Task 4) - Requires Tasks 1, 3
4. **Verification** (Task 5) - Requires Tasks 1-4
5. **Integration** (Task 6) - Requires Task 5

## Acceptance Criteria (Overall)

- ✅ All 5 integration tests pass consistently
- ✅ No DISABLED_ prefixes remain
- ✅ Tests run in <10 seconds
- ✅ Pass 100 consecutive runs (ultimate reliability test)
- ✅ CI automatically runs tests
- ✅ Zero manual UAT needed

## Notes

- Each task should be marked in-progress `[-]` before starting
- Use log-implementation tool after EACH task completes
- Mark complete `[x]` only after verification
- If a task blocks, debug with YAMY_DEBUG_KEYCODE=1 logs
