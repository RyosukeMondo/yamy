# Tasks Document

## Phase 1: IPC Protocol Extensions

- [ ] 1.1. Extend IPC message types for GUI commands
  - File: `src/core/platform/ipc_defs.h`
  - Add new message types: CmdGetStatus, CmdSetEnabled, CmdSwitchConfig, CmdReloadConfig
  - Add new response types: RspStatus, RspConfigList
  - Define message data structures for each command type
  - Purpose: Establish communication protocol between GUI and daemon
  - _Leverage: existing `yamy::ipc::Message` protocol in `ipc_defs.h`
  - _Requirements: 10_
  - _Prompt: **Role:** IPC Protocol Designer specializing in message-based communication | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Extend the existing IPC message protocol in src/core/platform/ipc_defs.h to add GUI-specific command and response types following requirement 10. Define CmdGetStatus, CmdSetEnabled, CmdSwitchConfig, CmdReloadConfig commands and RspStatus, RspConfigList responses with proper data structures for serialization. | **Restrictions:** Must maintain backward compatibility with existing IPC protocol, do not modify existing message types, follow established naming conventions for message types | **_Leverage:** Examine existing message definitions in src/core/platform/ipc_defs.h for patterns | **_Requirements:** 10 | **Success:** New message types compile without errors, data structures support all required GUI operations, protocol documentation is clear | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (classes, integrations), then mark task as complete [x]

- [ ] 1.2. Implement daemon-side IPC command handlers
  - Files: `src/core/platform/linux/ipc_control_server.cpp`, `src/core/platform/linux/ipc_control_server.h`
  - Add handler methods for each new GUI command type
  - Integrate with Engine for enable/disable, ConfigManager for config operations
  - Send appropriate responses back to GUI client
  - Purpose: Enable daemon to respond to GUI control commands
  - _Leverage: existing IPC server in `ipc_control_server.cpp`, `Engine` class, `ConfigManager` singleton
  - _Requirements: 2, 3, 4, 5_
  - _Prompt: **Role:** Backend Developer with expertise in IPC server implementation and Qt networking | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add command handler methods to IPCControlServer in src/core/platform/linux/ipc_control_server.cpp for processing GUI commands (GetStatus, SetEnabled, SwitchConfig, ReloadConfig) following requirements 2, 3, 4, 5. Handlers should call appropriate Engine and ConfigManager methods and send responses via IPC. | **Restrictions:** Do not block IPC server event loop, handle all error cases gracefully, maintain thread safety when accessing Engine state | **_Leverage:** Study existing IPC server structure in ipc_control_server.cpp, Engine API in src/core/engine/engine.h, ConfigManager singleton pattern | **_Requirements:** 2, 3, 4, 5 | **Success:** All GUI commands are handled correctly, daemon responds with proper status/error messages, no crashes or hangs under error conditions | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, apiEndpoints), then mark task as complete [x]

## Phase 2: IPC Client Wrapper for GUI

- [ ] 2.1. Create IPCClientGUI wrapper class
  - Files: `src/ui/qt/ipc_client_gui.cpp`, `src/ui/qt/ipc_client_gui.h`
  - Implement high-level wrapper around IPCChannelQt for GUI-specific commands
  - Add async command methods (sendGetStatus, sendSetEnabled, etc.)
  - Emit Qt signals for responses and connection status changes
  - Purpose: Provide clean API for GUI components to communicate with daemon
  - _Leverage: `IPCChannelQt` in `src/core/platform/linux/ipc_channel_qt.cpp`
  - _Requirements: 10_
  - _Prompt: **Role:** Qt Developer specializing in signals/slots and asynchronous communication | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create IPCClientGUI wrapper class in src/ui/qt/ipc_client_gui.cpp that wraps IPCChannelQt and provides high-level async methods for sending GUI commands following requirement 10. Use Qt signals to notify GUI of responses and connection changes. | **Restrictions:** Must not block GUI thread, handle message serialization/deserialization properly, emit signals on correct thread (use Qt::QueuedConnection if needed) | **_Leverage:** Study IPCChannelQt implementation in src/core/platform/linux/ipc_channel_qt.cpp for connection handling and message passing | **_Requirements:** 10 | **Success:** All command methods work asynchronously, signals are emitted correctly, connection state is properly tracked | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (classes, functions, integrations), then mark task as complete [x]

