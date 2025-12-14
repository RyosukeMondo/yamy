# YAMY Technical Architecture

## Technology Stack

### Core Technologies

**Language**: C++20
- **Rationale**: Performance-critical input handling, cross-platform portability, modern features (concepts, modules preparation)
- **Benefits**: Zero-overhead abstractions, RAII, standard library maturity, improved compile-time diagnostics
- **Trade-offs**: Longer compile times (mitigated by modern toolchain), steeper learning curve than C
- **Upgrade from C++17**: Enables concepts for template constraints, improved constexpr, designated initializers

**Build System**: CMake 3.28+ with Ninja
- **Rationale**: Industry standard for parallelism, CMakePresets.json for deterministic configuration across AI agents and developers
- **Benefits**: One build system for Windows/Linux, excellent IDE integration, compile_commands.json for tooling
- **Trade-offs**: Verbose syntax, but necessary for platform-specific code
- **Modern Features**: CMakePresets.json for reproducible builds, native Conan integration

### Modern Build Infrastructure

**Build Generator**: Ninja
- **Rationale**: Designed purely for speed with pre-computed dependency graphs
- **Performance**: Starts builds in ~0.4s vs 10-30s for Make systems
- **Benefits**: Zero overhead for incremental builds, perfect for <5s iteration cycles
- **Configuration**: Automatically selected via CMakePresets.json

**Linker Strategy**:

| Platform | Linker | Performance | Rationale |
|----------|--------|-------------|-----------|
| **Linux** | Mold | 10x faster than GNU ld | Massive parallelism, concurrent hashing. Links Clang in 1.35s vs 42s for GNU ld |
| **Windows** | LLD (lld-link) | 3-5x faster than MSVC link.exe | LLVM linker with clang-cl integration, production-ready |

**Build Cache**: ccache
- **Purpose**: Cache compilation results across branches and rebuilds
- **Benefits**: Near-instant rebuilds for unchanged translation units
- **Storage**: Configurable cache size (default 5GB)

**Dependency Management**: Conan 2.0
- **Rationale**: Reproducible builds, binary caching, CMake toolchain integration
- **Benefits**: Automatic dependency resolution, pre-built binaries from remote cache
- **Integration**: Generates conan_toolchain.cmake for CMakePresets.json
- **Dependencies**: fmt, Quill, Microsoft GSL, RapidCheck, Catch2

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
| **Compiler** | Clang-cl + LLD | Unified toolchain with Linux |

#### Linux
| Component | Technology | Purpose |
|-----------|------------|---------|
| **GUI** | Qt5 (Widgets) | System tray, dialogs, cross-DE compatibility |
| **Input Hook** | evdev | Keyboard event capture from `/dev/input/*` |
| **Input Injection** | uinput | Synthesize key events |
| **Window System** | X11 (Xlib + Xrandr) | Window manipulation, focus tracking |
| **IPC** | Unix Domain Sockets | Hook-to-engine communication |
| **Configuration** | QSettings (INI format) | Settings persistence |
| **Compiler** | GCC 11+ or Clang 16+ | C++20 support with Mold linker |

---

## Modern Development Infrastructure

### Zero-Latency Logging: Quill

**Technology**: Quill asynchronous logger
- **Architecture**: Thread-local SPSC ring buffer with backend thread for I/O
- **Performance**: Nanosecond-level latency on hot path (no mutex contention)
- **Timestamping**: RDTSC (Time Stamp Counter) for microsecond precision without syscalls
- **Format**: Structured JSON logging for AI analysis and time-travel debugging

**Rationale**: Traditional logging (spdlog, etc.) introduces mutex contention and I/O blocking on the critical input path. Quill decouples logging from I/O completely.

**Usage Pattern**:
```cpp
LOG_INFO(logger, "Input {key} mapped to {mapped}", "key", input.code, "mapped", output.code);
// Output: {"level":"INFO", "key": 32, "mapped": 45, "timestamp": 1234567890123}
```

**Benefits**:
- ✅ No perceptible latency on input processing (<1μs)
- ✅ JSON output compatible with Perfetto/Chrome tracing
- ✅ Structured logs enable AI-driven debugging

### Contract Programming: Microsoft GSL

**Technology**: Guidelines Support Library (GSL)
- **Purpose**: Production-ready Design by Contract (preconditions/postconditions)
- **Rationale**: C++26 Contracts postponed; GSL provides Expects/Ensures today
- **Safety**: gsl::span for bounds-safe array access, gsl::not_null for pointer validation

