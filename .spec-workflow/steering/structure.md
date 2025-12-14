# YAMY Codebase Structure

## Directory Layout

```
yamy/
├── src/                      # Source code
│   ├── app/                  # Application entry points
│   │   ├── yamy.cpp         # Windows launcher (32/64-bit selector)
│   │   ├── mayu.cpp         # Windows main (engine + Win32 GUI)
│   │   ├── yamyd.cpp        # Windows 32-bit helper
│   │   ├── main_linux.cpp   # Linux headless entry point
│   │   └── main_qt.cpp      # Linux Qt GUI entry point
│   │
│   ├── core/                 # Platform-agnostic core logic
│   │   ├── engine/           # Keyboard remapping engine
│   │   │   ├── engine.h/.cpp           # Main orchestrator
│   │   │   ├── engine_lifecycle.cpp    # Start/stop/pause
│   │   │   ├── engine_input.cpp        # Input event processing
│   │   │   ├── engine_modifier.cpp     # Modifier state tracking
│   │   │   ├── engine_generator.cpp    # Event generation
│   │   │   ├── engine_focus.cpp        # Focus tracking
│   │   │   ├── engine_window.cpp       # Window manipulation
│   │   │   ├── engine_setting.cpp      # Configuration loading
│   │   │   └── engine_log.cpp          # Logging
│   │   │
│   │   ├── settings/         # Configuration management
│   │   │   ├── setting.h/.cpp          # Configuration representation
│   │   │   ├── setting_loader.h/.cpp   # .mayu file parser
│   │   │   └── parser.cpp              # Parsing utilities
│   │   │
│   │   ├── input/            # Input abstractions
│   │   │   ├── keyboard.h/.cpp         # Keyboard state
│   │   │   ├── keymap.h/.cpp           # Key binding registry
│   │   │   ├── vkeytable.h/.cpp        # Virtual key table
│   │   │   └── input_event.h           # Event data structures
│   │   │
│   │   ├── window/           # Window system abstractions
│   │   │   ├── target.h/.cpp           # Target window selector
│   │   │   ├── focus.h/.cpp            # Focus management
│   │   │   └── layoutmanager.h/.cpp    # Window layout
│   │   │
│   │   ├── functions/        # Action system
│   │   │   ├── function.h/.cpp         # Action base classes
│   │   │   ├── function_creator.cpp    # Action factory
│   │   │   └── strexpr.h/.cpp          # String expressions
│   │   │
│   │   ├── commands/         # Command implementations
│   │   │   ├── cmd_keymap_*.cpp        # Keymap commands
│   │   │   ├── cmd_window_*.cpp        # Window commands
│   │   │   ├── cmd_clipboard_*.cpp     # Clipboard commands
│   │   │   ├── cmd_mouse_*.cpp         # Mouse commands
│   │   │   └── cmd_*.cpp               # Other commands (60+ files)
│   │   │
│   │   └── platform/         # Platform interface definitions
│   │       ├── window_system_interface.h
│   │       ├── input_injector_interface.h
│   │       ├── input_hook_interface.h
│   │       ├── input_driver_interface.h
│   │       └── message_constants.h
│   │
│   ├── platform/             # Platform-specific implementations
│   │   ├── windows/          # Windows implementations
│   │   │   ├── window_system_win32.h/.cpp
│   │   │   ├── input_injector_win32.h/.cpp
│   │   │   ├── input_hook_win32.h/.cpp
│   │   │   ├── input_driver_win32.h/.cpp
│   │   │   ├── windowstool.h/.cpp      # Win32 utilities
│   │   │   ├── registry.h/.cpp         # Registry access
│   │   │   ├── utf_conversion.h/.cpp   # UTF-8/UTF-16 conversion
│   │   │   ├── hook.h/.cpp             # DLL hook implementation
│   │   │   ├── thread_win32.cpp        # Threading
│   │   │   ├── sync.cpp                # Synchronization
│   │   │   └── hook_data.cpp           # Hook data structures
│   │   │
│   │   └── linux/            # Linux implementations
│   │       ├── window_system_linux.cpp
│   │       ├── window_system_linux_queries.cpp
│   │       ├── window_system_linux_manipulation.cpp
│   │       ├── window_system_linux_hierarchy.cpp
│   │       ├── window_system_linux_mouse.cpp
│   │       ├── window_system_linux_monitor.cpp
│   │       ├── input_injector_linux.cpp
│   │       ├── input_hook_linux.cpp
│   │       ├── input_driver_linux.cpp
│   │       ├── device_manager_linux.cpp
│   │       ├── keycode_mapping.cpp
│   │       ├── sync_linux.cpp
│   │       ├── thread_linux.cpp
│   │       ├── hook_data_linux.cpp
│   │       └── ipc_linux.cpp
│   │
│   ├── ui/                   # User interface
│   │   ├── dlgsetting.cpp/h          # Windows settings dialog
│   │   ├── dlgeditsetting.cpp/h      # Windows edit dialog
│   │   ├── dlginvestigate.cpp/h      # Windows debug dialog
│   │   ├── dlglog.cpp/h              # Windows log viewer
│   │   ├── dlgversion.cpp/h          # Windows about dialog
│   │   ├── mayu.rc                   # Windows resources
│   │   │
│   │   └── qt/               # Qt GUI for Linux
│   │       ├── tray_icon_qt.h/.cpp         # System tray
│   │       ├── dialog_settings_qt.h/.cpp   # Settings dialog
│   │       ├── dialog_log_qt.h/.cpp        # Log viewer
│   │       ├── dialog_about_qt.h/.cpp      # About dialog
│   │       ├── resources.qrc                # Qt resources
│   │       └── CMakeLists.txt               # Qt build config
│   │
│   ├── utils/                # Utilities
│   │   ├── stringtool.h/.cpp         # String manipulation
│   │   ├── compiler_specific.h       # Compiler macros
│   │   ├── compiler_specific_func.h/.cpp
│   │   ├── misc.h                    # Miscellaneous utilities
│   │   ├── msgstream.h               # Message stream logging
│   │   ├── config_store.h            # Configuration storage interface
│   │   └── errormessage.h            # Error handling
│   │
│   ├── resources/            # Resource files
│   │   └── icons/            # Application icons
│   │       ├── yamy_enabled.png
│   │       └── yamy_disabled.png
│   │
│   └── tests/                # Test suite
│       ├── googletest/       # Google Test framework
│       ├── test_main.cpp     # Test runner
│       ├── test_*.cpp        # Unit tests (Catch2)
│       └── property_*.cpp    # Property-based tests (RapidCheck)
│
├── docs/                     # Documentation
│   ├── GUI-ARCHITECTURE.md
│   ├── LINUX-GUI-IMPLEMENTATION-PLAN.md
│   ├── LINUX-QT-GUI-MANUAL.md
│   ├── LINUX-QT-REMAINING-WORK.md
│   ├── TRACK1-JULES-TASKS.md
│   ├── modern-c++.md         # Modern C++ toolchain research
│   ├── map.md                # AI-compatible codebase map
│   └── adr/                  # Architecture Decision Records
│
├── scripts/                  # Build and QA scripts
│   ├── check_missing_sources.ps1
│   ├── check_missing_sources.sh
│   ├── check_header_guards.ps1
│   └── check_encoding.ps1
│
├── .clinerules               # AI agent coding rules (Cline)
├── .cursorrules              # AI agent coding rules (Cursor)
├── CMakeLists.txt            # Root build configuration
├── CMakePresets.json         # CMake presets for deterministic builds
├── conanfile.txt             # Conan dependency specification
├── linux_qt_setup.sh         # Linux setup automation
└── README.md                 # Project overview
```

