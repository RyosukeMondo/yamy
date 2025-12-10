# YAMY Technical Architecture

## Technology Stack

### Core Technologies

**Language**: C++17
- **Rationale**: Performance-critical input handling, cross-platform portability
- **Benefits**: Zero-overhead abstractions, RAII, standard library maturity
- **Trade-offs**: Longer compile times, steeper learning curve than C

**Build System**: CMake 3.10+
- **Rationale**: Cross-platform builds, conditional compilation, IDE integration
- **Benefits**: One build system for Windows/Linux, widely understood
- **Trade-offs**: Verbose syntax, but necessary for platform-specific code

### Platform-Specific Technologies

#### Windows
| Component | Technology | Purpose |
|-----------|------------|---------|
| **GUI** | Win32 API | System tray, dialogs, native look-and-feel |
| **Input Hook** | SetWindowsHookEx | Global keyboard interception |
| **Input Injection** | SendInput API | Synthesize key events |
| **Window System** | Win32 Window API | Window manipulation, focus tracking |
| **IPC** | Named Pipes + Windows Messages | Hook-to-engine communication |
| **Configuration** | Windows Registry | Settings persistence |

#### Linux
| Component | Technology | Purpose |
|-----------|------------|---------|
| **GUI** | Qt5 (Widgets) | System tray, dialogs, cross-DE compatibility |
| **Input Hook** | evdev | Keyboard event capture from `/dev/input/*` |
| **Input Injection** | uinput | Synthesize key events |
| **Window System** | X11 (Xlib + Xrandr) | Window manipulation, focus tracking |
| **IPC** | Unix Domain Sockets | Hook-to-engine communication |
| **Configuration** | QSettings (INI format) | Settings persistence |

---

## Architecture Layers

### Layer 1: Platform Abstraction

**Purpose**: Hide OS-specific APIs behind uniform interfaces

**Key Interfaces**:
```cpp
namespace yamy::platform {
    // Window system operations
    class IWindowSystem {
        virtual WindowHandle getForegroundWindow() = 0;
        virtual std::string getWindowText(WindowHandle hwnd) = 0;
        virtual Rect getWindowRect(WindowHandle hwnd) = 0;
        virtual void moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
        // ... 40+ methods
    };

    // Input event injection
    class IInputInjector {
        virtual void keyDown(uint16_t scancode) = 0;
        virtual void keyUp(uint16_t scancode) = 0;
        virtual void mouseMove(int dx, int dy) = 0;
        // ... 10+ methods
    };

    // Input event capture
    class IInputHook {
        virtual void install(std::function<void(const InputEvent&)> callback) = 0;
        virtual void uninstall() = 0;
    };

    // Input device management (Linux-specific for multi-keyboard)
    class IInputDriver {
        virtual std::vector<DeviceInfo> enumerateDevices() = 0;
        virtual void openDevice(const std::string& path) = 0;
        virtual InputEvent readEvent() = 0;
    };
}
```

**Implementation Pattern**:
```cpp
// Factory functions in each platform
IWindowSystem* createWindowSystem() {
    #ifdef _WIN32
        return new WindowSystemWin32();
    #else
        return new WindowSystemLinux();
    #endif
}
```

---

### Layer 2: Core Engine

**Purpose**: Platform-agnostic keyboard remapping logic

**Key Components**:

1. **Engine** (`engine.cpp`) - Main orchestrator
   - Input event processing
   - Keymap selection
   - Modifier state tracking
   - Focus window awareness

2. **Setting** (`setting.cpp`) - Configuration representation
   - Parsed .mayu files
   - Keymap hierarchy
   - Key binding registry

3. **SettingLoader** (`setting_loader.cpp`) - Parser
   - Lexical analysis
   - Syntax parsing
   - Preprocessor (ifdef, define)
   - Error reporting

4. **Keymap** (`keymap.cpp`) - Key binding container
   - Hash table for fast lookup
   - Parent keymap chain
   - Per-window overrides

5. **Action/Function** (`function.cpp`) - Action executors
   - Shell commands
   - Window manipulation
   - Mouse control
   - Clipboard operations

**Data Flow**:
```
Input Device → IInputDriver → Engine::handleInput() →
    Keymap::lookup() → Action::execute() → IInputInjector
```

---

### Layer 3: UI Layer

**Windows**: Win32 native UI
- **Tray Icon**: `Shell_NotifyIcon`, `NOTIFYICONDATA`
- **Dialogs**: Resource-based (`DialogBox`, `.rc` files)
- **Log Stream**: `tomsgstream` redirects to edit control via `PostMessage`

**Linux**: Qt5 Widgets
- **Tray Icon**: `QSystemTrayIcon`, `QMenu`
- **Dialogs**: Code-based layouts (`QVBoxLayout`, `QHBoxLayout`)
- **Log Stream**: Qt signals/slots to `QTextEdit`

