# Requirements Document

## Introduction

The YAMY GUI Application is a standalone desktop control panel for managing the YAMY keyboard remapping daemon. This addresses the critical Qt5DBus crash issue that occurs when using system tray icons on Ubuntu 24.04+ with libayatana-appindicator3. By separating the GUI into an independent application that communicates with the headless yamy daemon via IPC, we eliminate the problematic interaction between Qt's DBus platform plugins and AppIndicator.

The GUI provides users with an intuitive interface to:
- Monitor YAMY daemon status in real-time
- Enable/disable keyboard remapping
- Switch between multiple configurations
- View logs and debug information
- Access the investigate window for window class inspection
- Manage application preferences

## Alignment with Product Vision

This feature aligns with YAMY's core mission of providing a robust, user-friendly keyboard remapping solution for Linux. By adopting a client-server architecture (daemon + GUI), we follow established Linux patterns used by applications like systemd, Docker, and various network daemons. This architectural decision:

- **Improves Stability**: Eliminates Qt/DBus crashes by avoiding system tray entirely
- **Enhances Flexibility**: Users can run daemon without GUI, control via CLI (yamy-ctl), or launch GUI on-demand
- **Follows Best Practices**: Separates concerns (engine logic vs UI) for better maintainability
- **Reduces Resource Usage**: GUI only runs when needed, daemon remains lightweight

## Requirements

### Requirement 1: Standalone GUI Application

**User Story:** As a YAMY user, I want to launch a desktop application to control the YAMY daemon, so that I can manage keyboard remapping without relying on system tray icons.

#### Acceptance Criteria

1. WHEN user runs `yamy-gui` executable THEN application SHALL launch as standalone Qt window
2. WHEN application launches THEN it SHALL attempt to connect to running yamy daemon via IPC
3. IF yamy daemon is not running THEN application SHALL display connection error with instructions to start daemon
4. WHEN connection succeeds THEN application SHALL display daemon status and configuration
5. WHEN user closes GUI window THEN yamy daemon SHALL continue running in background

### Requirement 2: Daemon Status Monitoring

**User Story:** As a YAMY user, I want to see real-time status of the keyboard remapping daemon, so that I know whether my key mappings are active.

#### Acceptance Criteria

1. WHEN GUI connects to daemon THEN it SHALL display current status (enabled/disabled/error)
2. WHEN daemon state changes THEN GUI SHALL update status display within 500ms
3. WHEN displaying status THEN GUI SHALL show current active configuration name
4. IF daemon reports error THEN GUI SHALL display error message with severity indicator
5. WHEN connection to daemon is lost THEN GUI SHALL show "Disconnected" status

### Requirement 3: Enable/Disable Toggle

**User Story:** As a YAMY user, I want to quickly enable or disable keyboard remapping, so that I can temporarily use default keyboard behavior when needed.

#### Acceptance Criteria

1. WHEN GUI shows daemon status THEN it SHALL display prominent Enable/Disable toggle button
2. WHEN user clicks toggle button THEN GUI SHALL send enable/disable command via IPC
3. WHEN daemon confirms state change THEN GUI SHALL update toggle button state
4. IF command fails THEN GUI SHALL display error notification and revert button state
5. WHEN daemon is disabled THEN status indicator SHALL show gray/inactive color

### Requirement 4: Configuration Management

**User Story:** As a YAMY user, I want to switch between multiple keyboard configurations, so that I can adapt mappings for different workflows or applications.

#### Acceptance Criteria

1. WHEN GUI launches THEN it SHALL retrieve list of available configurations from ConfigManager
2. WHEN displaying configurations THEN GUI SHALL show currently active configuration highlighted
3. WHEN user selects different configuration THEN GUI SHALL send switch command via IPC
4. WHEN configuration switch succeeds THEN GUI SHALL update active configuration display
5. IF configuration switch fails THEN GUI SHALL display error and keep previous selection

### Requirement 5: Configuration Reload

**User Story:** As a YAMY user, I want to reload the current configuration file, so that I can test configuration changes without restarting the daemon.

#### Acceptance Criteria

1. WHEN GUI displays configuration THEN it SHALL provide "Reload" action button
2. WHEN user clicks reload THEN GUI SHALL send reload command to daemon
3. WHEN daemon reloads configuration THEN GUI SHALL display success notification
4. IF reload fails due to syntax errors THEN GUI SHALL display error details from parser
5. WHEN reload succeeds THEN GUI SHALL refresh displayed configuration status

### Requirement 6: Log Viewer Integration