- [ ] 2.2. Add auto-reconnection logic to IPCClientGUI
  - File: `src/ui/qt/ipc_client_gui.cpp`
  - Implement exponential backoff retry logic for connection failures
  - Add QTimer-based periodic reconnection attempts
  - Emit connection state changes via signals
  - Purpose: Ensure GUI automatically reconnects if daemon restarts
  - _Leverage: QTimer, existing connection logic in IPCClientGUI
  - _Requirements: 10_
  - _Prompt: **Role:** Qt Developer with expertise in network reconnection patterns | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add auto-reconnection logic to IPCClientGUI in src/ui/qt/ipc_client_gui.cpp with exponential backoff retry strategy following requirement 10. Use QTimer for periodic reconnection attempts and emit proper signals for connection state changes. | **Restrictions:** Maximum 3 retry attempts with backoff (1s, 2s, 4s), do not spam reconnection attempts indefinitely, properly clean up timers on manual disconnect | **_Leverage:** Qt QTimer for scheduled retries, existing connection handling in IPCClientGUI | **_Requirements:** 10 | **Success:** GUI reconnects automatically when daemon restarts, retry backoff works correctly, no resource leaks from timer objects | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

## Phase 3: Main GUI Window

- [ ] 3.1. Create MainWindowGUI base structure
  - Files: `src/ui/qt/main_window_gui.cpp`, `src/ui/qt/main_window_gui.h`
  - Create main window class inheriting from QMainWindow
  - Design UI layout with status display area, control buttons, menu bar
  - Initialize IPCClientGUI instance and connect signals
  - Purpose: Provide main control panel window structure
  - _Leverage: Qt5::Widgets, existing dialog patterns in `src/ui/qt/`
  - _Requirements: 1, 2_
  - _Prompt: **Role:** Qt UI Developer specializing in desktop application design | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create MainWindowGUI class in src/ui/qt/main_window_gui.cpp with base window structure, UI layout, and IPCClientGUI integration following requirements 1, 2. Design clean layout with status display, control buttons (Enable/Disable, Reload), and menu bar for accessing dialogs. | **Restrictions:** Do not create new dialog classes (reuse existing ones), keep main window focused on status/control only, follow Qt widget best practices | **_Leverage:** Study existing Qt dialog implementations in src/ui/qt/ for Qt patterns, use IPCClientGUI for daemon communication | **_Requirements:** 1, 2 | **Success:** Main window displays correctly with all UI elements, connects to daemon via IPC, shows connection status | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (components, integrations), then mark task as complete [x]

- [ ] 3.2. Implement status display and indicators
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add status label showing daemon state (Connected/Disconnected, Enabled/Disabled)
  - Add color-coded indicators (green=active, gray=disabled, red=error)
  - Update status display when receiving daemon state changes
  - Purpose: Give users clear visual feedback of daemon status
  - _Leverage: Qt widgets (QLabel, QPalette), IPCClientGUI signals
  - _Requirements: 2_
  - _Prompt: **Role:** UI/UX Developer with expertise in status visualization | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Implement status display and color-coded indicators in MainWindowGUI (src/ui/qt/main_window_gui.cpp) following requirement 2. Connect to IPCClientGUI signals to update status labels and indicator colors when daemon state changes. | **Restrictions:** Use standard Qt color scheme (avoid hardcoded colors), ensure status text is readable and concise, update UI immediately on state changes | **_Leverage:** Qt QLabel for text display, QPalette for color coding, connect to IPCClientGUI::statusReceived signal | **_Requirements:** 2 | **Success:** Status display updates correctly when daemon state changes, color coding is clear and consistent, disconnected state is visually obvious | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

- [ ] 3.3. Implement enable/disable toggle button
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add prominent toggle button for enabling/disabling remapping
  - Connect button click to IPCClientGUI::sendSetEnabled()
  - Update button state based on daemon response
  - Handle error scenarios with error dialogs
  - Purpose: Allow users to quickly enable/disable keyboard remapping
  - _Leverage: QPushButton, IPCClientGUI, QMessageBox for errors
  - _Requirements: 3_
  - _Prompt: **Role:** Qt Developer with expertise in user interaction handling | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add enable/disable toggle button to MainWindowGUI (src/ui/qt/main_window_gui.cpp) following requirement 3. Button should send enable/disable command via IPCClientGUI and update state based on daemon response, showing error dialog on failure. | **Restrictions:** Disable button during command execution to prevent double-clicks, revert button state if command fails, provide clear error messages to user | **_Leverage:** QPushButton for toggle control, IPCClientGUI::sendSetEnabled for command, QMessageBox::critical for error display | **_Requirements:** 3 | **Success:** Toggle button works correctly, visual state matches daemon state, errors are shown to user with clear messages | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

