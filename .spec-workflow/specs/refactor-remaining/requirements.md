# Refactor Remaining Work - Requirements

## Overview

This spec addresses the remaining work to complete the JSON refactoring project. Based on comprehensive code review, several verification gaps and incomplete deletions were identified that prevent full validation of the implementation.

**Parent Spec**: json-refactoring (Phase 1-5 complete, verification incomplete)
**Status**: Active
**Priority**: P0 (Blocking production readiness)

---

## Functional Requirements

### FR-1: Test Suite Verification
**Priority**: P0 (Critical - Blocking)
**Description**: All unit and integration tests must build and pass to validate the JSON refactoring implementation.

**Acceptance Criteria**:
- [ ] Build system generates all test executables
- [ ] test_json_loader passes all 15+ test cases
- [ ] yamy_property_keymap_test passes all test cases
- [ ] No compilation errors in test targets
- [ ] Test coverage >90% on JsonConfigLoader confirmed
- [ ] All tests run in CI/local environment

**Current Status**: ❌ Build directory missing, tests not run
**Blocker**: Cannot verify claimed ">90% test coverage passes"

**Rationale**: Without passing tests, we cannot confirm the implementation works correctly.

---

### FR-2: M00-MFF End-to-End Validation
**Priority**: P0 (Critical)
**Description**: Virtual modifier system (M00-MFF) must work identically to .mayu version with tap/hold detection.

**Acceptance Criteria**:
- [ ] E2E test loads vim-mode.json config
- [ ] Tap behavior verified: CapsLock quick press → Escape output
- [ ] Hold behavior verified: CapsLock hold + H → Left arrow output
- [ ] Hold threshold (200ms) works correctly
- [ ] M00-MFF combinations work (e.g., Shift-M00-G)
- [ ] Multiple virtual modifiers (M00, M01) work simultaneously

**Current Status**: ⚠️ Implementation exists but not independently verified
**Evidence Needed**: E2E test execution results

**Rationale**: M00-MFF is core functionality - must be proven to work end-to-end.

---

### FR-3: Complete Phase 4 File Deletions
**Priority**: P1 (High)
**Description**: Remove all files marked for deletion in Phase 4 of json-refactoring spec.

**Acceptance Criteria**:
- [ ] All cmd_window_*.cpp files deleted (window manipulation commands)
- [ ] All cmd_clipboard_*.cpp files deleted (clipboard commands)
- [ ] All cmd_emacs_*.cpp files deleted (emacs-specific commands)
- [ ] test_parser.cpp deleted (legacy parser tests)
- [ ] parser.cpp and parser.h fully deleted (not just stubbed)
- [ ] CMakeLists.txt updated to remove deleted files
- [ ] No build references to deleted files
- [ ] Clean build succeeds after deletions

**Current Status**: ⚠️ Partial - 36 command files remain (should be ~20-25)
**Gap**: ~10-15 command files not deleted per original plan

**Rationale**: Complete the code simplification to achieve target LOC reduction.

---

### FR-4: Remove Stale References
**Priority**: P1 (High)
**Description**: Remove all references to deleted classes and systems.

**Acceptance Criteria**:
- [ ] No FocusOfThread references in command files
- [ ] cmd_sync.cpp updated (remove FocusOfThread usage)
- [ ] cmd_other_window_class.cpp updated or deleted
- [ ] cmd_keymap_window.cpp updated or deleted
- [ ] No compilation warnings about undefined symbols
- [ ] Code review confirms no dead code paths

**Current Status**: ❌ FocusOfThread still referenced in 4 files
**Files**: cmd_sync.cpp, cmd_other_window_class.cpp, cmd_keymap_window.cpp, engine.h

**Rationale**: Stale references indicate incomplete refactoring and create maintenance burden.

---

## Non-Functional Requirements

### NFR-1: Performance Verification
**Priority**: P0 (Critical)
**Description**: Independently verify all performance claims made in json-refactoring summary.

**Acceptance Criteria**:
- [ ] JSON config load time measured: target <10ms, claimed 2-5ms
- [ ] Event processing latency measured: target no degradation, claimed -4% to -5%
- [ ] Binary size measured: target no increase, claimed -3%
- [ ] Memory usage measured: target <10MB, claimed ~50KB peak
- [ ] Benchmark methodology documented
- [ ] Results reproducible on clean build

**Current Status**: ❌ Claims unverified, no independent measurements
**Metrics to Measure**:
- Config load time (vim-mode.json, emacs-mode.json, config.json)
- Event processing latency (key press → substitution → output)
- Binary size (yamy executable before/after)
- Memory usage (RSS during config load and event processing)

**Rationale**: Performance claims must be independently verified before claiming success.

---

### NFR-2: Documentation Accuracy
**Priority**: P1 (High)
**Description**: All documentation must accurately reflect verified implementation status.

**Acceptance Criteria**:
- [ ] docs/json-refactoring-summary.md updated with accurate file deletion counts
- [ ] Summary includes actual test execution results (not claims)
- [ ] Summary includes actual performance benchmark results (not claims)
- [ ] LOC reduction recalculated with accurate file counts
- [ ] Success criteria table shows verified vs claimed status
- [ ] No overstated accomplishments

**Current Status**: ⚠️ Summary overstates deletions ("45+ files" vs actual ~10-15)
**Discrepancies**:
- File deletion count: claimed 45+, actual ~10-15
- Performance: claimed but not measured
- Tests: claimed passing but not run

**Rationale**: Documentation must be trustworthy for future maintainers.

---

### NFR-3: Code Quality Maintenance
**Priority**: P1 (High)
**Description**: Maintain code quality standards throughout cleanup.

