# Requirements: Linux Complete Port

## Overview

Complete feature-parity Linux port of YAMY keyboard remapping utility, ensuring identical functionality and user experience across Windows and Linux platforms.

---

## User Stories

### US-1: Cross-Platform Developer Workflow
**As a** software developer working on both Windows and Linux
**I want** to use the same .mayu configuration file on both systems
**So that** my muscle memory and keyboard shortcuts work identically everywhere

**Acceptance Criteria**:
- [ ] Same .mayu file works without modification on Windows and Linux
- [ ] All keyboard shortcuts behave identically
- [ ] Configuration changes sync seamlessly
- [ ] No platform-specific syntax required

**EARS Criteria**:
- **Event**: When I copy my .mayu file from Windows to Linux
- **Action**: YAMY loads and applies the configuration
- **Result**: All key mappings work identically

---

### US-2: System Tray Control
**As a** power user
**I want** a system tray icon with context menu
**So that** I can quickly enable/disable remapping and switch configurations

**Acceptance Criteria**:
- [ ] System tray icon visible in all major DEs (GNOME, KDE, XFCE)
- [ ] Visual indication of enabled/disabled state (green/gray icon)
- [ ] Right-click opens context menu
- [ ] Double-click toggles enable/disable
- [ ] Menu includes: Enable, Reload, Settings, Log, About, Exit

**EARS Criteria**:
- **Event**: When I right-click the tray icon
- **Action**: A context menu appears
- **Result**: I can access all YAMY functions

---

### US-3: Configuration Management
**As a** user with multiple keyboard layouts
**I want** to manage multiple .mayu configurations and switch between them
**So that** I can adapt to different workflows (work, gaming, writing)

**Acceptance Criteria**:
- [ ] Can add/edit/remove multiple configurations
- [ ] Each configuration has: name, .mayu file path, preprocessor symbols
- [ ] Reload submenu lists all configurations with checkmark on active
- [ ] Configuration changes persist across restarts
- [ ] Can reorder configurations

**EARS Criteria**:
- **Event**: When I select a configuration from the Reload submenu
- **Action**: YAMY loads and activates that configuration
- **Result**: All keymappings reflect the new config within 100ms

---

### US-4: Real-Time Debugging
**As a** keymap developer
**I want** an investigate dialog showing window information and key events
**So that** I can debug window-specific bindings and verify key capture

**Acceptance Criteria**:
- [ ] Crosshair tool to select target window
- [ ] Displays: window handle, class name, title, position, size
- [ ] Keyboard input test field showing captured keys with modifiers
- [ ] Updates in real-time as focus changes
- [ ] Enables engine log mode when focused

**EARS Criteria**:
- **Event**: When I focus the keyboard input field
- **Action**: Engine log mode activates
- **Result**: Every key press shows in log with scancode, VK, and modifiers

---

### US-5: Log Monitoring
**As a** troubleshooting user
**I want** a log viewer with filtering and export
**So that** I can diagnose configuration errors and report bugs

**Acceptance Criteria**:
- [ ] Timestamped log entries
- [ ] Auto-scroll toggle
- [ ] Detail level control (0=minimal, 1=verbose)
- [ ] Clear log function
- [ ] Save to file (.log or .txt)
- [ ] Font customization
- [ ] 10,000 line limit with automatic trimming

**EARS Criteria**:
- **Event**: When engine encounters an error
- **Action**: Error appears in log dialog
- **Result**: User sees timestamp, error type, and detailed message

---

### US-6: Settings Management
**As a** new user
**I want** a GUI settings dialog
**So that** I can configure keymaps without editing text files

**Acceptance Criteria**:
- [ ] Three-column table: Name, Path, Symbols
- [ ] Add button opens file picker for .mayu files
- [ ] Edit button opens edit dialog for selected config
- [ ] Remove button deletes configuration with confirmation
- [ ] Up/Down buttons reorder configurations
- [ ] Double-click opens edit dialog
- [ ] Save button persists changes
- [ ] Cancel button discards changes

**EARS Criteria**:
- **Event**: When I click Add and select a .mayu file
- **Action**: File is added to configuration list
- **Result**: Configuration appears in table and Reload submenu

---

### US-7: Engine Integration
**As a** user
**I want** the Linux engine to process keyboard input identically to Windows
**So that** all functions work (window commands, mouse control, clipboard, etc.)

**Acceptance Criteria**:
- [ ] All 100+ functions from Windows version implemented
- [ ] evdev input capture from all keyboards
- [ ] uinput event injection
- [ ] X11 window manipulation
- [ ] Focus tracking with window class/title matching
- [ ] Modifier state tracking (Ctrl, Alt, Shift, Meta)
- [ ] Lock key handling (Caps, Num, Scroll)
- [ ] Sub-millisecond latency (99th percentile <1ms)

