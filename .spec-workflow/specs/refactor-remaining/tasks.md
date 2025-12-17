# Refactor Remaining Work - Tasks

## Critical Path (Blocking Production)

- [x] 1. Build and verify test suite
  - File: CMakeLists.txt, build_ninja/
  - Configure build system with Ninja generator
  - Build all targets including tests
  - Run test suite with ctest
  - Document test results (pass/fail counts, coverage)
  - Purpose: Validate JSON refactoring implementation works correctly
  - _Leverage: existing CMakeLists.txt, test_json_loader.cpp, GTest framework_
  - _Requirements: FR-1, NFR-4_
  - _Prompt: Role: DevOps Engineer with expertise in CMake build systems and test automation | Task: Configure and build the project with Ninja, run all tests, and document results following requirements FR-1 and NFR-4 | Restrictions: Must use Ninja generator for fast builds, must run tests with --output-on-failure for debugging, do not skip any tests, ensure clean build from scratch | Success: Build completes with no errors, all tests pass (test_json_loader 15+ tests, yamy_property_keymap_test all tests), test results documented with pass/fail counts and any issues noted_

- [x] 2. Verify M00-MFF end-to-end functionality
  - File: tests/test_e2e_vim_mode.cpp (create) or manual test procedure
  - Load vim-mode.json config
  - Test tap behavior: CapsLock quick press → Escape
  - Test hold behavior: CapsLock hold + H → Left arrow
  - Test combined modifiers: CapsLock hold + Shift + G → Ctrl-End
  - Test 200ms hold threshold timing
  - Document results in implementation log
  - Purpose: Validate virtual modifier system works end-to-end
  - _Leverage: EventProcessor, ModifierKeyHandler, vim-mode.json config_
  - _Requirements: FR-2_
  - _Prompt: Role: QA Engineer with expertise in end-to-end testing and keyboard input simulation | Task: Create and execute E2E tests for M00-MFF virtual modifiers following requirement FR-2, testing tap/hold detection and modifier combinations using vim-mode.json config | Restrictions: Must test all three scenarios (tap, hold, combined), verify timing threshold (200ms), can use manual testing if automated E2E is too complex, document exact test procedure for reproducibility | Success: All E2E scenarios pass (tap outputs Escape, hold+H outputs Left, combined modifiers work), timing threshold verified, test procedure documented for future verification_

- [x] 3. Run performance benchmarks and document results
  - File: tests/benchmark_config_load.cpp (create), docs/performance.md
  - Implement config load time benchmark (median of 100 runs)
  - Measure: config.json, vim-mode.json, emacs-mode.json load times
  - Implement event processing latency benchmark
  - Measure binary size before/after refactoring
  - Measure memory usage with valgrind massif
  - Document all results in docs/performance.md with methodology
  - Purpose: Independently verify performance claims from json-refactoring summary
  - _Leverage: std::chrono for timing, valgrind for memory, existing configs_
  - _Requirements: NFR-1_
  - _Prompt: Role: Performance Engineer with expertise in benchmarking and profiling C++ applications | Task: Create comprehensive performance benchmarks for config loading and event processing following requirement NFR-1, measuring load time, latency, binary size, and memory usage with proper methodology | Restrictions: Must run each benchmark 100+ times and report median, must document platform details (OS, compiler, CPU), must use valgrind for memory measurement, do not use micro-benchmarks that can be optimized away by compiler | Success: All benchmarks complete successfully, config load time <10ms verified, event latency improvement measured, binary size reduction measured, memory usage <10MB verified, results documented in docs/performance.md with reproducible methodology_

## Important (Quality & Cleanup)

