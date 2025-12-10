# Detailed Parallel Refinement Plan - Yamy Modernization (v2)

**Generated:** 2025-12-10
**Status:** Ready for precise, granular PR execution
**Baseline Metrics:**
- Legacy String Usages: 971 (tstring: 214, _T(): 596, _TCHAR: 80, tstringi: 81)
- Win32 Type Leakage: 57 usages across 21 files
- windows.h Includes: 0 ✅

---

## Track 1: Command Template Base (command_base.h)

**Priority:** 🔴 CRITICAL (Affects all 63 commands)
**Files:** 1 file
**Estimated Effort:** 30-45 minutes per subtask
**Dependencies:** None

### Subtask 1.1: Eliminate tstring from load() method
**File:** `src/core/commands/command_base.h`
**Lines:** 75-76

**Current Code:**
```cpp
tstring tsName = to_tstring(getName());
const _TCHAR* tName = tsName.c_str();
```

**Target Code:**
```cpp
std::string name = getName();
const char* cName = name.c_str();
```

**Changes Required:**
1. Line 75: Replace `tstring tsName = to_tstring(getName());` with `std::string name = getName();`
2. Line 76: Replace `const _TCHAR* tName = tsName.c_str();` with `const char* cName = name.c_str();`
3. Line 80: Replace `tName` with `cName`
4. Line 82: Replace `tName` with `cName`
5. Line 93: Replace `tName` with `cName`
6. Line 97: Replace `tName` with `cName`

**Test:**
```bash
# Build all commands
cd build && cmake --build . --target yamy_core
# Verify zero tstring in command_base.h lines 59-99
grep -n "tstring\|_TCHAR" src/core/commands/command_base.h | grep -E "^(7[5-9]|[89][0-9]|9[0-9]):"
```

**PR Title:** `refactor(command_base): Replace tstring with std::string in load() method`

**Acceptance Criteria:**
- [ ] No `tstring` on lines 75-99
- [ ] No `_TCHAR*` on lines 75-99
- [ ] All 63 commands compile successfully
- [ ] Tracking script shows tstring count reduced by ~2

---

### Subtask 1.2: Eliminate tstring from output() method
**File:** `src/core/commands/command_base.h`
**Lines:** 105

**Current Code:**
```cpp
i_ost << "&" << to_tstring(getName());
```

**Target Code:**
```cpp
i_ost << "&" << getName();  // getName() already returns std::string, tostream handles it
```

**Changes Required:**
1. Line 105: Remove `to_tstring()` wrapper around `getName()`
2. Verify `tostream` (msgstream.h) supports `std::string` directly

**Test:**
```bash
# Build and verify output format unchanged
cd build && cmake --build . --target yamy_core
# Manual test: Load .mayu file, verify command output format
```

**PR Title:** `refactor(command_base): Remove tstring conversion in output() method`

**Acceptance Criteria:**
- [ ] Line 105 uses `getName()` directly without `to_tstring()`
- [ ] Output format remains identical (backward compatibility)
- [ ] Tracking script shows tstring usage reduced by ~1

---

### Subtask 1.3: Eliminate tstring from loadArgsRecursive() helper
**File:** `src/core/commands/command_base.h`
**Lines:** 145

**Current Code:**
```cpp
if (!i_sl->getComma(false, to_tstring(getName()).c_str())) {
```

**Target Code:**
```cpp
if (!i_sl->getComma(false, getName().c_str())) {
```

**Changes Required:**
1. Line 145: Remove `to_tstring()` wrapper
2. Verify `SettingLoader::getComma()` signature accepts `const char*`

**Investigation Step:**
```bash
grep "getComma" src/core/settings/setting_loader.h
```

**If getComma() requires const _TCHAR*:**
- **Subtask 1.3a:** First migrate `SettingLoader::getComma()` to accept `const char*` (prerequisite)
- **Subtask 1.3b:** Then update command_base.h

**Test:**
```bash
cd build && cmake --build . --target yamy_core
# Test multi-argument commands load correctly
```

**PR Title:** `refactor(command_base): Remove tstring from loadArgsRecursive()`

**Acceptance Criteria:**
- [ ] Line 145 uses `getName().c_str()` directly
- [ ] Multi-argument commands parse correctly

---

### Subtask 1.4: Eliminate tstring from outputOneArg() helper
**File:** `src/core/commands/command_base.h`
**Lines:** 176-177

**Current Code:**
```cpp
if constexpr (std::is_same_v<typename std::decay<decltype(std::get<I>(m_args))>::type, std::string>) {
    i_ost << to_tstring(std::get<I>(m_args));
}
```

**Target Code:**
```cpp
if constexpr (std::is_same_v<typename std::decay<decltype(std::get<I>(m_args))>::type, std::string>) {
    i_ost << std::get<I>(m_args);  // tostream should handle std::string
}
```

**Changes Required:**
1. Line 177: Remove `to_tstring()` wrapper
2. Verify `tostream` (msgstream.h) has `operator<<(const std::string&)`

