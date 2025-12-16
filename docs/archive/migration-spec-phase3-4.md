# Migration Specification: Phase 3 & 4
**String Unification and Platform Abstraction Layer Completion**

## Executive Summary

This document provides a detailed, actionable specification for completing:
- **Phase 3:** String Unification (UTF-8 migration)
- **Phase 4:** Platform Abstraction Layer completion

**Current Status:**
- ~669 `tstring`/`_TCHAR` usages remain
- 1 `#include <windows.h>` in `src/core/` (cmd_wait.cpp)
- No Linux backend exists

**Goals:**
- 100% UTF-8 internally (`std::string`)
- UTF-16 conversions only at Windows API boundaries
- Zero Windows headers in `src/core/`
- Linux stub backend for architectural validation

---

## Phase 3: String Unification Migration

### Overview
Migrate all internal string handling from `tstring`/`_TCHAR` (UTF-16 on Windows) to `std::string` (UTF-8 everywhere).

### 3.1: Create UTF Conversion Infrastructure

**Priority:** P0 (Blocking all other string work)
**Effort:** 2-4 hours
**Files to create:**
- `src/platform/windows/utf_conversion.h`
- `src/platform/windows/utf_conversion.cpp`

#### Acceptance Criteria
- [ ] Create conversion functions:
  ```cpp
  namespace yamy::platform {
      std::string wstring_to_utf8(const std::wstring& wide);
      std::wstring utf8_to_wstring(const std::string& utf8);

      // Convenience for Windows API
      std::wstring utf8_to_wstring(const char* utf8);
      std::string wstring_to_utf8(const wchar_t* wide);
  }
  ```
- [ ] Use Windows `MultiByteToWideChar` / `WideCharToMultiByte` with `CP_UTF8`
- [ ] Handle empty strings correctly
- [ ] Handle null pointers gracefully (return empty string)
- [ ] Add unit tests for conversion functions
- [ ] Document that these functions are ONLY for PAL use

#### Implementation Notes
```cpp
// Example implementation
std::wstring utf8_to_wstring(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0,
        utf8.c_str(), (int)utf8.size(), nullptr, 0);

    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0,
        utf8.c_str(), (int)utf8.size(), &result[0], size_needed);

    return result;
}
```

---

### 3.2: Migrate Core String Types (Engine, Keymap, Settings)

**Priority:** P0 (Foundation)
**Effort:** 8-16 hours
**Dependencies:** 3.1

#### 3.2.1: Engine Class Migration

**Files:**
- `src/core/engine/engine.h`
- `src/core/engine/engine.cpp`
- `src/core/engine/engine_*.cpp` (all engine implementation files)

**Task Breakdown:**
- [ ] Identify all `tstring` members in `Engine` class
  ```bash
  grep -n "tstring" src/core/engine/engine.h
  ```
- [ ] Replace member variables:
  - [ ] `tstring m_name` → `std::string m_name`
  - [ ] `tstring m_settingPath` → `std::string m_settingPath`
  - [ ] Any other `tstring` members found
- [ ] Update all member functions that take/return `tstring`
- [ ] Update all internal string literals from `_T("...")` to `"..."`
- [ ] Build and fix compilation errors
- [ ] Run tests to verify functionality

**Acceptance Criteria:**
- [ ] No `tstring` in engine.h
- [ ] No `_T()` macros in engine implementation files
- [ ] All tests pass
- [ ] Engine still loads settings correctly

#### 3.2.2: Keymap Class Migration

**Files:**
- `src/core/input/keymap.h`
- `src/core/input/keymap.cpp`

**Task Breakdown:**
- [ ] Replace `tstring m_name` → `std::string m_name`
- [ ] Update keymap name handling
- [ ] Update keymap registry/lookup (currently uses `tstring` keys)
- [ ] Update all `_T()` literals to plain `""`
- [ ] Verify keymap switching works correctly

**Acceptance Criteria:**
- [ ] Keymap names are UTF-8
- [ ] Keymap switching functional
- [ ] No `tstring` in keymap.h

#### 3.2.3: Settings System Migration

**Files:**
- `src/core/settings/setting.h`
- `src/core/settings/setting.cpp`
- `src/core/settings/setting_loader.h`
- `src/core/settings/setting_loader.cpp`
- `src/core/settings/parser.h`
- `src/core/settings/parser.cpp`

