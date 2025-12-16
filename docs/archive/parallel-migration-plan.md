# Parallel Migration Execution Plan
**11 Concurrent Branches for Phase 3 & 4**

## Overview

This document splits the migration work into 11 independent branches that can be developed in parallel and merged sequentially based on dependencies.

## Dependency Graph

```
Foundation Layer (Week 1):
├─ Branch 1: UTF Conversion [BLOCKING: 2,3,4,5,6]
├─ Branch 8: Remove windows.h from cmd_wait [INDEPENDENT]
└─ Branch 9: Platform Interface Layer [BLOCKING: 10,11]

Core Migration (Week 2-3) - After Branch 1:
├─ Branch 2: Engine Migration [BLOCKING: 5]
├─ Branch 3: Keymap Migration [INDEPENDENT after Branch 1]
├─ Branch 4: Settings Migration [INDEPENDENT after Branch 1]
├─ Branch 6: UI Layer Migration [INDEPENDENT after Branch 1]
└─ Branch 7: Platform Utilities [INDEPENDENT after Branch 1]

Command Migration (Week 3) - After Branches 1,2:
└─ Branch 5: Command System [Needs Branch 1,2]

PAL Completion (Week 4-5) - After Branch 9:
├─ Branch 10: Win32 Type Audit [Needs Branch 9]
└─ Branch 11: Linux Stub Backend [Needs Branch 9,10]
```

## Merge Order

**Week 1 (Foundation):**
1. Branch 1 (UTF Conversion) - MUST merge first
2. Branch 8 (Remove windows.h) - Can merge anytime
3. Branch 9 (Platform Interfaces) - Can merge after Branch 1

**Week 2-3 (Core):**
4. Branch 2 (Engine) - After Branch 1
5. Branch 3 (Keymap) - After Branch 1
6. Branch 4 (Settings) - After Branch 1
7. Branch 6 (UI Layer) - After Branch 1
8. Branch 7 (Platform Utils) - After Branch 1

**Week 3-4 (Commands):**
9. Branch 5 (Commands) - After Branches 1,2

**Week 4-5 (PAL):**
10. Branch 10 (Win32 Audit) - After Branch 9
11. Branch 11 (Linux Stub) - After Branches 9,10

---

## Branch 1: UTF Conversion Infrastructure
**Priority:** P0 (BLOCKING - Must merge first)
**Effort:** 4-6 hours
**Branch:** `feat/utf-conversion-infrastructure`
**Blocks:** Branches 2,3,4,5,6,7

### Prompt for Jules

```
Create UTF-8 ↔ UTF-16 conversion infrastructure for Windows API boundary.

LOCATION: src/platform/windows/

CREATE FILES:
1. src/platform/windows/utf_conversion.h
2. src/platform/windows/utf_conversion.cpp
3. src/tests/test_utf_conversion.cpp (unit tests)

IMPLEMENTATION REQUIREMENTS:

Header (utf_conversion.h):
```cpp
#pragma once
#include <string>

namespace yamy::platform {

// UTF-8 to UTF-16 conversion for Windows APIs
std::wstring utf8_to_wstring(const std::string& utf8);
std::wstring utf8_to_wstring(const char* utf8);

// UTF-16 to UTF-8 conversion from Windows APIs
std::string wstring_to_utf8(const std::wstring& wide);
std::string wstring_to_utf8(const wchar_t* wide);

} // namespace yamy::platform
```

Implementation Requirements:
- Use Windows MultiByteToWideChar/WideCharToMultiByte with CP_UTF8
- Handle empty strings (return empty string)
- Handle nullptr (return empty string)
- Efficient: avoid unnecessary allocations
- No exceptions thrown (return empty on error)

Unit Tests Required:
- Round-trip conversion (UTF-8 → UTF-16 → UTF-8)
- Empty string handling
- Null pointer handling
- ASCII characters (should pass through unchanged)
- Japanese/Unicode characters (test with: "日本語テスト")
- Edge cases: very long strings, emoji

CMakeLists.txt Update:
Add utf_conversion.cpp to ENGINE_SOURCES

ACCEPTANCE CRITERIA:
- All unit tests pass
- No memory leaks (check with valgrind if possible)
- Functions are in yamy::platform namespace
- Header has include guards
- Code follows project style
```

**Files to Modify:**
- CMakeLists.txt (add utf_conversion.cpp to sources)

**Testing:**
```bash
# Build and run tests
cmake --build build --target yamy_test
./build/yamy_test --gtest_filter="*UTF*"
```

---

## Branch 2: Engine Class String Migration
**Priority:** P0
**Effort:** 8-12 hours
**Branch:** `feat/engine-string-migration`
**Depends on:** Branch 1
**Blocks:** Branch 5

### Prompt for Jules