**Abstraction Strategy**:
- UI and engine are **loosely coupled** via message passing
- Engine posts notifications → UI updates
- UI triggers actions → Engine executes
- No direct UI object access from engine

---

## Cross-Platform Strategy

### Conditional Compilation Approach

**Pattern 1: Platform-Specific Files**
```
src/platform/windows/
    window_system_win32.cpp
    input_injector_win32.cpp
    input_hook_win32.cpp

src/platform/linux/
    window_system_linux.cpp
    input_injector_linux.cpp
    input_hook_linux.cpp
```

**CMake Selection**:
```cmake
if(WIN32)
    list(APPEND SOURCES src/platform/windows/*.cpp)
else()
    list(APPEND SOURCES src/platform/linux/*.cpp)
endif()
```

**Pattern 2: Inline Conditionals (Minimal)**
```cpp
// Only for trivial differences
#ifdef _WIN32
    #include <windows.h>
    typedef HWND PlatformHandle;
#else
    #include <X11/Xlib.h>
    typedef Window PlatformHandle;
#endif
```

**Anti-Pattern (Avoid)**:
```cpp
// DON'T do this - creates ifdef hell
void someFunction() {
    #ifdef _WIN32
        // 50 lines of Windows code
    #else
        // 50 lines of Linux code
    #endif
}
```
**Instead**: Separate files with shared interface

---

## Technology Decisions

### Decision 1: Qt5 vs GTK+ for Linux GUI

**Options Considered**:
1. **Qt5 Widgets** (CHOSEN)
   - ✅ Excellent cross-DE compatibility (GNOME, KDE, XFCE)
   - ✅ Native system tray support
   - ✅ Signals/slots for event handling
   - ✅ Mature, stable API
   - ❌ Larger dependency (~50MB installed)

2. **GTK3/GTK4**
   - ✅ Native on GNOME
   - ✅ Smaller footprint
   - ❌ System tray deprecated (requires AppIndicator)
   - ❌ KDE integration issues

3. **Raw X11 + Xlib**
   - ✅ Zero dependencies
   - ❌ Massive development effort
   - ❌ Poor Wayland future

**Decision**: Qt5 for broad compatibility and velocity

---

### Decision 2: evdev vs X11 for Input Capture

**Options Considered**:
1. **evdev + uinput** (CHOSEN)
   - ✅ Low-level, reliable capture
   - ✅ Works on Wayland (via uinput)
   - ✅ Multi-keyboard support
   - ❌ Requires root or `input` group membership

2. **X11 XGrabKey**
   - ✅ No special permissions
   - ❌ Wayland incompatible
   - ❌ Can't intercept grabbed keys
   - ❌ Race conditions with other apps

3. **libinput API**
   - ✅ Modern, Wayland-ready
   - ❌ Still experimental (2025)
   - ❌ Complex integration

**Decision**: evdev now, libinput future

---

### Decision 3: IPC Mechanism

**Windows**: Named Pipes + PostMessage
- **Rationale**: Hook DLL runs in target app process, needs IPC to main
- **Implementation**: Mailslot for async, PostMessage for sync

**Linux**: Unix Domain Sockets
- **Rationale**: evdev runs in YAMY process (no injection), simpler IPC
- **Implementation**: Single socket for all communication

**Abstraction**: `IIPCChannel` interface hides platform differences

---

## Build System Architecture

### CMake Structure

```
CMakeLists.txt (root)
├── Windows Build
│   ├── yamy_hook.dll (32/64-bit hook library)
│   ├── yamy32.exe / yamy64.exe (main engine + Win32 GUI)
│   ├── yamyd32.exe (32-bit helper for 64-bit systems)
│   └── yamy.exe (launcher, selects 32/64-bit version)
├── Linux Build
│   ├── yamy_stub (main engine + evdev backend)
│   └── yamy_qt_gui.a (Qt GUI library, static linked)
└── Common
    ├── Shared core engine sources
    └── Platform-agnostic headers
```

### Build Targets

| Target | Platform | Purpose |
|--------|----------|---------|
| `yamy_hook` | Windows | DLL injected into target apps |
| `yamy_engine_new` | Windows | Main executable (engine + GUI) |
| `yamy_launcher` | Windows | Architecture selector |
| `yamyd` | Windows | 32-bit helper for WOW64 |
| `yamy_stub` | Linux | Main executable (engine + Qt GUI) |
| `yamy_qt_gui` | Linux | Static library for Qt widgets |
| `yamy_test` | Both | Google Test suite |

### Conditional Compilation Flags