**Task Breakdown:**
- [ ] Update `Setting` class string members
- [ ] Update `SettingLoader::load_ARGUMENT` for string parameters
- [ ] Update parser string handling
- [ ] File I/O: Create PAL wrapper for file reading (UTF-8 internally)
  - Windows: Read as UTF-16, convert to UTF-8
  - Linux: Direct UTF-8 read
- [ ] Update configuration file path handling
- [ ] Test with existing .mayu configuration files

**Acceptance Criteria:**
- [ ] Can load existing configuration files
- [ ] String arguments parsed correctly
- [ ] File paths work with non-ASCII characters
- [ ] No `tstring` in settings headers

---

### 3.3: Migrate Function/Command System

**Priority:** P1
**Effort:** 6-12 hours
**Dependencies:** 3.2

#### Files
- `src/core/functions/function.h`
- `src/core/functions/function.cpp`
- `src/core/functions/function_creator.cpp`
- All command files in `src/core/commands/`

#### Task Breakdown
- [ ] Update `FunctionParam` string members
- [ ] Update `Function` base class string handling
- [ ] Audit all 63 command files for `tstring` usage
  ```bash
  grep -r "tstring" src/core/commands/ | wc -l
  ```
- [ ] For each command using `tstring`:
  - [ ] Replace parameter types
  - [ ] Update `load()` method string handling
  - [ ] Update `exec()` method string handling
  - [ ] Update `outputArgs()` if it outputs strings
- [ ] Update `function_creator.cpp` string handling
- [ ] Test command execution with string arguments

**Acceptance Criteria:**
- [ ] All commands compile
- [ ] Commands with string arguments work correctly
- [ ] Command output/logging displays correctly
- [ ] No `tstring` in command headers

---

### 3.4: Migrate UI Layer

**Priority:** P1
**Effort:** 12-20 hours
**Dependencies:** 3.1, 3.2

#### Files
- `src/ui/dlgeditsetting.cpp`
- `src/ui/dlginvestigate.cpp`
- `src/ui/dlglog.cpp`
- `src/ui/dlgsetting.cpp`
- `src/ui/dlgversion.cpp`
- `src/app/mayu.cpp`
- `src/app/yamy.cpp`

#### Task Breakdown

##### 3.4.1: Dialog String Handling
- [ ] Identify all Win32 dialog functions using strings:
  - `SetWindowText` / `GetWindowText`
  - `SetDlgItemText` / `GetDlgItemText`
  - `MessageBox`
  - `SendMessage` with string parameters
- [ ] Create PAL wrapper functions in `src/platform/windows/windowstool.h`:
  ```cpp
  void setWindowText(HWND hwnd, const std::string& text);
  std::string getWindowText(HWND hwnd);
  void setDlgItemText(HWND hwnd, int itemId, const std::string& text);
  std::string getDlgItemText(HWND hwnd, int itemId);
  int messageBox(HWND hwnd, const std::string& text,
                 const std::string& caption, UINT type);
  ```
- [ ] Implement wrappers using UTF conversion (3.1)
- [ ] Replace all direct Win32 string API calls with wrappers
- [ ] Update dialog procedures to use `std::string`
- [ ] Test all dialogs for correct display

##### 3.4.2: Application Entry Points
- [ ] Update `WinMain` / `wWinMain` parameter handling
- [ ] Convert command line arguments to UTF-8
- [ ] Update window class registration (use `RegisterClassExW`)
- [ ] Update main window creation

**Acceptance Criteria:**
- [ ] All dialogs display correctly
- [ ] Japanese/Unicode text displays properly
- [ ] No `TCHAR*` / `_T()` in dialog code
- [ ] Message boxes work correctly

---

### 3.5: Migrate Platform Utilities

**Priority:** P2
**Effort:** 4-8 hours
**Dependencies:** 3.1

#### Files
- `src/platform/windows/registry.cpp`
- `src/platform/windows/windowstool.cpp`
- `src/utils/stringtool.h`
- `src/utils/stringtool.cpp`

#### Task Breakdown

##### 3.5.1: Registry Operations
- [ ] Update `Registry` class to use `std::string` API
- [ ] Wrap `RegQueryValueExW` / `RegSetValueExW`
- [ ] Internal storage: UTF-8
- [ ] Convert at Win32 API boundary
- [ ] Test registry read/write

##### 3.5.2: Window Tools
- [ ] Update `loadString()` to return `std::string`
- [ ] Update `getToplevelWindow()` if it uses strings
- [ ] Update window manipulation functions using strings
- [ ] Remove `to_tstring()` conversion function