**Contract Macros**:
```cpp
// Preconditions
Expects(input_code < MAX_KEYCODE);
Expects(ptr != nullptr);

// Postconditions
Ensures(result_queue.size() <= MAX_QUEUE);
Ensures(state->active_keys.empty());

// Bounds safety
void process(gsl::span<const InputEvent> events);  // replaces pointer+size
```

**Debug vs Release**:
- **Debug Mode**: Violations trigger `gsl::fail_fast` with debugger breakpoint
- **Release Mode**: Optimized out or `std::terminate` (prefer crash over corruption)

**Benefits**:
- ✅ Catches state machine bugs (missing key-up events) at precondition checks
- ✅ Self-documenting API contracts
- ✅ Zero runtime cost in release builds

### Property-Based Testing: RapidCheck

**Technology**: RapidCheck
- **Purpose**: Generative testing with automatic shrinking for input permutations
- **Rationale**: Unit tests cannot cover Layers × Modifiers × History combinatorial explosion

**How It Works**:
1. Generates random sequences of 100+ input events
2. Finds a bug (e.g., stuck key)
3. **Shrinks** the sequence to minimal reproduction (e.g., 4 events)
4. Reports: "Bug occurs with: KeyDown(A) → Shift → KeyDown(B) → KeyUp(A)"

**Example Property**:
```cpp
rc::check("Key Up event must eventually match every Key Down",
  [](const std::vector<InputEvent>& events) {
    InputSystem sys;
    for (auto e : events) sys.process(e);
    RC_ASSERT(sys.active_key_count() == 0);
  });
```

**Advantages over Catch2 Generators**:
- ✅ Automatic test case minimization (shrinking)
- ✅ Designed for QuickCheck-style property testing
- ✅ Better for state machine verification

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

## AI-Compatible Project Structure

### Context Engineering for AI Agents

Modern development involves AI agents (Claude, GPT-4o) constrained by context windows. YAMY implements "Context Density Optimization."

**1. Codebase Map (docs/map.md)**
- Single file summarizing project structure
- AI reads this first to locate relevant files
- Example entries:
  - `src/core/runner.cpp`: "Entry point. Initializes memory and loads logic."
  - `src/logic/remap.cpp`: "Core state machine for layer switching."

**2. Rule Enforcement (.clinerules / .cursorrules)**
- System prompts for IDE agents
- Constraint injection: "Use gsl::span for arrays. No iostream."
- Architecture guidance: "Global state forbidden in src/logic. Use AppContext."

**Sample .clinerules**:
```
YAMY Engineering Rules

Architecture:
- State: ALL persistent state is in struct AppContext
- Concurrency: Input loop is single-threaded

Coding Style:
- Use auto for iterators, explicit types for arithmetic
- Prefer Quill logging macros over printf
- Use GSL contracts (Expects/Ensures) for API boundaries
```

**3. Semantic Density**
- Headers: Detailed Doxygen comments (AI-readable intent)
- Implementation: Stripped of redundant comments
- Maximizes "Signal-to-Token" ratio for AI context windows

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

### Decision 4: Mold vs LLD for Linking

**Linux: Mold** (CHOSEN)
- **Performance**: 10x faster than GNU ld (1.35s vs 42s for Clang)
- **Mechanism**: Concurrent hashing, massive parallelism
- **Stability**: Production-ready for Linux

**Windows: LLD** (CHOSEN)
- **Rationale**: Mold Windows support not production-ready
- **Performance**: 3-5x faster than MSVC link.exe
- **Integration**: Perfect with clang-cl

---

### Decision 5: Quill vs spdlog for Logging

**Quill** (CHOSEN)
- **Latency**: Nanosecond-level (SPSC ring buffer)
- **Timestamping**: RDTSC (no syscall overhead)
- **Structured Logging**: Native JSON support
- **Critical Path**: Zero blocking on hot path

**spdlog** (NOT CHOSEN)
- ❌ Async mode still uses mutexes (contention)
- ❌ No RDTSC support
- ❌ JSON requires custom formatters

**Decision**: Quill essential for <1ms latency requirement

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
| `yamy_test` | Both | Catch2/RapidCheck test suite |

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
option(BUILD_TESTING "Build test suite" ON)
option(USE_MOLD "Use Mold linker on Linux" ON)
option(ENABLE_CONTRACTS "Enable GSL contract checking" ON)
```

### CMakePresets.json Integration

**Purpose**: Deterministic builds for AI agents and humans

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "linux-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_TOOLCHAIN_FILE": "conan_toolchain.cmake",
        "CMAKE_CXX_COMPILER": "clang++",
        "CMAKE_LINKER": "mold"
      }
    }
  ]
}
```