- [ ] 4. Complete Phase 4 command file deletions
  - File: src/core/commands/cmd_window_*.cpp, cmd_clipboard_*.cpp, cmd_emacs_*.cpp
  - Review all 36 remaining command files
  - Delete window manipulation commands (cmd_window_*.cpp)
  - Delete clipboard commands (cmd_clipboard_*.cpp)
  - Delete emacs-specific commands (cmd_emacs_*.cpp)
  - Update CMakeLists.txt to remove deleted files
  - Build after each deletion to verify no breakage
  - Purpose: Complete code simplification per json-refactoring Phase 4 plan
  - _Leverage: git rm for tracked deletion, CMakeLists.txt for build config_
  - _Requirements: FR-3, NFR-3_
  - _Prompt: Role: Software Engineer with expertise in code cleanup and build system maintenance | Task: Complete Phase 4 file deletions by removing unused command files following requirements FR-3 and NFR-3, updating CMakeLists.txt after each deletion and verifying build | Restrictions: Must review each file before deletion to ensure it's not needed, must delete incrementally (not all at once), must rebuild after each deletion, do not delete essential commands (keymap, load_setting, default, ignore), keep mouse commands if potentially needed | Success: All window/clipboard/emacs command files deleted (~10-15 files), CMakeLists.txt updated correctly, clean build succeeds after all deletions, only essential command files remain (~20-25 files)_

- [ ] 5. Remove stale FocusOfThread references
  - File: src/core/commands/cmd_sync.cpp, cmd_other_window_class.cpp, cmd_keymap_window.cpp
  - Grep for all FocusOfThread references in src/core/
  - Review cmd_sync.cpp: update or delete
  - Delete cmd_other_window_class.cpp (window focus not needed)
  - Delete cmd_keymap_window.cpp (per-window keymaps removed)
  - Update CMakeLists.txt for deleted files
  - Verify no FocusOfThread references remain
  - Purpose: Remove all references to deleted FocusOfThread class
  - _Leverage: grep for finding references, git rm for deletion_
  - _Requirements: FR-4_
  - _Prompt: Role: Software Engineer with expertise in code refactoring and dependency cleanup | Task: Remove all stale FocusOfThread references from command files following requirement FR-4, deleting files that depend on removed window focus system | Restrictions: Must grep entire src/core/ to find all references, must review each reference to determine if file can be updated or should be deleted, do not leave any dangling references that cause compilation errors | Success: Zero FocusOfThread references remain in src/core/ (verified by grep), cmd_other_window_class.cpp and cmd_keymap_window.cpp deleted, cmd_sync.cpp either updated or deleted, clean build succeeds_

- [ ] 6. Delete legacy parser test code
  - File: src/tests/test_parser.cpp
  - Verify parser.cpp and parser.h are fully deleted (not just stubbed)
  - Delete test_parser.cpp (tests for deleted parser)
  - Remove from CMakeLists.txt if present
  - Build to verify no breakage
  - Purpose: Remove tests for deleted parser implementation
  - _Leverage: git rm, CMakeLists.txt_
  - _Requirements: FR-3_
  - _Prompt: Role: QA Engineer with expertise in test maintenance and cleanup | Task: Delete legacy parser test code following requirement FR-3, removing tests for the deleted .mayu parser | Restrictions: Must verify parser.cpp and parser.h are actually deleted (not stubs), must remove test_parser.cpp and any references in build system, must verify no other tests depend on deleted parser code | Success: test_parser.cpp deleted, no parser references in build system, clean build succeeds, no test failures due to missing parser_

## Documentation & Polish

- [ ] 7. Update json-refactoring-summary.md with verified results
  - File: docs/json-refactoring-summary.md
  - Update file deletion counts with accurate numbers from Task 4
  - Add actual test execution results from Task 1
  - Add actual performance benchmark results from Task 3
  - Recalculate LOC reduction based on actual deletions
  - Update success criteria table with verified vs claimed status
  - Remove overstated claims
  - Purpose: Ensure implementation summary accurately reflects verified reality
  - _Leverage: test results from Task 1, benchmarks from Task 3, git diff for LOC_
  - _Requirements: NFR-2_
  - _Prompt: Role: Technical Writer with expertise in documentation accuracy and software metrics | Task: Update json-refactoring-summary.md with verified results following requirement NFR-2, replacing claims with measured data from test execution and benchmarks | Restrictions: Must use actual test results (not claims), must use actual benchmark measurements (not estimates), must recalculate LOC reduction with git diff, do not overstate accomplishments, be honest about what was verified vs claimed | Success: Summary updated with verified test results, verified performance benchmarks, accurate file deletion counts, recalculated LOC reduction, clear distinction between verified and unverified claims_