```cmake
# Platform detection
if(WIN32)
    add_compile_definitions(WIN32 _WINDOWS UNICODE _UNICODE)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(__linux__)
endif()

# Feature flags
option(BUILD_QT_GUI "Build Qt5 GUI for Linux" ON)
option(BUILD_TESTING "Build test suite" OFF)
option(USE_INI "Use INI instead of registry (Windows)" OFF)
```

---

## Threading Model

### Windows: Multi-Threaded

1. **Main Thread**: UI message loop (Win32 window messages)
2. **Engine Thread**: Keyboard handler (processes hook callbacks)
3. **Mailslot Thread**: IPC receiver (async I/O completion routine)

**Synchronization**: `CriticalSection` (Windows mutexes)

### Linux: Single-Threaded (Initially)

1. **Main Thread**: Qt event loop + engine + evdev polling

**Future**: Move evdev to worker thread if latency issues

**Synchronization**: Qt signals/slots (thread-safe by default)

---

## Configuration File Format (.mayu)

### Syntax Overview

```
# Comments start with #
include "base.mayu"          # File inclusion
define USE_EMACS_BINDINGS    # Preprocessor

key *C-A = &Ignore           # Global binding (all windows)
key ~C-x = &Prefix           # Prefix key (chording)
key A-Tab = &OtherWindow     # Modifier combinations

window Firefox /            # Window-specific bindings
    key C-r = &Reload
/end
```

### Parser Architecture

**Lexer** (`setting_loader.cpp`):
- Regex-based token recognition
- Line-by-line streaming
- UTF-8 support

**Parser**:
- Recursive descent
- No AST (direct object construction)
- Error recovery (continue after syntax errors)

**Preprocessor**:
- `include` - File merging
- `ifdef`/`ifndef` - Conditional compilation
- `define` - Symbol definition

---

## Testing Strategy

### Unit Tests (Google Test)

**Coverage**:
- String utilities (UTF-8 conversion, parsing)
- Keymap lookup (hash table correctness)
- Modifier state machine
- Configuration parsing edge cases

**Example**:
```cpp
TEST(KeymapTest, BasicLookup) {
    Keymap km;
    km.define(ModifiedKey(VK_A, M_Ctrl), new ActionFunction("test"));
    Action* result = km.lookup(ModifiedKey(VK_A, M_Ctrl));
    ASSERT_NE(nullptr, result);
}
```

### Integration Tests

**Windows**: `test.mayu` configuration
- Real keyboard simulation via SendInput
- Window focus changes
- Multi-monitor scenarios

**Linux**: evdev mock device
- Virtual `/dev/input/eventX` device
- Inject events, verify output
- uinput validation

### Manual Testing Checklist

```
[ ] All keys on 104/109 layout work
[ ] Modifiers combine correctly (C-M-S-A)
[ ] Prefix keys don't leak
[ ] Focus change triggers keymap switch
[ ] Multi-keyboard input (Linux)
[ ] Wayland compatibility (basic, via XWayland)
[ ] Performance: <1ms latency, <1% CPU
```

---

## Performance Considerations

### Critical Path Optimization

**Hot Path**: Input event → Action execution
```
evdev read (100μs) → Lookup (10μs) → Execute (100μs) = ~210μs total
```

**Optimization Strategies**:
1. **Hash table for keymap lookup** - O(1) average case
2. **Inline modifier state checks** - No function calls
3. **Pre-compiled actions** - No parsing at runtime
4. **Minimal allocations** - Reuse Action objects

### Memory Usage

**Target**: <10MB resident set size

**Breakdown**:
- Engine core: ~2MB
- Parsed .mayu: ~1MB (typical config)
- Qt GUI: ~5MB (when active)
- Buffers: ~1MB

### Latency Budget

| Operation | Target | Measurement |
|-----------|--------|-------------|
| Event read | <100μs | `clock_gettime()` before/after |
| Keymap lookup | <10μs | Hash table profiling |
| Action execute | <100μs | Per-action timing |
| **Total** | **<1ms** | 99th percentile |

**Monitoring**: Built-in `--benchmark` flag logs timing stats

---

## Dependencies

### Windows Dependencies
```
- Windows SDK 10.0+
- MSVC 2019+ or MinGW-w64
- CMake 3.10+
```

### Linux Dependencies

**Build-time**:
```
- GCC 7+ or Clang 8+
- CMake 3.10+
- Qt5 dev packages (qtbase5-dev, qttools5-dev)
- X11 dev packages (libx11-dev, libxrandr-dev)
- libudev-dev (for evdev device enumeration)
```

**Runtime**:
```
- Qt5 runtime (QtCore, QtWidgets, QtGui)
- X11 libraries (Xlib, Xrandr)
- libudev (for device discovery)
- Membership in 'input' group (for /dev/input/* access)
```