##### 3.5.3: String Tools
- [ ] Review `stringtool.h` functions
- [ ] Update to work with `std::string` instead of `tstring`
- [ ] Remove `tstring` typedef
- [ ] Remove `tregex` → use `std::regex`
- [ ] Update any string utility functions

**Acceptance Criteria:**
- [ ] Registry operations work with UTF-8
- [ ] Window title retrieval works correctly
- [ ] String utilities work with `std::string`
- [ ] No `tstring` typedef remains

---

### 3.6: Remove Legacy String Types

**Priority:** P3 (Cleanup)
**Effort:** 2-4 hours
**Dependencies:** 3.2, 3.3, 3.4, 3.5

#### Task Breakdown
- [ ] Remove `tstring` typedef from codebase
  ```bash
  grep -r "typedef.*tstring" src/
  ```
- [ ] Remove `tregex` typedef
- [ ] Remove `to_tstring()` function
- [ ] Remove `_T()` macro usages (search for stragglers)
- [ ] Clean up `#ifdef UNICODE` blocks (should be unused now)
- [ ] Update any remaining `_TCHAR` to `char`
- [ ] Full rebuild to catch any missed usages
- [ ] Run full test suite

**Acceptance Criteria:**
- [ ] `grep -r "tstring" src/` returns 0 results
- [ ] `grep -r "_T(" src/` returns 0 results (excluding comments)
- [ ] `grep -r "_TCHAR" src/` returns 0 results
- [ ] All tests pass
- [ ] Application runs correctly

**Verification Commands:**
```bash
# Should return 0:
grep -r "tstring\|_TCHAR" src/ --include="*.cpp" --include="*.h" | wc -l
grep -r "_T(" src/ --include="*.cpp" --include="*.h" | wc -l
```

---

## Phase 4: Platform Abstraction Layer Completion

### 4.1: Remove Last Windows Header from Core

**Priority:** P0
**Effort:** 1-2 hours
**Dependencies:** None

#### Task: Remove windows.h from cmd_wait.cpp

**File:** `src/core/commands/cmd_wait.cpp`

**Current Issue:**
```cpp
#include <windows.h> // For Sleep
```

**Solution:**
1. Create platform sleep abstraction
2. Move to PAL layer
3. Remove Windows dependency from command

#### Implementation

**Create PAL Sleep Function:**

File: `src/core/platform/thread.h`
```cpp
#pragma once
#include <cstdint>

namespace yamy::platform {
    // Sleep for specified milliseconds
    void sleep_ms(uint32_t milliseconds);
}
```

File: `src/platform/windows/thread_win32.cpp`
```cpp
#include "core/platform/thread.h"
#include <windows.h>

namespace yamy::platform {
    void sleep_ms(uint32_t milliseconds) {
        Sleep(milliseconds);
    }
}
```

File: `src/platform/linux/thread_linux.cpp` (stub)
```cpp
#include "core/platform/thread.h"
#include <thread>
#include <chrono>

namespace yamy::platform {
    void sleep_ms(uint32_t milliseconds) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(milliseconds));
    }
}
```

**Update cmd_wait.cpp:**
```cpp
#include "cmd_wait.h"
#include "core/platform/thread.h" // Instead of <windows.h>

void Command_Wait::exec(Engine* engine, FunctionParam* param) const {
    yamy::platform::sleep_ms(m_time);
}
```

**Update CMakeLists.txt:**
```cmake
# Add platform-specific thread implementation
if(WIN32)
    list(APPEND ENGINE_SOURCES src/platform/windows/thread_win32.cpp)
else()
    list(APPEND ENGINE_SOURCES src/platform/linux/thread_linux.cpp)
endif()
```

**Acceptance Criteria:**
- [ ] `src/core/commands/cmd_wait.cpp` has no `#include <windows.h>`
- [ ] Wait command compiles and works correctly
- [ ] `grep -r "#include <windows.h>" src/core/` returns 0 results
- [ ] Sleep abstraction works on Windows

---

### 4.2: Create Platform Interface Layer

**Priority:** P1
**Effort:** 8-12 hours
**Dependencies:** 4.1

#### Overview
Create abstract interfaces in `src/core/platform/` that define platform contracts.

#### 4.2.1: Create Interface Headers

**Directory Structure:**
```
src/core/platform/
├── window_system_interface.h
├── input_injector_interface.h
├── input_hook_interface.h
├── input_driver_interface.h
└── thread.h (already created in 4.1)
```