```
Migrate Engine class from tstring (UTF-16) to std::string (UTF-8).

LOCATION: src/core/engine/

FILES TO MODIFY:
- src/core/engine/engine.h
- src/core/engine/engine.cpp
- src/core/engine/engine_lifecycle.cpp
- src/core/engine/engine_input.cpp
- src/core/engine/engine_modifier.cpp
- src/core/engine/engine_generator.cpp
- src/core/engine/engine_focus.cpp
- src/core/engine/engine_window.cpp
- src/core/engine/engine_setting.cpp
- src/core/engine/engine_log.cpp

STEP 1: Audit Current String Usage
Run: grep -n "tstring\|_T(" src/core/engine/*.h src/core/engine/*.cpp

STEP 2: Replace Member Variables in engine.h
Find all: tstring m_*
Replace with: std::string m_*

Example:
- tstring m_name; → std::string m_name;
- tstring m_settingPath; → std::string m_settingPath;

STEP 3: Update Member Functions
- Change return types: tstring getName() → std::string getName()
- Change parameters: void setName(const tstring&) → void setName(const std::string&)

STEP 4: Update String Literals
Replace all: _T("...") → "..."
These are now UTF-8 literals.

STEP 5: Windows API Calls
For any Windows API calls that need UTF-16:
```cpp
// Old:
SetWindowText(hwnd, m_name.c_str());

// New:
#include "../../platform/windows/utf_conversion.h"
SetWindowText(hwnd, yamy::platform::utf8_to_wstring(m_name).c_str());
```

STEP 6: File I/O
If Engine loads/saves files with paths:
- Internally store as std::string (UTF-8)
- Convert to std::wstring for Windows file APIs (CreateFileW, etc.)

STEP 7: Logging
Ensure log messages work with UTF-8:
- Log strings should remain std::string
- Only convert when outputting to Windows console/file APIs

TESTING:
1. Build: cmake --build build
2. Run engine tests: ./build/yamy_test --gtest_filter="*Engine*"
3. Manual test: Start yamy, load a configuration, verify it works
4. Test with non-ASCII config path/content

ACCEPTANCE CRITERIA:
- No tstring in src/core/engine/
- No _T() macros in engine files
- Engine starts and loads configuration
- All engine tests pass
- No compilation warnings
```

---

## Branch 3: Keymap Class String Migration
**Priority:** P0
**Effort:** 4-6 hours
**Branch:** `feat/keymap-string-migration`
**Depends on:** Branch 1

### Prompt for Jules

```
Migrate Keymap class from tstring to std::string.

LOCATION: src/core/input/

FILES TO MODIFY:
- src/core/input/keymap.h
- src/core/input/keymap.cpp

STEP 1: Update Keymap Class
```cpp
// In keymap.h
class Keymap {
private:
    std::string m_name;  // Was: tstring m_name
    // ... other members
public:
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
};
```

STEP 2: Update Keymap Registry/Lookup
If keymaps are stored in a map/container:
```cpp
// Old:
std::map<tstring, Keymap*> m_keymaps;

// New:
std::map<std::string, Keymap*> m_keymaps;
```

STEP 3: Update String Literals
Replace _T("keymap_name") → "keymap_name"

STEP 4: Update Any Windows API Calls
If keymap names are displayed in UI:
```cpp
#include "../../platform/windows/utf_conversion.h"
SetWindowText(hwnd, yamy::platform::utf8_to_wstring(keymap->getName()).c_str());
```

TESTING:
1. Test keymap switching
2. Test keymap name display
3. Verify keymap lookup works correctly

ACCEPTANCE CRITERIA:
- No tstring in keymap.h
- Keymap switching works
- Keymap names display correctly in UI
```

---

## Branch 4: Settings System String Migration
**Priority:** P0
**Effort:** 8-10 hours
**Branch:** `feat/settings-string-migration`
**Depends on:** Branch 1

### Prompt for Jules