**EARS Criteria**:
- **Event**: When I press a remapped key
- **Action**: Engine processes via keymap lookup
- **Result**: Correct action executes within 1ms

---

### US-8: Multi-Keyboard Support (Linux-Specific)
**As a** Linux user with multiple keyboards
**I want** separate keymaps for different keyboards
**So that** my laptop keyboard and external keyboard can have different layouts

**Acceptance Criteria**:
- [ ] Detect all connected keyboards via evdev
- [ ] Assign keymap per device
- [ ] Hot-plug support (keyboards connect/disconnect)
- [ ] Device-specific configuration in .mayu file
- [ ] GUI shows device list with names

**EARS Criteria**:
- **Event**: When I connect a new USB keyboard
- **Action**: YAMY detects device and applies default keymap
- **Result**: New keyboard works with remapping immediately

---

### US-9: Session Management (Linux-Specific)
**As a** laptop user
**I want** YAMY to handle screen lock and suspend gracefully
**So that** remapping doesn't interfere with login or cause input issues after wake

**Acceptance Criteria**:
- [ ] Pause engine on screen lock
- [ ] Resume engine on unlock
- [ ] Suspend all input on system suspend
- [ ] Restore state after resume
- [ ] No stuck keys after lock/unlock
- [ ] Detect session events via D-Bus

**EARS Criteria**:
- **Event**: When the system is suspended
- **Action**: YAMY pauses engine and releases all keys
- **Result**: After resume, no keys are stuck and remapping works normally

---

### US-10: Performance Expectations
**As a** competitive typist / gamer
**I want** zero perceptible input lag
**So that** YAMY doesn't slow down my workflow or gameplay

**Acceptance Criteria**:
- [ ] Latency: 99th percentile <1ms, mean <500μs
- [ ] CPU usage: <1% idle, <3% under load
- [ ] Memory: <10MB resident set size
- [ ] No frame drops or stutters during heavy input
- [ ] Benchmark mode for profiling

**EARS Criteria**:
- **Event**: When I type at 120 WPM
- **Action**: YAMY processes all keys without lag
- **Result**: No perceptible delay, all keys registered correctly

---

### US-11: External Control (IPC API)
**As a** automation scripter
**I want** a command-line or D-Bus API to control YAMY
**So that** I can enable/disable remapping from scripts

**Acceptance Criteria**:
- [ ] D-Bus service: `net.gimy.yamy`
- [ ] Methods: Enable(bool), Reload(), Quit(), GetStatus()
- [ ] Command-line tool: `yamy-ctl enable|disable|reload|quit|status`
- [ ] Responds within 10ms
- [ ] Works from any user process

**EARS Criteria**:
- **Event**: When I run `yamy-ctl disable`
- **Action**: YAMY receives command via D-Bus
- **Result**: Remapping is disabled and command returns exit code 0

---

### US-12: Help and Documentation
**As a** first-time user
**I want** accessible help and examples
**So that** I can quickly learn how to configure YAMY

**Acceptance Criteria**:
- [ ] Help menu item opens browser with documentation
- [ ] About dialog shows version, build date, license
- [ ] Example .mayu files included (Emacs bindings, Vim bindings)
- [ ] Tooltip hints in dialogs
- [ ] Error messages link to troubleshooting docs

**EARS Criteria**:
- **Event**: When I click Help menu
- **Action**: Browser opens to documentation website
- **Result**: User guide is displayed

---

## Functional Requirements

### FR-1: Core Refactoring
**Priority**: CRITICAL
**Dependencies**: None
**Description**: Remove all Windows-specific types from core engine to enable Linux compilation

**Requirements**:
- FR-1.1: Replace `HWND` with `yamy::platform::WindowHandle` throughout codebase
- FR-1.2: Replace `tstring` with `std::string` throughout codebase
- FR-1.3: Replace `SW_*` constants with `yamy::platform::ShowCommand` enum
- FR-1.4: Abstract `PostMessage`/`SendMessage` with `IIPCChannel` interface
- FR-1.5: Remove `GetWindowRect`, `MoveWindow`, etc. → use `IWindowSystem`
- FR-1.6: Clean up `ConfigStore` - remove `tstring` overloads
- FR-1.7: Abstract GDI calls (BeginPaint, EndPaint, DrawIcon)
- FR-1.8: Abstract window messages (WM_KEYDOWN, WM_SETFOCUS)
- FR-1.9: Abstract caret functions (CreateCaret, ShowCaret)
- FR-1.10: Remove all `windowstool.h` includes from core

**Acceptance**: Engine compiles on Linux without Windows headers

