# Refactor Remaining Work - Design

## Overview

This document outlines the design approach for completing the JSON refactoring verification and cleanup work. The implementation is largely complete, but verification gaps and incomplete cleanup prevent full validation.

**Parent Spec**: json-refactoring
**Status**: Active

---

## Architecture Context

### Current State

```
yamy/
├── src/core/
│   ├── settings/
│   │   ├── json_config_loader.h      ✅ Implemented (200 LOC)
│   │   ├── json_config_loader.cpp    ✅ Implemented (647 LOC)
│   │   └── setting_loader.h          ⚠️ Stub (should be deleted)
│   ├── engine/
│   │   ├── engine.h                  ✅ FocusOfThread removed
│   │   └── engine_keyboard_handler.cpp ✅ Simplified
│   └── commands/
│       ├── cmd_*.cpp                 ⚠️ 36 files (should be ~25)
├── tests/
│   ├── test_json_loader.cpp          ✅ Implemented (320 LOC, 15+ tests)
│   └── test_parser.cpp               ❌ Should be deleted
├── keymaps/
│   ├── config.json                   ✅ Basic example
│   ├── vim-mode.json                 ✅ 28 bindings
│   └── emacs-mode.json               ✅ 18 bindings
└── docs/
    ├── json-schema.md                ✅ Comprehensive
    ├── migration-guide.md            ✅ Complete
    └── json-refactoring-summary.md   ⚠️ Overstated claims
```

### What's Missing

1. **Verification**: Tests not built/run, performance not measured
2. **Cleanup**: ~10-15 command files not deleted, stale references remain
3. **Documentation**: Summary contains unverified claims

---

## Design Approach

### 1. Build System & Test Verification

#### 1.1 Build Configuration

**Goal**: Ensure clean build on fresh checkout

**Approach**:
```bash
# Configure with Ninja (faster builds)
cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build all targets (main executable + tests)
cmake --build build_ninja

# Run tests
ctest --test-dir build_ninja --output-on-failure
```

**Expected Targets**:
- `yamy` (main executable)
- `test_json_loader` (JSON loader unit tests)
- `yamy_property_keymap_test` (keymap tests)
- Additional test executables

**Verification**:
- No compilation errors
- No linker errors
- All test executables generated

#### 1.2 Test Execution Strategy

**Unit Tests (test_json_loader)**:
```cpp
// Existing tests in tests/test_json_loader.cpp:
TEST_F(JsonConfigLoaderTest, LoadValidBasicConfig)
TEST_F(JsonConfigLoaderTest, LoadVirtualModifiers)
TEST_F(JsonConfigLoaderTest, LoadKeySequences)
TEST_F(JsonConfigLoaderTest, ErrorInvalidJsonSyntax)
TEST_F(JsonConfigLoaderTest, ErrorMissingVersion)
// ... 10+ more tests
```

**Expected Results**:
- All 15+ tests pass
- No crashes or segfaults
- Memory leaks: 0 (verified with valgrind)

**Failure Handling**:
- Document each failure with root cause
- Fix incrementally
- Re-run tests after each fix
- Update summary with actual results

---

### 2. M00-MFF End-to-End Test

#### 2.1 E2E Test Design

**Test File**: `tests/test_e2e_vim_mode.cpp`

**Test Scenarios**:

```cpp
// Test 1: Load vim-mode.json config
TEST(E2E_VimMode, LoadConfig) {
    Setting setting;
    JsonConfigLoader loader(&std::cerr);
    ASSERT_TRUE(loader.load(&setting, "keymaps/vim-mode.json"));

    // Verify M00 virtual modifier registered
    EXPECT_TRUE(setting.m_virtualModTriggers.size() > 0);
}

// Test 2: Tap behavior (CapsLock quick press → Escape)
TEST(E2E_VimMode, TapBehavior) {
    // Simulate CapsLock press
    // Wait < 200ms
    // Simulate CapsLock release
    // Verify Escape output
}

// Test 3: Hold behavior (CapsLock hold + H → Left)
TEST(E2E_VimMode, HoldBehavior) {
    // Simulate CapsLock press
    // Wait > 200ms (trigger hold threshold)
    // Simulate H press
    // Verify Left arrow output
}

// Test 4: Multiple modifiers (M00-Shift-G → Ctrl-End)
TEST(E2E_VimMode, CombinedModifiers) {
    // Simulate CapsLock hold + Shift + G
    // Verify Ctrl-End output
}
```