- [ ] 8. Create comprehensive performance documentation
  - File: docs/performance.md
  - Document benchmark methodology (platform, compiler, iterations)
  - Include raw benchmark results from Task 3
  - Create comparison table (target vs achieved)
  - Document reproducibility instructions
  - Add performance analysis notes
  - Purpose: Provide detailed performance documentation for future reference
  - _Leverage: benchmark results from Task 3, existing docs/json-schema.md as template_
  - _Requirements: NFR-1, NFR-2_
  - _Prompt: Role: Technical Writer with expertise in performance documentation and benchmarking | Task: Create comprehensive performance documentation following requirements NFR-1 and NFR-2, documenting methodology, results, and reproducibility for all benchmarks | Restrictions: Must include platform details (OS, compiler version, CPU model), must document exact commands for reproducing benchmarks, must show raw results (not just summaries), include statistical measures (median, variance if applicable) | Success: docs/performance.md created with complete methodology, raw results tables, reproducibility instructions, comparison to targets, and analysis notes for any unexpected results_

## Verification & Sign-off

- [ ] 9. Final verification and code review
  - File: All modified files
  - Run clean checkout build: `git clone → cmake → build → test`
  - Verify all tests pass
  - Verify no compiler warnings
  - Run valgrind for memory leak check
  - Review all code changes since json-refactoring start
  - Verify code quality standards maintained (functions <50 lines, files <500 lines)
  - Create final implementation log entry
  - Purpose: Final validation before marking spec complete
  - _Leverage: git clone for clean checkout, valgrind for leak detection, code review checklist_
  - _Requirements: All (FR-1 through FR-4, NFR-1 through NFR-4)_
  - _Prompt: Role: Senior Software Engineer with expertise in code quality and final verification | Task: Perform comprehensive final verification following all requirements (FR-1 through FR-4, NFR-1 through NFR-4), validating clean build, test pass, no leaks, and code quality standards | Restrictions: Must perform clean checkout (not incremental build), must run all tests, must check for warnings and errors, must run valgrind to verify zero leaks, must verify code metrics (functions <50 lines, files <500 lines maintained), do not skip any verification steps | Success: Clean checkout builds successfully, all tests pass, zero compiler warnings, zero memory leaks (valgrind), code quality standards maintained, all requirements verified, final implementation log entry created documenting completion_

---

## Task Dependencies

```
Task 1 (Build & Test)
  ↓
Task 2 (E2E Verification) ← depends on build
  ↓
Task 3 (Benchmarks) ← depends on build
  ↓
Task 4, 5, 6 (Cleanup) ← can run in parallel after Task 1
  ↓
Task 7 (Update Summary) ← depends on Tasks 1, 3, 4
  ↓
Task 8 (Performance Docs) ← depends on Task 3
  ↓
Task 9 (Final Verification) ← depends on ALL tasks
```

## Execution Order

**Phase 1: Verification (Tasks 1-3)** - BLOCKING
- Must complete first to understand current state
- Estimated: 4-6 hours

**Phase 2: Cleanup (Tasks 4-6)** - HIGH PRIORITY
- Can start after Phase 1 complete
- Estimated: 2-3 hours

**Phase 3: Documentation (Tasks 7-8)** - MEDIUM PRIORITY
- Requires results from Phase 1 and Phase 2
- Estimated: 2-3 hours

**Phase 4: Final Sign-off (Task 9)** - REQUIRED
- Final validation before completion
- Estimated: 1-2 hours

**Total Estimated Effort**: 9-14 hours (1-2 days)

---

**Document Version**: 1.0
**Created**: 2025-12-18
**Parent Spec**: json-refactoring
**Status**: Ready for Implementation