---

### FR-2: Configuration Management System
**Priority**: HIGH
**Dependencies**: FR-1
**Description**: Multi-configuration support with GUI editor

**Requirements**:
- FR-2.1: `MayuConfiguration` struct with name/path/symbols
- FR-2.2: QSettings storage format: `keymaps/configs/N`, `keymaps/activeIndex`
- FR-2.3: `ConfigManagerQt` class with CRUD operations
- FR-2.4: Configuration serialization to "name;path;symbols" format
- FR-2.5: Dynamic reload submenu population
- FR-2.6: Configuration switching via menu
- FR-2.7: Three-column settings dialog (Name/Path/Symbols)
- FR-2.8: Up/Down reordering buttons
- FR-2.9: Edit setting dialog with browse button
- FR-2.10: Validation (name required, path must exist)

**Acceptance**: User can manage 10+ configurations via GUI

---

### FR-3: Investigate Dialog
**Priority**: MEDIUM
**Dependencies**: FR-1
**Description**: Real-time debugging tool with window inspector

**Requirements**:
- FR-3.1: Crosshair window selector tool
- FR-3.2: Window info display (handle, thread ID, class, title, geometry)
- FR-3.3: MDI window detection
- FR-3.4: Keyboard input test field
- FR-3.5: VK code and modifier display (E-/U-/D- format)
- FR-3.6: Scancode display
- FR-3.7: Log mode toggle on focus
- FR-3.8: Position persistence (QSettings)
- FR-3.9: Relative positioning near log dialog

**Acceptance**: User can inspect any window and capture key events

---

### FR-4: Log Dialog Enhancement
**Priority**: MEDIUM
**Dependencies**: FR-1
**Description**: Full-featured log viewer matching Windows version

**Requirements**:
- FR-4.1: Detail level checkbox (0=minimal, 1=verbose)
- FR-4.2: Font selection dialog
- FR-4.3: Font persistence (QSettings: `ui/logFont`)
- FR-4.4: Engine log streaming integration
- FR-4.5: Startup banner display
- FR-4.6: Real-time log append (Qt signals)
- FR-4.7: Monospace font default
- FR-4.8: Line wrap toggle (optional enhancement)

**Acceptance**: Log dialog matches Windows functionality

---

### FR-5: Engine Notification System
**Priority**: CRITICAL
**Dependencies**: FR-1
**Description**: Engine-to-GUI communication for dynamic updates

**Requirements**:
- FR-5.1: `EngineNotifierQt` class with Qt signals
- FR-5.2: Handle `EngineNotify_shellExecute` → `QProcess::startDetached`
- FR-5.3: Handle `EngineNotify_loadSetting` → reload config
- FR-5.4: Handle `EngineNotify_helpMessage` → show tray balloon
- FR-5.5: Handle `EngineNotify_showDlg` → show/hide dialogs
- FR-5.6: Handle `EngineNotify_setForegroundWindow` → X11 focus change
- FR-5.7: Handle `EngineNotify_clearLog` → clear log dialog
- FR-5.8: Engine callback registration
- FR-5.9: Signal/slot connections to GUI
- FR-5.10: Thread-safe notification delivery

**Acceptance**: All 6 notification types trigger correct GUI actions

---

### FR-6: IPC API for External Control
**Priority**: LOW
**Dependencies**: FR-1, FR-5
**Description**: D-Bus service for programmatic control

**Requirements**:
- FR-6.1: D-Bus service interface definition
- FR-6.2: Method: `Enable(bool enabled)` → engine.enable()/disable()
- FR-6.3: Method: `Reload()` → reload active configuration
- FR-6.4: Method: `Quit()` → QApplication::quit()
- FR-6.5: Method: `GetStatus()` → return enabled state
- FR-6.6: Command-line tool `yamy-ctl` wrapping D-Bus calls
- FR-6.7: Shell completion script (bash/zsh)
- FR-6.8: Systemd service unit (optional)

**Acceptance**: `yamy-ctl disable` works from terminal

---

### FR-7: Session Management
**Priority**: MEDIUM
**Dependencies**: FR-1
**Description**: Handle Linux session events gracefully

**Requirements**:
- FR-7.1: D-Bus listener for `org.freedesktop.login1.SessionRemoved`
- FR-7.2: D-Bus listener for `org.freedesktop.ScreenSaver.ActiveChanged`
- FR-7.3: D-Bus listener for `org.freedesktop.login1.PrepareForSleep`
- FR-7.4: Engine pause on screen lock
- FR-7.5: Engine resume on unlock
- FR-7.6: Release all pressed keys on pause
- FR-7.7: Reinitialize key state on resume
- FR-7.8: Log session events to debug log