---

## AI-Compatible Project Layout

### Purpose

Modern development involves AI agents (Claude, GPT-4o, GitHub Copilot) that are constrained by **context windows**. Loading the entire codebase dilutes attention and causes hallucinations. YAMY implements **Context Density Optimization** to help AI agents navigate efficiently.

### 1. Codebase Map (docs/map.md)

**Purpose**: Single-file project summary for AI navigation

**Structure**:
```markdown
# YAMY Codebase Map

## Core Engine
- `src/core/engine/engine.cpp`: Main orchestrator. Entry point for input processing.
- `src/core/engine/engine_input.cpp`: Input event handler. Processes keyboard/mouse events.
- `src/core/input/keymap.cpp`: Key binding registry. Hash table for O(1) lookup.

## Platform Abstraction
- `src/core/platform/window_system_interface.h`: Window system interface. 40+ methods.
- `src/platform/linux/window_system_linux.cpp`: Linux implementation using X11/Xlib.
- `src/platform/windows/window_system_win32.cpp`: Windows implementation using Win32 API.

## Configuration
- `src/core/settings/setting_loader.cpp`: Parser for .mayu files. Recursive descent.
- `src/core/settings/setting.cpp`: Parsed configuration representation.

## UI Layer
- `src/ui/qt/main_window_gui.cpp`: Linux Qt main window (yamy-gui).
- `src/app/main.cpp`: Linux headless daemon entry point.
```

