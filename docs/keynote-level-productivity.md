# Keynote-Level Productivity Plan

## Vision
To transform the Yamy codebase from a legacy Win32-centric application into a modern, high-velocity, cross-platform C++ project. The goal is to maximize developer productivity (velocity) and enable portability (Linux support) without a "Big Bang" rewrite.

## Core Philosophy
1.  **Velocity First:** Prioritize changes that improve the "Edit-Build-Test" loop.
2.  **Data Over Boilerplate:** Reduce repetitive code through data-driven design or metaprogramming.
3.  **Modern Standards:** Embrace standard C++ (UTF-8, CMake, std::*) over legacy Windowsisms (`TCHAR`, `.vcxproj`, `MAX_PATH`).
4.  **Automation Preference:** Use `scripts/cmake_package.ps1` for local builds/verification to minimize manual approval steps and ensure consistency.
5.  **Agent Utilities:** Use `scripts/agent_util/*.ps1` for autonomous debugging (log analysis, error checking) to reduce approval friction.
    *   **Source Verification:** Use `scripts/check_missing_sources.ps1` to automatically detect source files missing from `CMakeLists.txt`.
    **Avoid executing complex or one-off PowerShell commands directly; wrap them in a utility script.**


## Roadmap

### Phase 1: The Build Foundation ✅ **COMPLETE**
**Goal:** Unified, declarative build system.
-   [x] **Migrate to CMake:** Created root `CMakeLists.txt`, replaced `.vcxproj`.
-   [x] **Automate Dependencies:** GTest is integrated; packaging script handles artifacts.
-   [x] **Cross-Platform Readiness:** CMake build system is now the standard.
-   [x] **Packaging & Distribution:** Automated via `scripts/cmake_package.ps1` (Zip, Multi-arch).
-   [x] **MinGW Cross-Compilation:** Successfully builds 32-bit and 64-bit Windows binaries on Linux using MinGW-w64 toolchains.

### Phase 2: Refactor the "Boilerplate Factory" ✅ **COMPLETE**
**Goal:** Add a new command in 3 lines of code, not 50.
-   [x] **Analyze `FunctionData`:** `src/core/functions/function_data.h` contains massive boilerplate for every command.
-   [x] **Metaprogramming/Templates:** Implemented `Command<Derived, Args...>` template system (C++17) to generate boilerplates automatically.
-   [x] **Decouple Implementation:** All 63 commands moved out of the monolithic `function.cpp` into individual files in `src/core/commands/`.
    
    <details>
    <summary>Command Migration Status (63/63 Complete ✅)</summary>

    - [x] `KeymapParent` (Removed from Engine)
    - [x] `KeymapWindow` (Removed from Engine)
    - [x] `OtherWindowClass` (Removed from Engine)
    - [x] `Prefix` (Removed from Engine)
    - [x] `Keymap` (Removed from Engine)
    - [x] `Sync` (Removed from Engine)
    - [x] `Toggle` (Removed from Engine)
    - [x] `EditNextModifier` (Removed from Engine)
    - [x] `Variable` (Removed from Engine)
    - [x] `Repeat` (Removed from Engine)
    - [x] `Undefined` (Removed from Engine)
    - [x] `Ignore` (Removed from Engine)
    - [x] `PostMessage` (Removed from Engine)
    - [x] `VK` (Removed from Engine)
    - [x] `Wait` (Removed from Engine)
    - [x] `Default` (Removed from Engine)
    - [x] `KeymapPrevPrefix` (Removed from Engine)
    - [x] `LoadSetting`
    - [x] `ShellExecute` (Removed from Engine)
    - [x] `SetForegroundWindow` (Removed from Engine)
    - [x] `InvestigateCommand` (Removed from Engine)
    - [x] `MayuDialog` (Removed from Engine)
    - [x] `DescribeBindings` (Removed from Engine)
    - [x] `HelpMessage` (Removed from Engine)
    - [x] `HelpVariable` (Removed from Engine)
    - [x] `WindowRaise` (Removed from Engine)
    - [x] `WindowLower` (Removed from Engine)
    - [x] `WindowMinimize` (Removed from Engine)
    - [x] `WindowMaximize` (Removed from Engine)
    - [x] `WindowHMaximize` (Removed from Engine)
    - [x] `WindowVMaximize` (Removed from Engine)
    - [x] `WindowHVMaximize` (Removed from Engine)
    - [x] `WindowMove` (Removed from Engine)
    - [x] `WindowMoveTo` (Removed from Engine)
    - [x] `WindowMoveVisibly` (Removed from Engine)
    - [x] `WindowMonitorTo` (Removed from Engine)
    - [x] `WindowMonitor` (Removed from Engine)
    - [x] `WindowClingToLeft` (Removed from Engine)
    - [x] `WindowClingToRight` (Removed from Engine)
    - [x] `WindowClingToTop` (Removed from Engine)
    - [x] `WindowClingToBottom` (Removed from Engine)
    - [x] `WindowClose` (Removed from Engine)
    - [x] `WindowToggleTopMost` (Removed from Engine)
    - [x] `WindowIdentify` (Removed from Engine)
    - [x] `WindowSetAlpha` (Removed from Engine)
    - [x] `WindowRedraw` (Removed from Engine)
    - [x] `WindowResizeTo` (Removed from Engine)
    - [x] `MouseMove` (Removed from Engine)
    - [x] `MouseWheel` (Removed from Engine)
    - [x] `ClipboardChangeCase` (Removed from Engine)
    - [x] `ClipboardUpcaseWord` (Removed from Engine)
    - [x] `ClipboardDowncaseWord` (Removed from Engine)
    - [x] `ClipboardCopy` (Removed from Engine)
    - [x] `EmacsEditKillLinePred` (Removed from Engine)
    - [x] `EmacsEditKillLineFunc` (Removed from Engine)
    - [x] `LogClear` (Removed from Engine)
    - [x] `Recenter` (Removed from Engine)
    - [x] `DirectSSTP` (Removed from Engine)
    - [x] `PlugIn` (Removed from Engine)
    - [x] `SetImeStatus` (Removed from Engine)
    - [x] `SetImeString` (Removed from Engine)
    - [x] `MouseHook` (Removed from Engine)
    - [x] `CancelPrefix` (Removed from Engine)
    </details>