#### Task Breakdown

**File: `src/core/platform/window_system_interface.h`**
```cpp
#pragma once
#include <string>
#include <cstdint>

namespace yamy::platform {

// Platform-agnostic window handle
using WindowHandle = void*;

// Platform-agnostic point/rect
struct Point {
    int32_t x;
    int32_t y;
};

struct Rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    // Window queries
    virtual WindowHandle getForegroundWindow() = 0;
    virtual WindowHandle windowFromPoint(const Point& pt) = 0;
    virtual bool getWindowRect(WindowHandle hwnd, Rect* rect) = 0;
    virtual std::string getWindowText(WindowHandle hwnd) = 0;

    // Window manipulation
    virtual bool setForegroundWindow(WindowHandle hwnd) = 0;
    virtual bool moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
    virtual bool showWindow(WindowHandle hwnd, int cmdShow) = 0;

    // Cursor
    virtual void getCursorPos(Point* pt) = 0;
    virtual void setCursorPos(const Point& pt) = 0;
};

} // namespace yamy::platform
```

**Tasks:**
- [ ] Create `window_system_interface.h` with abstract interface
- [ ] Create `input_injector_interface.h` with abstract interface
- [ ] Create `input_hook_interface.h` with abstract interface
- [ ] Create `input_driver_interface.h` with abstract interface
- [ ] Define platform-agnostic types (WindowHandle, Key codes, etc.)

#### 4.2.2: Implement Interfaces in PAL

**Tasks:**
- [ ] `src/platform/windows/window_system_win32.h` - inherit from IWindowSystem
- [ ] `src/platform/windows/window_system_win32.cpp` - implement interface
- [ ] Update existing implementation to match interface
- [ ] Repeat for InputInjector, InputHook, InputDriver
- [ ] Update Engine to use interfaces instead of concrete types

**Acceptance Criteria:**
- [ ] Core code uses interface types (`IWindowSystem*`)
- [ ] Platform-specific code implements interfaces
- [ ] No Win32 types in interface headers
- [ ] Compiles and works correctly

---

### 4.3: Audit Core for Win32 Type Leakage

**Priority:** P2
**Effort:** 4-6 hours
**Dependencies:** 4.2

#### Task Breakdown

**Step 1: Find All Win32 Types in Core**
```bash
# Common Win32 types to search for:
grep -rn "HWND\|DWORD\|WPARAM\|LPARAM\|UINT\|HDC\|HMENU\|HINSTANCE" \
    src/core/ --include="*.h" --include="*.cpp"
```

**Step 2: For Each Type Found:**
- [ ] Determine if it's in a header (leaking to other files)
- [ ] If in header: replace with platform-agnostic type
- [ ] If in implementation: consider if it can use PAL instead
- [ ] Document any intentional exceptions (if any)

**Common Replacements:**
- `HWND` → `yamy::platform::WindowHandle`
- `DWORD` → `uint32_t`
- `WPARAM` → `uintptr_t`
- `LPARAM` → `intptr_t`
- `UINT` → `uint32_t`
- `RECT` → `yamy::platform::Rect`
- `POINT` → `yamy::platform::Point`

**Step 3: Update Type Usage**
- [ ] Replace Win32 types with platform-agnostic equivalents
- [ ] Update function signatures
- [ ] Fix compilation errors
- [ ] Test functionality

**Acceptance Criteria:**
- [ ] No Win32 types in `src/core/` headers (except necessary internal impl)
- [ ] Core code compiles without `#include <windows.h>`
- [ ] All tests pass

---

### 4.4: Create Linux Stub Backend

**Priority:** P2
**Effort:** 12-16 hours
**Dependencies:** 4.2, 4.3

#### Overview
Create a minimal Linux implementation to validate architectural decoupling.

#### Directory Structure
```
src/platform/linux/
├── window_system_linux.h
├── window_system_linux.cpp
├── input_injector_linux.h
├── input_injector_linux.cpp
├── input_hook_linux.h
├── input_hook_linux.cpp
├── input_driver_linux.h
├── input_driver_linux.cpp
├── thread_linux.cpp (already created in 4.1)
└── utf_conversion_linux.cpp (trivial - UTF-8 native)
```

#### 4.4.1: Implement Stub Classes

**Tasks:**