```
Migrate Settings/SettingLoader/Parser from tstring to std::string.

LOCATION: src/core/settings/

FILES TO MODIFY:
- src/core/settings/setting.h
- src/core/settings/setting.cpp
- src/core/settings/setting_loader.h
- src/core/settings/setting_loader.cpp
- src/core/settings/parser.h
- src/core/settings/parser.cpp

STEP 1: Update Setting Class
```cpp
// In setting.h
class Setting {
private:
    std::string m_filename;     // Was: tstring
    std::string m_includeDir;   // Was: tstring
    // ... other members
};
```

STEP 2: Update SettingLoader
Key change: load_ARGUMENT for string parameters
```cpp
// In setting_loader.h
template<>
void load_ARGUMENT<std::string>(std::string* out);  // Was: tstring*
```

STEP 3: Update Parser String Handling
- Token values should be std::string
- String literals in config files are UTF-8
- Update any regex to use std::regex instead of tregex

STEP 4: File I/O (CRITICAL)
Configuration files need special handling:

Option A - Assume UTF-8 files (modern):
```cpp
std::ifstream file(path, std::ios::binary);
std::string content((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
```

Option B - Windows UTF-16 files (legacy compatibility):
```cpp
// Open as UTF-16, convert to UTF-8 internally
std::wifstream file(path);
file.imbue(std::locale(file.getloc(),
    new std::codecvt_utf8_utf16<wchar_t>));
std::wstring wide_content((std::istreambuf_iterator<wchar_t>(file)),
                          std::istreambuf_iterator<wchar_t>());
std::string utf8_content = yamy::platform::wstring_to_utf8(wide_content);
```

Recommendation: Start with Option A (UTF-8 files), add Option B if needed for backwards compatibility.

STEP 5: File Path Handling
```cpp
// Internally: UTF-8 paths
std::string configPath = "C:/Users/ユーザー/config.mayu";

// For Windows API (CreateFileW, etc):
std::wstring widePath = yamy::platform::utf8_to_wstring(configPath);
HANDLE hFile = CreateFileW(widePath.c_str(), ...);
```

TESTING:
1. Load existing .mayu configuration file
2. Test with non-ASCII file paths (e.g., Japanese username)
3. Test with non-ASCII content in config file
4. Verify all setting values parse correctly

ACCEPTANCE CRITERIA:
- Can load existing configuration files
- Non-ASCII paths work (e.g., C:/Users/ユーザー/)
- String settings parse correctly
- No tstring in settings headers
```

---

## Branch 5: Command System String Migration
**Priority:** P1
**Effort:** 10-14 hours
**Branch:** `feat/command-string-migration`
**Depends on:** Branches 1, 2

### Prompt for Jules

```
Migrate Function/Command system from tstring to std::string.

LOCATION: src/core/functions/, src/core/commands/

FILES TO MODIFY:
- src/core/functions/function.h
- src/core/functions/function.cpp
- src/core/functions/function_creator.cpp
- All files in src/core/commands/ that use tstring

STEP 1: Audit Command Files
```bash
grep -r "tstring" src/core/commands/ | wc -l
grep -r "tstring" src/core/commands/ > /tmp/tstring_usage.txt
```

Review /tmp/tstring_usage.txt to identify which commands use strings.

STEP 2: Update Function Base Class
```cpp
// In function.h
class Function {
protected:
    // Update any string members
    std::string m_description;  // Was: tstring
};
```

STEP 3: Update FunctionParam
```cpp
// In function.h
struct FunctionParam {
    // Update any string members
    std::string m_arg;  // Was: tstring
};
```

STEP 4: For Each Command Using Strings

Example: Command_LoadSetting
```cpp
// In cmd_load_setting.h
class Command_LoadSetting : public Command<Command_LoadSetting, std::string> {
    std::string m_path;  // Was: tstring
public:
    void load(SettingLoader* loader) override;
    void exec(Engine* engine, FunctionParam* param) const override;
};

// In cmd_load_setting.cpp
void Command_LoadSetting::load(SettingLoader* loader) {
    loader->load_ARGUMENT(&m_path);  // Now loads std::string
}

void Command_LoadSetting::exec(Engine* engine, FunctionParam* param) const {
    // m_path is now UTF-8
    engine->loadSetting(m_path);
}
```

STEP 5: Commands with String Output
If command has outputArgs():
```cpp
tostream& Command_Foo::outputArgs(tostream& ost) const override {
    // Old: ost << _T("\"") << m_str << _T("\"");
    // New: Convert for output
    ost << "\"" << m_str << "\"";
    return ost;
}
```

Note: tostream is still _TCHAR-based (for now), so may need temporary conversion:
```cpp
#include "../../utils/stringtool.h"
ost << to_tstring(m_str);  // Temporary bridge
```

STEP 6: Systematic Replacement

For each command file:
1. Search for tstring
2. Replace member variables
3. Replace _T() literals
4. Update load() method
5. Update exec() method
6. Update outputArgs() if present
7. Test the command

TESTING:
Test commands that use strings:
- LoadSetting: Load config with string path
- ShellExecute: Execute with string command
- HelpMessage: Display string message
- SetImeString: Set IME with string

ACCEPTANCE CRITERIA:
- No tstring in command headers
- All command tests pass
- Commands with string arguments work correctly
- String output displays correctly
```

---

## Branch 6: UI Layer String Migration
**Priority:** P1
**Effort:** 12-16 hours
**Branch:** `feat/ui-string-migration`
**Depends on:** Branch 1

### Prompt for Jules

```
Migrate UI dialogs and application entry points from tstring to std::string.

LOCATION: src/ui/, src/app/

FILES TO MODIFY:
- src/ui/dlgeditsetting.cpp
- src/ui/dlginvestigate.cpp
- src/ui/dlglog.cpp
- src/ui/dlgsetting.cpp
- src/ui/dlgversion.cpp
- src/app/mayu.cpp
- src/app/yamy.cpp

STEP 1: Create Windows API Wrappers

Add to src/platform/windows/windowstool.h:
```cpp
#include <string>
#include <windows.h>

namespace yamy::windows {

// Wrap SetWindowText to work with UTF-8
inline void setWindowText(HWND hwnd, const std::string& text) {
    SetWindowTextW(hwnd, yamy::platform::utf8_to_wstring(text).c_str());
}

// Wrap GetWindowText to return UTF-8
inline std::string getWindowText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len == 0) return std::string();

    std::wstring wide(len + 1, L'\0');
    GetWindowTextW(hwnd, &wide[0], len + 1);
    return yamy::platform::wstring_to_utf8(wide);
}

// Wrap SetDlgItemText
inline void setDlgItemText(HWND hwnd, int itemId, const std::string& text) {
    SetDlgItemTextW(hwnd, itemId, yamy::platform::utf8_to_wstring(text).c_str());
}

// Wrap GetDlgItemText
inline std::string getDlgItemText(HWND hwnd, int itemId) {
    int len = GetWindowTextLengthW(GetDlgItem(hwnd, itemId));
    if (len == 0) return std::string();

    std::wstring wide(len + 1, L'\0');
    GetDlgItemTextW(hwnd, itemId, &wide[0], len + 1);
    return yamy::platform::wstring_to_utf8(wide);
}

// Wrap MessageBox
inline int messageBox(HWND hwnd, const std::string& text,
                      const std::string& caption, UINT type) {
    return MessageBoxW(hwnd,
                      yamy::platform::utf8_to_wstring(text).c_str(),
                      yamy::platform::utf8_to_wstring(caption).c_str(),
                      type);
}

} // namespace yamy::windows
```

STEP 2: Update Dialog Procedures

For each dialog file:
1. Replace SetWindowText → yamy::windows::setWindowText
2. Replace GetWindowText → yamy::windows::getWindowText
3. Replace SetDlgItemText → yamy::windows::setDlgItemText
4. Replace GetDlgItemText → yamy::windows::getDlgItemText
5. Replace MessageBox → yamy::windows::messageBox
6. Update any string member variables: tstring → std::string
7. Replace _T() literals → ""

Example (dlgsetting.cpp):
```cpp
// Old:
SetWindowText(hwnd, _T("Settings Dialog"));
tstring value;
GetWindowText(GetDlgItem(hwnd, IDC_EDIT), ...);

