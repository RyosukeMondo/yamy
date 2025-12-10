# Jules Implementation Guide - All 12 Tracks

**Quick Start:** Copy the prompt for each track below and give to separate Jules instances.

**Base Structure:** Already committed (commit `9a7c76f`)

---

## Track Status & Prompts

### ✅ FOUNDATION TRACKS (Start Here)

**Track 8: Hook Data** - 1 hour - EASIEST
**Track 6: POSIX Synchronization** - 4 hours
**Track 12: Thread Management** - 2 hours

### ✅ WINDOW SYSTEM TRACKS

**Track 1: Window Queries** - 4 hours - See `jules-prompts-linux-tracks.md` (complete)
**Track 2: Window Manipulation** - 4 hours - See `jules-prompts-linux-tracks.md` (complete)
**Track 3: Window Hierarchy** - 3 hours
**Track 4: Mouse & Cursor** - 2 hours
**Track 5: Monitor Support** - 3 hours

### ✅ INPUT SYSTEM TRACKS (Most Complex)

**Track 11: Key Mapping** - 5 hours
**Track 9: evdev Capture** - 8 hours
**Track 10: uinput Injection** - 6 hours

### ✅ IPC TRACK

**Track 7: Unix IPC** - 4 hours

---

## Complete Prompts Below

For **Tracks 1-2**, see `docs/jules-prompts-linux-tracks.md` - they have full detail.

For **Tracks 3-12**, use the prompts below:

---

# TRACK 3: X11 Window Hierarchy

**File:** `src/platform/linux/window_system_linux_hierarchy.cpp`
**Header:** Already created
**Time:** 3 hours

## Jules Prompt

```
Implement Track 3: X11 Window Hierarchy for YAMY Linux port.

Create: src/platform/linux/window_system_linux_hierarchy.cpp

Implement 6 methods using X11 XQueryTree and window properties:

1. getParent() - Use XQueryTree to get parent window
2. isMDIChild() - Check _NET_WM_WINDOW_TYPE for MDI (always false on Linux)
3. isChild() - Use XQueryTree to check if has parent
4. getShowCommand() - Check _NET_WM_STATE for hidden/normal/maximized
5. isConsoleWindow() - Check WM_CLASS for terminal emulators
6. getToplevelWindow() - Walk up parent chain to root

Code template:
- Include: X11/Xlib.h, X11/Xatom.h, X11/Xutil.h
- Helper: getDisplay(), getAtom()
- Use XQueryTree() for hierarchy
- Use XGetWindowProperty() for state

PR Title: `feat(linux): Track 3 - X11 window hierarchy`

Build: g++ ... -lX11
```

---

# TRACK 4: Mouse & Cursor

**File:** `src/platform/linux/window_system_linux_mouse.cpp`
**Time:** 2 hours - SIMPLE

## Jules Prompt

```
Implement Track 4: Mouse & Cursor for YAMY.

Create: src/platform/linux/window_system_linux_mouse.cpp

2 methods only:
1. getCursorPos() - Use XQueryPointer()
2. setCursorPos() - Use XWarpPointer()

Very simple - just wrap X11 cursor functions.

PR Title: `feat(linux): Track 4 - Mouse and cursor support`
```

---

# TRACK 5: Monitor Support

**File:** `src/platform/linux/window_system_linux_monitor.cpp`
**Time:** 3 hours

## Jules Prompt

```
Implement Track 5: Multi-Monitor Support using XRandR.

Create: src/platform/linux/window_system_linux_monitor.cpp

5 methods using XRandR extension:
1. getMonitorFromWindow() - Find monitor containing window
2. getMonitorFromPoint() - Find monitor at coordinates
3. getMonitorRect() - Get monitor dimensions
4. getMonitorWorkArea() - Minus panels (use _NET_WORKAREA)
5. getPrimaryMonitor() - Get primary display

Use: XRRGetScreenResources(), XRRGetOutputInfo(), XRRGetCrtcInfo()

PR Title: `feat(linux): Track 5 - XRandR monitor support`

Link: -lX11 -lXrandr
```

---

# TRACK 6: POSIX Synchronization

**File:** `src/platform/linux/sync_linux.cpp`
**Time:** 4 hours

## Jules Prompt

```
Implement Track 6: POSIX Synchronization primitives.

Create: src/platform/linux/sync_linux.cpp

Implement waitForObject() using:
- sem_t for events (semaphores)
- sem_wait() for infinite wait
- sem_timedwait() for timeout wait
- Handle ETIMEDOUT → WaitResult::Timeout

Include: <semaphore.h>, <pthread.h>, <errno.h>, <time.h>

PR Title: `feat(linux): Track 6 - POSIX synchronization primitives`

Link: -lpthread
```

---

# TRACK 7: Unix IPC

**File:** `src/platform/linux/ipc_linux.cpp`
**Time:** 4 hours

## Jules Prompt

```
Implement Track 7: Unix IPC using domain sockets.

Add method to WindowSystemLinux class:
bool sendCopyData(WindowHandle sender, WindowHandle target,
                 const CopyData& data, uint32_t flags,
                 uint32_t timeout_ms, uintptr_t* result)

Implementation:
1. Create Unix domain socket (AF_UNIX, SOCK_STREAM)
2. Connect to /tmp/yamy_{target_handle}.sock
3. Send: data.id, data.size, data.data
4. Close socket

Include: <sys/socket.h>, <sys/un.h>

PR Title: `feat(linux): Track 7 - Unix domain socket IPC`
```

