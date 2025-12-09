# Branch 1: UTF Conversion Infrastructure - Takeover

**Status:** Implementation Complete, Verification In Progress (Blocked by Build Config)
**Branch:** `feat/utf-conversion-infrastructure`

## Overview
This branch implements the UTF-8 <-> UTF-16 conversion infrastructure required for the migration of the engine from `tstring` (Unicode/MBCS agnostic) to `std::string` (UTF-8). It also introduces a `yamy_unit_test` target for running isolated unit tests on Linux/Cross-platform environments.

## Changes Implemented

### 1. UTF Conversion Logic
*   **File:** `src/platform/windows/utf_conversion.h`
    *   Declared `utf8_to_wstring` and `wstring_to_utf8`.
*   **File:** `src/platform/windows/utf_conversion.cpp`
    *   Implemented Windows version using `MultiByteToWideChar` / `WideCharToMultiByte`.
    *   Implemented **Linux/Fallback** version using `std::codecvt` and `std::wstring_convert`. This allows the logic to be tested on non-Windows platforms (like the current CI/Agent environment).

### 2. Legacy Compatibility (`tstring` Fix)
*   **File:** `src/utils/stringtool.h`
    *   Added an overload `inline std::string to_UTF_8(const std::string &i_str) { return i_str; }`.
    *   **Reason:** The existing codebase assumes `tstring` is `std::wstring` (in Unicode builds). However, on Linux/MinGW without full configuration, `tstring` might fall back to `std::string`. The `function_creator.cpp` was failing because `to_UTF_8` only accepted `wstring`. This overload makes the code robust regardless of `tstring`'s underlying type.

### 3. Testing Infrastructure
*   **File:** `src/tests/test_utf_conversion.cpp`
    *   Added Google Test cases for round-trip conversion, empty strings, null pointers, and Japanese characters.
*   **File:** `src/tests/test_main_unit.cpp`
    *   Simple main entry point for the unit test executable.
*   **File:** `CMakeLists.txt`
    *   Added `yamy_unit_test` executable target.
    *   This target compiles *only* the necessary files (`utf_conversion.cpp`, tests, gtest) and avoids linking against the heavy `yamy_hook` or Windows API dependencies that are currently hard to mock effectively.
    *   Preserved the original `yamy_test` target for full Windows builds.

## Current State & Known Issues

### Linux Build Issue
The `yamy_unit_test` target is currently failing to build on Linux due to an include path issue:
```
fatal error: platform/windows/utf_conversion.h: No such file or directory
```
This is occurring in `src/platform/windows/utf_conversion.cpp`.
*   The CMake `target_include_directories` includes `src`.
*   The include directive is `#include "platform/windows/utf_conversion.h"`.
*   The file exists at `src/platform/windows/utf_conversion.h`.

**Suspected Cause:**
The CMake configuration might need explicit `${CMAKE_SOURCE_DIR}/src` or a clean rebuild to pick up the include paths correctly.

## Next Steps for Successor

1.  **Fix Include Path:**
    *   Modify `CMakeLists.txt` to use absolute paths for include directories or verify why `src` isn't resolving correctly.
    *   Alternatively, revert `utf_conversion.cpp` to use relative include `#include "utf_conversion.h"` if that simplifies the build, though preserving the full path is better for project structure.

2.  **Verify Tests:**
    *   Once it compiles, run `./bin/yamy_unit_test` (or wherever the binary ends up).
    *   Ensure all UTF conversion tests pass.

3.  **Merge:**
    *   Once verification passes, the branch is ready to merge. Ideally, `yamy_test` (legacy) should also be checked on a real Windows machine.
