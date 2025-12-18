# Requirements: M00 Integration Test Automation

## Problem Statement

We are stuck in a manual debugging loop for M00 virtual modifier functionality. The current approach has critical flaws:

1. **Integration tests are DISABLED** - we disabled 5 tests claiming "mock environment limitations" instead of fixing them
2. **Manual UAT required** - asking users to manually press keys to verify behavior
3. **No automated verification** - can't prove M00 works without human testing
4. **Low productivity** - endless debug cycles with no systematic approach
5. **Wrong assumption**: "integration tests can't work in mock environment" - this is false, they SHOULD work

**Root Issue**: For a keyboard remapper between OS and application, EVERYTHING is under control and should be fully automated. No manual UAT should be needed.

## User Stories

### US-1: Automated Integration Testing
**As a** developer
**I want** M00 integration tests to pass automatically
**So that** I can verify functionality without manual testing

**EARS Criteria**:
- **WHEN** integration tests run
- **IF** M00 virtual modifiers are configured
- **THEN** tests SHALL verify tap/hold behavior automatically
- **AND** tests SHALL pass without human interaction

**Acceptance Criteria**:
- All 5 disabled M00 integration tests are re-enabled and passing
- Tests run in <10 seconds
- Tests can run in CI/CD pipeline
- No manual key presses required

### US-2: Root Cause Investigation
**As a** developer
**I want** to understand why integration tests failed
**So that** I can fix the real issue instead of working around it

**EARS Criteria**:
- **WHEN** investigating test failures
- **IF** tests show "no output generated"
- **THEN** investigation SHALL identify the actual bug
- **AND** fix SHALL address root cause, not symptoms

**Acceptance Criteria**:
- Document exact failure mode (why mock injector not called)
- Identify if bug is in: test setup, Engine code, or EventProcessor
- Fix eliminates the root cause
- Tests pass reliably

### US-3: Comprehensive Observability
**As a** developer
**I want** detailed logging of M00 event processing
**So that** I can diagnose issues without manual debugging

**EARS Criteria**:
- **WHEN** M00 events are processed
- **IF** debug logging is enabled
- **THEN** system SHALL log: trigger registration, hold/tap detection, modifier activation, rule matching
- **AND** logs SHALL include timestamps and event sequences

**Acceptance Criteria**:
- Log M00 trigger registration with key codes
- Log PRESS/RELEASE events with timing
- Log hold threshold checks (waiting → active transitions)
- Log rule lookup and matching
- Log output generation
- Logs are structured and parseable

### US-4: Reproducible Test Scenarios
**As a** developer
**I want** declarative test scenarios
**So that** I can verify complex M00 behavior reliably

**EARS Criteria**:
- **WHEN** defining test scenarios
- **IF** testing tap vs hold behavior
- **THEN** tests SHALL use time-based event sequences
- **AND** tests SHALL verify exact output without race conditions

**Acceptance Criteria**:
- Test framework supports timed event injection
- Tests verify: tap output, hold suppression, M00+key remapping
- Tests wait for async processing properly
- Tests are deterministic (no flaky failures)

### US-5: CI/CD Integration
**As a** developer
**I want** tests to run in CI/CD automatically
**So that** regressions are caught before deployment

**EARS Criteria**:
- **WHEN** code is committed
- **IF** M00 functionality changes
- **THEN** CI SHALL run integration tests
- **AND** deployment SHALL be blocked if tests fail

**Acceptance Criteria**:
- Tests run in GitHub Actions / CI
- Test results are reported clearly
- Failed tests block merge
- Coverage report shows M00 code paths tested

## Non-Functional Requirements

### NFR-1: Performance
- Integration tests complete in <10 seconds total
- Individual test cases run in <2 seconds
- No timeouts or hangs

### NFR-2: Reliability
- Tests pass 100% consistently (no flaky tests)
- Tests work on clean checkout (no manual setup)
- Tests clean up after themselves

### NFR-3: Maintainability
- Test code is well-documented
- Test failures have clear error messages
- Adding new test cases is straightforward

## Out of Scope

- GUI testing (focus on Engine/EventProcessor integration)
- Performance benchmarking (functional tests only)
- Multi-threading stress tests
- Hardware device testing (use mocks)

## Success Metrics

1. **All 5 disabled tests re-enabled and passing** ✅
2. **Zero manual UAT required** - commit with confidence
3. **Test execution time <10 seconds**
4. **100% test reliability** - no flaky failures
5. **Coverage >90%** for M00 code paths

## Dependencies

- Existing M00 unit tests (already passing)
- Engine architecture (EventProcessor, ModifierKeyHandler)
- Test infrastructure (GoogleTest framework)
- Mock implementations (InputInjector, InputHook, InputDriver)

## Assumptions

1. M00 logic itself is correct (unit tests prove this)
2. Problem is in integration test setup, not M00 implementation
3. Mock environment CAN fully simulate real runtime
4. EventProcessor design supports testability

## Risks

1. **Root cause might be deep** - could require Engine refactoring
2. **Thread timing issues** - async operations need careful handling
3. **Test infrastructure gaps** - might need new test utilities

## Notes

This spec addresses the fundamental question: **"Why are we debugging manually when everything should be automated?"**

The answer requires:
1. Finding the real bug (not disabling tests)
2. Fixing test infrastructure
3. Proving it works automatically
4. Never needing manual UAT again
