# Windows Dependency Analysis - src/core

## Executive Summary

Despite Phase 3 & 4 progress, **src/core still has significant Windows dependencies** that prevent true cross-platform portability.

### Dependency Counts
- **6 files** with direct `windows.h` includes in src/core
- **16 files** including `platform/windows` headers
- **70 Win32 type references** (HWND, DWORD, HDC, RECT, etc.)
- **43 Win32 API function calls**
- **17 #ifdef _WIN32** conditional blocks

### Critical Finding
**The abstraction is incomplete.** While we've added platform types, many files still:
1. Include windows.h directly
2. Call Win32 APIs without abstraction
3. Use Windows-specific types (HDC, PAINTSTRUCT, COPYDATASTRUCT)
4. Depend on Windows synchronization primitives (WaitForSingleObject)

---

## Category 1: Direct windows.h Includes (6 files)

### 🔴 CRITICAL: window/focus.h
```cpp
#include <windows.h>  // Line 9

enum {
    WM_APP_notifyFocus = WM_APP + 103,  // Uses Windows constant
    WM_APP_notifyVKey  = WM_APP + 104,
};
```
**Impact:** Header file leaking Windows dependency to all consumers
**Solution:** Define WM_APP locally if needed, remove windows.h include

### 🔴 commands/cmd_direct_sstp.cpp
```cpp
// Line 163
COPYDATASTRUCT cd;  // Windows-specific inter-process communication
cd.dwData = 9801;
cd.cbData = (uint32_t)request_UTF_8.size();
cd.lpData = (void *)request_UTF_8.c_str();

// Line 169
i_engine->getWindowSystem()->sendMessageTimeout(..., 0x004A, ...);  // WM_COPYDATA
```
**Impact:** Hard dependency on Windows IPC mechanism
**Solution:** Abstract COPYDATA into platform IPC layer

### 🟡 commands/cmd_mouse_hook.cpp
**Impact:** Includes windows.h (indirectly via platform/windows/hook.h)
**Solution:** Move hook data to abstract interface

### 🟡 commands/cmd_window_identify.cpp
**Impact:** Includes windows.h (indirectly)
**Solution:** Abstract window identification

### 🟡 commands/cmd_investigate_command.cpp
**Impact:** Uses g_hookData from platform/windows/hook.h
**Solution:** Abstract hook data interface

### 🟡 commands/cmd_sync.cpp
```cpp
// Line 28
uint32_t r = WaitForSingleObject(i_engine->m_eSync, 5000);
if (r == WAIT_TIMEOUT) {
    // error handling
}
```
**Impact:** Direct Windows synchronization call
**Solution:** Abstract wait/sync primitives

---

## Category 2: platform/windows Includes (16 files)

All of these files depend on Windows-specific implementations:

### Commands (12 files)
1. `cmd_direct_sstp.cpp` - windowstool.h for loadString
2. `cmd_investigate_command.cpp` - hook.h for g_hookData
3. `cmd_mouse_hook.cpp` - hook.h, windowstool.h
4. `cmd_set_foreground_window.cpp` - Windows focus manipulation
5. `cmd_sync.cpp` - hook.h for sync key
6. `cmd_vk.cpp` - Virtual key handling
7. `cmd_window_hv_maximize.cpp` - Window manipulation
8. `cmd_window_identify.cpp` - hook.h, windowstool.h
9. `cmd_window_monitor_to.cpp` - Monitor APIs
10. `cmd_window_move.cpp` - Window positioning
11. `cmd_window_move_to.cpp` - Window positioning
12. `cmd_window_move_visibly.cpp` - Window positioning
13. `cmd_window_resize_to.cpp` - Window resizing

### Engine (2 files)
14. `engine_focus.cpp` - utf_conversion.h for string conversion
15. `engine_window.cpp` - Window management

### Functions (1 file)
16. `function.cpp` - Type conversion utilities

---

## Category 3: Win32 Type Usage (70 instances)

### Window Types
- **HWND**: Window handles (should use WindowHandle everywhere)
- **HDC**: Device context for drawing
- **PAINTSTRUCT**: Paint operation data
- **RECT**: Rectangle structures
- **POINT**: Point structures

### Message Types
- **WPARAM/LPARAM**: Message parameters (partially abstracted)
- **MSG**: Message structure
- **UINT**: Message identifiers

### Handle Types
- **HANDLE**: Generic handles (partially abstracted)
- **HPEN/HBRUSH**: GDI object handles
- **ATOM**: Window class registration

