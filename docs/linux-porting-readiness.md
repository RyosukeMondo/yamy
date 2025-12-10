# Linux Porting Readiness Assessment

**Assessment Date:** 2025-12-10
**After:** Phase 3, 4, 5 completion
**Question:** Is this the right timing to implement Linux porting?

---

## Answer: YES! Perfect Timing ✅

### Why Now Is Ideal

#### 1. All Abstractions Complete
- ✅ Phase 3: UTF-8 string handling throughout
- ✅ Phase 4: Platform types fully abstracted
- ✅ Phase 5: All Windows APIs abstracted
- ✅ **Zero Windows dependencies** in src/core
- ✅ **Clean interfaces** ready to implement

#### 2. Proven Architecture
The Windows implementation **validates** that all interfaces work:
- WindowSystem: 20+ methods tested in production
- Synchronization: Working with Windows primitives
- IPC: SSTP protocol functional
- Hook system: Keyboard/mouse hooks operational

#### 3. Infrastructure Ready
- ✅ CMake build system supports Linux
- ✅ Linux stubs already in place
- ✅ Directory structure established
- ✅ Build target exists (`yamy_stub`)

#### 4. Isolated Implementation
Linux code won't affect Windows at all:
```
src/platform/linux/     ← Implement here
src/core/              ← Never touch (platform-agnostic)
src/platform/windows/  ← Keep working
```

---

## Current Linux Implementation Status

### Files Present (6 files)
```
src/platform/linux/
├── window_system_linux.cpp    (319 lines, 66 stubs)
├── input_hook_linux.cpp       (stubs)
├── input_driver_linux.cpp     (stubs)
├── input_injector_linux.cpp   (stubs)
├── thread_linux.cpp           (stubs)
└── hook.h                     (header)
```

### Stub Implementations
- **70 total stubs** marked with `[STUB]`
- All return safe defaults (nullptr, false, empty strings)
- All log to stderr for debugging
- Ready to be replaced with real implementations

### Example Stub (WindowSystem)
```cpp
WindowHandle getForegroundWindow() override {
    std::cerr << "[STUB] getForegroundWindow()" << std::endl;
    return nullptr;  // ← Replace with X11/Wayland implementation
}
```

---

## What Needs to Be Implemented

### Phase 6A: Window System (X11/Wayland)

**Complexity:** High
**Estimated Effort:** 3-5 days
**Priority:** Critical (core functionality)

#### Option 1: X11 Implementation
```cpp
// Use Xlib or XCB
#include <X11/Xlib.h>
#include <X11/Xatom.h>

WindowHandle getForegroundWindow() {
    Display* display = XOpenDisplay(nullptr);
    Window focus;
    int revert;
    XGetInputFocus(display, &focus, &revert);
    XCloseDisplay(display);
    return reinterpret_cast<WindowHandle>(focus);
}
```

**Pros:**
- Mature, well-documented
- Works on most Linux systems
- Many examples available

**Cons:**
- X11 is being replaced by Wayland
- Some features may not work in Wayland apps

#### Option 2: Wayland Implementation
```cpp
// Use wlroots or KWin protocols
#include <wayland-client.h>

// Note: Wayland has no concept of "global" window focus
// Would need compositor-specific protocols
```

**Pros:**
- Modern, future-proof
- Better security model

**Cons:**
- No standard window management protocol
- Compositor-specific (GNOME Shell, KWin, etc.)
- More complex

#### Recommended: Hybrid Approach
- Implement X11 first (works everywhere)
- Detect Wayland and use XWayland compatibility
- Add native Wayland support later (Phase 7)

**Methods to Implement (20+):**
1. getForegroundWindow() - `_NET_ACTIVE_WINDOW`
2. getWindowRect() - XGetWindowAttributes
3. getWindowText() - `_NET_WM_NAME`
4. getClassName() - `WM_CLASS`
5. setForegroundWindow() - XSetInputFocus
6. moveWindow() - XMoveResizeWindow
7. showWindow() - XMapWindow/XUnmapWindow
8. closeWindow() - Send WM_DELETE_WINDOW
9. ... (12 more methods)

---

### Phase 6B: Synchronization (POSIX)

**Complexity:** Low
**Estimated Effort:** 1-2 days
**Priority:** High