**Benefits**:
- AI reads this first to locate relevant files
- Prevents scanning entire directory tree
- Each entry has: path + brief description + key detail

### 2. AI Agent Rules (.clinerules / .cursorrules)

**Purpose**: System prompts for IDE agents to enforce architecture

**File: .clinerules** (Cline IDE)
```markdown
# YAMY Engineering Rules

You are an expert C++20 systems programmer working on YAMY, a cross-platform keyboard remapper.

## Architecture Constraints

### Platform Abstraction
- ALL platform-specific code goes in `src/platform/windows/` or `src/platform/linux/`
- Core engine (`src/core/`) MUST be platform-agnostic
- Use factory functions (`createWindowSystem()`) to instantiate platform implementations
- NEVER use `#ifdef _WIN32` in core/ directory

### Memory Management
- Use RAII for all resources (files, OS handles, memory)
- Prefer `std::unique_ptr` over raw pointers for ownership
- Use `gsl::span` for array parameters (not pointer+size)
- NO global static variables (except constants)

### Contracts
- Decorate all public APIs with GSL contracts:
  - `Expects(condition)` for preconditions
  - `Ensures(condition)` for postconditions
  - `gsl::not_null<T*>` for non-nullable pointers
- Example: `void process(gsl::span<const InputEvent> events)`

### Logging
- Use Quill logging macros: `LOG_INFO`, `LOG_ERROR`, `LOG_DEBUG`
- NEVER use `printf`, `std::cout`, or `std::cerr`
- Use structured logging: `LOG_INFO(logger, "Key {code} mapped", "code", key.code)`
- NO logging on the critical input path (inside event processing loop)

### Testing
- Write property-based tests for state machines using RapidCheck
- Write unit tests for pure functions using Catch2
- Minimum 80% code coverage (90% for critical paths)

## Coding Style

