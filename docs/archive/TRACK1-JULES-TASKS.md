# Track 1: Core Refactoring - Jules Task Breakdown (60 Tasks)

**Total Tasks:** 60
**Max Parallel:** 60 (all independent in Phase 1-2)
**Estimated Total:** 80 hours
**Per-task Average:** 1.3 hours

---

## Task Organization

### Phase 1: Foundation Layer (32 tasks - ALL PARALLEL)
No dependencies, can all run simultaneously

### Phase 2: Window System Layer (15 tasks - ALL PARALLEL after Phase 1)
Depends on Phase 1 completion

### Phase 3: Integration Layer (13 tasks - ALL PARALLEL after Phase 2)
Depends on Phase 2 completion

---

## PHASE 1: FOUNDATION LAYER (32 Tasks)

### Group A: String Type Cleanup (10 tasks)

#### Task 1: Remove tstring from stringtool.h
**Files:** `src/utils/stringtool.h`
**Changes:**
- Remove all `tstring` typedef
- Remove `_T()` macro usage
- Keep only `std::string` versions of functions
- Update return types: `tstring` → `std::string`

**Acceptance:**
- [ ] No `tstring` references in file
- [ ] All functions use `std::string`
- [ ] File compiles on Linux

**PR Title:** `refactor(utils): Remove tstring from stringtool.h`

---

#### Task 2: Remove tstring from stringtool.cpp
**Files:** `src/utils/stringtool.cpp`
**Changes:**
- Update implementation to match Task 1
- Remove `_TCHAR` usage
- Use `std::string` consistently

**Dependencies:** Task 1
**Acceptance:**
- [ ] Matches stringtool.h interface
- [ ] Compiles on Linux

**PR Title:** `refactor(utils): Remove tstring from stringtool.cpp`

---

#### Task 3: Remove tstring from config_store.h
**Files:** `src/utils/config_store.h`
**Changes:**
- Remove all `tstring` overloads (lines 22, 32, 39, 47, 61, 70, 87, 98, 107)
- Keep only `std::string` versions
- Remove `BYTE*/DWORD` methods (Windows-specific)

**Acceptance:**
- [ ] Only `std::string` methods remain
- [ ] No compilation errors (duplicate overload)
- [ ] Interface simplified

**PR Title:** `refactor(utils): Remove tstring overloads from ConfigStore`

---

#### Task 4: Remove tstring from errormessage.h
**Files:** `src/utils/errormessage.h`
**Changes:**
- Replace `tstringstream` → `std::stringstream`
- Update error formatting to use `std::string`

**Acceptance:**
- [ ] No tstring references
- [ ] Compiles on both platforms

**PR Title:** `refactor(utils): Remove tstring from errormessage.h`

---

#### Task 5: Remove tstring from compiler_specific_func.h
**Files:** `src/utils/compiler_specific_func.h`
**Changes:**
- Change return type: `tstring getCurrentDirectory()` → `std::string`

**Acceptance:**
- [ ] Function signature updated
- [ ] Compiles

**PR Title:** `refactor(utils): Remove tstring from compiler_specific_func`

---

#### Task 6: Remove _T() macro from compiler_specific.h
**Files:** `src/utils/compiler_specific.h`
**Changes:**
- Remove `_T` macro definitions
- Clean up `TCHAR` references

**Acceptance:**
- [ ] No _T macro
- [ ] Compiles

**PR Title:** `refactor(utils): Remove _T macro from compiler_specific.h`

---

#### Task 7: Update misc.h Windows type shims
**Files:** `src/utils/misc.h`
**Changes:**
- Clean up HWND/LPARAM/WPARAM shim definitions
- Use `yamy::platform::WindowHandle` instead
- Keep only what's necessary for transition

**Acceptance:**
- [ ] Cleaner abstraction
- [ ] Compiles on Linux

**PR Title:** `refactor(utils): Clean up Windows type shims in misc.h`

---

#### Task 8: Remove tstring from setting.h
**Files:** `src/core/settings/setting.h`
**Changes:**
- Replace all `tstring` → `std::string`
- Update member variables

**Acceptance:**
- [ ] No tstring in header
- [ ] Compiles

**PR Title:** `refactor(settings): Remove tstring from setting.h`

---

#### Task 9: Remove tstring from setting.cpp
**Files:** `src/core/settings/setting.cpp`
**Changes:**
- Update implementation to match setting.h
- Use `std::string` throughout

