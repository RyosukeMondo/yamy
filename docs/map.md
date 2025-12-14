# YAMY Codebase Map

## Core Engine (Platform-Agnostic)

### Engine Core
- `src/core/engine/engine.cpp` - Main engine class coordinating all subsystems
- `src/core/engine/engine_event_processor.cpp` - Event processing pipeline (Layer 1→2→3 transformation)
- `src/core/engine/engine_input.cpp` - Input event handling and routing
- `src/core/engine/engine_modifier.cpp` - Modifier key state tracking (O(1) lookup)
- `src/core/engine/modifier_key_handler.cpp` - Modifier key press/release coordination
- `src/core/engine/engine_window.cpp` - Window focus and context management
- `src/core/engine/engine_setting.cpp` - Configuration loading and application

### Input System
- `src/core/input/keymap.cpp` - Keymap lookup and substitution (Layer 2, <1ms latency)
- `src/core/input/modifier_state.cpp` - Modifier state machine (guarantees no stuck keys)
- `src/core/input/keyboard.cpp` - Keyboard abstraction layer
- `src/core/input/vkeytable.cpp` - Virtual key translation tables

### Settings & Configuration
- `src/core/settings/config_manager.cpp` - Configuration file management and hot-reload
- `src/core/settings/parser.cpp` - YAMY DSL parser for .mayu files
- `src/core/settings/setting.cpp` - Setting data structures
- `src/core/settings/config_validator.cpp` - Configuration validation before load
- `src/core/settings/session_manager.cpp` - Multi-instance session coordination

### Commands (90+ Emacs-style Commands)
- `src/core/commands/command_base.h` - Base command interface
- `src/core/commands/cmd_*.cpp` - Individual command implementations (window manipulation, clipboard, etc.)
- `src/core/commands/cmd_keymap.cpp` - Runtime keymap switching
- `src/core/commands/cmd_prefix.cpp` - Multi-key prefix sequences

### Functions & String Expressions
- `src/core/functions/function.cpp` - Function evaluation engine
- `src/core/functions/strexpr.cpp` - String expression parser for dynamic commands

### Window Management
- `src/core/window/focus.cpp` - Focus tracking for context-aware keymaps
- `src/core/window/layoutmanager.cpp` - Window layout persistence
- `src/core/window/target.cpp` - Window targeting for commands

### Logging (Quill-based)
- `src/core/logging/logger.cpp` - Zero-latency structured logging wrapper
- `src/core/logger/journey_logger.cpp` - User journey event tracking

### Platform Abstraction Interfaces
- `src/core/platform/input_hook_interface.h` - Low-level keyboard hook abstraction
- `src/core/platform/input_injector_interface.h` - Key event injection abstraction
- `src/core/platform/window_system_interface.h` - Window system abstraction
- `src/core/platform/ipc_channel_interface.h` - Inter-process communication abstraction

## Platform Implementations

### Linux (X11 + Wayland)
- `src/platform/linux/input_hook_linux.cpp` - evdev-based keyboard hooking
- `src/platform/linux/input_injector_linux.cpp` - uinput-based key injection
- `src/platform/linux/keycode_mapping.cpp` - evdev ↔ YAMY scan code mapping (Layer 1 & 3)
- `src/platform/linux/window_system_linux.cpp` - X11 window system integration
- `src/platform/linux/x11_connection.cpp` - X11 connection management
- `src/platform/linux/ipc_linux.cpp` - Qt-based IPC for daemon/GUI communication

### Windows (Win32)
- `src/platform/windows/input_hook_win32.cpp` - Low-level keyboard hook (WH_KEYBOARD_LL)
- `src/platform/windows/input_injector_win32.cpp` - SendInput-based injection
- `src/platform/windows/window_system_win32.cpp` - Win32 window management
- `src/platform/windows/registry.cpp` - Windows registry integration

## User Interface

### Qt GUI (Cross-Platform)
- `src/ui/qt/main_window_gui.cpp` - Main Qt GUI window
- `src/ui/qt/dialog_settings_qt.cpp` - Settings editor dialog
- `src/ui/qt/dialog_log_qt.cpp` - Log viewer with search
- `src/ui/qt/config_manager_dialog.cpp` - Configuration file manager
- `src/ui/qt/tray_icon_qt.cpp` - System tray integration

## Applications
- `src/app/yamyd.cpp` - Daemon entry point (background service)
- `src/app/yamy.cpp` - GUI entry point (Qt application)
- `src/app/yamy_ctl.cpp` - Command-line control utility
- `src/app/engine_adapter.cpp` - Adapter bridging app layer to engine

## Utilities
- `src/utils/logger.cpp` - Quill logger initialization (RDTSC timestamps, JSON output)
- `src/utils/metrics.cpp` - Performance metrics collection
- `src/utils/stringtool.cpp` - String manipulation utilities

## Testing
- `tests/property_*.cpp` - RapidCheck property-based tests (keymap, modifier, layer invariants)
- `tests/platform/*_test.cpp` - Platform-specific unit tests
- `tests/test_*.cpp` - Core engine unit tests

## Key Constraints
- **Event Processing**: All keys flow through Layer 1→2→3 pipeline (no special cases)
- **Latency**: <1ms keymap lookup, <1μs logging on critical path
- **Correctness**: PRESS/RELEASE event symmetry (100% tests passing)
- **Concurrency**: Lock-free logging, modifier state machine guarantees no stuck keys
- **Platform Abstraction**: Core engine has ZERO platform-specific code