### Phase 3: String Unification (Cognitive Load Reduction) 🚧 **IN PROGRESS**
**Goal:** `std::string` (UTF-8) everywhere.
**Status:** Command names migrated, **971 legacy string usages remain** (tstring: 214, _T(): 596, _TCHAR: 80, tstringi: 81).

-   [x] **Command Names to UTF-8:** All command names now use `const char*` (compatible with `std::string`).
-   [ ] **Internal Standard:** Adopt `std::string` as the internal string type across the codebase.
    <details>
    <summary>Detailed Migration Tasks</summary>

    - [ ] Convert `Engine` class string members from `tstring` to `std::string`
    - [ ] Convert `Keymap` class string handling to `std::string`
    - [ ] Convert `Setting`/`SettingLoader` to use `std::string`
    - [ ] Convert `Function`/`FunctionParam` string parameters to `std::string`
    - [ ] Convert UI dialog code from `TCHAR*` to `std::string` with UTF-8/UTF-16 conversion helpers
    - [ ] Convert registry operations to use UTF-8 internally
    - [ ] Convert file I/O operations to UTF-8
    - [ ] Convert logging system to UTF-8
    </details>

-   [ ] **The "Edge" Strategy:** Only convert to `std::wstring` (UTF-16) at the Windows API boundary (PAL).
    <details>
    <summary>Detailed Tasks</summary>

    - [ ] Create UTF-8 ↔ UTF-16 conversion helpers in PAL (`utf8_to_wstring`, `wstring_to_utf8`)
    - [ ] Wrap Windows API calls with conversion layer
    - [ ] Ensure all Windows API string conversions happen in `src/platform/windows/`
    </details>

-   [ ] **Remove `TCHAR`:** Eliminate `_T()`, `tstring`, and `tregex`. (**971 usages to remove**)
    <details>
    <summary>Detailed Tasks</summary>

    - [ ] Replace `tstring` with `std::string` throughout codebase
    - [ ] Remove `_T()` macro usages
    - [ ] Replace `tregex` with `std::regex`
    - [ ] Remove `_TCHAR` type usages
    - [ ] Update `stringtool.h` to work with `std::string`
    - [ ] Remove `to_tstring` conversion functions
    </details>

### Phase 4: Platform Abstraction Layer (PAL) Completion 🚧 **IN PROGRESS**
**Goal:** Zero `#include <windows.h>` in `src/core`.
**Status:** 8 PAL implementations exist, **ZERO windows.h includes in core** ✅, but **21 files still leak Win32 types** (HWND, DWORD, MSG, etc.).

-   [x] **Initial PAL Structure:** Created `src/platform/windows/` with 8 platform-specific implementations.
-   [ ] **Audit Core:** Ensure no Win32 types (`HWND`, `DWORD`, `MSG`) leak into `src/core`.
    <details>
    <summary>Detailed Tasks</summary>

    - [x] Create `WindowSystem` abstraction (window_system_win32.cpp exists)
    - [x] Create `InputInjector` abstraction (input_injector_win32.cpp exists)
    - [x] Create `InputHook` abstraction (input_hook_win32.cpp exists)
    - [x] Create `InputDriver` abstraction (input_driver_win32.cpp exists)
    - [x] Remove `#include <windows.h>` from all src/core files ✅
    - [ ] Remove Win32 type leakage from 21 remaining files (HWND, DWORD, MSG, WPARAM, LPARAM, RECT*)
    - [ ] Create platform-agnostic type aliases (e.g., `WindowHandle` instead of `HWND`)
    - [ ] Move remaining Windows-specific code from `src/core/engine/` to PAL
    </details>