#### Implementation
```cpp
// src/platform/linux/sync_linux.cpp
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

namespace yamy::platform {

WaitResult waitForObject(void* handle, uint32_t timeout_ms) {
    // Assume handle is a sem_t* for events
    sem_t* sem = static_cast<sem_t*>(handle);

    if (timeout_ms == WAIT_INFINITE) {
        // Wait indefinitely
        if (sem_wait(sem) == 0) {
            return WaitResult::Success;
        }
        return WaitResult::Failed;
    } else {
        // Wait with timeout
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;

        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

        int result = sem_timedwait(sem, &ts);
        if (result == 0) {
            return WaitResult::Success;
        } else if (errno == ETIMEDOUT) {
            return WaitResult::Timeout;
        }
        return WaitResult::Failed;
    }
}

} // namespace yamy::platform
```

**Primitives Needed:**
- pthread_mutex_t for mutexes
- sem_t for events
- pthread_t for threads
- pthread_cond_t for condition variables

---

### Phase 6C: IPC (Unix Domain Sockets or D-Bus)

**Complexity:** Medium
**Estimated Effort:** 2-3 days
**Priority:** Medium (only for SSTP protocol)

#### Option 1: Unix Domain Sockets
```cpp
bool WindowSystemLinux::sendCopyData(WindowHandle target,
                                     const CopyData& data,
                                     uint32_t flags,
                                     uint32_t timeout_ms,
                                     uintptr_t* result) {
    // Create Unix domain socket
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path),
             "/tmp/yamy_%p.sock", target);

    // Connect and send data
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sockfd, &data.id, sizeof(data.id), 0);
        send(sockfd, &data.size, sizeof(data.size), 0);
        send(sockfd, data.data, data.size, 0);
        close(sockfd);
        return true;
    }

    close(sockfd);
    return false;
}
```

#### Option 2: D-Bus
```cpp
// Use libdbus or GDBus
#include <dbus/dbus.h>

bool sendCopyData(...) {
    DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, nullptr);
    DBusMessage* msg = dbus_message_new_method_call(
        "com.yamy.Service",
        "/com/yamy/Object",
        "com.yamy.Interface",
        "CopyData"
    );
    // Append data and send
    dbus_connection_send(conn, msg, nullptr);
    dbus_message_unref(msg);
    return true;
}
```

**Recommended:** Unix domain sockets (simpler, no dependencies)

---

### Phase 6D: Input System (evdev/uinput)

**Complexity:** Very High
**Estimated Effort:** 5-7 days
**Priority:** Critical (core functionality)

This is the **most complex** part because YAMY is a keyboard remapper.

#### Challenge: Low-Level Keyboard Access

**Windows approach:**
- Uses SetWindowsHookEx(WH_KEYBOARD_LL, ...)
- Global keyboard hook at low level
- Can intercept and modify keys before apps see them

**Linux approach:**
- **evdev** - Read raw input events from `/dev/input/eventX`
- **uinput** - Create virtual input device to inject events
- Requires root or appropriate permissions

#### Implementation Strategy

```cpp
// src/platform/linux/input_hook_linux.cpp
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>

class InputHookLinux : public IInputHook {
private:
    int m_fd_keyboard;      // evdev keyboard device
    int m_fd_uinput;        // uinput virtual device
    pthread_t m_thread;     // Input thread

public:
    bool initialize() override {
        // 1. Find keyboard device
        m_fd_keyboard = open("/dev/input/by-id/...-kbd", O_RDONLY);
        if (m_fd_keyboard < 0) {
            return false;  // Need permissions
        }

        // 2. Grab exclusive access (prevent other apps from seeing keys)
        ioctl(m_fd_keyboard, EVIOCGRAB, 1);

        // 3. Create uinput virtual keyboard
        m_fd_uinput = open("/dev/uinput", O_WRONLY);
        // ... setup uinput device ...

        // 4. Start input thread
        pthread_create(&m_thread, nullptr, inputThread, this);

        return true;
    }

    static void* inputThread(void* arg) {
        InputHookLinux* self = static_cast<InputHookLinux*>(arg);

        while (true) {
            struct input_event ev;
            read(self->m_fd_keyboard, &ev, sizeof(ev));

            if (ev.type == EV_KEY) {
                // Process key event
                // Apply remapping
                // Inject modified event via uinput
                write(self->m_fd_uinput, &ev, sizeof(ev));
            }
        }

        return nullptr;
    }
};
```