**Dependencies:** Task 8
**Acceptance:**
- [ ] Matches header
- [ ] Compiles

**PR Title:** `refactor(settings): Remove tstring from setting.cpp`

---

#### Task 10: Remove tstring from setting_loader.cpp
**Files:** `src/core/settings/setting_loader.cpp`
**Changes:**
- Replace `tstring` → `std::string`
- Update file I/O to use UTF-8 std::string

**Acceptance:**
- [ ] No tstring
- [ ] File parsing works

**PR Title:** `refactor(settings): Remove tstring from setting_loader`

---

### Group B: SW_* Constants (12 tasks)

#### Task 11: Define platform ShowCommand enum
**Files:** `src/core/platform/window_constants.h` (NEW)
**Changes:**
- Create new file with platform-agnostic enum:
```cpp
namespace yamy::platform {
    enum class ShowCommand {
        Hide = 0,
        Normal = 1,
        Minimized = 2,
        Maximized = 3,
        NoActivate = 4,
        Show = 5,
        Minimize = 6,
        MinNoActive = 7,
        NA = 8,
        Restore = 9,
        ShowDefault = 10,
        ForceMinimize = 11
    };
}
```

**Acceptance:**
- [ ] New file created
- [ ] Enum defined
- [ ] Header guards

**PR Title:** `feat(platform): Add platform-agnostic ShowCommand enum`

---

#### Task 12: Replace SW_* in function.h
**Files:** `src/core/functions/function.h`
**Changes:**
- Replace ShowCommandType enum values
- Use `yamy::platform::ShowCommand` values
- Remove SW_* references (12 occurrences)

**Dependencies:** Task 11
**Acceptance:**
- [ ] No SW_* constants
- [ ] Uses platform enum
- [ ] Compiles

**PR Title:** `refactor(functions): Replace SW_* with platform ShowCommand`

---

#### Task 13: Replace SW_* in engine_window.cpp
**Files:** `src/core/engine/engine_window.cpp`
**Changes:**
- Replace 6 SW_* constants
- Use `yamy::platform::ShowCommand::*`

**Dependencies:** Task 11
**Acceptance:**
- [ ] No SW_*
- [ ] Compiles

**PR Title:** `refactor(engine): Replace SW_* in engine_window.cpp`

---

#### Task 14: Replace SW_* in cmd_window_monitor_to.cpp
**Files:** `src/core/commands/cmd_window_monitor_to.cpp`
**Changes:**
- Replace hardcoded `9` with `ShowCommand::Restore`

**Dependencies:** Task 11
**Acceptance:**
- [ ] Uses enum
- [ ] Compiles

**PR Title:** `refactor(commands): Use ShowCommand in cmd_window_monitor_to`

---

#### Task 15-22: Replace SW_* in remaining command files (8 tasks, 1 file each)
**Files:** (one task per file)
- `src/core/commands/cmd_window_maximize.cpp`
- `src/core/commands/cmd_window_minimize.cpp`
- `src/core/commands/cmd_window_close.cpp`
- `src/core/commands/cmd_window_identify.cpp`
- `src/core/commands/cmd_mayu_dialog.cpp`
- `src/core/commands/cmd_help_message.cpp`
- `src/core/commands/cmd_shell_execute.cpp`
- `src/core/commands/cmd_set_foreground_window.cpp`

**Changes:** (each file)
- Find and replace SW_* → ShowCommand::*
- Update includes

**Dependencies:** Task 11
**Acceptance:** (each)
- [ ] No SW_*
- [ ] Compiles

**PR Titles:** `refactor(commands): Use ShowCommand in [cmd_name]`

---

### Group C: Message Constants (10 tasks)

#### Task 23: Define platform message constants
**Files:** `src/core/platform/message_constants.h` (already exists, enhance)
**Changes:**
- Add all WM_APP_* message type enums
- Platform-agnostic message IDs
```cpp
namespace yamy::platform {
    enum class MessageType {
        TaskTrayNotify,
        EngineNotify,
        LogNotify,
        TargetNotify,
        // ... etc
    };
}
```

**Acceptance:**
- [ ] All message types defined
- [ ] No WM_* dependencies

**PR Title:** `feat(platform): Add platform message type enums`

---

