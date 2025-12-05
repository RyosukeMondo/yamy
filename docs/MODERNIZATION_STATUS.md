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

### 3. Testing Infrastructure
*   **Google Test Integration:** Added `gtest` and created a test suite structure in `src/tests/`.
*   **Coverage:** Achieved passing tests for:
    *   `Regex` (via `src/tests/test_regex.cpp`)
    *   `LayoutManager`
    *   `SettingLoader` (basic parsing logic)

### 4. Component Decoupling
*   **StrExpr Subsystem Extracted:**
    *   Extracted `StrExpr`, `StrExprArg`, and subclasses (`StrExprClipboard`, `StrExprWindowClassName`, `StrExprWindowTitleName`) from `src/core/function.cpp` to `src/core/strexpr.h` and `src/core/strexpr.cpp`.
    *   Replaced raw pointers with `std::unique_ptr` for better memory management.
    *   Introduced `StrExprSystem` interface to abstract system dependencies (clipboard, window info).
    *   Implemented `StrExprSystem` in `Engine` (via `src/core/engine_window.cpp`).
    *   Standardized clipboard access using `clipboardGetText` and `clipboardClose` in `src/system/windowstool.cpp`.

## Architectural Analysis (Current State)

### Issues
1.  **Engine Coupling:** The `Engine` class is a monolithic entity. Although `StrExpr` is decoupled via `StrExprSystem`, `Engine` still implements it directly.
2.  **Threading:** Uses custom wrappers (`CriticalSection`, `Acquire`) around Windows primitives instead of standard C++ threading (`std::mutex`).
3.  **File Structure:** `src/core/setting.cpp` is extremely large and contains mixed concerns.

## Roadmap

### Phase 1: Component Decoupling (Immediate Priority)
The goal is to break the dependency on the monolithic `Engine` and Global State.

*   **Threading Modernization:**
    *   **Action:** Replace `src/utils/multithread.h` classes with `std::recursive_mutex` and `std::lock_guard`.
    *   **Improvement:** Standardize concurrency.

### Phase 2: Engine Decomposition
*   **InputHandler:** Extract input thread management.
*   **EventGenerator:** Separate event generation logic from state management.

### Phase 3: Setting Loader Modernization
*   **Split SettingLoader:** Break down `setting.cpp` into smaller, focused parsers.

## Session Log (Recent Changes)
*   **2023-12-05**:
    *   Extracted `StrExpr` subsystem from `function.cpp` to `strexpr.h/cpp`.
    *   Refactored `StrExpr` to use `std::unique_ptr`.
    *   Created `StrExprSystem` interface and implemented it in `Engine`.
    *   Moved clipboard logic to `windowstool.cpp`.
    *   Verified build with `scripts/build_yamy.bat`.
    *   Implemented `tregex` wrapper for `std::regex`.
    *   Fixed `funcWindowHVMaximize` implementation in `function.cpp`.
    *   Restored missing functions: `funcWindowMoveTo`, `funcWindowMove`, `funcWindowHMaximize`, `funcWindowVMaximize`.
    *   Verified all 42 tests passed.