#### Challenges

1. **Permission Requirements**
   - Need access to `/dev/input/eventX` (root or input group)
   - Need access to `/dev/uinput` (root or uinput group)
   - Solution: setuid binary or systemd service

2. **Device Detection**
   - Multiple input devices
   - Need to identify keyboard(s)
   - Hot-plug support (USB keyboards)
   - Solution: Use libudev for device enumeration

3. **Multiple Keyboards**
   - System may have multiple keyboards
   - Need to hook all of them
   - Solution: Monitor all input devices

4. **Key Mapping**
   - evdev keycodes ≠ Windows virtual keys
   - Need translation table
   - Some keys may not exist on Linux

5. **Wayland Considerations**
   - Wayland has no global keyboard hook
   - Would need compositor support
   - May not work in Wayland sessions

---

### Phase 6E: Hook Data (Simple)

**Complexity:** Low
**Estimated Effort:** 1 day
**Priority:** High

```cpp
// src/platform/linux/hook_data_linux.cpp
#include "core/platform/hook_interface.h"

namespace yamy::platform {

static HookData g_hookData_linux;

HookData* getHookData() {
    return &g_hookData_linux;
}

} // namespace yamy::platform
```

Simple - just need a global instance.

---

## Implementation Roadmap

### Phase 6A: Foundation (Week 1)
**Goal:** Get basic window management working

1. **Day 1-2:** Implement X11 WindowSystem
   - getForegroundWindow, getWindowText, getClassName
   - Basic window queries working

2. **Day 3:** Implement POSIX synchronization
   - waitForObject with pthread/semaphore
   - Basic threading working

3. **Day 4-5:** Test & Debug
   - Build on real Linux system
   - Test window enumeration
   - Fix crashes and issues

**Deliverable:** YAMY can detect windows and basic sync works

---

### Phase 6B: Input System (Week 2-3)
**Goal:** Keyboard remapping functional

1. **Day 1-3:** Implement evdev input capture
   - Device enumeration with libudev
   - Keyboard device identification
   - Read input events

2. **Day 4-6:** Implement uinput injection
   - Virtual keyboard creation
   - Event injection
   - Key remapping logic

3. **Day 7-10:** Integrate with YAMY engine
   - Hook callbacks
   - Key processing
   - Modifier handling

4. **Day 11-14:** Test & Debug
   - Test key remapping
   - Test modifier keys
   - Fix edge cases

**Deliverable:** Basic keyboard remapping works on Linux

---

### Phase 6C: Advanced Features (Week 4)
**Goal:** Feature parity with Windows version

1. **Day 1-2:** Implement IPC (if needed)
   - Unix domain sockets
   - Message passing

2. **Day 3-4:** Window manipulation
   - moveWindow, showWindow
   - Window positioning

3. **Day 5-7:** Testing & Polish
   - Edge cases
   - Performance optimization
   - Documentation

**Deliverable:** Linux version feature-complete

---

## Effort Estimation

### Total Effort
```
Phase 6A (Foundation):     5 days
Phase 6B (Input System):  14 days
Phase 6C (Advanced):       7 days
Testing & Polish:          4 days
-----------------------------------
Total:                    30 days (1 month)
```

### With Parallel Agents
If we use parallel implementation like Phases 3-5:
- Foundation + IPC: Agent 1
- Synchronization: Agent 2
- Input system (part 1): Agent 3
- Input system (part 2): Agent 4

Could reduce to **2-3 weeks** with parallel work.

---

## Technical Challenges

### 1. Permission Model ⚠️ High Risk
**Challenge:** Linux requires elevated privileges for keyboard access

**Solutions:**
- **Option A:** setuid root binary (security risk)
- **Option B:** systemd service running as root
- **Option C:** Add user to `input` and `uinput` groups
- **Option D:** Use capabilities (`CAP_SYS_ADMIN`)

**Recommended:** Option C (user group) + capabilities

### 2. Wayland Compatibility ⚠️ Medium Risk
**Challenge:** Wayland has no global keyboard hook

