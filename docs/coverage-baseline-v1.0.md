# YAMY v1.0 Coverage Baseline Report

**Generated**: 2025-12-12
**Test Suite**: yamy_regression_test (288 tests)
**Build Configuration**: `-DENABLE_COVERAGE=ON -DBUILD_REGRESSION_TESTS=ON -DCMAKE_BUILD_TYPE=Debug`

## Executive Summary

| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| **Overall Line Coverage** | 17.91% | 80% | BELOW TARGET |
| **Lines Covered** | 2,600 | 11,614 | Gap: 9,013 lines |
| **Total Executable Lines** | 14,517 | - | - |
| **Tests Passing** | 288/288 | 100% | PASS |

## Coverage by Module

| Module | Coverage | Lines | Status |
|--------|----------|-------|--------|
| core/audio | 0.00% | 0/3 | CRITICAL |
| core/commands | 0.25% | 11/4,438 | CRITICAL |
| core/engine | 0.00% | 0/904 | CRITICAL |
| core/functions | 1.75% | 8/457 | CRITICAL |
| core/input | 25.92% | 233/899 | CRITICAL |
| core/logging | 70.45% | 31/44 | LOW |
| core/notification_dispatcher | 50.00% | 52/104 | LOW |
| core/platform | 22.62% | 57/252 | CRITICAL |
| core/settings | 49.09% | 1,516/3,088 | CRITICAL |
| platform/linux | 19.26% | 563/2,923 | CRITICAL |
| utils | 9.96% | 129/1,405 | CRITICAL |

## Critical Coverage Gaps

### Priority 1: Large files with near-zero coverage

1. **src/core/commands/command_base.h** - 0.4% (11/3,035 lines)
   - Contains all command implementations
   - Most commands untested
   - Recommendation: Add unit tests for each command class

2. **src/core/engine/** - 0.0% (0/904 lines)
   - Engine core logic completely untested
   - Files: engine_lifecycle.cpp, engine_focus.cpp, engine_generator.cpp
   - Recommendation: Add integration tests for engine state machine

3. **src/core/functions/function.cpp** - 1.1% (4/365 lines)
   - Function evaluation untested
   - Recommendation: Add tests for built-in functions

### Priority 2: Platform layer coverage

4. **src/platform/linux/window_system_linux*.cpp** - ~15% average
   - X11 window operations minimally tested
   - Requires X server (Xvfb) for testing
   - Recommendation: Mock X11 calls or add integration tests

5. **src/platform/linux/input_hook_linux.cpp** - 29.7% (102/344 lines)
   - Input capture partially tested
   - Recommendation: Add device simulation tests

### Priority 3: Settings and configuration

6. **src/core/settings/setting_loader.cpp** - 31.2% (272/873 lines)
   - Config parsing partially tested
   - Recommendation: Add edge case tests for parser

7. **src/core/settings/session_manager.cpp** - 7.9% (27/341 lines)
   - Session management untested
   - Recommendation: Add session state tests

## Well-Covered Components

| File | Coverage | Notes |
|------|----------|-------|
| config_metadata.cpp | 90.12% | Good coverage |
| notification_dispatcher.cpp | 100.00% | Fully tested |
| parser.cpp | ~65% | Reasonable coverage |
| keymap.cpp | 41.0% | Core functionality tested |

## Recommendations for v1.1

### Short-term (to reach 50%)
1. Add unit tests for top 20 commands in command_base.h
2. Add engine state machine tests
3. Add parser edge case tests

### Medium-term (to reach 80%)
1. Mock X11 calls for window system tests
2. Add comprehensive function evaluation tests
3. Add input device simulation tests

## Test Execution Details

```
Build: cmake -B build-coverage -DENABLE_COVERAGE=ON -DBUILD_REGRESSION_TESTS=ON
Run:   xvfb-run -a build-coverage/bin/yamy_regression_test

Results:
[==========] 288 tests from 37 test suites ran.
[  PASSED  ] 288 tests.
```

### Test Categories Executed
- Unit tests: Settings, Parser, Input, Commands
- Integration tests: Engine, Platform, Session
- Performance tests: Latency benchmarks
- Stress tests: Memory leak tests

## Notes

- Coverage calculated using gcov (GCC 13.3.0)
- lcov not available for HTML report generation
- Test framework: Google Test
- Coverage excludes: `/usr/*`, test code, googletest library

## Action Items

- [ ] Install lcov for CI coverage reporting (Task 3.3)
- [ ] Prioritize command_base.h testing for v1.1
- [ ] Add mock infrastructure for X11 testing
- [ ] Target 50% coverage for v1.1 release
- [ ] Target 80% coverage for v1.2 release

**Note**: GitHub issues are disabled for this repository. Coverage gaps are tracked in this document.