**Acceptance Criteria**:
- [ ] All functions <50 lines (maintained)
- [ ] All files <500 lines (maintained)
- [ ] No compiler warnings
- [ ] No memory leaks (valgrind clean)
- [ ] GSL contracts used (Expects/Ensures)
- [ ] RAII patterns throughout

**Current Status**: ✅ JsonConfigLoader meets metrics (647 lines after refactor)
**Note**: JsonConfigLoader is 647 lines (vs target 400), but meets refactored <650 target

**Rationale**: Code quality must be maintained during cleanup work.

---

### NFR-4: Build System Reliability
**Priority**: P0 (Critical)
**Description**: Build system must work correctly on clean checkout.

**Acceptance Criteria**:
- [ ] CMake configures successfully
- [ ] Ninja/Make builds successfully
- [ ] All targets build (main executable, tests, tools)
- [ ] No missing dependencies
- [ ] Build succeeds on Linux (primary platform)
- [ ] Build time <2 minutes (incremental)

**Current Status**: ⚠️ Unknown - build directory missing, can't verify
**Risk**: Build may be broken after file deletions

**Rationale**: Reliable builds are essential for development and CI.

---

## Out of Scope

### Not Included in This Spec

1. **New Features**: No new functionality beyond json-refactoring spec
2. **Windows Support**: Focus on Linux platform only
3. **Per-Window Keymaps**: Still out of scope (json-refactoring decision)
4. **Additional Example Configs**: 3 examples sufficient (config.json, vim-mode.json, emacs-mode.json)
5. **.mayu to JSON Converter**: Nice to have, but not blocking

---

## Success Criteria

### Definition of Done

**All criteria must be met**:

1. ✅ All tests build and pass (FR-1)
2. ✅ M00-MFF E2E test passes (FR-2)
3. ✅ Phase 4 file deletions complete (FR-3)
4. ✅ No stale references remain (FR-4)
5. ✅ Performance verified independently (NFR-1)
6. ✅ Documentation accurate (NFR-2)
7. ✅ Code quality maintained (NFR-3)
8. ✅ Build system reliable (NFR-4)
9. ✅ Clean build on fresh checkout
10. ✅ All verification gaps from code review addressed

### Verification Checklist

- [ ] `cmake -B build_ninja -G Ninja` succeeds
- [ ] `cmake --build build_ninja` succeeds with no warnings
- [ ] `ctest --test-dir build_ninja --output-on-failure` all tests pass
- [ ] Benchmark results documented in docs/performance.md
- [ ] Updated summary reflects verified reality
- [ ] Git status shows only expected file deletions
- [ ] No compiler warnings or errors
- [ ] Valgrind reports zero leaks

---

## Risks & Mitigations

### Risk 1: Tests May Fail
**Impact**: High
**Probability**: Medium
**Mitigation**:
- Review test failures systematically
- Check if failures are due to API changes or actual bugs
- Fix issues incrementally
- Re-run tests after each fix

### Risk 2: Performance Claims May Not Be Accurate
**Impact**: Medium
**Probability**: Medium
**Mitigation**:
- Run benchmarks independently
- Document methodology clearly
- Update claims to match reality
- If performance is worse than claimed, investigate and optimize

### Risk 3: Breaking Build with File Deletions
**Impact**: High
**Probability**: Low
**Mitigation**:
- Delete files incrementally
- Build after each deletion
- Use git to track changes
- Can revert if needed

### Risk 4: Command Files May Be Still Needed
**Impact**: Medium
**Probability**: Low
**Mitigation**:
- Review each command file before deletion
- Check for references in codebase
- Keep essential commands (keymap, load_setting, etc.)
- Only delete truly unused commands

---

## Dependencies

### Prerequisite Specs
- ✅ json-refactoring (Phases 1-5 code complete)

### External Dependencies
- CMake 3.10+
- Ninja or Make
- nlohmann/json (via Conan)
- GTest (for tests)
- Valgrind (optional, for leak detection)

---

## User Stories

### Story 1: Developer Validates Implementation
**As a** developer taking over this codebase
**I want to** run all tests and see them pass
**So that** I can trust the JSON refactoring implementation works correctly

**Acceptance**:
```bash
cmake -B build_ninja -G Ninja
cmake --build build_ninja
ctest --test-dir build_ninja --output-on-failure
# All tests pass
```

---

### Story 2: Maintainer Trusts Documentation
**As a** future maintainer
**I want to** read docs/json-refactoring-summary.md
**So that** I understand what was actually accomplished (not just claimed)

**Acceptance**:
- Summary contains verified test results
- Summary contains measured performance benchmarks
- File deletion counts are accurate
- No overstated claims

---

### Story 3: User Verifies M00-MFF Works
**As a** vim user
**I want to** load vim-mode.json and use CapsLock as layer key
**So that** I can navigate with HJKL without leaving home row

**Acceptance**:
- Tapping CapsLock outputs Escape
- Holding CapsLock + H outputs Left arrow
- 200ms threshold feels responsive

---

## Timeline

**Estimated Effort**: 1-2 days

- **Task 1-3**: 4-6 hours (build, test, verify)
- **Task 4-6**: 2-3 hours (cleanup, deletions)
- **Task 7-8**: 2-3 hours (benchmarks, documentation)

**Priority Order**:
1. Build and run tests (Task 1) - BLOCKING
2. Verify M00-MFF E2E (Task 2) - BLOCKING
3. Run performance benchmarks (Task 3) - HIGH
4. Complete file deletions (Task 4-5) - HIGH
5. Update documentation (Task 6-8) - MEDIUM

---

**Document Version**: 1.0
**Created**: 2025-12-18
**Status**: Draft → Active (upon approval)
**Parent Spec**: json-refactoring
