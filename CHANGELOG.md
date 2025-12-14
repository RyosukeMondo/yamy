# Changelog

All notable changes to YAMY (Yet Another Mado tsukai no Yuutsu) are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Investigate Window Feature (Linux Qt GUI)**: Real-time debugging tool for inspecting window properties and keymap status
  - Crosshair window selection tool (click "Investigate Window" in tray menu)
  - Window info panel: displays title, class, process name/path, geometry, state
  - Keymap status panel: shows active keymap, matched regex patterns, modifier state
  - Live key event log: real-time stream of key presses/releases with timestamps
  - IPC communication between GUI and engine for real-time status updates
  - Useful for debugging .mayu configurations (e.g., "why isn't my C-x binding working in Emacs?")

### Fixed

- Window property queries now return real data (not stubs) on Linux via X11/EWMH
- IPC channel now functional between Qt GUI and engine (was stub returning "not connected")
- Engine now sends keymap status responses to investigate dialog via IPC
- X11 window queries properly handle BadWindow errors without crashing

### Changed

- Window property queries use caching (100ms TTL) to reduce X11 round-trip latency to <10ms
- Engine IPC message handler now connected via Qt signals/slots for asynchronous processing

## [1.0.0] - 2025-12-12

### Added

#### Linux Platform Support
- Complete Linux port with feature parity to Windows version
- Platform abstraction layer (PAL) enabling cross-platform development
- X11-based window system integration via `IWindowSystem` interface
- evdev/uinput kernel interface for keyboard input capture and injection
- Linux-specific input hook implementation (`InputHookLinux`)
- Unix domain socket IPC for daemon communication

#### Qt5 GUI for Linux
- System tray icon with context menu (enable/disable, reload, settings, exit)
- Configuration manager dialog for multiple .mayu profile management
- Investigate dialog with crosshair window selection tool
- Log viewer with filtering, search, syntax highlighting, and export
- Preferences dialog with tabbed interface (General, Notifications, Logging)
- About dialog with license, contributors, and build information
- Help menu with documentation, keyboard shortcuts, and examples dialogs
- Desktop notifications via Qt5 notification system
- Notification history accessible from tray icon

#### Session Management
- `SessionManager` for state persistence across restarts
- XDG autostart integration via desktop entries
- Session restore on startup

#### Command-Line Control
- `yamy-ctl` command-line tool for daemon control
- IPC query commands: status, config, keymaps, metrics
- Support for reload, stop, and configuration management

#### Configuration Management
- `ConfigManager` class for multiple configuration profiles
- `ConfigValidator` for syntax and semantic validation
- `ConfigWatcher` for automatic reload on file changes
- Configuration import/export functionality
- Configuration backup and restore
- Configuration templates for common use cases
- Configuration metadata storage (name, description, author)
- Config quick-switch hotkey support

#### Plugin System
- Plugin system foundation with dynamic loading
- Example plugin demonstrating API usage
- Plugin notification callbacks via `NotificationDispatcher`

#### Crash Reporting
- Crash reporting infrastructure with signal handlers
- Crash report dialog displayed on startup after crash
- Crash dump storage and reporting

#### Testing Infrastructure
- Comprehensive end-to-end integration test suite (18 test cases)
- Unit tests for Linux platform implementations
- Google Test framework integration
- Mock/stub implementations for all platform interfaces
- Memory leak detection with AddressSanitizer
- Performance benchmarks with metrics collection

#### Build System
- CMake build system with Linux target support
- Code coverage target with lcov/gcov integration
- CPack configuration for DEB packages (Debian/Ubuntu)
- CPack configuration for RPM packages (Fedora/openSUSE)
- Arch Linux PKGBUILD for AUR distribution
- GitHub Actions CI/CD pipeline with coverage reporting

#### Documentation
- Comprehensive end-user documentation
- Platform abstraction architecture documentation
- Migration guide for platform portability
- Building instructions for Linux
- Code quality guidelines

### Changed

- Migrated `tstring` typedef to `std::string` across entire codebase
- Replaced Windows types (`BYTE`, `DWORD`) with standard C++ types (`uint8_t`, `uint32_t`)
- Refactored `stringtool.cpp` into multiple focused files (<500 lines each)
- Refactored `interpretMetaCharacters()` into helper functions (<50 lines each)
- Added mutex protection for `m_readerThreads` vector in `InputHookLinux`
- Updated engine to use platform-agnostic modifier state tracking
- Updated key event processing to use `KeyEvent` structure directly

### Fixed

- ConfigWatcher MOC generation for Qt signals/slots
- IIPCChannel vtable undefined reference linker errors
- Qt5Test module linkage for test executable
- Build system compilation for headless Linux environments
- Pre-existing Linux build issues (Qt Multimedia optional)

### Security

- Input group membership required for keyboard device access
- No root privileges required after initial setup
- udev rules for secure device access

---

## [0.6] - Previous Windows Release

Legacy Windows-only release. See [docs/readme.txt](docs/readme.txt) for historical changelog.

---

## [0.5] - Previous Windows Release

Legacy Windows-only release.

---

## [0.04] - Initial Windows Release

Initial release derived from "Mado tsukai no Yuutsu" (Mayu).

- User-mode hook (`WH_KEYBOARD_LL`) replacing kernel filter driver
- `SendInput()` API for key injection
- Support for Windows Vista and later (32-bit and 64-bit)

---

[1.0.0]: https://github.com/user/yamy/releases/tag/v1.0.0
[0.6]: https://github.com/user/yamy/releases/tag/v0.6
[0.5]: https://github.com/user/yamy/releases/tag/v0.5
[0.04]: https://github.com/user/yamy/releases/tag/v0.04