// New:
yamy::windows::setWindowText(hwnd, "Settings Dialog");
std::string value = yamy::windows::getWindowText(GetDlgItem(hwnd, IDC_EDIT));
```

STEP 3: Update Application Entry Points

mayu.cpp / yamy.cpp:
```cpp
// Update WinMain to use wide char version
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    // Convert command line to UTF-8
    std::string cmdLine = yamy::platform::wstring_to_utf8(lpCmdLine);

    // Rest of application uses UTF-8 internally
    return appMain(cmdLine);
}
```

STEP 4: Window Class Registration

Use RegisterClassExW (wide version):
```cpp
WNDCLASSEXW wc = {};
wc.lpszClassName = L"YamyWindowClass";  // Wide string literal
wc.lpfnWndProc = WndProc;
// ...
RegisterClassExW(&wc);
```

TESTING:
1. Open each dialog and verify display
2. Enter Japanese/Unicode text in edit controls
3. Verify text displays correctly
4. Test message boxes with Unicode text
5. Test application startup with command line args

ACCEPTANCE CRITERIA:
- All dialogs display correctly
- Japanese/Unicode text works
- No TCHAR*/tstring in dialog code
- No _T() macros
- Application starts correctly
```

---

## Branch 7: Platform Utilities String Migration
**Priority:** P2
**Effort:** 6-8 hours
**Branch:** `feat/platform-utils-string-migration`
**Depends on:** Branch 1

### Prompt for Jules

```
Migrate platform utility functions from tstring to std::string.

LOCATION: src/platform/windows/, src/utils/

FILES TO MODIFY:
- src/platform/windows/registry.cpp
- src/platform/windows/windowstool.cpp
- src/utils/stringtool.h
- src/utils/stringtool.cpp

STEP 1: Registry Class

Update Registry class (registry.cpp):
```cpp
class Registry {
public:
    // Old: bool read(const tstring& key, tstring* value);
    // New:
    bool read(const std::string& key, std::string* value) {
        // Convert key to UTF-16 for Windows API
        std::wstring wideKey = yamy::platform::utf8_to_wstring(key);

        // Call RegQueryValueExW
        DWORD size = 0;
        RegQueryValueExW(hKey, wideKey.c_str(), nullptr, nullptr, nullptr, &size);

        std::wstring wideValue(size / sizeof(wchar_t), L'\0');
        RegQueryValueExW(hKey, wideKey.c_str(), nullptr, nullptr,
                        (LPBYTE)&wideValue[0], &size);

        // Convert back to UTF-8
        *value = yamy::platform::wstring_to_utf8(wideValue);
        return true;
    }