**Implementation Strategy**:
- Use existing EventProcessor for event simulation
- Mock input injector to capture output
- Verify output scan codes match expected keys

**Alternative**: Manual E2E Test
If automated E2E is complex, document manual test procedure:
```markdown
1. Build yamy: `cmake --build build_ninja`
2. Run yamy: `./build_ninja/yamy --config keymaps/vim-mode.json`
3. Test tap: Press and release CapsLock quickly → Should output Escape
4. Test hold: Hold CapsLock, press H → Should output Left arrow
5. Test combo: Hold CapsLock + Shift + G → Should output Ctrl-End
```

---

### 3. Performance Benchmarking

#### 3.1 Benchmark Design

**Benchmark File**: `tests/benchmark_config_load.cpp`

**Metrics to Measure**:

1. **Config Load Time**
```cpp
#include <chrono>

void benchmark_config_load(const std::string& config_path) {
    Setting setting;
    JsonConfigLoader loader(nullptr); // No logging for clean timing

    auto start = std::chrono::high_resolution_clock::now();
    loader.load(&setting, config_path);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << config_path << ": " << duration.count() << " μs" << std::endl;
}

int main() {
    // Run each benchmark 100 times, report median
    benchmark_config_load("keymaps/config.json");      // Basic config
    benchmark_config_load("keymaps/vim-mode.json");    // Complex config
    benchmark_config_load("keymaps/emacs-mode.json");  // Medium config
}
```

**Target**: <10ms (10,000 μs)
**Claimed**: 2-5ms (2,000-5,000 μs)

2. **Event Processing Latency**
```cpp
void benchmark_event_processing() {
    // Measure time from key press → substitution → output
    // Use EventProcessor with mock input/output

    auto start = std::chrono::high_resolution_clock::now();
    // Simulate key press event
    eventProcessor.processEvent(event);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Event latency: " << duration.count() << " ns" << std::endl;
}
```

**Target**: No degradation from .mayu version
**Claimed**: -4% to -5% improvement

3. **Binary Size**
```bash
# Before JSON refactoring (git checkout before phase 1)
git checkout <commit-before-json-refactoring>
cmake --build build_ninja
ls -lh build_ninja/yamy | awk '{print $5}'  # e.g., 2.0M

# After JSON refactoring (current)
git checkout master
cmake --build build_ninja
ls -lh build_ninja/yamy | awk '{print $5}'  # e.g., 1.94M

# Calculate reduction: (2.0 - 1.94) / 2.0 = 3%
```

4. **Memory Usage**
```bash
# Run yamy with valgrind massif (heap profiler)
valgrind --tool=massif --massif-out-file=massif.out ./build_ninja/yamy --config keymaps/vim-mode.json

# Analyze peak memory
ms_print massif.out | grep "peak"
```

**Target**: <10MB resident set
**Claimed**: ~50KB peak

#### 3.2 Benchmark Execution

**Steps**:
1. Create benchmark_config_load.cpp
2. Add to CMakeLists.txt as executable
3. Build: `cmake --build build_ninja --target benchmark_config_load`
4. Run: `./build_ninja/benchmark_config_load > benchmark_results.txt`
5. Document results in docs/performance.md

**Results Format**:
```markdown
# Performance Benchmark Results

**Date**: 2025-12-18
**Platform**: Linux x86_64
**Compiler**: GCC 11.4.0
**Build**: Debug

## Config Load Time (median of 100 runs)

| Config File      | Size  | Keys | Mappings | Load Time | Target  | Status |
|------------------|-------|------|----------|-----------|---------|--------|
| config.json      | 1.2KB | 15   | 8        | 2.3ms     | <10ms   | ✅ PASS |
| vim-mode.json    | 3.5KB | 89   | 28       | 4.8ms     | <10ms   | ✅ PASS |
| emacs-mode.json  | 2.8KB | 89   | 18       | 3.1ms     | <10ms   | ✅ PASS |

## Event Processing Latency

| Event Type               | Before  | After   | Change | Status |
|--------------------------|---------|---------|--------|--------|
| Key press (no subst)     | 180ns   | 180ns   | ±0%    | ✅ PASS |
| Key press (with subst)   | 250ns   | 240ns   | -4%    | ✅ PASS |
| Modifier key handling    | 200ns   | 190ns   | -5%    | ✅ PASS |

## Binary Size

| Component    | Before  | After   | Change | Status |
|--------------|---------|---------|--------|--------|
| yamy binary  | 2.00MB  | 1.94MB  | -3%    | ✅ PASS |

## Memory Usage

| Metric              | Value  | Target | Status |
|---------------------|--------|--------|--------|
| Peak heap usage     | 48KB   | <10MB  | ✅ PASS |
| Resident set size   | 8.2MB  | <10MB  | ✅ PASS |
```

