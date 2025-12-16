# Parallel Refinement Plan Summary

## Document Overview

**Main Document:** `docs/parallel-refinement-detailed.md` (comprehensive, granular plan)

This summary provides a quick overview of the 9 tracks and how to execute them.

---

## Track Breakdown

### 🔴 Critical Priority - Do First

#### Track 1: Command Template Base (4 subtasks)
**File:** `command_base.h`
**Goal:** Remove tstring/TCHAR from template that generates all 63 commands
**Effort:** 30-45 min per subtask
**Impact:** Fixes template foundation

**Subtasks:**
1. Fix `load()` method (lines 75-76)
2. Fix `output()` method (line 105)
3. Fix `loadArgsRecursive()` (line 145) - **Requires Track 2 first**
4. Fix `outputOneArg()` (line 177)

**Dependencies:** Subtask 1.3 depends on Track 2

---

#### Track 2: SettingLoader API Migration (2 subtasks)
**Files:** `setting_loader.h`, `setting_loader.cpp`
**Goal:** Migrate method signatures from `const _TCHAR*` to `const char*`
**Effort:** 1-2 hours total
**Impact:** Enables Track 1.3 and Track 6

**Subtasks:**
1. Migrate `getOpenParen()`, `getCloseParen()`, `getComma()` signatures
2. Migrate internal string members to `std::string`

**Dependencies:** None

---

### 🟡 Medium Priority - Core Cleanup

#### Track 3: Win32 Type Elimination - Commands (10 subtasks)
**Files:** 11 command files
**Goal:** Remove HWND, DWORD, MSG, WPARAM, LPARAM from commands
**Effort:** 15-30 min per file
**Impact:** Cleans 11 of 21 files with Win32 leakage

**Strategy:** Each file = 1 atomic PR (can parallelize!)

**Subtasks:** (sorted by complexity, easiest first)
1. `cmd_set_foreground_window.cpp` - Remove HWND cast
2. `cmd_window_move_to.cpp` - Remove HWND cast
3. `cmd_window_move.cpp` - Remove HWND cast
4. `cmd_window_resize_to.cpp` - Remove HWND cast
5. `cmd_window_move_visibly.cpp` - Remove HWND cast
6. `cmd_window_hv_maximize.cpp` - Remove HWND cast
7. `cmd_window_identify.cpp` - Replace "HWND" string + remove _T()
8. `cmd_mouse_hook.cpp` - Remove HWND cast
9. `cmd_sync.cpp` - Replace DWORD with uint32_t
10. `cmd_post_message.{h,cpp}` - Create platform message types

**Dependencies:** None (fully parallel)

---

#### Track 4: Win32 Type Elimination - Window Subsystem (5 subtasks)
**Files:** `layoutmanager.{h,cpp}`, `focus.cpp`, `target.cpp`
**Goal:** Clean src/core/window/ of Win32 types
**Effort:** 2-3 hours total
**Impact:** Cleans 4 more files

**Subtasks:**
1. `layoutmanager.h` - Replace HWND members
2. `layoutmanager.h` - Replace DWORD, WPARAM, LPARAM
3. `layoutmanager.cpp` - Update implementations
4. `focus.cpp` - Replace Win32 message types
5. `target.cpp` - Remove Win32 types

**Dependencies:** None

---

#### Track 5: Win32 Type Elimination - Engine Subsystem (5 subtasks)
**Files:** `engine.{h,cpp}`, `engine_focus.cpp`, `engine_lifecycle.cpp`, `engine_window.cpp`
**Goal:** Clean src/core/engine/ of Win32 types
**Effort:** 2-3 hours total
**Impact:** Cleans 5 more files

**Subtasks:**
1. `engine.h` - Replace MSG
2. `engine.h` - Rename WPARAM_T/LPARAM_T template params (low priority)
3. `engine_focus.cpp` - Replace HWND in log output
4. `engine_lifecycle.cpp` - Move ChangeWindowMessageFilter to PAL
5. `engine_window.cpp` - Remove HWND cast

**Dependencies:** None

---

#### Track 6: Settings & Parser String Migration (5 subtasks)
**Files:** 6 files in `src/core/settings/`
**Goal:** Replace tstring/tregex/tstringi with std::string/std::regex
**Effort:** 3-4 hours total
**Impact:** ~150 legacy string reduction

**Subtasks:**
1. `parser.h` - Replace tregex with std::regex
2. `parser.cpp` - Update regex usage
3. `setting.h` - Replace tstringi members
4. `setting.cpp` - Update string operations
5. `setting_loader.cpp` - Complete migration

**Dependencies:** Track 2 (SettingLoader API)

---

#### Track 7: Input & Keymap String Migration (6 subtasks)
**Files:** 6 files in `src/core/input/`
**Goal:** Replace tstring/tstringi/_TCHAR with std::string
**Effort:** 2-3 hours total
**Impact:** ~100 legacy string reduction

**Subtasks:**
1. `keymap.h` - Replace tstringi members
2. `keymap.cpp` - Update string operations
3. `keyboard.h` - Replace _TCHAR members
4. `keyboard.cpp` - Update string operations
5. `vkeytable.h` - Replace _TCHAR
6. `vkeytable.cpp` - Update table initialization

**Dependencies:** None

---

### 🟢 Low Priority - Cleanup & Enforcement

#### Track 8: Function System Cleanup (2 subtasks)
**Files:** `function.cpp`, `function.h`
**Goal:** Remove remaining _T() from type tables
**Effort:** 1 hour total
**Impact:** ~50 legacy string reduction