**Solutions:**
- **Phase 1:** X11 only (works via XWayland)
- **Phase 2:** Compositor-specific protocols
- **Phase 3:** Lobbying for standard protocol

**Recommended:** Start with X11, add Wayland later

### 3. Multiple Desktop Environments 🔵 Low Risk
**Challenge:** GNOME, KDE, XFCE have different behaviors

**Solution:** Test on multiple DEs, use standard protocols

### 4. Key Code Mapping 🔵 Low Risk
**Challenge:** evdev codes ≠ Windows virtual keys

**Solution:** Create translation table (well-documented)

---

## Prerequisites for Linux Porting

### Development Environment
- ✅ Linux system (physical or VM)
- ✅ C++17 compiler (gcc 9+ or clang 10+)
- ✅ CMake 3.15+
- ⚠️ X11 development headers (`libx11-dev`, `libxext-dev`)
- ⚠️ evdev/uinput headers (part of kernel headers)
- ⚠️ libudev development headers (`libudev-dev`)

### Testing Environment
- Real Linux desktop (not WSL)
- Multiple window managers (X11, Wayland/XWayland)
- Multiple keyboards for testing
- Root access or proper group permissions

---

## Decision Matrix

| Factor | Status | Impact |
|--------|--------|--------|
| Abstractions complete | ✅ Yes | Makes porting isolated and safe |
| Interfaces proven | ✅ Yes | Reduces risk of interface changes |
| Stubs in place | ✅ Yes | Clear implementation path |
| Build system ready | ✅ Yes | Can build and test incrementally |
| Technical complexity | ⚠️ High | Input system is challenging |
| Permission issues | ⚠️ High | May need root or setup |
| Time investment | ⚠️ Moderate | 1 month full-time |
| Risk to Windows build | ✅ Low | Isolated in platform/ |

---

## Recommendation

### YES - Start Linux Porting Now ✅

**Reasons:**
1. **Perfect architecture** - All abstractions in place
2. **Minimal risk** - Won't affect Windows code
3. **Clear path** - Stubs show exactly what to implement
4. **Good timing** - Before adding more Windows-specific features

### Suggested Approach

#### Option 1: Full Implementation (1 month)
Best if you have:
- Dedicated Linux development environment
- Time for full implementation
- Need for production-ready Linux version

**Timeline:** 4 weeks
**Result:** Feature-complete Linux port

#### Option 2: Incremental (Recommended)
Best if you want to:
- Validate approach early
- Get feedback from Linux users
- Minimize risk

**Week 1:** Foundation (WindowSystem + Sync)
- ✅ Can detect windows
- ✅ Basic functionality works
- ✅ Validate architecture

**Week 2-3:** Input system
- ✅ Keyboard remapping works
- ✅ Core functionality complete

**Week 4:** Polish & features
- ✅ Feature parity
- ✅ Production ready

#### Option 3: Parallel with Agents (Fastest)
Use same approach as Phases 3-5:
- Launch 4-6 agents in parallel
- Each implements different subsystem
- 2-3 weeks total

---

## Next Steps

If you decide to proceed:

### Step 1: Environment Setup
```bash
# Install dependencies
sudo apt install build-essential cmake
sudo apt install libx11-dev libxext-dev libxrandr-dev
sudo apt install libudev-dev
sudo apt install linux-headers-$(uname -r)

# Add user to input group
sudo usermod -a -G input $USER
```

### Step 2: Create Implementation Plan
Similar to Phase 5, create detailed plan:
- Break down into atomic tasks
- Identify agent boundaries
- Set up parallel execution

### Step 3: Start with Foundation
Implement WindowSystem X11 basics first:
- Can test immediately
- Validates architecture
- Low risk

---

## Conclusion

**This IS the perfect timing for Linux porting:**
- ✅ All abstractions complete
- ✅ Clean interfaces defined
- ✅ Stubs in place
- ✅ Architecture validated
- ✅ Risk is minimal

**Main challenge:** Input system complexity (requires low-level access)

**Recommended approach:** Incremental implementation starting with foundation

**Would you like me to:**
1. Create detailed Linux porting implementation plan?
2. Start implementing the foundation (WindowSystem + Sync)?
3. Set up parallel agents for faster implementation?