### Naming
- Classes: `PascalCase` (e.g., `WindowSystem`)
- Functions: `camelCase` (e.g., `getForegroundWindow`)
- Variables: `camelCase` (e.g., `inputEvent`)
- Member variables: `m_` prefix (e.g., `m_isEnabled`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_KEYS`)
- Parameters: `i_` prefix for input, `o_` for output (e.g., `i_keyCode`, `o_result`)

### File Organization
- Headers: Doxygen comments explaining intent and usage
- Implementation: Minimal comments (code should be self-documenting)
- Prefer `#pragma once` over header guards
- Include order: Corresponding header → System → Third-party → Local

### Error Handling
- Core engine: NO exceptions (use return codes or `std::optional`)
- UI layer: Exceptions OK (Qt uses them)
- Always check error conditions and log failures

## Build System

### CMake
- Use `target_*` commands (not global `include_directories`)
- Platform-specific sources via conditionals:
  ```cmake
  if(WIN32)
    list(APPEND SOURCES src/platform/windows/*.cpp)
  endif()
  ```
- Configure via CMakePresets.json (don't modify CMakeLists.txt for personal settings)

### Dependencies
- Add via Conan (conanfile.txt)
- Use versioned dependencies (e.g., `quill/4.1.0`)
- Never vendor third-party libraries

## Code Metrics

- Max 500 lines per file (excluding comments/blanks)
- Max 50 lines per function
- Max cyclomatic complexity: 15 per function (10 for critical paths)
- These are enforced by pre-commit hooks

## When You're Stuck

- Read `docs/map.md` to locate relevant files
- Check `tech.md` for technology decisions
- Look for similar existing code patterns
- Ask clarifying questions before making architectural changes
```

**File: .cursorrules** (Cursor IDE)
```markdown
You are working on YAMY, a C++20 keyboard remapper. Follow these rules strictly:

1. **Platform Abstraction**: Core engine is platform-agnostic. Platform code in `src/platform/`.
2. **Contracts**: Use `Expects()` and `Ensures()` from Microsoft GSL for all APIs.
3. **Logging**: Use Quill (`LOG_INFO`, `LOG_ERROR`), never `printf` or `cout`.
4. **Testing**: RapidCheck for properties, Catch2 for units. 80% coverage minimum.
5. **Style**: PascalCase classes, camelCase functions, m_ member prefix, i_/o_ parameter prefix.
6. **Build**: Ninja + Mold (Linux) or LLD (Windows). Configure via CMakePresets.json.
7. **Metrics**: Max 500 lines/file, 50 lines/function, complexity ≤15.
8. **No hot reload code**: Excluded from this implementation.

Read `docs/map.md` first to locate files. Check `tech.md` for decisions.
```

**Benefits**:
- Constraints injected into AI agent context
- Prevents architectural violations
- Enforces coding standards automatically
- Reduces need for human code review

### 3. Semantic Density (Headers vs Implementation)

**Principle**: Maximize "Signal-to-Token" ratio

**Headers** (High density - AI reads these):
```cpp
/**
 * @file keymap.h
 * @brief Key binding registry with O(1) lookup
 *
 * The Keymap class maintains a hash table of key bindings.
 * Supports layering (parent keymaps) and per-window overrides.
 *
 * Usage:
 * @code
 * Keymap km;
 * km.define(ModifiedKey(VK_A, M_Ctrl), new ActionFunction("copy"));
 * Action* action = km.lookup(ModifiedKey(VK_A, M_Ctrl));
 * @endcode
 */
class Keymap {
public:
    /**
     * @brief Define a key binding
     * @param i_key Key combination (e.g., Ctrl+A)
     * @param i_action Action to execute (takes ownership)
     * @pre i_action must not be nullptr
     */
    void define(const ModifiedKey& i_key, Action* i_action);

    /**
     * @brief Lookup action for key combination
     * @param i_key Key to lookup
     * @return Pointer to action, or nullptr if not found
     */
    Action* lookup(const ModifiedKey& i_key) const;
};
```

**Implementation** (Low density - AI skips these):
```cpp
#include "keymap.h"

void Keymap::define(const ModifiedKey& i_key, Action* i_action) {
    Expects(i_action != nullptr);
    m_bindings[i_key.hash()] = std::unique_ptr<Action>(i_action);
}

Action* Keymap::lookup(const ModifiedKey& i_key) const {
    auto it = m_bindings.find(i_key.hash());
    return (it != m_bindings.end()) ? it->second.get() : nullptr;
}
```

**Why This Works**:
- AI reads headers to understand **what** and **why**
- AI generates implementation from specification
- Reduces context window usage by 50-70%

---

## Modern CMake Patterns

### CMakePresets.json for Reproducible Builds

**Purpose**: Single source of truth for build configuration (AI-compatible)

**File: CMakePresets.json**
```json
{
  "version": 3,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 28,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "linux-debug",
      "displayName": "Linux Debug",
      "description": "Debug build with Mold linker and ccache",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/build/conan_toolchain.cmake",
        "CMAKE_CXX_COMPILER": "clang++",
        "CMAKE_C_COMPILER": "clang",
        "CMAKE_LINKER": "mold",
        "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
        "ENABLE_CONTRACTS": "ON",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "linux-release",
      "displayName": "Linux Release",
      "description": "Optimized release build with LTO",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/build/conan_toolchain.cmake",
        "CMAKE_CXX_COMPILER": "clang++",
        "CMAKE_C_COMPILER": "clang",
        "CMAKE_LINKER": "mold",
        "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
        "CMAKE_INTERPROCEDURAL_OPTIMIZATION": "ON",
        "ENABLE_CONTRACTS": "OFF",
        "BUILD_TESTING": "OFF"
      }
    },
    {
      "name": "windows-debug",
      "displayName": "Windows Debug",
      "description": "Debug build with LLD and clang-cl",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/build/conan_toolchain.cmake",
        "CMAKE_CXX_COMPILER": "clang-cl",
        "CMAKE_C_COMPILER": "clang-cl",
        "CMAKE_LINKER": "lld-link",
        "ENABLE_CONTRACTS": "ON",
        "BUILD_TESTING": "ON"
      },
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "linux-debug",
      "configurePreset": "linux-debug",
      "jobs": 8
    },
    {
      "name": "linux-release",
      "configurePreset": "linux-release",
      "jobs": 8
    }
  ],
  "testPresets": [
    {
      "name": "linux-debug",
      "configurePreset": "linux-debug",
      "output": {"outputOnFailure": true}
    }
  ]
}
```

**Usage**:
```bash
# Configure
cmake --preset linux-debug