---

### 4. File Deletion Strategy

#### 4.1 Command Files to Delete

**Review Criteria**:
- Is command used by JSON configs? (No - JSON doesn't use commands)
- Is command referenced in engine core? (Check with grep)
- Is command essential for keymap system? (Keep: keymap, load_setting, etc.)

**Files to DELETE** (verify each):
```bash
src/core/commands/cmd_window_*.cpp         # Window manipulation (not needed)
src/core/commands/cmd_clipboard_*.cpp      # Clipboard commands (not needed)
src/core/commands/cmd_emacs_*.cpp          # Emacs commands (not needed)
src/core/commands/cmd_other_window_class.cpp  # Window focus (removed)
tests/test_parser.cpp                      # Legacy parser tests
```

**Files to KEEP** (essential):
```bash
src/core/commands/cmd_keymap.cpp           # Keymap commands (used by engine)
src/core/commands/cmd_load_setting.cpp     # Config loading
src/core/commands/cmd_default.cpp          # Default action
src/core/commands/cmd_ignore.cpp           # Ignore key
src/core/commands/cmd_help_*.cpp           # Help system
src/core/commands/cmd_log_*.cpp            # Logging
src/core/commands/cmd_mouse_*.cpp          # Mouse support (may be needed)
```

**Deletion Process**:
```bash
# 1. List all command files
find src/core/commands -name "cmd_*.cpp" | sort > /tmp/all_commands.txt

# 2. Review each file
for file in src/core/commands/cmd_window_*.cpp; do
    echo "Reviewing: $file"
    grep -r $(basename $file .cpp) src/core --exclude-dir=commands
    # If no references, mark for deletion
done

# 3. Delete files incrementally
git rm src/core/commands/cmd_window_close.cpp
cmake --build build_ninja  # Verify build still works
git rm src/core/commands/cmd_window_maximize.cpp
cmake --build build_ninja  # Verify again
# ... repeat for each file

# 4. Update CMakeLists.txt
# Remove deleted files from add_executable() or add_library()

# 5. Final build
cmake --build build_ninja
```

#### 4.2 Stale Reference Cleanup

**Files with FocusOfThread References**:

1. **src/core/commands/cmd_sync.cpp**
   - Review: Does it need FocusOfThread?
   - Option A: Update to not use FocusOfThread
   - Option B: Delete if command not needed

2. **src/core/commands/cmd_other_window_class.cpp**
   - Review: Window focus command (should be deleted)
   - Action: Delete file

3. **src/core/commands/cmd_keymap_window.cpp**
   - Review: Per-window keymap (removed feature)
   - Action: Delete file

**Cleanup Process**:
```bash
# 1. Find all FocusOfThread references
grep -rn "FocusOfThread" src/core/

# 2. For each file, decide:
#    - Delete file (if command not needed)
#    - Update code (if command needed but can work without FocusOfThread)
#    - Keep as-is (if actually needed - unlikely)

# 3. After cleanup, verify no references remain
grep -rn "FocusOfThread" src/core/ --exclude-dir=.git
# Should return no results
```

---

### 5. Documentation Updates

#### 5.1 Summary Document Corrections

**File**: `docs/json-refactoring-summary.md`

**Sections to Update**:

1. **Files Deleted**
   - Current: "45+ command files deleted"
   - Update to: "14 command files deleted (list specific files)"
   - Provide accurate count after Task 4 completion

2. **Test Results**
   - Current: "Claims >90% coverage passes"
   - Update to: "Test execution results: [actual results from Task 1]"
   - Include: pass/fail count, coverage percentage, any failures

3. **Performance Metrics**
   - Current: "Claims 2-5ms load time"
   - Update to: "Measured performance: [actual results from Task 3]"
   - Include: methodology, platform details, reproducibility notes

4. **LOC Reduction**
   - Current: "-1,675 LOC (62.8%)"
   - Recalculate: Based on actual files deleted
   - Show calculation: `git diff --shortstat <before-commit> <after-commit>`

**Template for Updates**:
```markdown
## Verified Implementation Results

**Test Execution** (verified 2025-12-18):
- test_json_loader: 15 tests, 15 passed, 0 failed
- yamy_property_keymap_test: 11 tests, 11 passed, 0 failed
- Coverage: 92% on JsonConfigLoader

**Performance Benchmarks** (measured 2025-12-18, Linux x86_64, GCC 11.4):
- Config load time: 2.3-4.8ms (target <10ms) ✅
- Event latency: -4% to -5% improvement ✅
- Binary size: -3% reduction ✅
- Memory usage: 48KB peak (target <10MB) ✅

**Code Reduction** (verified):
- Files deleted: 14 command files (cmd_window_*, cmd_clipboard_*, cmd_emacs_*)
- LOC removed: 1,450 lines
- LOC added: 992 lines
- Net reduction: -458 LOC
```

#### 5.2 Performance Documentation

**File**: `docs/performance.md`

**Contents**:
- Benchmark methodology
- Platform details (OS, compiler, CPU)
- Raw benchmark results
- Comparison to targets
- Reproducibility instructions

---

## Data Flow

### Test Execution Flow

```
Developer
    ↓
cmake -B build_ninja -G Ninja
    ↓
CMakeLists.txt (configure)
    ↓
Ninja/Make (compile)
    ↓
test_json_loader (executable)
    ↓
GTest runner
    ↓
JsonConfigLoader tests
    ↓
Results (pass/fail)
    ↓
Update docs/json-refactoring-summary.md
```

### Benchmark Flow

```
benchmark_config_load (executable)
    ↓
Load config.json, vim-mode.json, emacs-mode.json
    ↓
Measure time with std::chrono
    ↓
Repeat 100 times, calculate median
    ↓
Output results to benchmark_results.txt
    ↓
Document in docs/performance.md
```

---

## Error Handling

### Build Failures

**Scenario**: Build fails after file deletion
**Recovery**:
1. Review error message
2. Check CMakeLists.txt for references to deleted files
3. Remove references
4. Re-run build

### Test Failures

**Scenario**: Tests fail unexpectedly
**Recovery**:
1. Run tests individually: `./build_ninja/test_json_loader --gtest_filter=TestName`
2. Analyze failure output
3. Fix code or test
4. Re-run tests

### Performance Below Target

**Scenario**: Benchmark shows load time >10ms
**Recovery**:
1. Profile with perf or gprof
2. Identify bottlenecks
3. Optimize critical paths
4. Re-run benchmarks

---

## Testing Strategy

### Unit Tests
- **Existing**: test_json_loader.cpp (15+ tests)
- **Coverage**: >90% on JsonConfigLoader
- **Execution**: `ctest --test-dir build_ninja`

### Integration Tests
- **Existing**: yamy_property_keymap_test
- **Coverage**: Keymap system integration
- **Execution**: `ctest --test-dir build_ninja`

### E2E Tests
- **New**: test_e2e_vim_mode.cpp (manual or automated)
- **Coverage**: M00-MFF tap/hold behavior
- **Execution**: Manual testing or automated if implemented

### Performance Tests
- **New**: benchmark_config_load.cpp
- **Coverage**: Load time, event latency, memory usage
- **Execution**: `./build_ninja/benchmark_config_load`

---

## Deployment

### Verification Checklist

Before marking refactor-remaining as complete:

- [ ] Clean checkout builds successfully
- [ ] All tests pass
- [ ] Benchmarks documented
- [ ] Documentation updated
- [ ] No stale references
- [ ] Code review complete
- [ ] Git history clean

### Sign-off Criteria

**Technical Lead**:
- [ ] Reviews test results
- [ ] Reviews performance benchmarks
- [ ] Approves file deletions
- [ ] Approves documentation updates

**QA**:
- [ ] Verifies all tests pass
- [ ] Verifies benchmarks reproducible
- [ ] Checks no regressions

---

## Maintenance

### Future Work

After refactor-remaining completion:

1. **CI/CD Integration**: Add tests to CI pipeline
2. **Performance Monitoring**: Track performance over time
3. **Additional Configs**: Create more example configs if needed
4. **Tool Development**: Build .mayu to JSON converter (optional)

---

**Document Version**: 1.0
**Created**: 2025-12-18
**Status**: Design Complete → Ready for Implementation
