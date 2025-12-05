# Yamy C++ Modernization Status

## Overview
This document tracks the ongoing modernization of the legacy C++ codebase (Yamy).
The primary goal is to improve maintainability, testability, and remove legacy dependencies (like Boost) to prepare the application for long-term stability or future architectural shifts.

**Note:** This document focuses on the tactical C++ refactoring. For the long-term strategic vision (e.g., Rust rewrite), refer to `ARCHITECTURE_FUTURE.md`.

## Completed Achievements (Phase 0)

### 1. Dependency Removal
*   **Boost.Regex Removed:** Successfully replaced the external Boost.Regex dependency with the standard C++11 `std::regex`.
    *   Implemented `tregex` wrapper in `src/utils/stringtool.h` to maintain compatibility with `_TCHAR` and provide missing functionality (like `str()` on match results).
    *   Updated all call sites in `keymap.cpp`, `setting.cpp`, `function.cpp`, `registry.cpp`, `mayu.cpp`, `dlgsetting.cpp`.

### 2. Logic Extraction & Refactoring
*   **LayoutManager:** Decoupled window layout calculation logic from GUI code. Created `src/core/layoutmanager.h` and added unit tests.
*   **Parser:** Refactored `Parser::getLine` to remove complex `goto` statements, improving readability.
*   **Window Management:** Restored missing window movement functions (`funcWindowMoveTo`, etc.) in `src/core/function.cpp` and fixed broken maximization logic.
*   **KeySeq:** Refactored `KeySeq` to use `std::vector<std::unique_ptr<Action>>` instead of raw pointers, implementing deep copy logic.
*   **Keyboard/Key:** Replaced C-style arrays with `std::vector` and `std::array`.

### 3. Engine Decomposition (Completed)
*   **Monolithic Split:** Refactored the massive `src/core/engine.cpp` into 10 files adhering to the Single Responsibility Principle (SRP):
    *   `engine.cpp` (Main Loop), `engine_lifecycle.cpp`, `engine_hook.cpp` (OS Hooks)
    *   `engine_input.cpp` (Injection), `engine_modifier.cpp`, `engine_generator.cpp` (Events)
    *   `engine_focus.cpp`, `engine_window.cpp`, `engine_setting.cpp`, `engine_log.cpp`.
*   **Bug Fixes:** Resolved stuck modifier key issues by separating mouse event processing in the queue.

### 4. Project Structure & Infrastructure
*   **Directory Reorganization:** Restructured `src/` into semantic subdirectories:
    *   `core/` (Engine logic), `ui/` (Dialogs), `system/` (Hooks, Driver), `utils/` (Helpers), `app/` (Entry points).
    *   Moved root folders: `driver/`, `keymaps/`, `resources/`, `setup/`, `docs/`.
*   **Build System:**
    *   **VS2022/v143:** Standardized on Visual Studio 2022 (Toolset v143).
    *   **GitHub Actions:** Created `release.yml` for automated building and releases on `windows-latest`.
    *   **Scripts:** Created `scripts/build_yamy.bat` and `scripts/setup_tests.ps1` for reproducible builds.

### 5. Testing Infrastructure
*   **Google Test Integration:** Migrated from `doctest` to `gtest` 1.14.0.
*   **Coverage Expanded:**
    *   `Regex` (`test_regex.cpp`)
    *   `LayoutManager` (`test_layoutmanager.cpp`)
    *   `SettingLoader` (`test_setting.cpp` - definitions, conditionals, modifiers)
    *   `FunctionData` (`test_function.cpp` - serialization, cloning)
    *   `Keyboard` (`test_keyboard.cpp` - scan codes, aliasing)
    *   `Array/Misc` (`test_misc.cpp`)

### 6. Component Decoupling
*   **StrExpr Subsystem Extracted:**
    *   Extracted `StrExpr` and subclasses from `function.cpp` to `strexpr.h/cpp`.
    *   Replaced raw pointers with `std::unique_ptr` for better memory management.
    *   Introduced `StrExprSystem` interface to abstract system dependencies.

### 7. Modern Concurrency (Completed)
*   **CriticalSection Replacement:** Replaced Windows `CRITICAL_SECTION` in `src/utils/multithread.h` with C++11 `std::recursive_mutex`.
    *   Maintained `SyncObject` interface for backward compatibility.
    *   Removed `<windows.h>` dependency from `multithread.h`.
    *   Verified with all 42 tests passing.

## Architectural Analysis (Current State)

### Issues
1.  **Setting Loader:** `src/core/setting.cpp` is still large and contains mixed concerns, though testing has improved.
2.  **Driver:** The kernel driver (`mayud.sys`) remains a legacy component that needs review for modern Windows security compliance.

## Roadmap

### Phase 1: Setting Loader De-spaghettification (Next Priority)
*   **Action:** Break down `setting.cpp` into smaller, focused parsers (e.g., `KeymapParser`, `DefParser`).

### Phase 2: Platform Abstraction Layer (PAL) - Long Term
*   **Goal:** Enable swapping the OS layer for cross-platform support (Linux/macOS).
*   **Input Driver Interface:** Abstract the communication with the kernel driver.
*   **Config Abstraction:** Replace direct Registry usage with an abstract `SettingsStore`.

## Session Log (Recent Changes)
*   **2023-12-05**:
    *   **Concurrency:** Replaced `CRITICAL_SECTION` with `std::recursive_mutex` in `multithread.h`. Verified 42 tests pass.
    *   **Refactoring:** Refactored `engine.cpp` into 10 separate files.
    *   **Structure:** Reorganized entire project directory structure.
    *   **CI:** Established GitHub Actions CI pipeline.
    *   **Modifiers:** Expanded modifiers support to Mod10-Mod19.
    *   **Testing:** Migrated all tests to Google Test and expanded coverage significantly.
    *   **Bug Fix:** Fixed stuck modifier key bug (`0x59414D59` tag).
