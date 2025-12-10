# Linux Parallel Implementation Plan
**For:** Jules (Google AI Agent) parallel execution
**Goal:** Maximum parallelization with zero conflicts

---

## Overview

This plan breaks Linux porting into **12 independent tracks** that can be implemented in parallel without conflicts. Each track has clear boundaries, dependencies, and verification steps.

## Architecture Principle

```
Track Independence Rule:
- Each track modifies DIFFERENT files
- Each track has CLEAR interfaces
- Each track can be tested INDEPENDENTLY
- Each track creates a SEPARATE PR
```

---

## Base Structure Setup

### Prerequisites (Do This First)

Before launching parallel tracks, create base structure:

1. **Create base header files** with empty implementations
2. **Set up build system** for Linux
3. **Create test harness** for validation

This ensures all tracks have a common foundation.

---

## Parallel Tracks (12 Total)

### Track 1: X11 Window Queries (Basic)
**Priority:** P0 (Critical)
**Complexity:** Medium
**Files:** `src/platform/linux/window_system_linux_queries.cpp`
**Estimated Time:** 4 hours

**Methods to Implement (8):**
- `getForegroundWindow()` - Get active window
- `windowFromPoint()` - Window at position
- `getWindowText()` - Window title
- `getTitleName()` - Same as above
- `getClassName()` - Window class
- `getWindowThreadId()` - Thread ID
- `getWindowProcessId()` - Process ID
- `getWindowRect()` - Window dimensions

**Dependencies:** X11 libraries (libx11-dev)
**No dependencies on other tracks**

---

### Track 2: X11 Window Manipulation
**Priority:** P0 (Critical)
**Complexity:** Medium
**Files:** `src/platform/linux/window_system_linux_manipulation.cpp`
**Estimated Time:** 4 hours

**Methods to Implement (6):**
- `setForegroundWindow()` - Activate window
- `moveWindow()` - Position/resize
- `showWindow()` - Show/hide/minimize
- `closeWindow()` - Close window
- `registerWindowMessage()` - Custom messages
- `sendMessageTimeout()` - Send message

**Dependencies:** X11 libraries (libx11-dev)
**No dependencies on other tracks**

---

### Track 3: X11 Window Hierarchy
**Priority:** P1 (High)
**Complexity:** Low
**Files:** `src/platform/linux/window_system_linux_hierarchy.cpp`
**Estimated Time:** 3 hours

**Methods to Implement (6):**
- `getParent()` - Parent window
- `isMDIChild()` - MDI check
- `isChild()` - Child check
- `getShowCommand()` - Window state
- `isConsoleWindow()` - Terminal check
- `getToplevelWindow()` - Root window

**Dependencies:** X11 libraries
**No dependencies on other tracks**

---

### Track 4: X11 Mouse & Cursor
**Priority:** P1 (High)
**Complexity:** Low
**Files:** `src/platform/linux/window_system_linux_mouse.cpp`
**Estimated Time:** 2 hours

**Methods to Implement (2):**
- `getCursorPos()` - Mouse position
- `setCursorPos()` - Set mouse position

**Dependencies:** X11 libraries
**No dependencies on other tracks**

---

### Track 5: X11 Monitor Support
**Priority:** P2 (Medium)
**Complexity:** Medium
**Files:** `src/platform/linux/window_system_linux_monitor.cpp`
**Estimated Time:** 3 hours

**Methods to Implement (5):**
- `getMonitorFromWindow()` - Monitor for window
- `getMonitorFromPoint()` - Monitor at position
- `getMonitorRect()` - Monitor dimensions
- `getMonitorWorkArea()` - Work area (minus taskbar)
- `getPrimaryMonitor()` - Primary display

**Dependencies:** XRandR extension (libxrandr-dev)
**No dependencies on other tracks**

---

### Track 6: POSIX Synchronization
**Priority:** P0 (Critical)
**Complexity:** Medium
**Files:**
- `src/platform/linux/sync_linux.cpp`
- `src/platform/linux/sync_linux.h`
**Estimated Time:** 4 hours

**Functions to Implement:**
- `waitForObject()` - Wait for semaphore/mutex
- Helper: Create semaphore
- Helper: Create mutex
- Helper: Signal semaphore
- Helper: Release mutex

**Dependencies:** pthread library
**No dependencies on other tracks**

---

### Track 7: Unix IPC (Domain Sockets)
**Priority:** P1 (High)
**Complexity:** Medium
**Files:** `src/platform/linux/ipc_linux.cpp`
**Estimated Time:** 4 hours

**Methods to Implement:**
- `sendCopyData()` in WindowSystem
- Helper: Create Unix socket server
- Helper: Send message via socket
- Helper: Receive message from socket

**Dependencies:** Unix socket APIs
**No dependencies on other tracks**

---

### Track 8: Hook Data Accessor
**Priority:** P0 (Critical)
**Complexity:** Low
**Files:** `src/platform/linux/hook_data_linux.cpp`
**Estimated Time:** 1 hour

**Functions to Implement:**
- `getHookData()` - Return global hook data instance
- Global HookData instance initialization

**Dependencies:** None
**No dependencies on other tracks**

---

### Track 9: evdev Input Capture
**Priority:** P0 (Critical)
**Complexity:** High
**Files:**
- `src/platform/linux/input_hook_linux.cpp`
- `src/platform/linux/device_manager_linux.cpp`
**Estimated Time:** 8 hours

**Features to Implement:**
- Device enumeration with libudev
- Keyboard device identification
- Open evdev device
- Grab exclusive access
- Read input events in thread
- Device hotplug handling

**Dependencies:** libudev, Linux input subsystem
**No dependencies on other tracks** (but Track 10 will use output)