**Window System Stub:**
```cpp
// src/platform/linux/window_system_linux.cpp
#include "core/platform/window_system_interface.h"
#include <iostream>

namespace yamy::platform {

class WindowSystemLinux : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override {
        std::cerr << "STUB: getForegroundWindow" << std::endl;
        return nullptr;
    }

    WindowHandle windowFromPoint(const Point& pt) override {
        std::cerr << "STUB: windowFromPoint" << std::endl;
        return nullptr;
    }

    // ... implement all interface methods as stubs
};

// Factory function
IWindowSystem* createWindowSystem() {
    return new WindowSystemLinux();
}

} // namespace
```

**Tasks:**
- [ ] Implement `WindowSystemLinux` stub class
- [ ] Implement `InputInjectorLinux` stub class
- [ ] Implement `InputHookLinux` stub class (no-op)
- [ ] Implement `InputDriverLinux` stub class
- [ ] Each stub should log method calls to stderr
- [ ] Return safe defaults (nullptr, false, empty strings)

#### 4.4.2: Linux Build Configuration

**Update CMakeLists.txt:**
```cmake
# Platform-specific sources
if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows/window_system_win32.cpp
        src/platform/windows/input_injector_win32.cpp
        src/platform/windows/input_hook_win32.cpp
        src/platform/windows/input_driver_win32.cpp
        src/platform/windows/thread_win32.cpp
        src/platform/windows/utf_conversion.cpp
        src/platform/windows/registry.cpp
        src/platform/windows/windowstool.cpp
    )
elseif(UNIX)
    set(PLATFORM_SOURCES
        src/platform/linux/window_system_linux.cpp
        src/platform/linux/input_injector_linux.cpp
        src/platform/linux/input_hook_linux.cpp
        src/platform/linux/input_driver_linux.cpp
        src/platform/linux/thread_linux.cpp
    )
endif()

add_executable(yamy_engine ${CORE_SOURCES} ${PLATFORM_SOURCES})
```

**Tasks:**
- [ ] Update CMakeLists.txt with platform-specific source selection
- [ ] Create Linux toolchain file (optional, for native Linux build)
- [ ] Add conditional compilation for platform-specific features
- [ ] Ensure core code compiles on Linux

#### 4.4.3: Linux Build Testing

**Tasks:**
- [ ] Set up Linux build environment (Docker or VM)
- [ ] Attempt Linux build: `cmake -S . -B build/linux && cmake --build build/linux`
- [ ] Fix compilation errors (missing headers, wrong types, etc.)
- [ ] Verify executable runs (even if it does nothing useful)
- [ ] Document Linux build process in README

**Acceptance Criteria:**
- [ ] Code compiles on Linux without errors
- [ ] Executable runs without crashing
- [ ] No Windows-specific headers included in core
- [ ] Stub implementation logs calls correctly

---

### 4.5: CI Integration for Cross-Platform Validation

**Priority:** P3
**Effort:** 4-8 hours
**Dependencies:** 4.4

#### Tasks

**Create GitHub Actions Workflow:**
```yaml
# .github/workflows/build.yml
name: Build

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Configure
        run: cmake -S . -B build
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        run: ctest --test-dir build -C Release

  build-linux-stub:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: sudo apt-get install -y build-essential cmake
      - name: Configure
        run: cmake -S . -B build
      - name: Build
        run: cmake --build build
      - name: Verify binary
        run: file build/yamy_engine
```

**Tasks:**
- [ ] Create `.github/workflows/build.yml`
- [ ] Configure Windows build job
- [ ] Configure Linux stub build job
- [ ] Run unit tests in CI
- [ ] Add build status badge to README

**Acceptance Criteria:**
- [ ] CI builds Windows binaries
- [ ] CI builds Linux stub
- [ ] Build failures caught automatically
- [ ] Tests run in CI

---

## Migration Timeline & Prioritization

### Week 1-2: UTF Conversion Foundation (Phase 3.1-3.2)
- **Days 1-2:** Create UTF conversion infrastructure (3.1)
- **Days 3-7:** Migrate Engine, Keymap, Settings (3.2)
- **Days 8-10:** Buffer/testing

**Deliverable:** Core data structures use UTF-8

### Week 3-4: Command & UI Migration (Phase 3.3-3.4)
- **Days 1-5:** Migrate Function/Command system (3.3)
- **Days 6-10:** Migrate UI layer (3.4)

**Deliverable:** Commands and UI work with UTF-8

### Week 5: Platform Utils & Cleanup (Phase 3.5-3.6)
- **Days 1-3:** Migrate platform utilities (3.5)
- **Days 4-5:** Remove legacy types (3.6)