### Data Types
- **DWORD**: 32-bit unsigned (should be uint32_t)
- **LONG**: 32-bit signed (should be int32_t)
- **ULONG_PTR**: Pointer-sized integer (should be uintptr_t)

---

## Category 4: Win32 API Calls (43 instances)

### Synchronization (4 calls)
```cpp
WaitForSingleObject(handle, timeout)  // cmd_sync.cpp, engine_lifecycle.cpp
WAIT_TIMEOUT constant
INFINITE constant
```
**Solution:** Create platform::wait_for_object() abstraction

### Window Management (20+ calls)
```cpp
GetWindowDC(), ReleaseDC()           // GDI device context
BeginPaint(), EndPaint()             // Paint operations
GetWindowRect(), GetClientRect()     // Window dimensions
GetCapture(), SetCapture()           // Mouse capture
InvalidateRect(), UpdateWindow()     // Window refresh
GetWindowLongPtr(), GetWindowLong()  // Window properties
IsWindowVisible()                    // Visibility check
EnumChildWindows()                   // Child enumeration
GetDesktopWindow()                   // Desktop handle
SendMessage(), PostMessage()         // Message sending
```
**Solution:** Most are in target.cpp/layoutmanager.cpp - keep Windows implementation but isolate

### IPC (2 calls)
```cpp
ChangeWindowMessageFilter()          // Security for WM_COPYDATA
SendMessageTimeout()                 // IPC with timeout
```
**Solution:** Abstract IPC mechanism

### GDI (4 calls)
```cpp
GetStockObject()                     // Standard GDI objects
SelectObject()                       // Select into DC
```
**Solution:** Keep in Windows rendering implementation

---

## Category 5: Windows-Specific Constants

### Message Constants
```cpp
WM_APP          // Base for custom messages (0x8000)
WM_COPYDATA     // 0x004A - IPC message
SMTO_ABORTIFHUNG, SMTO_BLOCK  // SendMessageTimeout flags
MSGFLT_ADD      // Message filter constant
```

### Synchronization Constants
```cpp
INFINITE        // Wait forever
WAIT_TIMEOUT    // Wait timed out
```

### GDI Constants
```cpp
WHITE_PEN, NULL_BRUSH  // Stock objects
GWL_STYLE, WS_CHILD    // Window styles
```

---

## Breakdown by File Severity

### 🔴 CRITICAL (Must Fix for Cross-Platform)

**window/focus.h**
- Direct windows.h include in header
- Leaks to all consumers
- Uses WM_APP constants

**commands/cmd_direct_sstp.cpp**
- COPYDATASTRUCT usage (Windows IPC)
- Hardcoded WM_COPYDATA message
- No abstraction path

**commands/cmd_sync.cpp**
- WaitForSingleObject() call
- WAIT_TIMEOUT constant
- No sync abstraction

**engine/engine_lifecycle.cpp**
- WaitForSingleObject() calls
- CreateThread/CreateMutex/CreateEvent calls
- ChangeWindowMessageFilter() call

### 🟡 HIGH (Windows-Specific but Isolated)

**window/target.cpp**
- 20+ Windows GDI calls
- But: Only used for Windows rendering
- Solution: Keep as Windows impl, create abstract interface

**window/layoutmanager.cpp**
- GetWindowRect, GetWindowLong calls
- Windows-specific layout logic
- Solution: Abstract layout interface

**window/focus.cpp**
- SendMessage calls with WM_APP
- Windows message loop integration
- Solution: Abstract focus notification

### 🟢 LOW (Already Abstracted or Minimal Impact)

**engine/engine.cpp**
- Uses abstracted types mostly
- Few Windows constants (WM_APP + 201)
- Solution: Define constants in platform layer

**Most command files**
- Use WindowSystem abstraction mostly
- Call getToplevelWindow() from windowstool.h
- Solution: Move getToplevelWindow to WindowSystem interface

---

## Recommended Elimination Strategy

### Phase 5A: Critical Abstractions (Required for Cross-Platform)

1. **Synchronization Abstraction**
   ```cpp
   // Add to platform/types.h
   namespace yamy::platform {
       enum class WaitResult { Success, Timeout, Failed };
       WaitResult waitForObject(Handle handle, uint32_t timeout_ms);
       constexpr uint32_t WAIT_INFINITE = 0xFFFFFFFF;
   }
   ```
   - Replace WaitForSingleObject in cmd_sync.cpp
   - Replace WaitForSingleObject in engine_lifecycle.cpp