# Build
cmake --build --preset linux-debug

# Test
ctest --preset linux-debug
```

**Benefits**:
- AI agents can reproduce builds exactly
- No manual CMake flags (`-DCMAKE_BUILD_TYPE=...`)
- Platform-specific settings isolated
- Version controlled

### Modern CMakeLists.txt Patterns

**Mold Linker Integration (Linux)**:
```cmake
# Detect Mold and use it if available
if(UNIX AND NOT APPLE)
    find_program(MOLD_LINKER mold)
    if(MOLD_LINKER)
        add_link_options("-fuse-ld=mold")
        message(STATUS "Using Mold linker: ${MOLD_LINKER}")
    else()
        message(WARNING "Mold linker not found. Install with: sudo apt install mold")
    endif()
endif()
```

**LLD Linker Integration (Windows)**:
```cmake
# Use LLD with clang-cl on Windows
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_link_options("/clang:-fuse-ld=lld")
    message(STATUS "Using LLD linker with clang-cl")
endif()
```

**ccache Integration**:
```cmake
# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()
```

**Conan Dependency Integration**:
```cmake
# Load Conan toolchain (generated by conan install)
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
else()
    message(FATAL_ERROR "Conan toolchain not found. Run: conan install . --build=missing")
endif()

# Find Conan packages
find_package(quill REQUIRED)
find_package(Microsoft.GSL REQUIRED)
find_package(RapidCheck REQUIRED)
find_package(Catch2 REQUIRED)

# Link to targets
target_link_libraries(yamy_core PRIVATE
    quill::quill
    Microsoft.GSL::GSL
)
```

**Code Metrics Enforcement**:
```cmake
# Optional: Enforce code metrics with lizard
find_program(LIZARD_PROGRAM lizard)
if(LIZARD_PROGRAM AND BUILD_TESTING)
    add_custom_target(check-metrics
        COMMAND ${LIZARD_PROGRAM}
            --length 500          # Max 500 lines per file
            --arguments 5         # Max 5 function parameters
            --CCN 15              # Max cyclomatic complexity 15
            ${CMAKE_SOURCE_DIR}/src
        COMMENT "Checking code metrics (500 lines/file, CCN ≤15)"
    )
endif()
```

---

## Coding Conventions

### Naming Conventions

**Files**:
```
snake_case.cpp         # Implementation files
snake_case.h           # Header files
PascalCase.cpp         # Legacy Windows files (gradually migrating)
```

**Classes**:
```cpp
class MyClass { };     # PascalCase for classes
```

**Functions**:
```cpp
void myFunction();     # camelCase for functions
void my_function();    # snake_case acceptable for platform layer
```

**Variables**:
```cpp
int myVariable;        # camelCase for local variables
int m_memberVariable;  # m_ prefix for member variables
int g_globalVariable;  # g_ prefix for globals (avoid)
const int MAX_SIZE = 100;  # UPPER_CASE for constants
```

**Prefixes**:
```cpp
class Interface { };   # I prefix for interfaces (Windows style)
IWindowSystem          # Platform interface
bool i_input;          # i_ prefix for input parameters
int o_output;          # o_ prefix for output parameters
int io_both;           # io_ prefix for input/output parameters
```

### File Organization

**Header File Pattern**:
```cpp
#pragma once           // Prefer over header guards