**Acceptance**: No stuck keys after lock/unlock/suspend

---

### FR-8: Keyboard State Checker
**Priority**: LOW
**Dependencies**: FR-3 (can be part of Investigate dialog)
**Description**: Display all key states for debugging

**Requirements**:
- FR-8.1: List of all pressed keys (from engine)
- FR-8.2: Lock key states (Caps, Num, Scroll)
- FR-8.3: Modifier states (Shift, Ctrl, Alt, Meta)
- FR-8.4: Refresh on timer (100ms)
- FR-8.5: Highlight state changes
- FR-8.6: Export state to clipboard

**Acceptance**: Shows accurate key states in real-time

---

### FR-9: Help Menu
**Priority**: LOW
**Dependencies**: None
**Description**: Link to documentation

**Requirements**:
- FR-9.1: Help menu item in tray context menu
- FR-9.2: Opens default browser with `QDesktopServices::openUrl()`
- FR-9.3: URL points to GitHub wiki or docs site
- FR-9.4: Fallback to local HTML docs if offline

**Acceptance**: Help opens documentation in browser

---

## Non-Functional Requirements

### NFR-1: Performance
**Priority**: CRITICAL

- NFR-1.1: Input latency: 99th percentile <1ms, mean <500μs
- NFR-1.2: CPU usage: <1% idle, <3% under heavy load (500+ keys/sec)
- NFR-1.3: Memory: <10MB RSS (excluding Qt GUI)
- NFR-1.4: Startup time: <1 second to tray icon visible
- NFR-1.5: Configuration reload: <100ms

---

### NFR-2: Compatibility
**Priority**: HIGH

- NFR-2.1: Supports X11 (Xorg)
- NFR-2.2: Basic Wayland support via XWayland
- NFR-2.3: Works on GNOME, KDE, XFCE, i3, Sway
- NFR-2.4: Kernel 4.19+ (for evdev/uinput APIs)
- NFR-2.5: Qt 5.12+ (LTS)
- NFR-2.6: GCC 7+ or Clang 8+
- NFR-2.7: All .mayu files from Windows version work unchanged

---

### NFR-3: Usability
**Priority**: HIGH

- NFR-3.1: System tray icon visible on all DEs
- NFR-3.2: Dialogs remember size/position
- NFR-3.3: No configuration required for basic use (defaults work)
- NFR-3.4: Error messages are actionable (not cryptic)
- NFR-3.5: Keyboard-navigable dialogs (Tab, Enter, Esc)

---

### NFR-4: Reliability
**Priority**: HIGH

- NFR-4.1: No crashes during normal operation
- NFR-4.2: Graceful degradation if Qt not available (headless mode)
- NFR-4.3: Handle keyboard hot-plug without restart
- NFR-4.4: Recover from X11 server disconnect
- NFR-4.5: No stuck keys even on abnormal exit

---

### NFR-5: Maintainability
**Priority**: MEDIUM

- NFR-5.1: Code coverage: 80% minimum
- NFR-5.2: All public APIs documented (Doxygen)
- NFR-5.3: No platform-specific code in core/ directory
- NFR-5.4: CMake builds on both Windows and Linux
- NFR-5.5: CI pipeline tests both platforms

---

### NFR-6: Security
**Priority**: MEDIUM

- NFR-6.1: No setuid/setgid binaries
- NFR-6.2: Runs as regular user (input group membership)
- NFR-6.3: .mayu files are user-created (trusted by design)
- NFR-6.4: No network access (local only)
- NFR-6.5: No telemetry or data collection

---

## Out of Scope

The following are explicitly NOT part of this spec:

- ❌ Wayland native protocol support (future work)
- ❌ macOS port
- ❌ Mobile keyboards (Android/iOS)
- ❌ Cloud configuration sync
- ❌ Visual keymap editor (drag-and-drop)
- ❌ Plugin/scripting system (Lua)
- ❌ Mouse remapping (out of project scope)
- ❌ Macro recording (out of project scope)

---

## Success Criteria

Release v1.0 is considered complete when:

1. ✅ All user stories US-1 through US-12 are implemented
2. ✅ All functional requirements FR-1 through FR-9 are implemented
3. ✅ All non-functional requirements NFR-1 through NFR-6 are met
4. ✅ Zero P0 bugs in issue tracker
5. ✅ Community beta testing (20+ users, 2+ weeks)
6. ✅ Documentation complete (user guide, API docs, examples)
7. ✅ Benchmarks pass on 3 reference systems (laptop, desktop, server)

---

**Document Version**: 1.0
**Last Updated**: 2025-12-10
**Status**: Draft (Pending Approval)
**Reviewed By**: (Pending)
