# Phase 5: Complete Windows Elimination - Implementation Plan

## Objective
Eliminate ALL Windows dependencies from src/core to achieve 100% platform independence.

## Parallel Execution Strategy

### Agent 1: Synchronization Abstraction (5A-1)
**Files:** 3 files
- `src/core/platform/sync.h` (NEW)
- `src/core/commands/cmd_sync.cpp`
- `src/core/engine/engine_lifecycle.cpp`

**Tasks:**
1. Create sync.h with platform-agnostic wait primitives:
```cpp
namespace yamy::platform {
    enum class WaitResult { Success, Timeout, Failed, Abandoned };
    constexpr uint32_t WAIT_INFINITE = 0xFFFFFFFF;

    WaitResult waitForObject(void* handle, uint32_t timeout_ms);
}
```

2. Update cmd_sync.cpp:
```cpp
// Before (line 28-29)
uint32_t r = WaitForSingleObject(i_engine->m_eSync, 5000);
if (r == WAIT_TIMEOUT) {

// After
auto r = yamy::platform::waitForObject(i_engine->m_eSync, 5000);
if (r == yamy::platform::WaitResult::Timeout) {
```

3. Update engine_lifecycle.cpp (3 locations):
```cpp
// Lines 143, 149, 210
// Before
WaitForSingleObject(m_queueMutex, INFINITE);

// After
yamy::platform::waitForObject(m_queueMutex, yamy::platform::WAIT_INFINITE);
```

4. Create Windows implementation in `src/platform/windows/sync.cpp`

**Verification:** Build succeeds, no WaitForSingleObject in src/core

---

### Agent 2: IPC Abstraction (5A-2)
**Files:** 3 files
- `src/core/platform/ipc.h` (NEW)
- `src/core/commands/cmd_direct_sstp.cpp`
- `src/core/window/window_system.h`

**Tasks:**
1. Create ipc.h with platform-agnostic IPC:
```cpp
namespace yamy::platform {
    struct CopyData {
        uint32_t id;
        uint32_t size;
        const void* data;
    };
}
```

2. Add sendCopyData to WindowSystem interface:
```cpp
// window_system.h
virtual bool sendCopyData(WindowHandle target,
                         const CopyData& data,
                         uint32_t flags,
                         uint32_t timeout_ms,
                         uintptr_t* result) = 0;
```

3. Update cmd_direct_sstp.cpp:
```cpp
// Before (lines 163-173)
COPYDATASTRUCT cd;
cd.dwData = 9801;
cd.cbData = (uint32_t)request_UTF_8.size();
cd.lpData = (void *)request_UTF_8.c_str();
i_engine->getWindowSystem()->sendMessageTimeout(..., 0x004A, ...);

// After
yamy::platform::CopyData cd{9801, (uint32_t)request_UTF_8.size(),
                            request_UTF_8.c_str()};
i_engine->getWindowSystem()->sendCopyData(i->second.m_hwnd, cd,
                                          0x0003, 5000, &result);
```

4. Remove windows.h include from cmd_direct_sstp.cpp

**Verification:** Build succeeds, no COPYDATASTRUCT in src/core

---

### Agent 3: Focus Header Cleanup (5A-3)
**Files:** 3 files
- `src/core/window/focus.h`
- `src/core/window/focus.cpp`
- `src/core/platform/message_constants.h` (NEW)

**Tasks:**
1. Create message_constants.h:
```cpp
namespace yamy::platform {
    constexpr uint32_t MSG_APP_BASE = 0x8000;
    constexpr uint32_t MSG_APP_NOTIFY_FOCUS = MSG_APP_BASE + 103;
    constexpr uint32_t MSG_APP_NOTIFY_VKEY = MSG_APP_BASE + 104;
    constexpr uint32_t MSG_COPYDATA = 0x004A;
    constexpr uint32_t MSG_ENGINE_NOTIFY = MSG_APP_BASE + 110;
}
```

2. Update focus.h:
```cpp
// Remove line 9
// #include <windows.h>

// Replace lines 17-18
#include "../platform/message_constants.h"

enum {
    WM_APP_notifyFocus = yamy::platform::MSG_APP_NOTIFY_FOCUS,
    WM_APP_notifyVKey  = yamy::platform::MSG_APP_NOTIFY_VKEY,
};
```

3. Update engine.h to use message_constants.h

**Verification:** focus.h has no windows.h include

---

### Agent 4: Hook Data Abstraction (5B)
**Files:** 6 files
- `src/core/platform/hook_interface.h` (NEW)
- `src/core/commands/cmd_mouse_hook.cpp`
- `src/core/commands/cmd_investigate_command.cpp`
- `src/core/commands/cmd_sync.cpp`
- `src/core/engine/engine.h`

**Tasks:**
1. Create hook_interface.h:
```cpp
namespace yamy::platform {
    struct HookData {
        struct Point { int32_t x, y; };
        Point m_mousePos;
        int32_t m_mouseHookType;
        int32_t m_mouseHookParam;
        uint32_t m_hwndMouseHookTarget;
        bool m_doesNotifyCommand;
        uint8_t m_syncKey;
        bool m_syncKeyIsExtended;
    };

    HookData* getHookData();
}
```

