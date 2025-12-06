# Yamy C++ Modernization Status

## Overview
This document tracks the ongoing modernization of the legacy C++ codebase (Yamy).
The primary goal is to improve maintainability, testability, and remove legacy dependencies (like Boost) to prepare the application for long-term stability or future architectural shifts.

**Note:** This document focuses on the tactical C++ refactoring. For the long-term strategic vision (e.g., Rust rewrite), refer to `ARCHITECTURE_FUTURE.md`.

## Completed Achievements (Phase 0 & Phase 1)

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

### 8. Setting Loader Refactoring (Completed)
*   **Separation of Concerns:** Split `src/core/setting.cpp` into:
    *   `setting.cpp`: Contains only `Event` namespace and global utility functions (`getFilenameFromRegistry`, `getHomeDirectories`).
    *   `setting_loader.cpp`: Encapsulates the `SettingLoader` class and configuration parsing logic.
*   **Header Separation:** Ensured `SettingLoader` class definition is available via `setting_loader.h`.
*   **Build System:** Updated all project files (`.vcxproj`) to include the new files.

### 9. Functions Refactoring (Phase 1 Completed)
*   **Macro Elimination:** Eliminated `makefunc`-based code generation for `FunctionData` subclasses.
    *   Converted `functions.h` (generated) into static C++ files: `src/core/function_data.h` (classes), `src/core/function_friends.h` (friend decls), and `src/core/function_creator.cpp` (factory).
    *   Updated `function.cpp` and `engine.h` to use the new headers.
    *   Removed `makefunc.vcxproj` dependency and obsolete files.
    *   Verified build with `scripts/build_yamy.bat` (including tests).

## Phase 2: Platform Abstraction Layer (PAL) - In Progress

### 1. Config Abstraction (Completed)
*   **ConfigStore Interface:** Created `src/utils/config_store.h` to abstract configuration storage (Registry, Ini, etc.).
*   **Registry Implementation:** Updated `src/system/registry.h` to inherit from `ConfigStore`.
*   **Dependency Injection:** Updated `SettingLoader` to accept `ConfigStore` interface, removing direct dependency on `Registry` class for file loading.
*   **Refactoring:** Renamed `getFilenameFromRegistry` to `getFilenameFromConfig` and updated usage in `SettingLoader` and `Mayu` app.
*   **Engine Update:** Updated `Engine` to use `ConfigStore` interface instead of direct `Registry` usage.
*   **Mayu Update:** Updated `Mayu` application to instantiate `Registry` (as `ConfigStore`) and pass it to `Engine`.
*   **Dependency Removal:** Removed `registry.h` includes from `Core` components (`function.cpp`, `setting.cpp`, `setting_loader.cpp`).

### 2. Input Driver Interface (Started)
*   **Input Event Abstraction:** Created `src/core/input_event.h` defining `KEYBOARD_INPUT_DATA` to decouple core logic from `driver.h` (which depends on `winioctl.h`).
*   **Decoupling:** Updated `src/core/keyboard.h` and `src/system/driver.h` to use the new `input_event.h`.

### 3. WindowSystem Abstraction (Completed)
*   **WindowSystem Interface:** Created `src/core/window_system.h` to abstract window management (hierarchy, state) and clipboard operations.
    *   Extended with `getCursorPos`, `windowFromPoint`, and `getSystemMetrics` for input injection support.
*   **Win32 Implementation:** Implemented `src/platform/windows/window_system_win32.h/cpp` encapsulating Win32 APIs.
*   **Injection:** Updated `Engine` to accept `WindowSystem` via dependency injection.
*   **Refactoring:** Refactored `src/core/engine_window.cpp` and `src/core/engine/engine_input.cpp` to use `WindowSystem`.
*   **Symmetry:** Created `src/platform/linux/README.md` placeholder for future Linux implementation.