---

# TRACK 8: Hook Data

**File:** `src/platform/linux/hook_data_linux.cpp`
**Time:** 1 hour - SIMPLEST

## Jules Prompt

```
Implement Track 8: Hook Data accessor - SIMPLEST TRACK.

Create: src/platform/linux/hook_data_linux.cpp

Just create global instance and accessor:

HookData* getHookData() {
    static HookData g_hookData;
    return &g_hookData;
}

That's it! 20 lines total.

PR Title: `feat(linux): Track 8 - Hook data accessor`
```

---

# TRACK 9: evdev Input Capture

**File:** `src/platform/linux/input_hook_linux.cpp`
**Time:** 8 hours - COMPLEX

## Jules Prompt

```
Implement Track 9: evdev keyboard capture - MOST COMPLEX.

Create:
- src/platform/linux/input_hook_linux.cpp
- src/platform/linux/device_manager_linux.cpp

Features:
1. Device enumeration with libudev
2. Identify keyboard devices
3. Open /dev/input/eventX
4. Grab exclusive (EVIOCGRAB ioctl)
5. Read input_event structures in thread
6. Handle device hotplug

Includes: <linux/input.h>, <libudev.h>, <fcntl.h>, <unistd.h>

Key APIs:
- udev_new(), udev_enumerate_new()
- open(), ioctl(EVIOCGRAB)
- read() for struct input_event
- pthread_create() for event thread

PR Title: `feat(linux): Track 9 - evdev keyboard capture`

Link: -ludev -lpthread

Note: Needs root or input group membership
```

---

# TRACK 10: uinput Injection

**File:** `src/platform/linux/input_injector_linux.cpp`
**Time:** 6 hours - COMPLEX

## Jules Prompt

```
Implement Track 10: uinput virtual keyboard - COMPLEX.

Create: src/platform/linux/input_injector_linux.cpp

Features:
1. Create virtual keyboard (/dev/uinput)
2. Set device capabilities (UI_SET_EVBIT, UI_SET_KEYBIT)
3. Configure uinput_user_dev structure
4. Create device (UI_DEV_CREATE ioctl)
5. Inject key events (write input_event)
6. Handle EV_KEY, EV_SYN events

Includes: <linux/uinput.h>, <fcntl.h>

Key APIs:
- open("/dev/uinput")
- ioctl(UI_SET_EVBIT/KEYBIT)
- ioctl(UI_DEV_CREATE)
- write() for struct input_event

PR Title: `feat(linux): Track 10 - uinput key injection`

Note: Needs uinput kernel module loaded
```

---

# TRACK 11: Key Code Mapping

**File:** `src/platform/linux/keycode_mapping.cpp`
**Time:** 5 hours

## Jules Prompt

```
Implement Track 11: evdev ↔ YAMY key code translation.

Create: src/platform/linux/keycode_mapping.cpp

Implement 4 functions:
1. evdevToYamyKeyCode() - Convert evdev → YAMY
2. yamyToEvdevKeyCode() - Convert YAMY → evdev
3. isModifierKey() - Check if modifier
4. getKeyName() - Get key name string

Use large switch/case or lookup tables for:
- Letters (KEY_A=30 → VK_A=0x41)
- Numbers (KEY_1=2 → VK_1=0x31)
- Function keys (KEY_F1=59 → VK_F1=0x70)
- Modifiers (KEY_LEFTCTRL=29 → VK_CONTROL=0x11)
- Special keys (arrows, page up/down, etc.)

Reference: linux/input-event-codes.h

PR Title: `feat(linux): Track 11 - evdev/YAMY keycode mapping`
```

---

# TRACK 12: Thread Management

**File:** `src/platform/linux/thread_linux.cpp`
**Time:** 2 hours - SIMPLE

## Jules Prompt

```
Implement Track 12: pthread wrapper functions.

Create: src/platform/linux/thread_linux.cpp

Implement simple pthread wrappers:
- createThread() → pthread_create()
- joinThread() → pthread_join()
- detachThread() → pthread_detach()
- setThreadPriority() → pthread_setschedparam()

Simple wrapper functions, not much code.

PR Title: `feat(linux): Track 12 - pthread thread management`

Link: -lpthread
```

---

## Integration Instructions

1. **Launch all 12 Jules instances** with above prompts
2. **Collect PRs** as they complete
3. **Merge in order:**
   - Foundation: 8, 6, 12
   - Window: 1, 2, 3, 4, 5
   - Input: 11, 9, 10
   - IPC: 7
4. **Test after each phase**

## Build After All Tracks

```bash
cmake -B build
cmake --build build
```

## Expected Result

After all 12 tracks merge:
- ✅ 46 hours of work done in ~8 hours (parallel)
- ✅ Complete Linux window system
- ✅ Complete input system
- ✅ Full keyboard remapping on Linux
- ✅ X11 support functional

## Success Metrics

- All 12 PRs merged
- Build succeeds on Linux
- Can detect windows
- Can remap keyboard keys
- Zero conflicts (each track independent)