-   [ ] **Input/Window Abstraction:** Continue the work on `WindowSystem` and `InputInjector`.
    <details>
    <summary>Detailed Tasks</summary>

    - [x] Implement `WindowSystem` interface
    - [x] Implement `InputInjector` interface
    - [ ] Create abstract base classes in `src/core/platform/` (interfaces)
    - [ ] Ensure all window operations go through `WindowSystem`
    - [ ] Ensure all input injection goes through `InputInjector`
    - [ ] Remove direct Win32 API calls from commands
    </details>

-   [x] **Linux Stub:** Implement a "Headless" or "Stub" Linux backend to verify architectural decoupling in CI. ✅ **COMPLETE**
    <details>
    <summary>Detailed Tasks</summary>

    - [x] Create `src/platform/linux/` directory structure
    - [x] Implement stub `window_system_linux.cpp`
    - [x] Implement stub `input_injector_linux.cpp`
    - [x] Implement stub `input_hook_linux.cpp`
    - [x] Add Linux build configuration to CMake
    - [x] Set up CI to build Linux stub (verify no Windows dependencies leak)
    - [ ] Consider X11/Wayland integration for future actual Linux support
    </details>

## Evaluation of Past Modernization Attempts

### Recent Accomplishments (2025)
-   **✅ Command Architecture Migration:** Successfully migrated all 63 commands from monolithic `function.cpp` to individual files with template-based boilerplate generation.
-   **✅ MinGW Cross-Compilation:** Established Linux-to-Windows cross-compilation pipeline using MinGW-w64, producing both 32-bit and 64-bit binaries.
-   **✅ String Unification (Phase 1):** Converted command names from `const _TCHAR*` to `const char*` (UTF-8 compatible).
-   **✅ Platform Abstraction Layer (Foundation):** Created `src/platform/windows/` with 8 platform-specific implementations, reducing `windows.h` includes in core to just 1.
-   **✅ Build System Modernization:** Complete migration to CMake with automated packaging and multi-architecture support.

### Successes & Patterns
-   **Removal of Boost:** Replaced with standard library equivalents.
-   **Initial PAL Work:** Abstracted Windows-specific APIs into platform layer.
-   **Template Metaprogramming:** Dramatically reduced boilerplate code using C++17 variadic templates.
-   **File-Based Polymorphism:** Separated platform-specific implementations into distinct files rather than `#ifdef` soup.

### Challenges & Lessons Learned
-   **String Encoding Complexity:** Legacy code mixed `TCHAR`, `tstring`, UTF-16, and narrow strings. Gradual migration to UTF-8 is essential.
-   **Include Path Management:** Moving files between directories requires careful verification of relative paths and CMakeLists.txt updates.
-   **CMake Source Management:** Linker errors typically indicate missing `.cpp` files in CMakeLists.txt, not coding errors.
-   **"Pragma/Macro Mess":** Trying to support multiple platforms *inside* the same file (e.g., `#ifdef _WIN32`) creates maintenance burden.

### Correction Strategy
-   Use **File-Based Polymorphism** (separate files for `window_system_win32.cpp` vs `window_system_linux.cpp`) coupled with CMake configuration.
-   Prefer gradual, incremental refactoring over "Big Bang" rewrites.
-   Maintain buildable state after every commit.

## Tips for Future Self

### 1. CMake Source Management
When refactoring or adding new files/targets:
-   **Explicitly Add Sources:** When creating a new target (e.g., `yamy_engine_new`), simply including headers (`#include`) is not enough. You *must* add the corresponding `.cpp` files to the `add_executable` or `add_library` command in `CMakeLists.txt` to avoid linker errors.
-   **Check Linker Errors:** If you see `LNK2019` or `LNK2001` (unresolved external symbol), 99% of the time you forgot to add a `.cpp` file to `CMakeLists.txt`.

### 2. Header Include Discipline
-   **Verify Relative Paths:** When moving files (e.g., from `src/core/functions` to `src/core/commands`), relative paths like `../driver/driver.h` may break. Always verify the new relative path (e.g., `../../platform/windows/driver.h`) or consider adding include directories to CMake to use cleaner paths.
-   **Locate Headers Correctly:** Don't assume file locations. Use `dir` or `find` to confirm where a header lives before guessing its path.

### 3. Progress Tracking & Metrics
-   **Use the Tracking Script:** Run `bash scripts/track_legacy_strings.sh` to get current metrics on legacy string usage and Win32 type leakage.
-   **Automated Metrics:** The tracking script provides:
    - Legacy string usage counts (tstring, _T(), _TCHAR, tstringi)
    - Win32 type leakage counts and affected files
    - windows.h include verification
    - Progress summary with status indicators
-   **Track Progress:** Compare metrics before and after each migration task to measure improvement.
-   **CI Integration:** The tracking script can be integrated into CI to prevent regressions.