**Investigation Step:**
```bash
grep "operator<<.*std::string" src/core/utils/msgstream.h
```

**If tostream lacks std::string support:**
- **Subtask 1.4a:** Add `tostream& operator<<(const std::string&)` to msgstream.h
- **Subtask 1.4b:** Then update command_base.h

**Test:**
```bash
cd build && cmake --build . --target yamy_core
# Test commands with string arguments output correctly
```

**PR Title:** `refactor(command_base): Remove tstring from outputOneArg()`

**Acceptance Criteria:**
- [ ] Line 177 outputs `std::get<I>(m_args)` directly
- [ ] String argument commands serialize correctly

---

### Track 1 Summary
**Total Subtasks:** 4 (potentially 6 if prerequisites needed)
**Expected Reduction:** ~4-6 legacy string usages
**Branch Strategy:** One PR per subtask OR combine 1.1-1.4 into single PR

---

## Track 2: SettingLoader String API Migration

**Priority:** 🔴 HIGH (Required for Track 1.3)
**Files:** 2 files
**Estimated Effort:** 1-2 hours total
**Dependencies:** None

### Subtask 2.1: Migrate SettingLoader method signatures to const char*
**Files:**
- `src/core/settings/setting_loader.h` (declarations)
- `src/core/settings/setting_loader.cpp` (implementations)

**Methods to Migrate:**
1. `getOpenParen(bool, const _TCHAR*)` → `getOpenParen(bool, const char*)`
2. `getCloseParen(bool, const _TCHAR*)` → `getCloseParen(bool, const char*)`
3. `getComma(bool, const _TCHAR*)` → `getComma(bool, const char*)`

**Changes Required:**

**In setting_loader.h:**
```cpp
// Before
bool getOpenParen(bool i_isThrow, const _TCHAR *i_name = NULL);
bool getCloseParen(bool i_isThrow, const _TCHAR *i_name = NULL);
bool getComma(bool i_isThrow, const _TCHAR *i_name = NULL);

// After
bool getOpenParen(bool i_isThrow, const char *i_name = nullptr);
bool getCloseParen(bool i_isThrow, const char *i_name = nullptr);
bool getComma(bool i_isThrow, const char *i_name = nullptr);
```

**In setting_loader.cpp:**
- Update implementations to use `const char*`
- If error messages use `i_name`, ensure they work with `const char*`

**Test:**
```bash
cd build && cmake --build . --target yamy_core
# Test .mayu file parsing with syntax errors to verify error messages
```

**PR Title:** `refactor(setting_loader): Migrate method signatures to const char*`

**Acceptance Criteria:**
- [ ] All three methods use `const char*` in both .h and .cpp
- [ ] Error messages display correctly
- [ ] .mayu file parsing works correctly

---

### Subtask 2.2: Migrate SettingLoader internal strings to std::string
**File:** `src/core/settings/setting_loader.h`, `src/core/settings/setting_loader.cpp`

**Goal:** Replace any internal `tstring` members with `std::string`

**Investigation Step:**
```bash
grep -n "tstring" src/core/settings/setting_loader.h src/core/settings/setting_loader.cpp
```

**Expected Changes:**
- Replace member variables `tstring m_*` with `std::string m_*`
- Update methods that construct/return strings

**Test:**
```bash
cd build && cmake --build . --target yamy_core
bash scripts/track_legacy_strings.sh
```

**PR Title:** `refactor(setting_loader): Replace tstring with std::string internally`

**Acceptance Criteria:**
- [ ] Zero `tstring` in setting_loader.h
- [ ] Zero `tstring` in setting_loader.cpp
- [ ] All setting loading tests pass

---

## Track 3: Win32 Type Elimination - Command Files (Atomic PRs)

**Priority:** 🟡 MEDIUM
**Files:** 11 command files
**Estimated Effort:** 15-30 minutes per file
**Dependencies:** None (each file is independent)

### Strategy
Each command file gets its own atomic PR. Files are sorted by complexity (simplest first).

---

### Subtask 3.1: cmd_set_foreground_window.cpp
**File:** `src/core/commands/cmd_set_foreground_window.cpp`
**Line:** 68
**Win32 Usage:** `static_cast<HWND>(data.found)`

**Current Code:**
```cpp
setForegroundWindow(static_cast<HWND>(data.found));
```

**Target Code:**
```cpp
setForegroundWindow(data.found);  // data.found is already WindowHandle
```

**Changes Required:**
1. Line 68: Remove `static_cast<HWND>()` wrapper
2. Verify `setForegroundWindow()` accepts `WindowHandle`

**Test:**
```bash
cd build && cmake --build . --target yamy_core
# Manual test: Use &SetForegroundWindow command
```

**PR Title:** `refactor(cmd_set_foreground_window): Remove HWND cast`

**Acceptance Criteria:**
- [ ] No HWND reference in cmd_set_foreground_window.cpp
- [ ] Command executes correctly

---

### Subtask 3.2: cmd_window_move_to.cpp
**File:** `src/core/commands/cmd_window_move_to.cpp`
**Line:** 29
**Win32 Usage:** `static_cast<HWND>(hwnd)`