---

## Threading Model

### Windows: Multi-Threaded

1. **Main Thread**: UI message loop (Win32 window messages)
2. **Engine Thread**: Keyboard handler (processes hook callbacks)
3. **Mailslot Thread**: IPC receiver (async I/O completion routine)
4. **Quill Backend**: Log I/O thread (managed by Quill)

**Synchronization**: `CriticalSection` (Windows mutexes)

### Linux: Single-Threaded (Initially)

1. **Main Thread**: Qt event loop + engine + evdev polling
2. **Quill Backend**: Log I/O thread (managed by Quill)

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

### Unit Tests (Catch2)

**Coverage**:
- String utilities (UTF-8 conversion, parsing)
- Keymap lookup (hash table correctness)
- Modifier state machine
- Configuration parsing edge cases

**Example**:
```cpp
TEST_CASE("Keymap basic lookup") {
    Keymap km;
    km.define(ModifiedKey(VK_A, M_Ctrl), new ActionFunction("test"));
    Action* result = km.lookup(ModifiedKey(VK_A, M_Ctrl));
    REQUIRE(result != nullptr);
}
```

### Property-Based Tests (RapidCheck)

**Coverage**:
- Input event sequences (state machine verification)
- Layer switching invariants
- Modifier combination correctness

**Example**:
```cpp
rc::check("All key-down events have matching key-up",
  [](const std::vector<InputEvent>& events) {
    Engine engine;
    for (auto e : events) engine.process(e);
    RC_ASSERT(engine.pressedKeys().empty());
  });
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
5. **Quill logging** - Zero blocking on hot path

### Build Performance Targets

| Operation | Target | Tool |
|-----------|--------|------|
| **Clean build** | <2 minutes | Conan binary cache |
| **Incremental rebuild** | <5 seconds | Ninja + Mold/LLD + ccache |
| **Null build (no changes)** | <1 second | Ninja |

### Memory Usage

**Target**: <10MB resident set size

**Breakdown**:
- Engine core: ~2MB
- Parsed .mayu: ~1MB (typical config)
- Qt GUI: ~5MB (when active)
- Buffers: ~1MB
- Quill ring buffers: ~1MB

### Latency Budget

| Operation | Target | Measurement |
|-----------|--------|-------------|
| Event read | <100μs | `clock_gettime()` before/after |
| Keymap lookup | <10μs | Hash table profiling |
| Action execute | <100μs | Per-action timing |
| Logging | <1μs | Quill RDTSC timestamps |
| **Total** | **<1ms** | 99th percentile |

**Monitoring**: Built-in `--benchmark` flag logs timing stats

---

## Dependencies

### Windows Dependencies
```
- Windows SDK 10.0+
- Clang-cl 16+ (LLVM) or MSVC 2022+
- CMake 3.28+
- Conan 2.0
- Ninja
```

### Linux Dependencies

**Build-time**:
```
- GCC 11+ or Clang 16+
- CMake 3.28+
- Conan 2.0
- Ninja
- Mold linker
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

**Conan Dependencies** (all platforms):
```
- fmt/10.2.1
- quill/4.1.0
- ms-gsl/4.0.0
- rapidcheck/cci.20230815
- catch2/3.5.0
```

---

## Code Metrics Enforcement

### File Size Limits
- **Max 500 lines per file** (excluding comments/blank lines)
- **Max 50 lines per function**
- Enforced via pre-commit hooks

### Test Coverage
- **Minimum 80% code coverage** (90% for critical paths)
- Measured with gcov/lcov (Linux) or OpenCppCoverage (Windows)
- Enforced in CI pipeline

### Complexity Metrics
- **Max cyclomatic complexity: 15** per function
- Measured with lizard or cppcheck
- Critical path functions must be <10

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

### 2026+: Advanced Optimization

**Potential Areas**:
- Profile-Guided Optimization (PGO) for critical paths
- SIMD optimization for batch input processing
- io_uring for async evdev reads (Linux 5.1+)

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

**Q1 2025**: Modern toolchain adoption (this spec)
**Q2 2025**: Remove Windows types (Track 1)
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

**Document Version**: 2.0
**Last Updated**: 2025-12-15
**Changes**: Added modern C++ toolchain (Mold/LLD, Conan 2.0, Quill, GSL, RapidCheck, AI compatibility)
**Reviewed By**: (Pending approval)