2. **Message Constants Abstraction**
   ```cpp
   // Add to platform/types.h
   namespace yamy::platform {
       constexpr uint32_t MSG_APP_BASE = 0x8000;
       constexpr uint32_t MSG_COPYDATA = 0x004A;
   }
   ```
   - Remove windows.h from focus.h
   - Define WM_APP locally

3. **IPC Abstraction**
   ```cpp
   // Add to WindowSystem interface
   virtual bool sendData(WindowHandle target, uint32_t id,
                        const void* data, size_t size) = 0;
   ```
   - Abstract COPYDATASTRUCT in cmd_direct_sstp.cpp
   - Implement for Windows, stub for Linux

### Phase 5B: Header Cleanup

4. **Remove windows.h from focus.h**
   - Define message constants locally
   - Remove WM_APP dependency

5. **Abstract g_hookData**
   - Create HookDataInterface
   - Remove direct hook.h includes from commands

### Phase 5C: API Cleanup

6. **Move getToplevelWindow to WindowSystem**
   - Add to WindowSystem interface
   - Remove windowstool.h includes from commands

7. **Abstract window message filtering**
   - Wrap ChangeWindowMessageFilter
   - Add to platform initialization

### Phase 5D: Isolation (Optional, Windows-Specific)

8. **Keep Windows-Specific Rendering**
   - target.cpp GDI calls are OK (Windows renderer)
   - layoutmanager.cpp window APIs are OK (Windows layout)
   - These are implementation details of Windows WindowSystem

---

## Files That Can Never Be Fully Platform-Agnostic

These files implement Windows-specific functionality and **should remain Windows-only**:

1. **window/target.cpp** - Windows window rendering using GDI
2. **window/layoutmanager.cpp** - Windows window layout management
3. **window/focus.cpp** - Windows focus tracking
4. **commands using monitor APIs** - Windows multi-monitor support

**Solution:** These belong in `platform/windows` directory, not `src/core`.

---

## Proposed Directory Restructure

```
src/core/
├── platform/
│   ├── types.h              // All platform types
│   ├── sync.h               // Sync abstractions
│   ├── ipc.h                // IPC abstractions
│   └── window_system.h      // Window abstraction interface
└── [rest of core files]

src/platform/windows/
├── window_system_impl.cpp   // Windows WindowSystem
├── target.cpp               // Windows rendering
├── layoutmanager.cpp        // Windows layout
├── focus.cpp                // Windows focus
└── hook.cpp                 // Windows hooks

src/platform/linux/ [future]
├── window_system_impl.cpp   // X11/Wayland WindowSystem
└── [Linux implementations]
```

---

## Summary

### Current State
- **Abstraction Progress:** ~40% (types abstracted, but APIs not abstracted)
- **Cross-Platform Ready:** ❌ NO
- **Windows Dependencies:** High (70 type refs, 43 API calls, 6 direct includes)

### To Achieve 100% Freedom from Windows in src/core

**Must Complete:**
1. ✅ Abstract all Win32 types (mostly done)
2. ❌ Abstract synchronization (WaitForSingleObject)
3. ❌ Abstract IPC (COPYDATASTRUCT)
4. ❌ Remove windows.h from headers
5. ❌ Abstract hook data interface
6. ❌ Move Windows-specific implementations to platform/windows

**Can Keep in src/core with #ifdef _WIN32:**
- Message constants (if defined locally)
- Windows callback signatures (if in .cpp files only)

**Must Move to src/platform/windows:**
- GDI rendering (target.cpp)
- Window layout management (layoutmanager.cpp)
- Windows-specific commands

### Effort Estimate
- **Phase 5A (Critical):** ~6-8 files, 3-4 hours
- **Phase 5B (Headers):** ~2-3 files, 1-2 hours
- **Phase 5C (API):** ~12-15 files, 4-5 hours
- **Phase 5D (Restructure):** ~8-10 files, 2-3 hours
- **Total:** ~30 files, 10-14 hours work (or 2-3 hours with parallel agents)

---

## Conclusion

**We are NOT 100% free from Windows.** Phase 4 completed type abstraction, but **API abstraction** and **architectural separation** remain incomplete.

To truly achieve cross-platform freedom:
1. Complete synchronization/IPC abstractions
2. Remove all windows.h includes from src/core
3. Move Windows-specific implementations to src/platform/windows
4. Create clean interface boundaries

**Recommendation:** Proceed with Phase 5 (API Abstraction & Architectural Separation) to achieve true platform independence.