---

## Security Considerations

### Privilege Requirements

**Windows**: Administrator for install (hook DLL registration)
- Runtime: Normal user (hooks work unprivileged)

**Linux**: User must be in `input` group
```bash
sudo usermod -aG input $USER
# Log out and back in for group to take effect
```

**Rationale**: evdev requires `/dev/input/*` read access

### Attack Surface

**Threat 1: Arbitrary Code Execution via .mayu**
- **Risk**: User loads malicious .mayu file with shell commands
- **Mitigation**: .mayu files are **user-created configuration**, not untrusted input
- **Policy**: YAMY does not download/fetch .mayu files automatically

**Threat 2: Privilege Escalation (Linux)**
- **Risk**: Malicious app tricks YAMY into injecting input as root
- **Mitigation**: YAMY runs as regular user, no setuid bit
- **Defense**: Input group membership is by design, not escalation

**Threat 3: Keylogger Perception**
- **Risk**: Antivirus flags YAMY as keylogger
- **Mitigation**: Code signing (Windows), reputation building
- **Transparency**: Open source, auditable

---

## Future Technology Roadmap

### 2025 Q2-Q3: Wayland Native Support

**Goal**: Remove X11 dependency

**Approach**:
- `libinput` integration for event capture
- `wlroots` virtual keyboard protocol for injection
- Per-compositor quirks (GNOME vs KDE vs Sway)

**Challenges**:
- Wayland security model restricts input capture
- Need compositor cooperation (not guaranteed)
- Fallback to XWayland may be permanent

### 2025 Q4: Plugin System

**Goal**: User-extensible actions via scripting

**Technology**: Lua 5.4 embedded
- **Rationale**: Lightweight, easy C++ integration, fast
- **API**: Expose `yamy.*` namespace for actions
- **Sandboxing**: Restricted I/O, no filesystem except config dir

**Example**:
```lua
-- Custom action: Open URL in browser
function yamy.action.openURL(url)
    os.execute("xdg-open " .. url)
end
```

### 2026+: GPU Acceleration (Experimental)

**Goal**: Sub-100μs latency via GPU-accelerated lookup

**Technology**: Vulkan compute shaders
- **Rationale**: Keymap lookup is embarrassingly parallel
- **Approach**: Upload keymap to VRAM, GPU does hash table lookup
- **Challenge**: CPU-GPU transfer overhead may negate benefit

**Status**: Research phase, may not be practical

---

## Technology Debt & Cleanup

### Known Technical Debt

1. **Windows-Specific Types in Core**
   - **Problem**: `HWND`, `tstring`, `SW_*` constants everywhere
   - **Impact**: Prevents Linux compilation of core engine
   - **Fix**: Track 1 refactoring (60 tasks, 80 hours)
   - **Priority**: CRITICAL (blocks all Linux work)

2. **Single-Threaded Engine (Linux)**
   - **Problem**: evdev polling blocks event processing
   - **Impact**: Potential latency spikes under load
   - **Fix**: Move evdev to worker thread with queue
   - **Priority**: MEDIUM (optimize after v1.0)

3. **Hardcoded Paths**
   - **Problem**: Windows uses `C:\mayu`, Linux uses `~/.yamy`
   - **Impact**: Poor cross-platform config sharing
   - **Fix**: XDG Base Directory spec compliance
   - **Priority**: LOW (cosmetic)

4. **Lack of Async I/O (Linux)**
   - **Problem**: Synchronous evdev reads
   - **Impact**: Could miss events under heavy load
   - **Fix**: `io_uring` for async evdev reads
   - **Priority**: LOW (io_uring requires Linux 5.1+)

### Refactoring Priorities

**Q1 2025** (Track 1): Remove Windows types
**Q2 2025** (Track 2-5): Implement missing features
**Q3 2025**: Performance optimization
**Q4 2025**: Plugin system

---

## Documentation Standards

### Code Documentation

**Doxygen Comments**:
```cpp
/**
 * @brief Brief description
 *
 * Detailed description with examples.
 *
 * @param i_param Input parameter
 * @param o_param Output parameter
 * @return Return value description
 */
```

**Internal Comments**:
- Why, not what (code shows what)
- Edge cases and non-obvious behavior
- Performance considerations
- TODO with GitHub issue links

### Architecture Decision Records (ADRs)

**Format**:
```markdown
# ADR-NNN: Title

Date: YYYY-MM-DD
Status: Accepted | Rejected | Superseded

## Context
Problem statement

## Decision
What we decided

## Consequences
Trade-offs and implications
```

**Location**: `docs/adr/`

---

**Document Version**: 1.0
**Last Updated**: 2025-12-10
**Reviewed By**: (Pending approval)
