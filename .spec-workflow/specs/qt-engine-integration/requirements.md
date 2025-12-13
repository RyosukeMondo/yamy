# Requirements: Qt Engine Integration

## Overview
Replace the stub Engine implementation in the Qt GUI application with the real keyboard remapping engine to enable actual key mapping functionality on Linux.

## User Stories

### US-1: Real Keyboard Remapping
**As a** Linux user
**I want** the Qt GUI to use the real keyboard remapping engine
**So that** I can actually remap keys according to my .mayu configuration files

**Acceptance Criteria:**
- WHEN I load a .mayu configuration file
- THEN the real engine parses and applies the key mappings
- AND keyboard inputs are captured and remapped according to the configuration

### US-2: Configuration Loading
**As a** user
**I want** to load and switch between different .mayu configuration files
**So that** I can use different key mappings for different tasks

**Acceptance Criteria:**
- WHEN I select a configuration file in the Settings dialog
- THEN the real engine loads and validates the .mayu file
- AND the engine applies the new configuration without crashing
- AND I receive notification of success or error

### US-3: Engine State Management
**As a** user
**I want** to enable/disable the engine and see its current state
**So that** I can control when key remapping is active

**Acceptance Criteria:**
- WHEN I click Enable/Disable in the tray menu
- THEN the real engine starts/stops capturing keyboard input
- AND the tray icon reflects the current state (running/stopped)
- AND keyboard input is only remapped when enabled

### US-4: Real-time Status Information
**As a** user
**I want** to see accurate status information (key count, uptime, current keymap)
**So that** I can verify the engine is working correctly

**Acceptance Criteria:**
- WHEN I check status via yamy-ctl or the GUI
- THEN I see real metrics from the actual engine (not stub data)
- AND the key count increases when I press keys
- AND the current keymap matches the active configuration

### US-5: Error Handling
**As a** user
**I want** clear error messages when configuration loading fails
**So that** I can fix problems in my .mayu files

**Acceptance Criteria:**
- WHEN the engine fails to parse a .mayu file
- THEN I receive a desktop notification with the error message
- AND the error is logged for debugging
- AND the engine doesn't crash

## Functional Requirements

### FR-1: Engine Lifecycle Management
The application shall instantiate and manage the real Engine class from `src/core/engine/engine.h` instead of the stub implementation.

**EARS Format:**
- **WHEN** the Qt application starts
- **THEN** the system shall create an instance of the real Engine
- **AND** initialize it with the Linux platform implementations (InputHook, InputInjector, WindowSystem)

### FR-2: Platform Integration
The real Engine shall use the fully-implemented Linux platform abstractions.

**EARS Format:**
- **WHEN** the Engine is created
- **THEN** the system shall provide instances of:
  - `InputHookLinux` for keyboard capture
  - `InputInjectorLinux` for key injection
  - `WindowSystemLinux` for window management
  - `IPCChannelLinux` for inter-process communication

### FR-3: Configuration Loading
The real Engine shall load and parse .mayu configuration files.

**EARS Format:**
- **WHEN** a user selects a .mayu file path
- **THEN** the Engine shall parse the file using the Setting/Parser subsystem
- **AND** validate the configuration syntax
- **AND** notify the GUI of success or failure

### FR-4: GUI-Engine Communication
The GUI shall communicate with the real Engine through a well-defined interface.

**EARS Format:**
- **WHEN** the GUI needs engine status
- **THEN** the system shall query the real Engine's state
- **AND** return actual runtime data (not mock values)

### FR-5: Session Persistence
The Engine state shall be persisted and restored across application restarts.

**EARS Format:**
- **WHEN** the application exits
- **THEN** the system shall save the current configuration path and engine state
- **WHEN** the application starts
- **THEN** the system shall restore the previous configuration

## Non-Functional Requirements

### NFR-1: Performance
- Engine initialization shall complete within 500ms
- Configuration loading shall complete within 2 seconds for typical .mayu files
- Key event latency shall be less than 10ms (p99)

### NFR-2: Stability
- The application shall not crash if a .mayu file contains syntax errors
- The Engine shall handle invalid input gracefully
- The application shall remain responsive during configuration loading

### NFR-3: Compatibility
- The integrated solution shall work with existing .mayu configuration files
- All Qt GUI features (tray icon, dialogs, IPC) shall continue to function
- The solution shall not break Windows compatibility

### NFR-4: Maintainability
- The integration shall use the existing Engine API without modifications
- Platform-specific code shall remain isolated in `src/platform/linux/`
- The solution shall not introduce circular dependencies

## Constraints

### Technical Constraints
- Must use the existing `Engine` class from `src/core/engine/engine.h`
- Must not modify the core Engine implementation
- Must maintain compatibility with the Qt GUI framework
- Must use C++17 standard

### Platform Constraints
- Linux-specific integration in `main_qt.cpp`
- Must use existing Linux platform implementations (InputHook, InputInjector, WindowSystem)
- Must work with X11 window system

## Dependencies

### Internal Dependencies
- Real Engine class (`src/core/engine/engine.h`)
- Linux platform implementations (`src/platform/linux/`)
- Setting/Parser subsystem (`src/core/settings/`)
- Qt GUI components (`src/ui/qt/`)

### External Dependencies
- Qt5 (Core, Widgets, X11Extras)
- X11 libraries (libX11, libXrandr)
- Linux kernel uinput/evdev support

## Success Metrics

1. **Functional Success**: All user stories pass acceptance criteria
2. **Performance**: Key remapping latency < 10ms (p99)
3. **Reliability**: No crashes during 1-hour continuous operation with active key remapping
4. **User Validation**: User confirms keyboard remapping works as expected with their .mayu files