// Includes (grouped and sorted)
#include <system>      // Standard library
#include <third-party> // Third-party libraries
#include "local.h"     // Local headers

// Forward declarations
class Foo;

// Main declarations
class MyClass {
public:
    // Public interface
private:
    // Private implementation
};
```

**Implementation File Pattern**:
```cpp
#include "myclass.h"   // Corresponding header first

// Other includes
#include <iostream>
#include "other.h"

// Anonymous namespace for file-local helpers
namespace {
    void helperFunction() { }
}

// Class implementation
void MyClass::method() {
    // Implementation
}
```

### Code Style

**Indentation**: 4 spaces (no tabs)

**Braces**:
```cpp
// Function braces on new line (Windows convention)
void function()
{
    if (condition) {
        // Statement braces on same line
    }
}
```

**Line Length**: 120 characters (flexible)

**Comments**:
```cpp
// Single-line comment for brief notes

/**
 * Doxygen comment for documentation
 * @param i_param Description
 * @return Description
 */

///  Triple-slash for member documentation
```

---

## Platform Abstraction Patterns

### Pattern 1: Interface + Factory

**Interface Definition** (`src/core/platform/`):
```cpp
// window_system_interface.h
class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;
    virtual WindowHandle getForegroundWindow() = 0;
    // ... other methods
};

// Factory function (implemented per-platform)
IWindowSystem* createWindowSystem();
```

**Platform Implementation** (`src/platform/windows/` or `src/platform/linux/`):
```cpp
// window_system_win32.cpp
class WindowSystemWin32 : public IWindowSystem {
    WindowHandle getForegroundWindow() override {
        return ::GetForegroundWindow();
    }
};

IWindowSystem* createWindowSystem() {
    return new WindowSystemWin32();
}
```

### Pattern 2: Type Aliases

**Header** (`src/core/platform/types.h`):
```cpp
namespace yamy::platform {
    #ifdef _WIN32
        typedef HWND WindowHandle;
    #else
        typedef uint64_t WindowHandle;  // X11 Window ID
    #endif
}
```

**Usage**:
```cpp
using yamy::platform::WindowHandle;
WindowHandle hwnd = windowSystem->getForegroundWindow();
```

### Pattern 3: Conditional Source Files

**CMakeLists.txt**:
```cmake
if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows/window_system_win32.cpp
        src/platform/windows/input_injector_win32.cpp
    )
else()
    set(PLATFORM_SOURCES
        src/platform/linux/window_system_linux.cpp
        src/platform/linux/input_injector_linux.cpp
    )
endif()

add_executable(yamy ${CORE_SOURCES} ${PLATFORM_SOURCES})
```

---

## Module Dependencies

### Dependency Graph

```
┌─────────────────────────────────────────────────┐
│                    ui/                           │
│  (Windows: Win32 GUI, Linux: Qt GUI)            │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│            core/engine/                          │
│  (Platform-agnostic remapping logic)            │
└┬────────────┬────────────┬──────────────────────┘
 │            │            │
 │            │            └──────┐
 ▼            ▼                   ▼
┌────────────────┐  ┌──────────────────┐  ┌──────────────┐
│ core/settings/ │  │ core/functions/  │  │ core/window/ │
└────────────────┘  └──────────────────┘  └──────────────┘
         │                   │                     │
         └───────────────────┴─────────────────────┘
                             │
              ┌──────────────▼──────────────┐
              │   core/platform/            │
              │  (Interfaces)               │
              └──────────────┬──────────────┘
                             │
              ┌──────────────▼──────────────────────┐
              │   platform/windows/  OR             │
              │   platform/linux/                   │
              │  (OS-specific implementations)      │
              └─────────────────────────────────────┘