#### Task 24-32: Update message usage in files (9 tasks)
**Files:** (one task per file)
- `src/core/window/target.h`
- `src/core/window/target.cpp`
- `src/core/window/focus.h`
- `src/core/window/focus.cpp`
- `src/core/engine/engine.h`
- `src/core/commands/cmd_post_message.cpp`
- `src/core/commands/cmd_mayu_dialog.cpp`
- `src/core/commands/cmd_help_message.cpp`
- `src/core/commands/cmd_load_setting.cpp`

**Changes:** (each file)
- Replace WM_APP_* → `MessageType::*`
- Update message handling

**Dependencies:** Task 23
**Acceptance:** (each)
- [ ] No WM_* constants
- [ ] Compiles

**PR Titles:** `refactor: Use MessageType in [file_name]`

---

## PHASE 2: WINDOW SYSTEM LAYER (15 Tasks)

### Group D: HWND Replacement (8 tasks)

#### Task 33: Create WindowHandle wrapper utilities
**Files:** `src/core/platform/window_handle_utils.h` (NEW)
**Changes:**
- Helper functions for WindowHandle operations
- Type conversions if needed
- Null handle constant

**Acceptance:**
- [ ] Utility functions defined
- [ ] Works on both platforms

**PR Title:** `feat(platform): Add WindowHandle utility functions`

---

#### Task 34-37: Replace HWND in engine files (4 tasks)
**Files:** (one per task)
- `src/core/engine/engine.h`
- `src/core/engine/engine_focus.cpp`
- `src/core/engine/engine_window.cpp`
- `src/core/engine/engine_setting.cpp`

**Changes:** (each)
- Replace `HWND` → `yamy::platform::WindowHandle`
- Update function signatures

**Dependencies:** Task 33
**Acceptance:** (each)
- [ ] No HWND
- [ ] Uses WindowHandle
- [ ] Compiles

**PR Titles:** `refactor(engine): Replace HWND in [file_name]`

---

#### Task 38-41: Replace HWND in function files (4 tasks)
**Files:** (one per task)
- `src/core/functions/function.h`
- `src/core/functions/function.cpp`
- `src/core/functions/function_creator.cpp`
- `src/core/functions/strexpr.cpp`

**Changes:** (each)
- Replace HWND → WindowHandle

**Dependencies:** Task 33
**Acceptance:** (each)
- [ ] No HWND
- [ ] Compiles

**PR Titles:** `refactor(functions): Replace HWND in [file_name]`

---

### Group E: Message System Abstraction (7 tasks)

#### Task 42: Design IPC interface
**Files:** `src/core/platform/ipc_interface.h` (NEW)
**Changes:**
- Define cross-platform IPC interface:
```cpp
namespace yamy::platform {
    class IIPCChannel {
    public:
        virtual ~IIPCChannel() = default;
        virtual void sendMessage(MessageType type, const void* data) = 0;
        virtual void setCallback(std::function<void(MessageType, const void*)> cb) = 0;
    };

    IIPCChannel* createIPCChannel();
}
```

**Acceptance:**
- [ ] Interface defined
- [ ] Platform factory declared

**PR Title:** `feat(platform): Design cross-platform IPC interface`

---

#### Task 43: Implement Linux IPC backend
**Files:** `src/platform/linux/ipc_channel_linux.cpp/h` (NEW)
**Changes:**
- Implement IIPCChannel for Linux
- Use Qt signals or Unix domain socket
- Match interface from Task 42

**Dependencies:** Task 42
**Acceptance:**
- [ ] IIPCChannel implemented
- [ ] Compiles on Linux
- [ ] Basic send/receive works

**PR Title:** `feat(linux): Implement IPC channel for Linux`

---

#### Task 44: Implement Windows IPC adapter
**Files:** `src/platform/windows/ipc_channel_windows.cpp/h` (NEW)
**Changes:**
- Wrap existing PostMessage mechanism
- Implement IIPCChannel
- Maintain Windows behavior

**Dependencies:** Task 42
**Acceptance:**
- [ ] IIPCChannel implemented
- [ ] Compiles on Windows
- [ ] Backward compatible

**PR Title:** `feat(windows): Implement IPC channel adapter`

---

#### Task 45: Refactor msgstream.h to use IPC
**Files:** `src/utils/msgstream.h`
**Changes:**
- Remove `PostMessage()` calls (2 occurrences)
- Use `IIPCChannel` instead
- Remove HWND dependency

**Dependencies:** Task 42, 43
**Acceptance:**
- [ ] No PostMessage
- [ ] Uses IPC interface
- [ ] Compiles on Linux