2. Update cmd_mouse_hook.cpp:
```cpp
// Replace g_hookData with yamy::platform::getHookData()
auto* hookData = yamy::platform::getHookData();
hookData->m_mousePos.x = wp.x;
```

3. Update cmd_investigate_command.cpp, cmd_sync.cpp similarly

4. Remove platform/windows/hook.h includes

**Verification:** No g_hookData references, no hook.h includes from platform/windows

---

### Agent 5: WindowSystem API Cleanup (5C)
**Files:** 5 files
- `src/core/window/window_system.h`
- `src/core/commands/cmd_mouse_hook.cpp`
- `src/core/commands/cmd_window_identify.cpp`
- `src/core/engine/engine_lifecycle.cpp`

**Tasks:**
1. Add getToplevelWindow to WindowSystem interface:
```cpp
virtual WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI) = 0;
```

2. Add changeMessageFilter to WindowSystem interface:
```cpp
virtual bool changeMessageFilter(uint32_t message, uint32_t action) = 0;
```

3. Update cmd_mouse_hook.cpp:
```cpp
// Before
getToplevelWindow(target, &isMDI)

// After
i_engine->getWindowSystem()->getToplevelWindow(target, &isMDI)
```

4. Update engine_lifecycle.cpp:
```cpp
// Before (line 55)
pChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);

// After
m_windowSystem->changeMessageFilter(yamy::platform::MSG_COPYDATA, 1);
```

5. Remove platform/windows/windowstool.h includes

**Verification:** No getToplevelWindow direct calls, no windowstool.h includes

---

### Agent 6: Command Files Cleanup (5C)
**Files:** 8 command files
- `cmd_set_foreground_window.cpp`
- `cmd_vk.cpp`
- `cmd_window_hv_maximize.cpp`
- `cmd_window_monitor_to.cpp`
- `cmd_window_move.cpp`
- `cmd_window_move_to.cpp`
- `cmd_window_move_visibly.cpp`
- `cmd_window_resize_to.cpp`

**Tasks:**
1. Remove all `#include "../../platform/windows/*"` includes
2. Replace any direct Windows API calls with WindowSystem interface calls
3. Ensure all use platform-agnostic types

**Verification:** No platform/windows includes in these files

---

## Implementation Order

1. **Launch Agents 1-3 in parallel** (Critical abstractions - 5A)
   - Sync, IPC, Focus header cleanup
   - These are independent and can run simultaneously

2. **Launch Agents 4-5 in parallel** (API cleanup - 5B/5C)
   - Hook data, WindowSystem APIs
   - Depends on Agent 3 completing (message constants)

3. **Launch Agent 6** (Command cleanup - 5C)
   - Can run in parallel with 4-5

4. **Final verification and build**

---

## Success Criteria

- ✅ Zero `windows.h` includes in src/core
- ✅ Zero `platform/windows` includes in src/core
- ✅ Zero Win32 API calls in src/core (except in #ifdef _WIN32 implementation blocks)
- ✅ Zero Win32 types in src/core (except in #ifdef _WIN32 implementation blocks)
- ✅ All abstractions in src/core/platform/
- ✅ Build succeeds on Linux stub
- ✅ All interfaces documented

---

## Files Summary

**New Files (7):**
- `src/core/platform/sync.h`
- `src/core/platform/ipc.h`
- `src/core/platform/message_constants.h`
- `src/core/platform/hook_interface.h`
- `src/platform/windows/sync.cpp`
- `src/platform/windows/ipc.cpp`
- `src/platform/windows/hook_data.cpp`

**Modified Files (~25):**
- Engine: 3 files
- Commands: 15 files
- Window: 3 files
- Platform: 4 files

**Total Effort:** 2-3 hours with 6 parallel agents

---

## Post-Phase 5 State

```
src/core/                      ✅ 100% Platform-agnostic
├── platform/                  ✅ Abstract interfaces only
│   ├── types.h               ✅ Platform types
│   ├── sync.h                ✅ NEW: Sync abstraction
│   ├── ipc.h                 ✅ NEW: IPC abstraction
│   ├── message_constants.h   ✅ NEW: Message constants
│   └── hook_interface.h      ✅ NEW: Hook abstraction
├── commands/                  ✅ No Windows dependencies
├── engine/                    ✅ No Windows dependencies
├── window/                    ✅ No Windows dependencies
└── [other core modules]       ✅ No Windows dependencies

src/platform/windows/          ✅ Windows implementations
├── window_system_impl.cpp    ✅ Windows-specific
├── sync.cpp                  ✅ NEW: Windows sync impl
├── ipc.cpp                   ✅ NEW: Windows IPC impl
└── hook_data.cpp             ✅ NEW: Windows hook impl
```

**Result:** src/core becomes a truly platform-agnostic library, ready for Linux/macOS/BSD implementations.