**Subtasks:**
1. `function.cpp` - Remove _T() from type tables
2. `function.h` - Header cleanup

**Dependencies:** None

---

#### Track 9: CI Enforcement (2 subtasks)
**File:** `.github/workflows/ci.yml`
**Goal:** Prevent regressions
**Effort:** 1 hour total
**Impact:** Automated enforcement

**Subtasks:**
1. Add Win32 type leakage check
2. Add legacy string regression check

**Dependencies:** Tracks 3-5 should be mostly complete

---

## Execution Strategy

### Recommended Order

**Week 1:**
1. **Track 2** (SettingLoader API) - 2 PRs
2. **Track 1** (Command Template) - 4 PRs
3. **Track 3** (Start) - 3-5 command file PRs

**Week 2:**
4. **Track 3** (Finish) - Remaining 5-7 command file PRs
5. **Track 4** (Window subsystem) - 5 PRs

**Week 3:**
6. **Track 5** (Engine subsystem) - 5 PRs
7. **Track 6** (Settings/Parser) - 5 PRs

**Week 4:**
8. **Track 7** (Input/Keymap) - 6 PRs
9. **Track 8** (Function cleanup) - 2 PRs
10. **Track 9** (CI enforcement) - 2 PRs

### Parallelization Opportunities

**Can Work in Parallel:**
- Track 3: All 10 command files (completely independent)
- Track 4 + Track 5 (different subsystems)
- Track 6 + Track 7 (different subsystems)

**Must Be Sequential:**
- Track 2 → Track 1.3
- Track 2 → Track 6

---

## Quick Start Guide

### For Each PR:

1. **Setup:**
   ```bash
   git checkout master
   git pull origin master
   git checkout -b refactor/track-{N}-{subtask}-{name}
   bash scripts/track_legacy_strings.sh > before.txt
   ```

2. **Implement:**
   - Open `docs/parallel-refinement-detailed.md`
   - Find your subtask
   - Make ONLY the changes specified
   - Build after each change

3. **Test:**
   ```bash
   cd build
   rm -rf *
   cmake .. -G "MinGW Makefiles"
   cmake --build . --target yamy_core
   bash ../scripts/track_legacy_strings.sh > after.txt
   ```

4. **Submit:**
   ```bash
   git add .
   git commit -m "refactor(component): description"
   git push origin HEAD
   # Create PR with metrics from before.txt vs after.txt
   ```

---

## Metrics Tracking

### Baseline (2025-12-10)
```
Legacy Strings: 971 (tstring: 214, _T(): 596, _TCHAR: 80, tstringi: 81)
Win32 Leakage: 57 usages in 21 files
windows.h: 0 ✅
```

### Track-by-Track Expected Reduction

| Track | Legacy Strings | Win32 Files |
|-------|---------------|-------------|
| Baseline | 971 | 21 |
| After Track 1 | ~965 | 21 |
| After Track 2 | ~955 | 21 |
| After Track 3 | ~955 | 10 ✅ |
| After Track 4 | ~955 | 6 ✅ |
| After Track 5 | ~955 | 1 ✅ |
| After Track 6 | ~800 | 1 |
| After Track 7 | ~700 | 1 |
| After Track 8 | ~650 | 1 |
| **Target** | **< 200** | **0** |

---

## Common Patterns

### Replacing tstring
```cpp
// Before
tstring name = to_tstring(getName());
const _TCHAR* cname = name.c_str();

// After
std::string name = getName();
const char* cname = name.c_str();
```

### Replacing Win32 Types
```cpp
// Before
HWND hwnd = static_cast<HWND>(windowHandle);
DWORD value = 42;
WPARAM wParam = 0;
LPARAM lParam = 0;

// After
yamy::platform::WindowHandle hwnd = windowHandle;
uint32_t value = 42;
uintptr_t wParam = 0;
intptr_t lParam = 0;
```

### Replacing _T() Macro
```cpp
// Before
m_log << _T("Message");

// After
m_log << "Message";
```

### Replacing tregex
```cpp
// Before
tregex pattern(_T("\\d+"));

// After
std::regex pattern("\\d+");
```

---

## Getting Help

**Documentation:**
- Main plan: `docs/parallel-refinement-detailed.md`
- Verification: `docs/verification-report.md`
- Original plan: `docs/parallel-refinement.md`

**Code Patterns:**
- Platform types: `src/core/platform/types.h`
- PAL interfaces: `src/core/platform/*_interface.h`
- PAL implementations: `src/platform/windows/*.cpp`

**Tools:**
```bash
# Find examples of a pattern
grep -r "WindowHandle" src/core --include="*.cpp" | head -5

# Check function signature
grep "functionName" src/core -r -B2 -A2

# Track progress
bash scripts/track_legacy_strings.sh
```

---

## Success Criteria

### Track 1 Complete When:
- [ ] Zero tstring in command_base.h
- [ ] Zero _TCHAR in command_base.h
- [ ] All 63 commands build successfully

### Track 3 Complete When:
- [ ] Zero Win32 types in all 11 command files
- [ ] Tracking script shows 10 fewer files with leakage

### All Tracks Complete When:
- [ ] Legacy string count < 200 (80% reduction)
- [ ] Win32 file count = 0 in src/core
- [ ] CI enforces no regressions
- [ ] Full build succeeds (32-bit + 64-bit)
- [ ] Manual tests pass

---

**Start with Track 2, then Track 1, then parallelize everything else!**