**PR Title:** `refactor(utils): Use IPC interface in msgstream`

---

#### Task 46: Update msgstream.cpp implementation
**Files:** `src/utils/msgstream.cpp`
**Changes:**
- Match msgstream.h changes

**Dependencies:** Task 45
**Acceptance:**
- [ ] Compiles
- [ ] Works on both platforms

**PR Title:** `refactor(utils): Update msgstream implementation`

---

#### Task 47: Replace PostMessage in engine.h
**Files:** `src/core/engine/engine.h`
**Changes:**
- Remove any remaining PostMessage references
- Use IPC interface

**Dependencies:** Task 42
**Acceptance:**
- [ ] No PostMessage
- [ ] Compiles

**PR Title:** `refactor(engine): Remove PostMessage from engine.h`

---

#### Task 48: Replace SendMessage in focus.cpp
**Files:** `src/core/window/focus.cpp`
**Changes:**
- Replace 3 `SendMessage` calls
- Use IPC or WindowSystem interface

**Dependencies:** Task 42
**Acceptance:**
- [ ] No SendMessage
- [ ] Functionality preserved

**PR Title:** `refactor(window): Replace SendMessage in focus.cpp`

---

## PHASE 3: INTEGRATION LAYER (13 Tasks)

### Group F: Window System Integration (13 tasks)

#### Task 49: Abstract window manipulation in layoutmanager.cpp
**Files:** `src/core/window/layoutmanager.cpp`
**Changes:**
- Replace GetWindowRect → `windowSystem->getWindowRect()`
- Replace MoveWindow → `windowSystem->moveWindow()`
- Remove 16 HWND references
- Use WindowHandle throughout

**Acceptance:**
- [ ] Uses IWindowSystem interface
- [ ] No direct Win32 API calls
- [ ] Compiles on Linux

**PR Title:** `refactor(window): Abstract Win32 API in layoutmanager`

---

#### Task 50: Abstract painting in layoutmanager.cpp
**Files:** `src/core/window/layoutmanager.cpp`
**Changes:**
- Replace BeginPaint/EndPaint
- Replace DrawFrameControl
- Use platform drawing abstraction (or remove if not needed)

**Acceptance:**
- [ ] No GDI calls
- [ ] Compiles

**PR Title:** `refactor(window): Remove GDI from layoutmanager`

---

#### Task 51: Abstract system metrics in layoutmanager.cpp
**Files:** `src/core/window/layoutmanager.cpp`
**Changes:**
- Replace 6 GetSystemMetrics calls
- Use platform interface

**Acceptance:**
- [ ] No GetSystemMetrics
- [ ] Compiles

**PR Title:** `refactor(window): Abstract system metrics in layoutmanager`

---

#### Task 52-54: Refactor target.cpp (3 tasks - split large file)

**Task 52: Abstract window enumeration**
**Files:** `src/core/window/target.cpp`
**Changes:**
- Replace EnumWindows, EnumChildWindows
- Use IWindowSystem interface

**Acceptance:**
- [ ] No EnumWindows
- [ ] Compiles

**PR Title:** `refactor(window): Abstract window enumeration in target.cpp`

---

**Task 53: Abstract window painting**
**Files:** `src/core/window/target.cpp`
**Changes:**
- Remove BeginPaint, EndPaint, DrawIcon (11 calls)
- Remove HDC, HICON, HBRUSH
- Platform abstraction or removal

**Acceptance:**
- [ ] No GDI calls
- [ ] Compiles

**PR Title:** `refactor(window): Remove GDI from target.cpp`

---

**Task 54: Abstract mouse capture**
**Files:** `src/core/window/target.cpp`
**Changes:**
- Replace GetCapture, SetCapture, ReleaseCapture
- Use platform interface

**Acceptance:**
- [ ] No capture API calls
- [ ] Compiles

**PR Title:** `refactor(window): Abstract mouse capture in target.cpp`

---

#### Task 55: Abstract window messages in focus.cpp
**Files:** `src/core/window/focus.cpp`
**Changes:**
- Remove WM_KEYDOWN, WM_SETFOCUS, WM_KILLFOCUS
- Use platform event types

**Acceptance:**
- [ ] No WM_* constants
- [ ] Compiles

**PR Title:** `refactor(window): Abstract window messages in focus.cpp`

---