- [ ] 3.4. Implement configuration selector and reload
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add QComboBox populated with available configurations from ConfigManager
  - Highlight currently active configuration
  - Add reload button to reload current configuration
  - Connect to IPCClientGUI for config switch and reload commands
  - Purpose: Enable users to switch configurations and reload changes
  - _Leverage: QComboBox, QPushButton, IPCClientGUI, ConfigManager
  - _Requirements: 4, 5_
  - _Prompt: **Role:** Qt Developer with expertise in dropdown controls and data binding | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add configuration selector (QComboBox) and reload button to MainWindowGUI (src/ui/qt/main_window_gui.cpp) following requirements 4, 5. Query ConfigManager for available configs, highlight active one, send switch/reload commands via IPCClientGUI. | **Restrictions:** Query ConfigManager through IPC (not directly), handle config switch failures gracefully, disable controls during operations | **_Leverage:** QComboBox for selection dropdown, ConfigManager::instance() for config list (via IPC), IPCClientGUI for switch/reload commands | **_Requirements:** 4, 5 | **Success:** Config dropdown shows all available configs, active config is highlighted, switch and reload operations work correctly with proper error handling | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

## Phase 4: Menu Bar and Dialog Integration

- [ ] 4.1. Create menu bar with dialog actions
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add QMenuBar with menus: File, Tools, Help
  - Add menu actions: Settings, Preferences, Log Viewer, Investigate, Notification History, About, Exit
  - Connect menu actions to slots that show corresponding dialogs
  - Purpose: Provide access to all YAMY functionality from menu
  - _Leverage: QMenuBar, QAction, existing dialog classes
  - _Requirements: 6, 7, 8, 9_
  - _Prompt: **Role:** Qt UI Developer with expertise in menu systems | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create menu bar in MainWindowGUI (src/ui/qt/main_window_gui.cpp) with File, Tools, Help menus following requirements 6, 7, 8, 9. Add actions for launching existing dialogs (Settings, Log Viewer, Investigate, etc). | **Restrictions:** Do not modify existing dialog classes, use standard Qt menu patterns, add keyboard shortcuts for common actions | **_Leverage:** QMenuBar for menu creation, QAction for menu items, existing dialog constructors | **_Requirements:** 6, 7, 8, 9 | **Success:** All menus display correctly, menu actions launch appropriate dialogs, keyboard shortcuts work | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (components), then mark task as complete [x]

- [ ] 4.2. Implement dialog launching slots
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add slots for each menu action (onShowLog, onShowInvestigate, etc.)
  - Instantiate existing dialog classes as needed (modeless or modal)
  - Pass Engine pointer to dialogs that need it (investigate window)
  - Purpose: Connect menu actions to dialog display logic
  - _Leverage: DialogLogQt, DialogInvestigateQt, DialogSettingsQt, DialogAboutQt, NotificationHistoryDialog, PreferencesDialog
  - _Requirements: 6, 7, 8, 9_
  - _Prompt: **Role:** Qt Developer with expertise in dialog management | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add slots to MainWindowGUI (src/ui/qt/main_window_gui.cpp) for launching existing dialogs following requirements 6, 7, 8, 9. Instantiate dialog classes and show them as modal or modeless windows as appropriate. | **Restrictions:** Do not modify dialog classes themselves, use static instances for modeless dialogs to prevent duplicates, properly manage dialog lifetime | **_Leverage:** DialogLogQt, DialogInvestigateQt, DialogSettingsQt, PreferencesDialog, NotificationHistoryDialog, DialogAboutQt constructors | **_Requirements:** 6, 7, 8, 9 | **Success:** All dialogs launch correctly from menu, dialogs function properly when shown from GUI, no crashes or memory leaks | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, integrations), then mark task as complete [x]

## Phase 5: yamy-gui Executable