**Deliverable:** Phase 3 complete, zero tstring usage

### Week 6: PAL Completion (Phase 4.1-4.3)
- **Days 1-2:** Remove windows.h from core (4.1)
- **Days 3-5:** Create interface layer (4.2)
- **Days 6-7:** Audit for Win32 leakage (4.3)

**Deliverable:** Core is platform-agnostic

### Week 7: Linux Stub (Phase 4.4)
- **Days 1-3:** Implement stub classes
- **Days 4-5:** Configure Linux build
- **Days 6-7:** Test and debug

**Deliverable:** Compiles on Linux

### Week 8: CI & Documentation (Phase 4.5 + Polish)
- **Days 1-3:** Set up CI
- **Days 4-5:** Update documentation
- **Days 6-7:** Final testing & bug fixes

**Deliverable:** Full migration complete

---

## Testing Strategy

### Unit Tests
- [ ] UTF conversion function tests (round-trip, edge cases)
- [ ] String handling tests for each migrated component
- [ ] Platform interface tests (Windows & Linux)

### Integration Tests
- [ ] Load configuration file with Unicode characters
- [ ] Execute commands with string arguments
- [ ] Display UI with non-ASCII text
- [ ] Registry operations with Unicode paths

### Regression Tests
- [ ] All existing tests must continue to pass
- [ ] Manual testing of key workflows
- [ ] Performance testing (ensure no significant slowdown)

### Cross-Platform Validation
- [ ] Windows build + full test suite
- [ ] Linux build (smoke test that it compiles and runs)

---

## Risk Mitigation

### High Risk Items
1. **Breaking Configuration Files**
   - Mitigation: Test with real .mayu files, ensure backwards compatibility

2. **UI Display Issues (Mojibake)**
   - Mitigation: Thorough testing with Japanese/Unicode text
   - Keep old tstring version in git history for easy rollback

3. **Performance Regression**
   - Mitigation: Profile before/after, benchmark key operations
   - UTF-8 to UTF-16 conversion overhead should be minimal at API boundary

### Medium Risk Items
1. **Windows API Integration**
   - Mitigation: Use proven UTF conversion approach (MultiByteToWideChar)

2. **Registry Key Encoding**
   - Mitigation: Test thoroughly with registry operations

### Rollback Plan
- Each phase should be completable independently
- Commit after each major milestone
- Tag known-good states
- Document any breaking changes

---

## Definition of Done

### Phase 3 Complete
- [ ] Zero `grep -r "tstring"` results in src/
- [ ] Zero `grep -r "_T("` results in src/
- [ ] Zero `grep -r "_TCHAR"` results in src/
- [ ] All unit tests pass
- [ ] Application runs and loads configuration
- [ ] UI displays correctly (including Unicode)

### Phase 4 Complete
- [ ] Zero `#include <windows.h>` in src/core/
- [ ] Zero Win32 types in src/core/ public headers
- [ ] Linux stub builds successfully
- [ ] CI validates both Windows and Linux builds
- [ ] Documentation updated

### Overall Success Criteria
- [ ] 100% UTF-8 internal representation
- [ ] Platform-agnostic core architecture
- [ ] Linux build demonstrates decoupling
- [ ] All functionality preserved
- [ ] Code is more maintainable than before

---

## Appendix

### Useful Commands

**Find String Type Usage:**
```bash
# Count tstring usage
grep -r "tstring" src/ --include="*.cpp" --include="*.h" | wc -l

# Find _T() macros
grep -rn "_T(" src/ --include="*.cpp"

# Find TCHAR usage
grep -rn "_TCHAR\|TCHAR" src/ --include="*.h"
```

**Find Win32 Types:**
```bash
# Find Windows-specific types in core
grep -rn "HWND\|DWORD\|WPARAM" src/core/ --include="*.h"

# Find windows.h includes
grep -rn "#include <windows.h>" src/core/
```

**Verify Clean State:**
```bash
# Should all return 0:
grep -r "tstring" src/ --include="*.cpp" --include="*.h" | wc -l
grep -r "_T(" src/ --include="*.cpp" | wc -l
grep -r "#include <windows.h>" src/core/ | wc -l
```

### References
- [UTF-8 Everywhere Manifesto](http://utf8everywhere.org/)
- [Windows UTF-8 Code Page Support](https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page)
- [Platform Abstraction Layer Design Patterns](https://en.wikipedia.org/wiki/Hardware_abstraction)
