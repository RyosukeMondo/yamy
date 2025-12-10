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
│       └── test_*.cpp        # Unit tests
│
├── docs/                     # Documentation
│   ├── GUI-ARCHITECTURE.md
│   ├── LINUX-GUI-IMPLEMENTATION-PLAN.md
│   ├── LINUX-QT-GUI-MANUAL.md
│   ├── LINUX-QT-REMAINING-WORK.md
│   ├── TRACK1-JULES-TASKS.md
│   └── adr/                  # Architecture Decision Records
│
├── scripts/                  # Build and QA scripts
│   ├── check_missing_sources.ps1
│   ├── check_missing_sources.sh
│   ├── check_header_guards.ps1
│   └── check_encoding.ps1
│
├── CMakeLists.txt            # Root build configuration
├── linux_qt_setup.sh         # Linux setup automation
└── README.md                 # Project overview
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

### Unit Test Structure

```cpp
#include <gtest/gtest.h>
#include "myclass.h"

// Test fixture
class MyClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }

    void TearDown() override {
        // Test cleanup
    }

    MyClass testObject;  // Shared test object
};

// Test case
TEST_F(MyClassTest, MethodReturnsExpectedValue) {
    // Arrange
    int input = 42;

    // Act
    int result = testObject.method(input);

    // Assert
    EXPECT_EQ(84, result);
}
```

### Parameterized Tests

```cpp
class KeymapTest : public ::testing::TestWithParam<std::pair<int, std::string>> {
};

TEST_P(KeymapTest, LookupsWork) {
    auto [vk, expected] = GetParam();
    std::string result = lookupKeyName(vk);
    EXPECT_EQ(expected, result);
}

INSTANTIATE_TEST_SUITE_P(
    KeyNames,
    KeymapTest,
    ::testing::Values(
        std::make_pair(VK_A, "A"),
        std::make_pair(VK_CONTROL, "Control"),
        std::make_pair(VK_ESCAPE, "Escape")
    )
);
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

**Document Version**: 1.0
**Last Updated**: 2025-12-10
**Reviewed By**: (Pending approval)