- [ ] 5.1. Create main_gui.cpp entry point
  - File: `src/app/main_gui.cpp`
  - Create QApplication instance (full Qt with platform plugins - safe without system tray)
  - Instantiate and show MainWindowGUI
  - Run Qt event loop
  - Handle command-line arguments (if any)
  - Purpose: Provide executable entry point for yamy-gui application
  - _Leverage: Qt5::Widgets, MainWindowGUI
  - _Requirements: 1_
  - _Prompt: **Role:** Application Developer with expertise in Qt application lifecycle | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create main_gui.cpp entry point in src/app/ following requirement 1. Set up QApplication, initialize MainWindowGUI, and run event loop. Add basic command-line argument parsing if needed. | **Restrictions:** Do not include any system tray code, use QApplication (not QCoreApplication) for full widget support, handle cleanup properly on exit | **_Leverage:** Qt QApplication for app initialization, MainWindowGUI for main window | **_Requirements:** 1 | **Success:** yamy-gui executable launches successfully, main window shows correctly, application exits cleanly on window close | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (components), then mark task as complete [x]

- [ ] 5.2. Add CMake build target for yamy-gui
  - File: `CMakeLists.txt`
  - Add executable target for yamy-gui linking necessary libraries
  - Link yamy_qt_gui library (contains existing dialogs)
  - Add install target to bin directory
  - Purpose: Build yamy-gui as separate executable
  - _Leverage: existing yamy_qt_gui library target
  - _Requirements: 1_
  - _Prompt: **Role:** Build Engineer with expertise in CMake | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add CMake build target for yamy-gui executable in CMakeLists.txt following requirement 1. Create executable from src/app/main_gui.cpp and link required Qt and yamy_qt_gui libraries. Add install rule. | **Restrictions:** Do not modify existing build targets, ensure yamy-gui links only necessary libraries, verify build succeeds on Linux | **_Leverage:** Existing yamy_qt_gui library target, Qt5::Widgets, Qt5::Network | **_Requirements:** 1 | **Success:** yamy-gui builds successfully, executable installs to bin/, application runs without library errors | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (integrations), then mark task as complete [x]

## Phase 6: Daemon Headless Refactoring

- [ ] 6.1. Refactor main.cpp to headless daemon
  - File: `src/app/main.cpp`
  - Replace QApplication with QCoreApplication (minimal Qt, no GUI)
  - Remove all SystemTrayAppIndicator code and system tray references
  - Remove QSystemTrayIcon availability check
  - Keep Engine initialization and IPC server
  - Purpose: Make yamy daemon run without Qt GUI platform plugins
  - _Leverage: QCoreApplication, existing Engine and IPC code
  - _Requirements: 1_
  - _Prompt: **Role:** System Developer with expertise in daemon processes | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Refactor src/app/main.cpp to run as headless daemon following requirement 1. Replace QApplication with QCoreApplication, remove all system tray code (SystemTrayAppIndicator, QSystemTrayIcon checks), keep Engine and IPC server functional. | **Restrictions:** Do not break existing Engine functionality, ensure IPC server still works, verify daemon runs without loading Qt platform plugins | **_Leverage:** QCoreApplication for minimal Qt event loop, existing Engine initialization code, IPCControlServer | **_Requirements:** 1 | **Success:** yamy daemon runs without GUI dependencies, no Qt/DBus crashes, IPC server accessible for yamy-gui connection | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, integrations), then mark task as complete [x]

- [ ] 6.2. Update CMakeLists.txt for headless daemon
  - File: `CMakeLists.txt`
  - Update yamy target to link QCoreApplication (remove Widgets dependency)
  - Remove AppIndicator, GTK, libnotify dependencies from yamy target
  - Verify yamy executable builds as headless
  - Purpose: Ensure daemon builds without GUI libraries
  - _Leverage: existing yamy build target
  - _Requirements: 1_
  - _Prompt: **Role:** Build Engineer with expertise in dependency management | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Update yamy target in CMakeLists.txt to build as headless daemon following requirement 1. Change from Qt5::Widgets to Qt5::Core, remove AppIndicator/GTK/libnotify dependencies, verify build succeeds. | **Restrictions:** Do not remove dependencies still needed by Engine core, ensure IPC still works (needs Qt5::Network), test build completes successfully | **_Leverage:** Existing yamy CMake target, Qt5::Core for minimal Qt | **_Requirements:** 1 | **Success:** yamy builds without GUI library dependencies, binary size is reduced, daemon runs headless successfully | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 7: Desktop Integration

