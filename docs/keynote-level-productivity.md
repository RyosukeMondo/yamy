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

### Phase 1: The Build Foundation (Immediate Impact)
**Goal:** Unified, declarative build system.
-   [x] **Migrate to CMake:** Created root `CMakeLists.txt`, replaced `.vcxproj`.
-   [x] **Automate Dependencies:** GTest is integrated; packaging script handles artifacts.
-   [x] **Cross-Platform Readiness:** CMake build system is now the standard.
-   [x] **Packaging & Distribution:** Automated via `scripts/cmake_package.ps1` (Zip, Multi-arch).

### Phase 2: Refactor the "Boilerplate Factory" (High Velocity)
**Goal:** Add a new command in 3 lines of code, not 50.
-   [x] **Analyze `FunctionData`:** `src/core/functions/function_data.h` contains massive boilerplate for every command.
-   [x] **Metaprogramming/Templates:** Implemented `Command<Derived, Args...>` template system (C++17) to generate boilerplates automatically.
-   [ ] **Decouple Implementation:** Move command logic out of the monolithic `function.cpp` into smaller, cohesive units (e.g., `src/core/commands/`). (Migrated `Default`, `KeymapPrevPrefix`, `KeymapParent`, `KeymapWindow`, `OtherWindowClass`, `Prefix`, `Keymap`, `Sync`)

### Phase 3: String Unification (Cognitive Load Reduction)
**Goal:** `std::string` (UTF-8) everywhere.
-   [ ] **Internal Standard:** Adopt `std::string` as the internal string type.
-   [ ] **The "Edge" Strategy:** Only convert to `std::wstring` (UTF-16) at the Windows API boundary (PAL).
-   [ ] **Remove `TCHAR`:** Eliminate `_T()`, `tstring`, and `tregex`. This removes the constant mental overhead of "Wide vs Narrow" and prepares the code for Linux (which uses UTF-8).

### Phase 4: Platform Abstraction Layer (PAL) Completion
**Goal:** Zero `#include <windows.h>` in `src/core`.
-   [ ] **Audit Core:** Ensure no Win32 types (`HWND`, `DWORD`, `MSG`) leak into `src/core`.
-   [ ] **Input/Window Abstraction:** Continue the work on `WindowSystem` and `InputInjector`.
-   [ ] **Linux Stub:** Implement a "Headless" or "Stub" Linux backend to verify architectural decoupling in CI.

## Evaluation of Past Modernization Attempts
(To be populated with analysis of recent commits)
-   **Successes:** Removal of Boost, initial PAL work.
-   **Challenges:** "Pragma/Macro Mess" in feature branches often results from trying to support multiple platforms *inside* the same file (e.g., `#ifdef _WIN32`).
-   **Correction:** Use **File-Based Polymorphism** (separate files for `window_system_win32.cpp` vs `window_system_linux.cpp`) coupled with CMake configuration, rather than `#ifdef` soup.

## Tips for Future Self

### 1. CMake Source Management
When refactoring or adding new files/targets:
-   **Explicitly Add Sources:** When creating a new target (e.g., `yamy_engine_new`), simply including headers (`#include`) is not enough. You *must* add the corresponding `.cpp` files to the `add_executable` or `add_library` command in `CMakeLists.txt` to avoid linker errors.
-   **Check Linker Errors:** If you see `LNK2019` or `LNK2001` (unresolved external symbol), 99% of the time you forgot to add a `.cpp` file to `CMakeLists.txt`.

### 2. Header Include Discipline
-   **Verify Relative Paths:** When moving files (e.g., from `src/core/functions` to `src/core/commands`), relative paths like `../driver/driver.h` may break. Always verify the new relative path (e.g., `../../platform/windows/driver.h`) or consider adding include directories to CMake to use cleaner paths.
-   **Locate Headers Correctly:** Don't assume file locations. Use `dir` or `find` to confirm where a header lives before guessing its path.