**Current Code:**
```cpp
asyncMoveWindow(static_cast<HWND>(hwnd), m_x, m_y);
```

**Investigation:**
Check if `asyncMoveWindow()` signature needs updating to accept `WindowHandle`:
```bash
grep "asyncMoveWindow" src/core -r
```

**Target Code:**
```cpp
asyncMoveWindow(hwnd, m_x, m_y);
```

**Changes Required:**
1. Line 29: Remove `static_cast<HWND>()` wrapper
2. If needed: Update `asyncMoveWindow()` signature in window helper (separate subtask)

**Test:**
```bash
cd build && cmake --build . --target yamy_core
```

**PR Title:** `refactor(cmd_window_move_to): Remove HWND cast`

**Acceptance Criteria:**
- [ ] No HWND in cmd_window_move_to.cpp
- [ ] Window move command works

---

### Subtask 3.3: cmd_window_move.cpp
**File:** `src/core/commands/cmd_window_move.cpp`
**Line:** 34
**Win32 Usage:** `static_cast<HWND>(hwnd)`

**Current Code:**
```cpp
asyncMoveWindow(static_cast<HWND>(hwnd), rc.left + m_dx, rc.top + m_dy);
```

**Target Code:**
```cpp
asyncMoveWindow(hwnd, rc.left + m_dx, rc.top + m_dy);
```

**Changes Required:**
1. Line 34: Remove `static_cast<HWND>()` wrapper

**Test:**
```bash
cd build && cmake --build . --target yamy_core
```

**PR Title:** `refactor(cmd_window_move): Remove HWND cast`

**Acceptance Criteria:**
- [ ] No HWND in cmd_window_move.cpp

---

### Subtask 3.4: cmd_window_resize_to.cpp
**File:** `src/core/commands/cmd_window_resize_to.cpp`
**Line:** 29
**Win32 Usage:** `static_cast<HWND>(hwnd)`

**Current Code:**
```cpp
asyncResize(static_cast<HWND>(hwnd), m_width, m_height);
```

**Target Code:**
```cpp
asyncResize(hwnd, m_width, m_height);
```

**Test:**
```bash
cd build && cmake --build . --target yamy_core
```

**PR Title:** `refactor(cmd_window_resize_to): Remove HWND cast`

---

### Subtask 3.5: cmd_window_move_visibly.cpp
**File:** `src/core/commands/cmd_window_move_visibly.cpp`
**Line:** 61
**Win32 Usage:** `static_cast<HWND>(hwnd)`

**Current Code:**
```cpp
asyncMoveWindow(static_cast<HWND>(hwnd), x, y);
```

**Target Code:**
```cpp
asyncMoveWindow(hwnd, x, y);
```

**PR Title:** `refactor(cmd_window_move_visibly): Remove HWND cast`

---

### Subtask 3.6: cmd_window_hv_maximize.cpp
**File:** `src/core/commands/cmd_window_hv_maximize.cpp`
**Line:** 32
**Win32 Usage:** `static_cast<HWND>(hwnd)`

**Current Code:**
```cpp
asyncMoveWindow(static_cast<HWND>(hwnd), monitorWorkArea.left, monitorWorkArea.top, ...);
```

**Target Code:**
```cpp
asyncMoveWindow(hwnd, monitorWorkArea.left, monitorWorkArea.top, ...);
```

**PR Title:** `refactor(cmd_window_hv_maximize): Remove HWND cast`

---

### Subtask 3.7: cmd_window_identify.cpp
**File:** `src/core/commands/cmd_window_identify.cpp`
**Line:** 19
**Win32 Usage:** `_T("HWND:\t0x")` and `(uintptr_t)i_param->m_hwnd`

**Current Code:**
```cpp
i_engine->m_log << _T("HWND:\t0x") << std::hex << (uintptr_t)i_param->m_hwnd << std::dec << std::endl;
```

**Target Code:**
```cpp
i_engine->m_log << "WindowHandle:\t0x" << std::hex << (uintptr_t)i_param->m_hwnd << std::dec << std::endl;
```