```

### Layer Rules

1. **UI Layer** may depend on **Core Layer** and **Utils**
2. **Core Layer** may depend on **Platform Interfaces** and **Utils**
3. **Platform Implementations** implement **Platform Interfaces**
4. **Platform Interfaces** depend on **nothing** (pure virtual)
5. **Utils** depend on **nothing** (leaf nodes)

**Violations** (technical debt):
- ❌ `core/engine/engine.cpp` includes `windowstool.h` (Windows-specific)
- ❌ `utils/msgstream.h` uses `PostMessage` (Windows-specific)
- ❌ `core/functions/function.h` uses `SW_*` constants (Windows-specific)

**Fix**: Track 1 refactoring removes these violations

---

## Build System Patterns

### Target Organization

**Executable Targets**:
```cmake
add_executable(target_name ${SOURCES})
target_link_libraries(target_name PRIVATE dependencies)
target_include_directories(target_name PRIVATE include_dirs)
target_compile_definitions(target_name PRIVATE DEFINES)
```

**Library Targets**:
```cmake
add_library(library_name STATIC ${SOURCES})
# OR
add_library(library_name SHARED ${SOURCES})
```

**Conditional Targets**:
```cmake
if(WIN32)
    add_executable(windows_only_target ...)
elseif(UNIX AND NOT APPLE)
    add_executable(linux_only_target ...)
endif()
```

### Include Directory Hierarchy

**Private Includes** (implementation-only):
```cmake
target_include_directories(target PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
)
```

**Public Includes** (exported to dependents):
```cmake
target_include_directories(target PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

**Interface Includes** (header-only libraries):
```cmake
add_library(interface_lib INTERFACE)
target_include_directories(interface_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

---

## Common Patterns & Idioms

### RAII for Resource Management

```cpp
class ResourceHolder {
public:
    ResourceHolder() : m_handle(acquire()) { }
    ~ResourceHolder() { release(m_handle); }

    // Delete copy, allow move
    ResourceHolder(const ResourceHolder&) = delete;
    ResourceHolder& operator=(const ResourceHolder&) = delete;
    ResourceHolder(ResourceHolder&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr)) { }

private:
    Handle m_handle;
};
```

### Singleton Pattern (Engine)

```cpp
class Engine {
public:
    static Engine& getInstance() {
        static Engine instance;
        return instance;
    }

private:
    Engine() = default;  // Private constructor
    ~Engine() = default;

    // Delete copy and move
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};
```

### Interface Segregation

```cpp
// DON'T: Fat interface
class IWindowSystem {
    virtual void method1() = 0;
    virtual void method2() = 0;
    // ... 50 methods
};

// DO: Split into focused interfaces
class IWindowQueries {
    virtual WindowHandle getForegroundWindow() = 0;
    virtual std::string getWindowText(WindowHandle) = 0;
};

class IWindowManipulation {
    virtual void moveWindow(WindowHandle, Rect) = 0;
    virtual void setForegroundWindow(WindowHandle) = 0;
};
```

---

## Error Handling Strategy

### Exception Policy

**Core Engine**: NO exceptions (performance-critical)
```cpp
// Use return codes or output parameters
bool loadConfig(const std::string& path, Setting* o_setting);
```

**UI Layer**: Exceptions OK (Qt uses exceptions)
```cpp
try {
    dialog.exec();
} catch (const std::exception& e) {
    QMessageBox::critical(nullptr, "Error", e.what());
}
```

### Error Reporting Pattern

```cpp
enum class ErrorCode {
    Success,
    FileNotFound,
    ParseError,
    InvalidSyntax
};

struct Result {
    ErrorCode code;
    std::string message;

    bool ok() const { return code == ErrorCode::Success; }
    explicit operator bool() const { return ok(); }
};

Result loadFile(const std::string& path) {
    if (!fileExists(path)) {
        return {ErrorCode::FileNotFound, "File not found: " + path};
    }
    return {ErrorCode::Success, ""};
}

// Usage
auto result = loadFile("config.mayu");
if (!result) {
    log << "Error: " << result.message << std::endl;
}
```

---

## Testing Patterns

### Unit Test Structure (Catch2)

```cpp
#include <catch2/catch_test_macros.hpp>
#include "myclass.h"