- [ ] 7.1. Create desktop entry file
  - File: `resources/yamy-gui.desktop`
  - Create freedesktop.org compliant .desktop file
  - Add application name, icon, categories, exec command
  - Purpose: Enable launching yamy-gui from application menu
  - _Leverage: freedesktop.org desktop entry specification
  - _Requirements: 1_
  - _Prompt: **Role:** Linux Desktop Integration Specialist | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create freedesktop.org compliant desktop entry file at resources/yamy-gui.desktop following requirement 1. Include application name, icon, categories (Utility;Settings;), exec command, and other required fields. | **Restrictions:** Follow desktop entry specification exactly, use standard categories, ensure Icon field references installed icon | **_Leverage:** Freedesktop.org desktop entry spec, existing YAMY icon resources | **_Requirements:** 1 | **Success:** Desktop file validates with desktop-file-validate, yamy-gui appears in application launcher, clicking launches GUI correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 7.2. Add desktop file installation to CMake
  - File: `CMakeLists.txt`
  - Add install rule for yamy-gui.desktop to share/applications/
  - Add install rule for application icon to share/icons/
  - Purpose: Install desktop entry and icon during make install
  - _Leverage: CMake install() command
  - _Requirements: 1_
  - _Prompt: **Role:** Build Engineer with expertise in Linux installation | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add CMake install rules for yamy-gui.desktop file and application icon following requirement 1. Install desktop file to share/applications/, icon to share/icons/hicolor/. | **Restrictions:** Follow Linux filesystem hierarchy standard, ensure files install to correct locations, test with `make install DESTDIR=/tmp/test` | **_Leverage:** CMake install() function, existing icon installation patterns in CMakeLists.txt | **_Requirements:** 1 | **Success:** Desktop file and icon install correctly, application appears in launcher after installation, icon displays properly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 8: Documentation and Testing

- [ ] 8.1. Update user documentation
  - Files: `docs/user-manual.md`, `README.md`
  - Document new yamy-gui application usage
  - Update architecture section explaining daemon + GUI separation
  - Add troubleshooting section for connection issues
  - Purpose: Help users understand and use new yamy-gui application
  - _Leverage: existing documentation structure
  - _Requirements: All_
  - _Prompt: **Role:** Technical Writer with Linux application expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Update user documentation in docs/user-manual.md and README.md covering all requirements. Explain new daemon+GUI architecture, how to launch yamy-gui, troubleshoot connection issues, and use all features. | **Restrictions:** Keep documentation concise and clear, use screenshots if helpful, ensure all features are documented | **_Leverage:** Existing documentation structure and style in docs/ | **_Requirements:** All | **Success:** Documentation is complete and accurate, users can understand architecture and use all features, troubleshooting guide helps resolve common issues | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 8.2. Add integration tests for GUI-daemon communication
  - File: `tests/integration/test_gui_daemon_communication.cpp`
  - Write tests for IPC command/response flow
  - Test connection failure and reconnection scenarios
  - Test all GUI commands (status, enable, switch config, reload)
  - Purpose: Ensure GUI-daemon communication works reliably
  - _Leverage: existing test infrastructure, IPCClientGUI, mock daemon
  - _Requirements: 10_
  - _Prompt: **Role:** QA Engineer with expertise in Qt testing and IPC | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create integration tests in tests/integration/ for GUI-daemon IPC communication following requirement 10. Test all command types, error scenarios, connection/reconnection logic. | **Restrictions:** Use mock daemon or test fixture for reliable tests, ensure tests are deterministic and fast, cover both success and failure paths | **_Leverage:** Qt Test framework, IPCClientGUI class, existing test patterns in tests/ | **_Requirements:** 10 | **Success:** All IPC communication paths are tested, tests pass reliably, error scenarios are covered | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, integrations), then mark task as complete [x]

- [ ] 8.3. Manual testing and bug fixes
  - Test all GUI features end-to-end
  - Test on fresh Ubuntu installation
  - Verify no Qt/DBus crashes occur
  - Fix any discovered bugs
  - Purpose: Ensure yamy-gui works correctly in real-world usage
  - _Leverage: full yamy-gui and yamy daemon
  - _Requirements: All_
  - _Prompt: **Role:** QA Tester with Linux desktop expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Perform comprehensive manual testing of yamy-gui on Ubuntu system covering all requirements. Test all features, edge cases, and error scenarios. Document and fix any bugs found. | **Restrictions:** Test on clean system if possible, verify no Qt/DBus crashes, test daemon restart scenarios | **_Leverage:** Full yamy-gui and yamy daemon, test Ubuntu environment | **_Requirements:** All | **Success:** All features work correctly, no crashes or major bugs, application is ready for release | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts including bug fixes, then mark task as complete [x]