### 4. InputInjector Abstraction (Completed)
*   **Interface:** Created `src/core/input/input_injector.h` defining `InputInjector` interface for injecting keyboard/mouse events.
*   **Win32 Implementation:** Created `src/platform/windows/input_injector_win32.h/cpp` implementing the interface using `SendInput`.
*   **Engine Update:** Updated `Engine` to use `InputInjector` for event injection, removing `SendInput` calls from `src/core/engine/engine_input.cpp`.
*   **Dependency Injection:** `Mayu` application now instantiates `InputInjectorWin32` and passes it to `Engine`.

### 5. Directory Structure Modernization (Completed)
*   **Reorganization:** Reorganized `src/core` into `engine`, `functions`, `input`, `settings`, `window` subdirectories.
*   **Platform Separation:** Renamed `src/system` to `src/platform/windows` and created `src/platform/linux`.
*   **Build Update:** Updated `yamy.props` and all `.vcxproj` files to reflect the new directory structure.
*   **Test Fixes:** Updated test files (`src/tests/*.cpp`) to use correct include paths.


## Architectural Analysis (Current State)

### Issues
1.  **Driver:** The kernel driver (`mayud.sys`) remains a legacy component that needs review for modern Windows security compliance.
2.  **Legacy Macros:** Still using many preprocessor macros (`USE_INI`, etc.) which should be replaced with runtime configuration or const expressions.
3.  **Direct Win32 API Calls:** `Engine` and `Mayu` classes still make many direct Win32 API calls (`SendInput`, `RegisterWindowMessage`, etc.).

## Roadmap

### Phase 2: Platform Abstraction Layer (PAL) - Long Term
*   **Goal:** Enable swapping the OS layer for cross-platform support (Linux/macOS).
*   **Input Driver Interface:** Abstract the communication with the kernel driver.
*   **Config Abstraction:** Replace direct Registry usage with an abstract `SettingsStore` (In Progress).
*   **Platform API Abstraction:** Create `Platform` interface for window management, input injection, and clipboard operations.

## Session Log (Recent Changes)
*   **2023-12-05**:
    *   **Concurrency:** Replaced `CRITICAL_SECTION` with `std::recursive_mutex` in `multithread.h`. Verified 42 tests pass.
    *   **Refactoring:** Refactored `engine.cpp` into 10 separate files.
    *   **Structure:** Reorganized entire project directory structure.
    *   **CI:** Established GitHub Actions CI pipeline.
    *   **Modifiers:** Expanded modifiers support to Mod10-Mod19.
    *   **Testing:** Migrated all tests to Google Test and expanded coverage significantly.
    *   **Bug Fix:** Fixed stuck modifier key bug (`0x59414D59` tag).
*   **2025-12-05**:
    *   **Refactoring:** Successfully split `setting.cpp` into `setting.cpp` and `setting_loader.cpp` to separate concerns.
    *   **Build:** Updated project files and resolved circular dependency issues in `function.cpp`.
    *   **Cleanup:** Removed `SettingLoader` class from `setting.cpp` and ensured proper header inclusion.
    *   **PAL (Phase 2):** Introduced `ConfigStore` interface and `input_event.h` to begin decoupling core logic from Windows Registry and Driver headers. Updated `SettingLoader` to use `ConfigStore`.
*   **2025-12-06**:
    *   **PAL (Phase 2):** Completed `WindowSystem` abstraction and Win32 implementation.
    *   **Structure:** Reorganized `src/core` and `src/platform` directories for better modularity and cross-platform preparation.
    *   **Fixes:** Resolved build issues in `mayu.cpp` and updated test suite include paths.
    *   **PAL (Phase 2):** Extended `WindowSystem` for cursor/screen support. Implemented `InputInjector` abstraction to remove direct `SendInput` calls. Verified full build success (32/64-bit) and tests.
    *   **PAL (Phase 2):** Completed `InputHook` abstraction (moving `engine_hook.cpp` logic to `input_hook_win32.cpp`) and `InputDriver` abstraction (moving `DeviceIoControl` etc. to `input_driver_win32.cpp`).
    *   **Cleanup:** Removed obsolete `engine_hook.cpp` from project.
    *   **PAL (Phase 2):** Completed `ConfigStore` abstraction. Decoupled `Engine`, `Function`, `SettingLoader` from `Registry` class. `Mayu` app now injects `ConfigStore` dependency.