    bool write(const std::string& key, const std::string& value) {
        std::wstring wideKey = yamy::platform::utf8_to_wstring(key);
        std::wstring wideValue = yamy::platform::utf8_to_wstring(value);

        return RegSetValueExW(hKey, wideKey.c_str(), 0, REG_SZ,
                             (const BYTE*)wideValue.c_str(),
                             (wideValue.length() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS;
    }
};
```

STEP 2: Window Tools

Update windowstool.cpp functions:
```cpp
// Old: tstring loadString(UINT id);
// New:
std::string loadString(UINT id) {
    wchar_t buffer[1024];
    LoadStringW(GetModuleHandle(nullptr), id, buffer, 1024);
    return yamy::platform::wstring_to_utf8(buffer);
}

// Update getToplevelWindow if it uses strings (probably doesn't)
```

STEP 3: String Tools

Update stringtool.h/cpp:
```cpp
// REMOVE these typedefs:
// typedef std::wstring tstring;  // DELETE
// typedef std::wregex tregex;    // DELETE

// UPDATE functions to use std::string
// If there are string manipulation utilities, update them

// REMOVE this function:
// tstring to_tstring(const std::string&);  // DELETE
```

If stringtool has utility functions, update them:
```cpp
// Example: string trimming
// Old: tstring trim(const tstring& str);
// New:
std::string trim(const std::string& str) {
    // Implementation...
}
```

STEP 4: Remove Legacy Conversion Functions

Delete or mark deprecated:
- to_tstring()
- from_tstring()
- Any other TCHAR conversion utilities

TESTING:
1. Test registry read/write with Unicode keys/values
2. Test loadString with various string resource IDs
3. Verify window title retrieval works
4. Test any string utility functions

ACCEPTANCE CRITERIA:
- Registry operations work with UTF-8
- No tstring typedef remains
- No to_tstring() function
- String utilities work with std::string
```

---

## Branch 8: Remove windows.h from cmd_wait
**Priority:** P0 (INDEPENDENT)
**Effort:** 2-3 hours
**Branch:** `feat/remove-windows-h-from-core`
**Depends on:** None

### Prompt for Jules

```
Remove #include <windows.h> from src/core/commands/cmd_wait.cpp by creating a platform sleep abstraction.

STEP 1: Create Platform Interface

Create src/core/platform/thread.h:
```cpp
#pragma once
#include <cstdint>

namespace yamy::platform {

// Cross-platform sleep function
// Sleeps for the specified number of milliseconds
void sleep_ms(uint32_t milliseconds);

} // namespace yamy::platform
```

STEP 2: Windows Implementation

Create src/platform/windows/thread_win32.cpp:
```cpp
#include "core/platform/thread.h"
#include <windows.h>

namespace yamy::platform {

void sleep_ms(uint32_t milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
}

} // namespace yamy::platform
```

STEP 3: Linux Implementation (Stub)

Create src/platform/linux/thread_linux.cpp:
```cpp
#include "core/platform/thread.h"
#include <thread>
#include <chrono>

namespace yamy::platform {

void sleep_ms(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

} // namespace yamy::platform
```

STEP 4: Update cmd_wait.cpp

Modify src/core/commands/cmd_wait.cpp:
```cpp
#include "cmd_wait.h"
// REMOVE: #include <windows.h>
#include "core/platform/thread.h"  // ADD THIS

void Command_Wait::exec(Engine* engine, FunctionParam* param) const {
    // Old: Sleep(m_time);
    yamy::platform::sleep_ms(m_time);  // NEW
}
```

STEP 5: Update CMakeLists.txt

Add platform-specific thread implementation:
```cmake
# In CMakeLists.txt, add to platform sources:
if(WIN32)
    list(APPEND PLATFORM_SOURCES
        src/platform/windows/thread_win32.cpp
    )
elseif(UNIX)
    list(APPEND PLATFORM_SOURCES
        src/platform/linux/thread_linux.cpp
    )
endif()
```

TESTING:
1. Use wait command: execute &Wait(1000)  # Wait 1 second
2. Verify it still waits correctly
3. Verify no windows.h in src/core/:
   ```bash
   grep -r "#include <windows.h>" src/core/
   # Should return 0 results
   ```

ACCEPTANCE CRITERIA:
- cmd_wait.cpp has no #include <windows.h>
- Wait command still works correctly
- Cross-platform abstraction in place
- Both Windows and Linux implementations exist
```

---

## Branch 9: Platform Interface Layer
**Priority:** P1
**Effort:** 10-12 hours
**Branch:** `feat/platform-interface-layer`
**Depends on:** None (but Branch 1 recommended)
**Blocks:** Branches 10, 11

### Prompt for Jules

```
Create abstract platform interfaces to decouple core from Windows-specific types.

LOCATION: src/core/platform/

CREATE NEW FILES:
1. src/core/platform/window_system_interface.h
2. src/core/platform/input_injector_interface.h
3. src/core/platform/input_hook_interface.h
4. src/core/platform/input_driver_interface.h
5. src/core/platform/types.h (platform-agnostic types)

STEP 1: Create Platform Types

File: src/core/platform/types.h
```cpp
#pragma once
#include <cstdint>

namespace yamy::platform {

// Platform-agnostic window handle
using WindowHandle = void*;

// Platform-agnostic point
struct Point {
    int32_t x;
    int32_t y;

    Point() : x(0), y(0) {}
    Point(int32_t x, int32_t y) : x(x), y(y) {}
};

// Platform-agnostic rectangle
struct Rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;

    Rect() : left(0), top(0), right(0), bottom(0) {}
    Rect(int32_t l, int32_t t, int32_t r, int32_t b)
        : left(l), top(t), right(r), bottom(b) {}

    int32_t width() const { return right - left; }
    int32_t height() const { return bottom - top; }
};

// Key codes (abstracted from Windows VK_*)
enum class KeyCode : uint32_t {
    Unknown = 0,
    Escape = 0x1B,
    Space = 0x20,
    // ... define key codes as needed
};

// Mouse buttons
enum class MouseButton {
    Left,
    Right,
    Middle,
    X1,
    X2
};

} // namespace yamy::platform
```

STEP 2: Create IWindowSystem Interface

File: src/core/platform/window_system_interface.h
```cpp
#pragma once
#include "types.h"
#include <string>

namespace yamy::platform {

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    // Window queries
    virtual WindowHandle getForegroundWindow() = 0;
    virtual WindowHandle windowFromPoint(const Point& pt) = 0;
    virtual bool getWindowRect(WindowHandle hwnd, Rect* rect) = 0;
    virtual std::string getWindowText(WindowHandle hwnd) = 0;
    virtual std::string getClassName(WindowHandle hwnd) = 0;

    // Window manipulation
    virtual bool setForegroundWindow(WindowHandle hwnd) = 0;
    virtual bool moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
    virtual bool showWindow(WindowHandle hwnd, int cmdShow) = 0;
    virtual bool closeWindow(WindowHandle hwnd) = 0;

    // Cursor
    virtual void getCursorPos(Point* pt) = 0;
    virtual void setCursorPos(const Point& pt) = 0;

    // Monitor info
    virtual int getMonitorCount() = 0;
    virtual bool getMonitorRect(int monitorIndex, Rect* rect) = 0;
};

// Factory function (implemented per platform)
IWindowSystem* createWindowSystem();

} // namespace yamy::platform
```

STEP 3: Create IInputInjector Interface

File: src/core/platform/input_injector_interface.h
```cpp
#pragma once
#include "types.h"
#include <cstdint>

namespace yamy::platform {

class IInputInjector {
public:
    virtual ~IInputInjector() = default;