---

### Track 10: uinput Key Injection
**Priority:** P0 (Critical)
**Complexity:** High
**Files:** `src/platform/linux/input_injector_linux.cpp`
**Estimated Time:** 6 hours

**Features to Implement:**
- Create uinput virtual keyboard
- Configure device capabilities
- Inject key events
- Inject modifier events
- Sync events properly

**Dependencies:** uinput kernel module
**No dependencies on other tracks** (but uses Track 11 key mapping)

---

### Track 11: Key Code Mapping
**Priority:** P0 (Critical)
**Complexity:** Medium
**Files:**
- `src/platform/linux/keycode_mapping.cpp`
- `src/platform/linux/keycode_mapping.h`
**Estimated Time:** 5 hours

**Features to Implement:**
- evdev keycode → YAMY key code translation table
- YAMY key code → evdev keycode reverse table
- Modifier key handling
- Special key handling (multimedia, etc.)
- Lookup functions

**Dependencies:** None
**No dependencies on other tracks**

---

### Track 12: POSIX Thread Management
**Priority:** P1 (High)
**Complexity:** Low
**Files:** `src/platform/linux/thread_linux.cpp`
**Estimated Time:** 2 hours

**Features to Implement:**
- Create thread wrapper
- Thread join/detach
- Thread priority setting
- Thread affinity (if needed)

**Dependencies:** pthread
**No dependencies on other tracks**

---

## Integration Order

Even though tracks are independent, integrate in this order for easier testing:

```
Phase 1: Foundation (Can test immediately)
├── Track 8: Hook Data (1 hour) ✅
├── Track 6: Synchronization (4 hours) ✅
└── Track 12: Thread Management (2 hours) ✅
    Total: 7 hours

Phase 2: Window System (Can test window queries)
├── Track 1: Window Queries (4 hours) ✅
├── Track 2: Window Manipulation (4 hours) ✅
├── Track 3: Window Hierarchy (3 hours) ✅
├── Track 4: Mouse & Cursor (2 hours) ✅
└── Track 5: Monitor Support (3 hours) ✅
    Total: 16 hours

Phase 3: Input System (Can test key remapping)
├── Track 11: Key Mapping (5 hours) ✅
├── Track 9: evdev Capture (8 hours) ✅
└── Track 10: uinput Injection (6 hours) ✅
    Total: 19 hours

Phase 4: IPC (Optional, for SSTP)
└── Track 7: IPC (4 hours) ✅
    Total: 4 hours

Grand Total: 46 hours sequential
With 12 parallel agents: ~8 hours wall time
```

---

## File Structure After Implementation

```
src/platform/linux/
├── window_system_linux.cpp          (main class, delegates to below)
├── window_system_linux_queries.cpp  (Track 1)
├── window_system_linux_manipulation.cpp (Track 2)
├── window_system_linux_hierarchy.cpp (Track 3)
├── window_system_linux_mouse.cpp    (Track 4)
├── window_system_linux_monitor.cpp  (Track 5)
├── sync_linux.cpp                   (Track 6)
├── sync_linux.h                     (Track 6)
├── ipc_linux.cpp                    (Track 7)
├── hook_data_linux.cpp              (Track 8)
├── input_hook_linux.cpp             (Track 9)
├── device_manager_linux.cpp         (Track 9)
├── input_injector_linux.cpp         (Track 10)
├── keycode_mapping.cpp              (Track 11)
├── keycode_mapping.h                (Track 11)
└── thread_linux.cpp                 (Track 12)
```

---

## Verification Strategy

Each track should include:

1. **Unit Tests** - Test individual functions
2. **Integration Test** - Test with simple main()
3. **Build Verification** - Ensure it compiles
4. **Documentation** - Document what was implemented

### Example Verification (Track 1)

```cpp
// test_track1.cpp
int main() {
    WindowSystemLinux ws;

    // Test getForegroundWindow
    WindowHandle hwnd = ws.getForegroundWindow();
    if (hwnd) {
        std::cout << "✅ getForegroundWindow works" << std::endl;

        // Test getWindowText
        std::string title = ws.getWindowText(hwnd);
        std::cout << "✅ getWindowText: " << title << std::endl;

        // Test getClassName
        std::string className = ws.getClassName(hwnd);
        std::cout << "✅ getClassName: " << className << std::endl;
    }

    return 0;
}
```

Build and run:
```bash
g++ -o test_track1 test_track1.cpp \
    src/platform/linux/window_system_linux_queries.cpp \
    -lX11 -I src/
./test_track1
```

---

## Conflict Prevention

### Rule 1: One File Per Track
Each track works on DIFFERENT files. No two tracks modify the same file.

### Rule 2: Clear Interfaces
All tracks implement methods defined in:
- `src/core/platform/window_system_interface.h` (already exists)
- `src/core/platform/sync.h` (already exists)
- `src/core/platform/ipc.h` (already exists)
- `src/core/platform/hook_interface.h` (already exists)

No interface changes needed!

### Rule 3: No Shared Globals
Each track manages its own state. Use proper encapsulation.

### Rule 4: Separate PR Per Track
Each track creates its own PR:
- `feat(linux): Track 1 - X11 window queries`
- `feat(linux): Track 2 - X11 window manipulation`
- etc.

This allows:
- Independent review
- Independent merging
- Easy rollback if needed

---

## Next Steps

1. **Create Base Structure** (see next document)
2. **Generate Jules Prompts** (one per track)
3. **Launch Parallel Tracks** (all 12 at once)
4. **Integrate PRs** (in phase order)
5. **Test Complete System**

Ready to proceed?