**Changes Required:**
1. Replace `_T("HWND:\t0x")` with `"WindowHandle:\t0x"`
2. Keep `(uintptr_t)` cast (it's platform-agnostic)

**PR Title:** `refactor(cmd_window_identify): Replace HWND with WindowHandle in output`

**Acceptance Criteria:**
- [ ] No `_T()` macro
- [ ] No "HWND" string literal

---

### Subtask 3.8: cmd_mouse_hook.cpp
**File:** `src/core/commands/cmd_mouse_hook.cpp`
**Line:** 51
**Win32 Usage:** `static_cast<HWND>(target)`

**Current Code:**
```cpp
(uint32_t)((uintptr_t)getToplevelWindow(static_cast<HWND>(target), &isMDI));
```

**Target Code:**
```cpp
(uint32_t)((uintptr_t)getToplevelWindow(target, &isMDI));
```

**Changes Required:**
1. Remove `static_cast<HWND>()` wrapper
2. Verify `getToplevelWindow()` signature accepts `WindowHandle`

**PR Title:** `refactor(cmd_mouse_hook): Remove HWND cast`

---

### Subtask 3.9: cmd_sync.cpp
**File:** `src/core/commands/cmd_sync.cpp`
**Line:** 28
**Win32 Usage:** `DWORD r = WaitForSingleObject(...)`

**Current Code:**
```cpp
DWORD r = WaitForSingleObject(i_engine->m_eSync, 5000);
```

**Target Code:**
```cpp
uint32_t r = WaitForSingleObject(i_engine->m_eSync, 5000);
```

**Changes Required:**
1. Replace `DWORD` with `uint32_t`

**PR Title:** `refactor(cmd_sync): Replace DWORD with uint32_t`

**Acceptance Criteria:**
- [ ] No DWORD in cmd_sync.cpp

---

### Subtask 3.10: cmd_post_message.h + cmd_post_message.cpp
**Files:**
- `src/core/commands/cmd_post_message.h`
- `src/core/commands/cmd_post_message.cpp`

**Win32 Usages:**
- Header line 8: Template args `UINT, WPARAM, LPARAM`
- Implementation lines 9-11: Variable types `UINT, WPARAM, LPARAM`

**Strategy:**
Create platform-agnostic message types in `src/core/platform/types.h`:

```cpp
// Add to types.h
namespace yamy::platform {
    using MessageId = uint32_t;
    using MessageWParam = uintptr_t;
    using MessageLParam = intptr_t;
}
```

**Changes Required:**

**cmd_post_message.h:**
```cpp
// Before
class Command_PostMessage : public Command<Command_PostMessage, ToWindowType, UINT, WPARAM, LPARAM>

// After
#include "../platform/types.h"
class Command_PostMessage : public Command<Command_PostMessage, ToWindowType,
    yamy::platform::MessageId, yamy::platform::MessageWParam, yamy::platform::MessageLParam>
```

**cmd_post_message.cpp:**
```cpp
// Before
UINT i_message = std::get<1>(m_args);
WPARAM i_wParam = std::get<2>(m_args);
LPARAM i_lParam = std::get<3>(m_args);

// After
yamy::platform::MessageId i_message = std::get<1>(m_args);
yamy::platform::MessageWParam i_wParam = std::get<2>(m_args);
yamy::platform::MessageLParam i_lParam = std::get<3>(m_args);
```

**Test:**
```bash
cd build && cmake --build . --target yamy_core
# Test PostMessage command
```

**PR Title:** `refactor(cmd_post_message): Replace Win32 message types with platform types`

**Acceptance Criteria:**
- [ ] No UINT, WPARAM, LPARAM in cmd_post_message.h
- [ ] No UINT, WPARAM, LPARAM in cmd_post_message.cpp
- [ ] PostMessage command executes correctly

---

### Track 3 Summary
**Total Subtasks:** 10 atomic PRs
**Expected Reduction:** 11 files with Win32 leakage → 10 files (commands cleaned)
**Branch Strategy:** One branch per file: `refactor/cmd-{name}-win32-types`

---

## Track 4: Win32 Type Elimination - Window Subsystem

**Priority:** 🟡 MEDIUM
**Files:** 4 files in src/core/window
**Estimated Effort:** 2-3 hours
**Dependencies:** None

### Subtask 4.1: layoutmanager.h - Replace HWND members
**File:** `src/core/window/layoutmanager.h`
**Lines:** 39, 40, 50, 61, 72

**Current Code:**
```cpp
HWND m_hwnd;                // Line 39
HWND m_hwndParent;          // Line 40
HWND m_hwnd;                // Line 50
HWND m_hwnd;                // Line 61
LayoutManager(HWND i_hwnd); // Line 72
bool addItem(HWND i_hwnd, ...); // Line 93
```

**Target Code:**
```cpp
yamy::platform::WindowHandle m_hwnd;
yamy::platform::WindowHandle m_hwndParent;
yamy::platform::WindowHandle m_hwnd;
yamy::platform::WindowHandle m_hwnd;
LayoutManager(yamy::platform::WindowHandle i_hwnd);
bool addItem(yamy::platform::WindowHandle i_hwnd, ...);
```

**Changes Required:**
1. Add `#include "../platform/types.h"`
2. Replace all `HWND` with `yamy::platform::WindowHandle`

**PR Title:** `refactor(layoutmanager): Replace HWND with WindowHandle`

---

### Subtask 4.2: layoutmanager.h - Replace DWORD, WPARAM, LPARAM
**File:** `src/core/window/layoutmanager.h`
**Lines:** 111, 115-116

**Current Code:**
```cpp
virtual BOOL wmSize(DWORD /* i_fwSizeType */, ...);
virtual BOOL defaultWMHandler(UINT i_message, WPARAM i_wParam, LPARAM i_lParam);
```

**Target Code:**
```cpp
virtual bool wmSize(uint32_t /* i_fwSizeType */, ...);
virtual bool defaultWMHandler(uint32_t i_message, uintptr_t i_wParam, intptr_t i_lParam);
```

**Changes Required:**
1. Replace `BOOL` → `bool`
2. Replace `DWORD` → `uint32_t`
3. Replace `UINT` → `uint32_t`
4. Replace `WPARAM` → `uintptr_t`
5. Replace `LPARAM` → `intptr_t`

**PR Title:** `refactor(layoutmanager): Replace Win32 message types`

---

### Subtask 4.3: layoutmanager.cpp - Update implementations
**File:** `src/core/window/layoutmanager.cpp`
**Lines:** 12, 51, 209, 211, 220, 232, 236, 242

**Changes Required:**
1. Update constructor signature (line 12)
2. Update `addItem()` signature (line 51)
3. Replace Win32 API calls with PAL equivalents:
   - `SetWindowLongPtr` → PAL wrapper (if exists) or keep with cast
4. Update `wmSize()` and `defaultWMHandler()` signatures

**Investigation:**
```bash
grep "SetWindowLongPtr\|GET_X_LPARAM\|GET_Y_LPARAM\|LOWORD\|HIWORD" src/core/window/layoutmanager.cpp
```

**Note:** If layoutmanager is Windows-only GUI code, consider moving to `src/platform/windows/` instead of abstracting.

**PR Title:** `refactor(layoutmanager): Update implementation to match header changes`

---

### Subtask 4.4: focus.cpp - Replace Win32 message types
**File:** `src/core/window/focus.cpp`
**Lines:** 11, 35, 42

**Current Code:**
```cpp
HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam  // Line 11
TRUE, (LPARAM)i_hwnd    // Line 35
FALSE, (LPARAM)i_hwnd   // Line 42
```

**Target Code:**
```cpp
yamy::platform::WindowHandle i_hwnd, uint32_t i_message, uintptr_t i_wParam, intptr_t i_lParam
true, (intptr_t)i_hwnd
false, (intptr_t)i_hwnd
```

**PR Title:** `refactor(focus): Replace Win32 types with platform types`

---

### Subtask 4.5: target.cpp - Remove Win32 types
**File:** `src/core/window/target.cpp`

**Investigation:**
```bash
grep -n "HWND\|DWORD\|WPARAM\|LPARAM" src/core/window/target.cpp
```

**PR Title:** `refactor(target): Replace Win32 types`

---

### Track 4 Summary
**Total Subtasks:** 5
**Expected Reduction:** 4 files cleaned in src/core/window
**Decision Point:** Consider moving layoutmanager to platform layer if it's GUI-specific

---

## Track 5: Win32 Type Elimination - Engine Subsystem

**Priority:** 🟡 MEDIUM
**Files:** 5 files in src/core/engine
**Estimated Effort:** 2-3 hours
**Dependencies:** None

### Subtask 5.1: engine.h - Replace MSG
**File:** `src/core/engine/engine.h`
**Context:** Line ~55 in engine.cpp: `MSG message;`

**Investigation:**
```bash
grep -n "MSG " src/core/engine/engine.h src/core/engine/engine.cpp
```

**Strategy:**
If `MSG` is only used locally in engine.cpp, replace with platform-agnostic equivalent or keep cast at boundary.

**PR Title:** `refactor(engine): Replace MSG with platform message type`

---

### Subtask 5.2: engine.h - Replace template WPARAM_T, LPARAM_T
**File:** `src/core/engine/engine.h`
**Lines:** 419-421

**Current Code:**
```cpp
template <typename WPARAM_T, typename LPARAM_T>
void commandNotify(yamy::platform::WindowHandle i_hwnd, unsigned int i_message,
                   WPARAM_T i_wParam, LPARAM_T i_lParam)
```

**Analysis:**
This is a generic template - `WPARAM_T` and `LPARAM_T` are template parameter names, not Win32 types!

**Action:**
Rename for clarity:
```cpp
template <typename WParam, typename LParam>
void commandNotify(yamy::platform::WindowHandle i_hwnd, unsigned int i_message,
                   WParam i_wParam, LParam i_lParam)
```

**PR Title:** `refactor(engine): Rename WPARAM_T/LPARAM_T template params for clarity`

**Note:** This is LOW priority - just a naming issue, not actual Win32 dependency.

---

### Subtask 5.3: engine_focus.cpp - Replace HWND in log output
**File:** `src/core/engine/engine_focus.cpp`
**Lines:** 49, 75, 96

**Current Code:**
```cpp
m_log << "\tHWND:\t" << std::hex << (ULONG_PTR)fot->m_hwndFocus ...
```

**Target Code:**
```cpp
m_log << "\tWindowHandle:\t" << std::hex << (uintptr_t)fot->m_hwndFocus ...
```

**Changes:**
1. Replace `HWND` string literal with `WindowHandle`
2. Replace `ULONG_PTR` with `uintptr_t`

**PR Title:** `refactor(engine_focus): Replace HWND/ULONG_PTR in logging`

---

### Subtask 5.4: engine_lifecycle.cpp - Abstract Win32 API call
**File:** `src/core/engine/engine_lifecycle.cpp`
**Lines:** 47-51

**Current Code:**
```cpp
BOOL (WINAPI *pChangeWindowMessageFilter)(UINT, DWORD) = ...
pChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
```

**Strategy:**
Move this Windows-specific security feature to PAL:
- Create `IWindowSystem::enableMessageFilter(uint32_t message)` in PAL
- Implement in `window_system_win32.cpp`
- Stub in `window_system_linux.cpp`

**PR Title:** `refactor(engine_lifecycle): Move ChangeWindowMessageFilter to PAL`

---

### Subtask 5.5: engine_window.cpp - Remove HWND cast
**File:** `src/core/engine/engine_window.cpp`
**Line:** 24

**Current Code:**
```cpp
HWND hwnd = static_cast<HWND>(i_hwnd);
```

**Investigation:**
Check if this local variable is needed or if `i_hwnd` can be used directly.

**Target Code:**
```cpp
// Remove line entirely, use i_hwnd directly
```

**PR Title:** `refactor(engine_window): Remove redundant HWND cast`

---

### Track 5 Summary
**Total Subtasks:** 5
**Expected Reduction:** 5 files cleaned in src/core/engine
**Note:** Subtask 5.2 is optional (naming only)

---

## Track 6: Settings & Parser String Migration

**Priority:** 🟡 MEDIUM
**Files:** 6 files
**Estimated Effort:** 3-4 hours
**Dependencies:** Track 2 (SettingLoader)

### Subtask 6.1: parser.h - Replace tregex with std::regex
**File:** `src/core/settings/parser.h`

**Investigation:**
```bash
grep -n "tregex\|tstring\|_TCHAR" src/core/settings/parser.h
```

**Expected Changes:**
- Replace `tregex` with `std::regex`
- Replace `tstring` with `std::string`
- Replace `tstringi` with custom case-insensitive string type or std::string

**PR Title:** `refactor(parser): Replace tregex with std::regex`

**Acceptance Criteria:**
- [ ] No `tregex` in parser.h
- [ ] Regex patterns work identically
- [ ] .mayu file parsing succeeds

---

### Subtask 6.2: parser.cpp - Update regex usage
**File:** `src/core/settings/parser.cpp`

**Changes:**
- Update all regex operations to use `std::regex`
- Replace `_T("pattern")` with `"pattern"` (UTF-8)
- Ensure regex patterns remain identical

**Test:**
```bash
# Test complex .mayu file with various syntax patterns
cd build && cmake --build . --target yamy_core
./yamy_test --load complex.mayu
```

**PR Title:** `refactor(parser): Update implementation to use std::regex`

---

### Subtask 6.3: setting.h - Replace tstringi members
**File:** `src/core/settings/setting.h`

**Investigation:**
```bash
grep -n "tstringi\|tstring" src/core/settings/setting.h
```

**Strategy:**
If `tstringi` is used for case-insensitive comparisons:
- Option A: Replace with `std::string` and use case-insensitive comparison functions
- Option B: Create a `CaseInsensitiveString` wrapper class

**Recommendation:** Option A (simpler)

**PR Title:** `refactor(setting): Replace tstringi with std::string`

---

### Subtask 6.4: setting.cpp - Update string operations
**File:** `src/core/settings/setting.cpp`

**Changes:**
- Replace all `tstring` with `std::string`
- Replace `_T()` macros with string literals
- Update any case-insensitive comparisons

**Test:**
```bash
cd build && cmake --build . --target yamy_core
bash scripts/track_legacy_strings.sh
```

**PR Title:** `refactor(setting): Migrate to std::string`

---

### Subtask 6.5: setting_loader.cpp - Complete string migration
**File:** `src/core/settings/setting_loader.cpp`

**Note:** This builds on Track 2 (Subtask 2.2)

**Changes:**
- Replace all remaining `tstring` with `std::string`
- Replace all `_T()` macros
- Ensure UTF-8 file I/O

**PR Title:** `refactor(setting_loader): Complete std::string migration`

---

### Track 6 Summary
**Total Subtasks:** 5
**Expected Reduction:** ~100-150 legacy string usages
**Critical Path:** Must complete Track 2 first

---

## Track 7: Input & Keymap String Migration

**Priority:** 🟡 MEDIUM
**Files:** 6 files
**Estimated Effort:** 2-3 hours
**Dependencies:** None

### Subtask 7.1: keymap.h - Replace tstringi members
**File:** `src/core/input/keymap.h`

**Investigation:**
```bash
grep -n "tstringi\|tstring" src/core/input/keymap.h
```

**Expected:** Keymap name storage uses `tstringi`

**Changes:**
- Replace `tstringi m_name` with `std::string m_name`
- Update getter/setter methods
- Use case-insensitive comparison functions where needed

**PR Title:** `refactor(keymap): Replace tstringi with std::string`

---

### Subtask 7.2: keymap.cpp - Update string operations
**File:** `src/core/input/keymap.cpp`

**Changes:**
- Update string operations
- Replace `_T()` macros
- Update case-insensitive comparisons

**Test:**
```bash
cd build && cmake --build . --target yamy_core
# Test keymap switching
```

**PR Title:** `refactor(keymap): Migrate to std::string`

---

### Subtask 7.3: keyboard.h - Replace _TCHAR members
**File:** `src/core/input/keyboard.h`

**Investigation:**
```bash
grep -n "_TCHAR\|tstring" src/core/input/keyboard.h
```

**PR Title:** `refactor(keyboard): Replace _TCHAR with char`

---

### Subtask 7.4: keyboard.cpp - Update string operations
**File:** `src/core/input/keyboard.cpp`

**Changes:**
- Replace `_T()` macros
- Update string operations

**PR Title:** `refactor(keyboard): Migrate to std::string`

---

### Subtask 7.5: vkeytable.h - Replace _TCHAR
**File:** `src/core/input/vkeytable.h`

**PR Title:** `refactor(vkeytable): Replace _TCHAR with char`

---

### Subtask 7.6: vkeytable.cpp - Update table initialization
**File:** `src/core/input/vkeytable.cpp`

**Changes:**
- Replace `_T()` macros in VKey name table

**PR Title:** `refactor(vkeytable): Remove _T() macros from key names`

---

### Track 7 Summary
**Total Subtasks:** 6
**Expected Reduction:** ~50-100 legacy string usages

---

## Track 8: Function System - Remaining Legacy Strings

**Priority:** 🟢 LOW
**Files:** 2 files
**Estimated Effort:** 1 hour
**Dependencies:** None

### Subtask 8.1: function.cpp - Remaining _T() macros
**File:** `src/core/functions/function.cpp`

**Investigation:**
```bash
grep -n "_T(" src/core/functions/function.cpp | head -20
```

**Expected:** Type table string literals still use `_T()`

**Changes:**
Replace `_T("VK_...")` with `"VK_..."` in VKey type tables

**PR Title:** `refactor(function): Remove _T() from type tables`

---

### Subtask 8.2: function.h - Header cleanup
**File:** `src/core/functions/function.h`

**Investigation:**
```bash
grep -n "tstring\|_TCHAR" src/core/functions/function.h
```

**PR Title:** `refactor(function): Remove remaining legacy string types`

---

## Track 9: CI Enforcement & Automation

**Priority:** 🟢 LOW
**Files:** CI configuration
**Estimated Effort:** 1 hour
**Dependencies:** Tracks 3-5 complete

### Subtask 9.1: Add Win32 type leakage check to CI
**File:** `.github/workflows/ci.yml`

**Add Step:**
```yaml
- name: Check for Win32 type leakage in core
  run: |
    # Fail if Win32 types found in src/core
    if grep -r "HWND\|DWORD\|MSG\|WPARAM\|LPARAM" src/core --include="*.cpp" --include="*.h" | grep -v "template.*WPARAM_T\|template.*LPARAM_T"; then
      echo "ERROR: Win32 types leaked into src/core"
      exit 1
    fi
```

**PR Title:** `ci: Add Win32 type leakage detection`

---

### Subtask 9.2: Add legacy string regression check
**File:** `.github/workflows/ci.yml`

**Add Step:**
```yaml
- name: Check legacy string usage
  run: |
    bash scripts/track_legacy_strings.sh > metrics.txt
    LEGACY_COUNT=$(grep "TOTAL legacy string usages:" metrics.txt | awk '{print $5}')
    echo "Legacy string usages: $LEGACY_COUNT"
    # Fail if count increases (baseline: 971)
    if [ "$LEGACY_COUNT" -gt 971 ]; then
      echo "ERROR: Legacy string usage increased!"
      exit 1
    fi
```

**PR Title:** `ci: Add legacy string usage regression check`

---

## Coordination & Strategy

### Recommended Execution Order

**Phase 1: Foundation (Weeks 1-2)**
1. Track 2: SettingLoader API migration (enables Track 1)
2. Track 1: Command template cleanup (highest impact)
3. Track 3: Command file Win32 cleanup (10 atomic PRs, can parallelize)

**Phase 2: Subsystems (Weeks 3-4)**
4. Track 4: Window subsystem cleanup
5. Track 5: Engine subsystem cleanup
6. Track 6: Settings/Parser string migration

**Phase 3: Completion (Week 5)**
7. Track 7: Input/Keymap string migration
8. Track 8: Function system cleanup
9. Track 9: CI enforcement

### PR Strategy

**Atomic PRs (Preferred):**
- Each subtask = 1 PR
- Easier to review
- Safer to merge
- Can be parallelized across team members

**Batched PRs (Alternative):**
- Combine related subtasks within same file
- Example: Subtasks 1.1-1.4 → single PR for command_base.h
- Faster but harder to review

### Testing Strategy

**Per-PR Testing:**
```bash
# After each change
cd build
rm -rf *
cmake .. -G "MinGW Makefiles"
cmake --build . --target yamy_core
bash ../scripts/track_legacy_strings.sh
```

**Integration Testing:**
After each track completion:
1. Full build (32-bit + 64-bit)
2. Run test suite (if exists)
3. Manual smoke test: Load .mayu, test key remapping, test window commands

**Regression Prevention:**
- Run tracking script before/after each PR
- Document metric changes in PR description
- Update parallel-refinement.md status after merge

### Branch Naming Convention

```
refactor/track-{N}-{subtask}-{description}
```

Examples:
- `refactor/track-1-1-command-base-load`
- `refactor/track-3-1-cmd-set-foreground-window`
- `refactor/track-6-1-parser-regex`

### PR Template

```markdown
## Track {N}: {Track Name}
### Subtask {N.M}: {Subtask Name}

**Changes:**
- [ ] Change 1
- [ ] Change 2

**Files Modified:**
- `path/to/file.cpp`

**Metrics:**
- Before: {tstring: X, _T(): Y, Win32: Z}
- After: {tstring: X-a, _T(): Y-b, Win32: Z-c}
- Reduction: {a tstring, b _T(), c Win32}

**Testing:**
- [ ] Build successful
- [ ] Tracking script run
- [ ] Manual test: {specific test}

**Acceptance Criteria:**
- [ ] Criterion 1
- [ ] Criterion 2
```

---

## Success Metrics

### Overall Goals

**Phase 3 (String Unification):**
- **Start:** 971 legacy usages
- **Target:** < 200 usages (80% reduction)
- **Critical:** Zero tstring/TCHAR in command_base.h

**Phase 4 (PAL Completion):**
- **Start:** 21 files with Win32 leakage
- **Target:** 0 files with Win32 leakage in src/core/commands
- **Stretch:** < 5 files total with Win32 leakage
- **Maintain:** Zero windows.h includes in src/core

### Per-Track Metrics

| Track | Files | Expected Reduction | Priority |
|-------|-------|-------------------|----------|
| Track 1 | 1 | ~6 legacy strings | 🔴 Critical |
| Track 2 | 2 | ~10 legacy strings | 🔴 High |
| Track 3 | 11 | 11 files cleaned | 🟡 Medium |
| Track 4 | 4 | 4 files cleaned | 🟡 Medium |
| Track 5 | 5 | 5 files cleaned | 🟡 Medium |
| Track 6 | 6 | ~150 legacy strings | 🟡 Medium |
| Track 7 | 6 | ~100 legacy strings | 🟡 Medium |
| Track 8 | 2 | ~50 legacy strings | 🟢 Low |
| Track 9 | 1 | CI enforcement | 🟢 Low |

---

## Quick Reference

### Key Files by Track

**Track 1:** `src/core/commands/command_base.h`
**Track 2:** `src/core/settings/setting_loader.{h,cpp}`
**Track 3:** `src/core/commands/cmd_*.cpp` (11 files)
**Track 4:** `src/core/window/*.{h,cpp}` (4 files)
**Track 5:** `src/core/engine/*.{h,cpp}` (5 files)
**Track 6:** `src/core/settings/*.{h,cpp}` (6 files)
**Track 7:** `src/core/input/*.{h,cpp}` (6 files)
**Track 8:** `src/core/functions/*.{h,cpp}` (2 files)
**Track 9:** `.github/workflows/ci.yml`

### Useful Commands

```bash
# Track progress
bash scripts/track_legacy_strings.sh

# Find specific legacy usage
grep -r "tstring" src/core --include="*.h" --include="*.cpp"
grep -r "_T(" src/core --include="*.h" --include="*.cpp"
grep -r "HWND\|DWORD\|MSG\|WPARAM\|LPARAM" src/core --include="*.cpp" --include="*.h"

# Build and test
cd build && rm -rf * && cmake .. -G "MinGW Makefiles" && cmake --build .

# Full rebuild and package
bash scripts/cmake_package.sh
```

---

## Notes for Implementers

1. **Before Starting Any Subtask:**
   - Pull latest master
   - Run tracking script to get baseline metrics
   - Read the file you're modifying completely

2. **During Implementation:**
   - Make minimal changes (only what's specified)
   - Test after each change
   - Commit frequently with clear messages

3. **Before Submitting PR:**
   - Run tracking script, document metric changes
   - Ensure build succeeds
   - Update parallel-refinement.md status section
   - Fill out PR template completely

4. **Common Pitfalls:**
   - Don't assume SettingLoader methods accept `const char*` - check first!
   - Don't assume tostream supports std::string - verify!
   - Don't remove Win32 casts without checking the called function signature
   - Don't change multiple files in one PR unless they're tightly coupled

5. **Getting Help:**
   - Check existing PAL patterns in `src/platform/windows/`
   - Check `src/core/platform/types.h` for platform-agnostic types
   - Use `grep` to find similar patterns in the codebase
   - Test thoroughly before submitting

---

**Ready to execute! Start with Track 2, then Track 1, then parallelize Track 3.**