    // Keyboard
    virtual void keyDown(KeyCode key) = 0;
    virtual void keyUp(KeyCode key) = 0;

    // Mouse
    virtual void mouseMove(int32_t dx, int32_t dy) = 0;
    virtual void mouseButton(MouseButton button, bool down) = 0;
    virtual void mouseWheel(int32_t delta) = 0;
};

IInputInjector* createInputInjector();

} // namespace yamy::platform
```

STEP 4: Create IInputHook Interface

File: src/core/platform/input_hook_interface.h
```cpp
#pragma once
#include "types.h"
#include <functional>

namespace yamy::platform {

struct KeyEvent {
    KeyCode key;
    bool isKeyDown;
    uint32_t scanCode;
    uint32_t timestamp;
};

using KeyCallback = std::function<bool(const KeyEvent&)>;

class IInputHook {
public:
    virtual ~IInputHook() = default;

    virtual bool install(KeyCallback callback) = 0;
    virtual void uninstall() = 0;
    virtual bool isInstalled() const = 0;
};

IInputHook* createInputHook();

} // namespace yamy::platform
```

STEP 5: Create IInputDriver Interface

File: src/core/platform/input_driver_interface.h
```cpp
#pragma once
#include <cstdint>

namespace yamy::platform {

class IInputDriver {
public:
    virtual ~IInputDriver() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void processEvents() = 0;
    virtual bool isKeyPressed(uint32_t key) const = 0;
};

IInputDriver* createInputDriver();

} // namespace yamy::platform
```

STEP 6: Update Windows Implementations

Modify existing Windows classes to implement interfaces:

File: src/platform/windows/window_system_win32.h
```cpp
#pragma once
#include "core/platform/window_system_interface.h"
#include <windows.h>

namespace yamy::platform {

class WindowSystemWin32 : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override;
    WindowHandle windowFromPoint(const Point& pt) override;
    // ... implement all interface methods

private:
    // Helper: Convert platform WindowHandle to HWND
    static HWND toHWND(WindowHandle handle) {
        return static_cast<HWND>(handle);
    }

    static WindowHandle fromHWND(HWND hwnd) {
        return static_cast<WindowHandle>(hwnd);
    }
};

} // namespace yamy::platform
```

Implement factory in window_system_win32.cpp:
```cpp
IWindowSystem* createWindowSystem() {
    return new WindowSystemWin32();
}
```

Repeat for InputInjector, InputHook, InputDriver.

STEP 7: Update Engine to Use Interfaces

In src/core/engine/engine.h:
```cpp
#include "core/platform/window_system_interface.h"
#include "core/platform/input_injector_interface.h"
// ...

class Engine {
private:
    // Old: WindowSystem* m_windowSystem;
    // New:
    yamy::platform::IWindowSystem* m_windowSystem;
    yamy::platform::IInputInjector* m_inputInjector;
    // ...
public:
    Engine() {
        m_windowSystem = yamy::platform::createWindowSystem();
        m_inputInjector = yamy::platform::createInputInjector();
    }

    ~Engine() {
        delete m_windowSystem;
        delete m_inputInjector;
    }
};
```

TESTING:
1. Verify Engine still compiles
2. Verify application runs
3. Verify no Windows types in interface headers:
   ```bash
   grep -r "HWND\|DWORD" src/core/platform/
   # Should return 0 results
   ```

ACCEPTANCE CRITERIA:
- Abstract interfaces defined
- Windows implementations use interfaces
- Engine uses interfaces (not concrete types)
- No Win32 types in interface headers
- All functionality preserved
```

---

## Branch 10: Win32 Type Audit and Cleanup
**Priority:** P2
**Effort:** 6-8 hours
**Branch:** `feat/win32-type-audit`
**Depends on:** Branch 9

### Prompt for Jules

```
Audit src/core/ for Win32 type leakage and replace with platform-agnostic types.

STEP 1: Find All Win32 Types in Core

Run audit script:
```bash
#!/bin/bash
# save as audit_win32_types.sh

echo "=== Win32 Types in Core ==="
grep -rn "HWND\|DWORD\|WPARAM\|LPARAM\|UINT\|HDC\|HMENU\|HINSTANCE\|HANDLE\|RECT\|POINT\|MSG" \
    src/core/ --include="*.h" --include="*.cpp" | \
    grep -v "// OK:" > /tmp/win32_audit.txt