TEST_CASE("MyClass basic functionality") {
    // Arrange
    MyClass obj;
    int input = 42;

    // Act
    int result = obj.method(input);

    // Assert
    REQUIRE(result == 84);
}
```

### Property-Based Tests (RapidCheck)

```cpp
#include <rapidcheck.h>

rc::check("Key-down events must have matching key-up",
  [](const std::vector<InputEvent>& events) {
    Engine engine;
    for (auto e : events) {
        engine.process(e);
    }
    RC_ASSERT(engine.activeKeys().empty());
  });
```

---

## Documentation Patterns

### Header Documentation

```cpp
/**
 * @file myclass.h
 * @brief Brief description of file purpose
 *
 * Detailed description of what this file contains and how to use it.
 */

/**
 * @class MyClass
 * @brief Brief class description
 *
 * Detailed class description with usage examples:
 * @code
 * MyClass obj;
 * obj.doSomething();
 * @endcode
 */
class MyClass {
public:
    /**
     * @brief Brief method description
     *
     * Detailed method description.
     *
     * @param i_input Input parameter description
     * @param o_output Output parameter description (must not be nullptr)
     * @return true on success, false on failure
     *
     * @note Special note about edge cases
     * @warning Warning about dangerous behavior
     */
    bool method(int i_input, int* o_output);
};
```

### Implementation Comments

```cpp
void MyClass::complexMethod() {
    // High-level algorithm overview:
    // 1. Validate input
    // 2. Transform data
    // 3. Perform operation
    // 4. Cleanup

    validateInput();  // Self-explanatory, no comment needed

    // Edge case: Handle empty list by returning early
    if (list.empty()) {
        return;
    }

    // Performance note: This loop is O(n²), but n is typically <100
    for (...) {
        for (...) {
            // Complex logic explained
        }
    }
}
```

---

## Anti-Patterns to Avoid

### ❌ Ifdef Hell

```cpp
// DON'T
void function() {
    #ifdef _WIN32
        // 100 lines of Windows code
    #else
        // 100 lines of Linux code
    #endif
}

// DO: Separate files
// windows/function_impl.cpp
// linux/function_impl.cpp
```

### ❌ God Objects

```cpp
// DON'T
class Mayu {
    // 5000 lines, does everything
};

// DO: Split responsibilities
class Engine { /* remapping */ };
class WindowTracker { /* focus tracking */ };
class ConfigManager { /* settings */ };
```

### ❌ Magic Numbers

```cpp
// DON'T
if (value == 9) { ... }

// DO
const int RESTORE_FLAG = 9;
if (value == RESTORE_FLAG) { ... }

// BETTER
enum class ShowCommand { Restore = 9 };
if (value == ShowCommand::Restore) { ... }
```

### ❌ Raw Pointers for Ownership

```cpp
// DON'T
Thing* thing = new Thing();
// Who deletes this? When?

// DO
std::unique_ptr<Thing> thing = std::make_unique<Thing>();
// Automatic cleanup
```

---

## Migration Strategies

### From Windows Types to Platform Types

**Step 1**: Alias (backward compatible)
```cpp
using WindowHandle = HWND;  // Transition step
```

**Step 2**: Gradual replacement
```cpp
void method(WindowHandle hwnd);  // Not HWND
```

**Step 3**: Remove alias
```cpp
using WindowHandle = uint64_t;  // Platform-specific definition
```

### From Registry to QSettings

**Windows**:
```cpp
Registry::read("Software\\YAMY", ".mayu0", &value);
```

**Cross-Platform**:
```cpp
ConfigStore* store = createConfigStore();  // Factory
store->read("keymaps/configs/0", &value);
```

**Linux Implementation**:
```cpp
// Uses QSettings under the hood
QSettings settings("YAMY", "YAMY");
value = settings.value("keymaps/configs/0").toString();
```

---

**Document Version**: 2.0
**Last Updated**: 2025-12-15
**Changes**: Added AI-compatible project layout, CMakePresets.json patterns, modern CMake integration
**Reviewed By**: (Pending approval)