**User Story:** As a YAMY user or developer, I want to view application logs within the GUI, so that I can troubleshoot issues without accessing log files manually.

#### Acceptance Criteria

1. WHEN user opens log viewer THEN GUI SHALL display existing DialogLogQt component
2. WHEN logs update THEN viewer SHALL auto-scroll to latest entries if enabled
3. WHEN user applies filters THEN log display SHALL update to show only matching entries
4. WHEN exporting logs THEN GUI SHALL save to user-selected file location
5. WHEN closing log viewer THEN main GUI window SHALL remain open

### Requirement 7: Investigate Window Access

**User Story:** As a YAMY power user, I want to access the investigate window from the GUI, so that I can identify window classes for creating custom keymaps.

#### Acceptance Criteria

1. WHEN user activates investigate feature THEN GUI SHALL launch DialogInvestigateQt
2. WHEN investigate window is active THEN it SHALL display crosshair for window selection
3. WHEN user selects window THEN investigate window SHALL query daemon for keymap status via IPC
4. WHEN daemon responds THEN investigate window SHALL display window class, title, and matched keymap
5. WHEN investigate window closes THEN main GUI SHALL remain open

### Requirement 8: Settings and Preferences

**User Story:** As a YAMY user, I want to configure application preferences through the GUI, so that I can customize behavior without editing configuration files.

#### Acceptance Criteria

1. WHEN user opens settings THEN GUI SHALL display DialogSettingsQt component
2. WHEN user modifies settings THEN changes SHALL be saved to Qt settings storage
3. WHEN settings include keymap paths THEN GUI SHALL validate paths exist
4. WHEN user saves settings THEN relevant changes SHALL be applied immediately
5. WHEN settings dialog closes THEN main GUI SHALL reflect any preference changes

### Requirement 9: Notification History

**User Story:** As a YAMY user, I want to review past system notifications, so that I can see previous warnings or errors I might have missed.

#### Acceptance Criteria

1. WHEN user opens notification history THEN GUI SHALL display NotificationHistoryDialog
2. WHEN displaying history THEN notifications SHALL be sorted by timestamp descending
3. WHEN user filters notifications THEN display SHALL show only matching severity levels
4. WHEN user clears history THEN all notifications SHALL be removed from storage
5. WHEN notification history is empty THEN GUI SHALL display appropriate empty state message

### Requirement 10: IPC Communication

**User Story:** As the yamy-gui application, I need to communicate with the yamy daemon reliably, so that user commands are executed correctly.

#### Acceptance Criteria

1. WHEN GUI starts THEN it SHALL connect to daemon IPC socket at `/tmp/yamy-engine.sock`
2. WHEN sending command THEN GUI SHALL use IPCChannelQt with proper message serialization
3. WHEN daemon responds THEN GUI SHALL deserialize response and update UI accordingly
4. IF IPC connection fails THEN GUI SHALL retry connection up to 3 times with exponential backoff
5. WHEN daemon terminates THEN GUI SHALL detect disconnection and display appropriate status

## Non-Functional Requirements

### Code Architecture and Modularity

- **Single Responsibility Principle**: Separate IPC communication logic from UI components
- **Modular Design**: Reuse existing Qt dialogs (DialogLogQt, DialogInvestigateQt, etc.) without modification
- **Dependency Management**: GUI depends on IPC layer; avoid direct Engine dependencies
- **Clear Interfaces**: Define clean IPC message protocol for GUI-daemon communication

### Performance

- GUI launch time SHALL be under 1 second on modern hardware
- IPC command round-trip latency SHALL be under 100ms
- Status updates SHALL reflect daemon state changes within 500ms
- GUI SHALL remain responsive during all operations (no blocking UI thread)

### Security

- IPC socket permissions SHALL restrict access to user's UID only
- Configuration file paths SHALL be validated to prevent directory traversal
- No sensitive data (passwords, keys) SHALL be logged or displayed unencrypted
- Command injection SHALL be prevented through proper input sanitization

### Reliability

- GUI SHALL gracefully handle daemon crashes and display connection lost status
- Application SHALL not crash if configuration files are malformed
- Network socket errors SHALL be caught and reported with user-friendly messages
- GUI SHALL recover from IPC disconnections automatically when daemon restarts

### Usability

- Main window layout SHALL be intuitive with clear visual hierarchy
- Status indicators SHALL use color coding (green=active, gray=disabled, red=error)
- All actions SHALL provide immediate visual feedback
- Error messages SHALL be descriptive and suggest corrective actions
- Window SHALL remember size and position across sessions