cat /tmp/win32_audit.txt
echo ""
echo "Total occurrences: $(wc -l < /tmp/win32_audit.txt)"
```

STEP 2: Categorize Findings

Review /tmp/win32_audit.txt and categorize:
1. Types in public headers (HIGH PRIORITY - leaks to other files)
2. Types in private implementation (MEDIUM - can stay if needed)
3. Types that can be replaced with platform types

STEP 3: Replace Types Systematically

Common replacements:
```cpp
// Old → New
HWND → yamy::platform::WindowHandle
DWORD → uint32_t
WPARAM → uintptr_t
LPARAM → intptr_t
UINT → uint32_t
RECT → yamy::platform::Rect
POINT → yamy::platform::Point
MSG → (create yamy::platform::Message if needed)
```

STEP 4: Update Function Signatures

Example in engine.h:
```cpp
// Old:
void setWindow(HWND hwnd);
RECT getWindowRect();

// New:
void setWindow(yamy::platform::WindowHandle hwnd);
yamy::platform::Rect getWindowRect();
```

STEP 5: Update Implementations

Example in engine.cpp:
```cpp
void Engine::setWindow(yamy::platform::WindowHandle hwnd) {
    m_hwnd = hwnd;
    // If need to call Windows API internally:
    HWND win32Hwnd = static_cast<HWND>(hwnd);
    // Use win32Hwnd for Windows API calls
}
```

STEP 6: Special Cases

For MSG (Windows message):
If really needed, create abstraction:
```cpp
// In src/core/platform/types.h
struct Message {
    WindowHandle hwnd;
    uint32_t message;
    uintptr_t wParam;
    intptr_t lParam;
    uint32_t time;
    Point pt;
};
```

STEP 7: Verify No Leakage

After replacements:
```bash
grep -r "HWND\|DWORD\|WPARAM" src/core/*.h | \
    grep -v "platform/types.h"
# Should return minimal/zero results
```

TESTING:
1. Full rebuild: cmake --build build
2. Run all tests
3. Manual testing of key features

ACCEPTANCE CRITERIA:
- No Win32 types in src/core/ public headers
- Minimal Win32 types in implementations (only where necessary)
- All tests pass
- Application runs correctly
```

---

## Branch 11: Linux Stub Backend
**Priority:** P2
**Effort:** 12-16 hours
**Branch:** `feat/linux-stub-backend`
**Depends on:** Branches 9, 10

### Prompt for Jules

```
Create Linux stub implementations to validate platform abstraction.

LOCATION: src/platform/linux/

CREATE NEW FILES:
1. src/platform/linux/window_system_linux.cpp
2. src/platform/linux/input_injector_linux.cpp
3. src/platform/linux/input_hook_linux.cpp
4. src/platform/linux/input_driver_linux.cpp

STEP 1: Window System Stub

File: src/platform/linux/window_system_linux.cpp
```cpp
#include "core/platform/window_system_interface.h"
#include <iostream>

namespace yamy::platform {

class WindowSystemLinux : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override {
        std::cerr << "[STUB] getForegroundWindow()" << std::endl;
        return nullptr;
    }

    WindowHandle windowFromPoint(const Point& pt) override {
        std::cerr << "[STUB] windowFromPoint(" << pt.x << ", " << pt.y << ")" << std::endl;
        return nullptr;
    }

    bool getWindowRect(WindowHandle hwnd, Rect* rect) override {
        std::cerr << "[STUB] getWindowRect()" << std::endl;
        if (rect) {
            *rect = Rect(0, 0, 800, 600); // Default rect
        }
        return false;
    }

    std::string getWindowText(WindowHandle hwnd) override {
        std::cerr << "[STUB] getWindowText()" << std::endl;
        return "Stub Window";
    }

    std::string getClassName(WindowHandle hwnd) override {
        std::cerr << "[STUB] getClassName()" << std::endl;
        return "StubClass";
    }

    bool setForegroundWindow(WindowHandle hwnd) override {
        std::cerr << "[STUB] setForegroundWindow()" << std::endl;
        return false;
    }

    bool moveWindow(WindowHandle hwnd, const Rect& rect) override {
        std::cerr << "[STUB] moveWindow(" << rect.left << "," << rect.top
                  << "," << rect.right << "," << rect.bottom << ")" << std::endl;
        return false;
    }

    bool showWindow(WindowHandle hwnd, int cmdShow) override {
        std::cerr << "[STUB] showWindow()" << std::endl;
        return false;
    }

    bool closeWindow(WindowHandle hwnd) override {
        std::cerr << "[STUB] closeWindow()" << std::endl;
        return false;
    }

    void getCursorPos(Point* pt) override {
        std::cerr << "[STUB] getCursorPos()" << std::endl;
        if (pt) {
            pt->x = 0;
            pt->y = 0;
        }
    }

    void setCursorPos(const Point& pt) override {
        std::cerr << "[STUB] setCursorPos(" << pt.x << ", " << pt.y << ")" << std::endl;
    }

    int getMonitorCount() override {
        std::cerr << "[STUB] getMonitorCount()" << std::endl;
        return 1;
    }

    bool getMonitorRect(int monitorIndex, Rect* rect) override {
        std::cerr << "[STUB] getMonitorRect(" << monitorIndex << ")" << std::endl;
        if (rect) {
            *rect = Rect(0, 0, 1920, 1080);
        }
        return false;
    }
};

// Factory implementation
IWindowSystem* createWindowSystem() {
    std::cerr << "[Linux] Creating stub WindowSystem" << std::endl;
    return new WindowSystemLinux();
}

} // namespace yamy::platform
```

STEP 2: Input Injector Stub

Create similar stub for InputInjector:
- Log all method calls to stderr
- Return safe defaults
- Don't crash

STEP 3: Input Hook Stub

For InputHook (no-op implementation):
```cpp
class InputHookLinux : public IInputHook {
    bool install(KeyCallback callback) override {
        std::cerr << "[STUB] InputHook::install() - not supported on Linux" << std::endl;
        return false;
    }

    void uninstall() override {}
    bool isInstalled() const override { return false; }
};
```

STEP 4: Input Driver Stub

Similar stub for InputDriver.

STEP 5: Update CMakeLists.txt

Add platform selection:
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
        # ... other Windows files
    )
    set(PLATFORM_LIBS user32 gdi32 advapi32)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES
        src/platform/linux/window_system_linux.cpp
        src/platform/linux/input_injector_linux.cpp
        src/platform/linux/input_hook_linux.cpp
        src/platform/linux/input_driver_linux.cpp
        src/platform/linux/thread_linux.cpp
    )
    set(PLATFORM_LIBS pthread)
endif()

add_executable(yamy_engine ${CORE_SOURCES} ${PLATFORM_SOURCES})
target_link_libraries(yamy_engine ${PLATFORM_LIBS})
```

STEP 6: Conditional Compilation for GUI

UI dialogs won't work on Linux. Add guard:
```cpp
// In mayu.cpp
#ifdef _WIN32
    // Initialize Windows GUI
#else
    std::cerr << "GUI not supported on Linux (stub build)" << std::endl;
    return 0;
#endif
```

STEP 7: Linux Build Instructions

Create docs/building-linux.md:
```markdown
# Building YAMY on Linux (Stub)

This is a stub build for architectural validation only.
It will not provide actual functionality.

## Requirements
- GCC 7+ or Clang 6+
- CMake 3.10+

## Build
```bash
mkdir -p build/linux
cd build/linux
cmake ../..
make
```

## Run
```bash
./yamy_engine
# Will print stub messages and exit
```
```

STEP 8: Test Linux Build

On Linux machine (or Docker):
```bash
docker run -it ubuntu:22.04
apt-get update && apt-get install -y build-essential cmake git
git clone <repo>
cd yamy
mkdir build && cd build
cmake ..
make
./yamy_engine  # Should run without crashing
```

ACCEPTANCE CRITERIA:
- Code compiles on Linux without errors
- Executable runs without crashing
- Stub implementations log method calls
- No Windows headers included in Linux build
- README documents Linux build process
```

---

## Execution Workflow

### Week 1: Foundation (Parallel)
```bash
# Start all foundation branches simultaneously
git checkout -b feat/utf-conversion-infrastructure origin/master
# Work on Branch 1...
git push origin feat/utf-conversion-infrastructure

git checkout -b feat/remove-windows-h-from-core origin/master
# Work on Branch 8...
git push origin feat/remove-windows-h-from-core

git checkout -b feat/platform-interface-layer origin/master
# Work on Branch 9...
git push origin feat/platform-interface-layer
```

**Merge order:**
1. Merge Branch 1 first (BLOCKING)
2. Merge Branch 8 (independent)
3. Merge Branch 9

### Week 2-3: Core Migration (Parallel after Branch 1)
```bash
# After Branch 1 is merged, start these in parallel:
git checkout -b feat/engine-string-migration origin/master
git checkout -b feat/keymap-string-migration origin/master
git checkout -b feat/settings-string-migration origin/master
git checkout -b feat/ui-string-migration origin/master
git checkout -b feat/platform-utils-string-migration origin/master
```

**Merge order:** Any order after Branch 1

### Week 3-4: Commands (After Branches 1,2)
```bash
git checkout -b feat/command-string-migration origin/master
# Work on Branch 5...
```

### Week 4-5: PAL Completion (After Branch 9)
```bash
git checkout -b feat/win32-type-audit origin/master
# After Branch 10 merged:
git checkout -b feat/linux-stub-backend origin/master
```

---

## Quality Checks

Before merging each branch:
1. Code compiles without warnings
2. All unit tests pass
3. Manual smoke test
4. Code review
5. CI passes (if configured)

---

## Communication & Coordination

### Daily Standup Topics
- Which branches are in progress?
- Any blockers waiting for dependencies?
- Merge conflicts to resolve?
- Testing status

### Merge Conflict Resolution
If conflicts occur (e.g., same file modified in multiple branches):
1. The later branch rebases on latest master
2. Resolve conflicts manually
3. Re-test after rebase

### Branch Naming Convention
- `feat/` prefix for new features
- Descriptive name matching task
- Example: `feat/engine-string-migration`

---

## Success Criteria

All 11 branches merged successfully:
- [ ] Phase 3 complete: Zero tstring usage
- [ ] Phase 4 complete: Zero windows.h in core
- [ ] Linux stub builds
- [ ] All tests pass
- [ ] Application runs on Windows

Estimated completion: 6-8 weeks with 11 concurrent developers (or 1 developer working serially over 8-12 weeks).