#### Task 56: Abstract caret in focus.cpp
**Files:** `src/core/window/focus.cpp`
**Changes:**
- Remove CreateCaret, ShowCaret, HideCaret
- Platform abstraction or removal

**Acceptance:**
- [ ] No caret API
- [ ] Compiles

**PR Title:** `refactor(window): Remove caret API from focus.cpp`

---

#### Task 57: Remove WNDCLASS from focus.cpp
**Files:** `src/core/window/focus.cpp`
**Changes:**
- Remove window class registration
- Use platform window creation

**Acceptance:**
- [ ] No WNDCLASS
- [ ] Compiles

**PR Title:** `refactor(window): Remove WNDCLASS from focus.cpp`

---

#### Task 58: Remove WNDCLASS from target.cpp
**Files:** `src/core/window/target.cpp`
**Changes:**
- Remove window class registration
- Platform abstraction

**Acceptance:**
- [ ] No WNDCLASS
- [ ] Compiles

**PR Title:** `refactor(window): Remove WNDCLASS from target.cpp`

---

#### Task 59: Update all command includes
**Files:** All `src/core/commands/cmd_*.cpp` files
**Changes:**
- Remove `#include "windowstool.h"`
- Add platform includes
- Update to use new abstractions

**Acceptance:**
- [ ] No windowstool.h
- [ ] All compile

**PR Title:** `refactor(commands): Update includes for platform abstraction`

---

#### Task 60: Final cleanup and testing
**Files:** Entire codebase
**Changes:**
- Remove any remaining Windows types
- Verify Linux compilation
- Run tests
- Update documentation

**Dependencies:** Tasks 1-59
**Acceptance:**
- [ ] No compilation errors on Linux
- [ ] No Windows-specific types in core
- [ ] Tests pass
- [ ] Documentation updated

**PR Title:** `refactor: Track 1 final cleanup and verification`

---

## Task Execution Strategy

### Batch 1: Phase 1 Foundation (32 tasks)
**Run all 32 tasks in parallel via Jules**
```bash
# All tasks 1-32 can run simultaneously
jules task1.md task2.md task3.md ... task32.md
```

**Wait for:** All 32 PRs complete → Merge all

---

### Batch 2: Phase 2 Window System (15 tasks)
**Run all 15 tasks in parallel after Batch 1 merged**
```bash
# All tasks 33-47 can run simultaneously
jules task33.md task34.md ... task47.md
```

**Wait for:** All 15 PRs complete → Merge all

---

### Batch 3: Phase 3 Integration (13 tasks)
**Run all 13 tasks in parallel after Batch 2 merged**
```bash
# All tasks 48-60 can run simultaneously
jules task48.md task49.md ... task60.md
```

**Wait for:** All 13 PRs complete → Merge all

---

## Progress Tracking

### Phase 1 (Foundation)
- [ ] Tasks 1-10: String cleanup (10 tasks)
- [ ] Tasks 11-22: SW_* constants (12 tasks)
- [ ] Tasks 23-32: Message constants (10 tasks)
**Total: 32 tasks → 32 PRs**

### Phase 2 (Window System)
- [ ] Task 33: WindowHandle utils (1 task)
- [ ] Tasks 34-41: HWND replacement (8 tasks)
- [ ] Tasks 42-48: IPC abstraction (7 tasks)
**Total: 15 tasks → 15 PRs**

### Phase 3 (Integration)
- [ ] Tasks 49-51: layoutmanager (3 tasks)
- [ ] Tasks 52-54: target.cpp (3 tasks)
- [ ] Tasks 55-58: focus.cpp (4 tasks)
- [ ] Tasks 59-60: Cleanup (2 tasks)
**Total: 13 tasks → 13 PRs**

---

## Grand Total

**60 tasks → 60 PRs → 3 batches**

**Timeline:**
- Batch 1 (32 parallel): 2-3 days
- Merge + Batch 2 (15 parallel): 2-3 days
- Merge + Batch 3 (13 parallel): 2-3 days
**Total: 6-9 days with Jules parallelism**

**vs Sequential: 80 hours / 8 hours per day = 10 days**
**Speedup: ~5-8x faster with 60 Jules instances**

---

## Success Metrics

- [ ] All 60 PRs merged
- [ ] Core YAMY compiles on Linux
- [ ] No Windows types in core (HWND, tstring, SW_*, WM_*)
- [ ] All tests pass
- [ ] Ready for Track 2 (Configuration Management)
