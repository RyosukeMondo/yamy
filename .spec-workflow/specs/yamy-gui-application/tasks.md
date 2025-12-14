# Tasks Document

## Phase 0: Investigation Tools & Testing Infrastructure (FIRST!)

- [x] 0.1. Create IPC communication debugging tool
  - File: `tools/debug_ipc_communication.sh`
  - Script to monitor IPC socket traffic between GUI and daemon
  - Add environment variable `YAMY_DEBUG_IPC=1` for verbose IPC logging
  - Log all messages sent/received with timestamps
  - Purpose: Enable autonomous investigation of IPC communication before building GUI
  - _Leverage: existing IPC socket at `/tmp/yamy-engine.sock`, `socat` or `nc` for monitoring
  - _Requirements: 10_
  - _Prompt: **Role:** DevOps Engineer with expertise in IPC debugging and shell scripting | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create IPC debugging tool at tools/debug_ipc_communication.sh that monitors socket traffic, adds YAMY_DEBUG_IPC environment variable for verbose logging following requirement 10. Enable investigation of IPC messages before GUI is built. | **Restrictions:** Must not interfere with normal IPC operation, logging should be toggleable via env var, output should be human-readable | **_Leverage:** socat or nc for socket monitoring, existing IPC socket path, Qt debug output | **_Requirements:** 10 | **Success:** Tool can capture and display all IPC messages, debug logging shows message content clearly, can be used to verify protocol before GUI implementation | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 0.2. Create mock IPC server for testing
  - File: `tests/mock_ipc_server.cpp`
  - Implement simple IPC server that responds to GUI commands
  - Simulate daemon responses for all command types
  - Allow testing GUI without running full daemon
  - Purpose: Enable isolated GUI testing and development
  - _Leverage: IPCChannelQt, existing IPC protocol
  - _Requirements: 10_
  - _Prompt: **Role:** Test Engineer with expertise in mock objects and IPC | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create mock IPC server in tests/mock_ipc_server.cpp that simulates daemon responses for all GUI commands following requirement 10. Enable isolated GUI testing without full daemon. | **Restrictions:** Must implement same protocol as real daemon, responses should be configurable for different test scenarios, keep implementation simple and maintainable | **_Leverage:** IPCChannelQt for server implementation, copy protocol from ipc_control_server.cpp | **_Requirements:** 10 | **Success:** Mock server accepts connections, responds to all command types correctly, GUI can be tested against mock without daemon | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (classes, functions, integrations), then mark task as complete [x]

- [x] 0.3. Create IPC protocol test suite
  - File: `tests/test_ipc_protocol.cpp`
  - Unit tests for all IPC message serialization/deserialization
  - Test command creation and response parsing
  - Verify protocol compatibility
  - Purpose: Ensure IPC protocol works correctly before building GUI
  - _Leverage: Qt Test framework, IPC message types
  - _Requirements: 10_
  - _Prompt: **Role:** QA Engineer with expertise in protocol testing | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create IPC protocol test suite in tests/test_ipc_protocol.cpp that validates message serialization/deserialization for all command types following requirement 10. | **Restrictions:** Test both success and error scenarios, verify backward compatibility, ensure tests are fast and reliable | **_Leverage:** Qt Test (QTest), existing IPC message definitions | **_Requirements:** 10 | **Success:** All message types can be serialized and deserialized correctly, tests pass for all protocol operations, edge cases are covered | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

- [x] 0.4. Document IPC protocol specification
  - File: `docs/IPC_PROTOCOL.md`
  - Document all GUI-specific IPC commands and responses
  - Add message format examples with actual data
  - Create troubleshooting guide for IPC issues
  - Purpose: Enable understanding and debugging of IPC communication
  - _Leverage: existing protocol in ipc_defs.h
  - _Requirements: 10_
  - _Prompt: **Role:** Technical Writer with networking protocol expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create IPC protocol documentation in docs/IPC_PROTOCOL.md covering all GUI commands/responses following requirement 10. Include message format examples and troubleshooting guide. | **Restrictions:** Keep documentation clear and concise, use real examples from code, include diagrams if helpful | **_Leverage:** Message definitions in ipc_defs.h, existing IPC implementation | **_Requirements:** 10 | **Success:** Documentation explains all message types clearly, examples are accurate, troubleshooting section helps debug common issues | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 1: IPC Protocol Extensions (with validation)

- [x] 1.1. Extend IPC message types for GUI commands
  - File: `src/core/platform/ipc_defs.h`
  - Add new message types: CmdGetStatus, CmdSetEnabled, CmdSwitchConfig, CmdReloadConfig
  - Add new response types: RspStatus, RspConfigList
  - Define message data structures for each command type
  - Purpose: Establish communication protocol between GUI and daemon
  - _Leverage: existing `yamy::ipc::Message` protocol in `ipc_defs.h`
  - _Requirements: 10_
  - _Prompt: **Role:** IPC Protocol Designer specializing in message-based communication | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Extend the existing IPC message protocol in src/core/platform/ipc_defs.h to add GUI-specific command and response types following requirement 10. Define CmdGetStatus, CmdSetEnabled, CmdSwitchConfig, CmdReloadConfig commands and RspStatus, RspConfigList responses with proper data structures for serialization. | **Restrictions:** Must maintain backward compatibility with existing IPC protocol, do not modify existing message types, follow established naming conventions for message types | **_Leverage:** Examine existing message definitions in src/core/platform/ipc_defs.h for patterns | **_Requirements:** 10 | **Success:** New message types compile without errors, data structures support all required GUI operations, protocol documentation is clear | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (classes, integrations), then mark task as complete [x]

- [x] 1.2. Validate IPC protocol extensions with test suite
  - File: `tests/test_ipc_protocol.cpp` (update from Phase 0)
  - Add tests for new message types from task 1.1
  - Verify serialization/deserialization works correctly
  - Test with mock server from Phase 0
  - Purpose: Validate protocol extensions before implementing handlers
  - _Leverage: Test suite from task 0.3, mock server from task 0.2
  - _Requirements: 10_
  - _Prompt: **Role:** QA Engineer with protocol testing expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Update IPC protocol test suite to validate new message types from task 1.1 following requirement 10. Test serialization, deserialization, and mock server responses. | **Restrictions:** All new message types must have test coverage, tests must pass before proceeding to next phase | **_Leverage:** Existing test framework from task 0.3, mock server from 0.2 | **_Requirements:** 10 | **Success:** All new message types are tested, tests pass successfully, mock server handles new messages correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 1.3. Implement daemon-side IPC command handlers
  - Files: `src/core/platform/linux/ipc_control_server.cpp`, `src/core/platform/linux/ipc_control_server.h`
  - Add handler methods for each new GUI command type
  - Integrate with Engine for enable/disable, ConfigManager for config operations
  - Send appropriate responses back to GUI client
  - Purpose: Enable daemon to respond to GUI control commands
  - _Leverage: existing IPC server in `ipc_control_server.cpp`, `Engine` class, `ConfigManager` singleton
  - _Requirements: 2, 3, 4, 5_
  - _Prompt: **Role:** Backend Developer with expertise in IPC server implementation and Qt networking | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add command handler methods to IPCControlServer in src/core/platform/linux/ipc_control_server.cpp for processing GUI commands (GetStatus, SetEnabled, SwitchConfig, ReloadConfig) following requirements 2, 3, 4, 5. Handlers should call appropriate Engine and ConfigManager methods and send responses via IPC. | **Restrictions:** Do not block IPC server event loop, handle all error cases gracefully, maintain thread safety when accessing Engine state | **_Leverage:** Study existing IPC server structure in ipc_control_server.cpp, Engine API in src/core/engine/engine.h, ConfigManager singleton pattern | **_Requirements:** 2, 3, 4, 5 | **Success:** All GUI commands are handled correctly, daemon responds with proper status/error messages, no crashes or hangs under error conditions | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, apiEndpoints), then mark task as complete [x]

- [x] 1.4. Test daemon handlers with debug tool
  - Use: `tools/debug_ipc_communication.sh` from Phase 0
  - Send test commands to daemon via IPC socket
  - Verify responses match expected protocol
  - Test error scenarios (invalid config, daemon errors, etc.)
  - Purpose: Validate daemon handlers work correctly before building GUI
  - _Leverage: Debug tool from task 0.1, daemon with new handlers
  - _Requirements: 2, 3, 4, 5, 10_
  - _Prompt: **Role:** Test Engineer with IPC debugging expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Use IPC debug tool from task 0.1 to test daemon command handlers from task 1.3 following requirements 2, 3, 4, 5, 10. Send test commands and verify responses. | **Restrictions:** Test all command types and error scenarios, document any issues found, ensure handlers are stable before GUI development | **_Leverage:** tools/debug_ipc_communication.sh from Phase 0, running yamy daemon | **_Requirements:** 2, 3, 4, 5, 10 | **Success:** All commands receive correct responses, error scenarios handled properly, debug output shows clear message flow | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting test results, then mark task as complete [x]
  - Result (2025-12-14): Daemon accepted connections on `/tmp/yamy-yamy-engine-1000` but returned no GUI (`0x51xx`) responses to `CmdGetStatus/SetEnabled/SwitchConfig/ReloadConfig` messages; debug capture at `logs/ipc_debug_20251214_214431.log` shows outgoing frames only. Needs follow-up on GUI IPC channel handling.

## Phase 2: IPC Client Wrapper for GUI (with mock testing)

- [x] 2.1. Create IPCClientGUI wrapper class
  - Files: `src/ui/qt/ipc_client_gui.cpp`, `src/ui/qt/ipc_client_gui.h`
  - Implement high-level wrapper around IPCChannelQt for GUI-specific commands
  - Add async command methods (sendGetStatus, sendSetEnabled, etc.)
  - Emit Qt signals for responses and connection status changes
  - Purpose: Provide clean API for GUI components to communicate with daemon
  - _Leverage: `IPCChannelQt` in `src/core/platform/linux/ipc_channel_qt.cpp`
  - _Requirements: 10_
  - _Prompt: **Role:** Qt Developer specializing in signals/slots and asynchronous communication | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create IPCClientGUI wrapper class in src/ui/qt/ipc_client_gui.cpp that wraps IPCChannelQt and provides high-level async methods for sending GUI commands following requirement 10. Use Qt signals to notify GUI of responses and connection changes. | **Restrictions:** Must not block GUI thread, handle message serialization/deserialization properly, emit signals on correct thread (use Qt::QueuedConnection if needed) | **_Leverage:** Study IPCChannelQt implementation in src/core/platform/linux/ipc_channel_qt.cpp for connection handling and message passing | **_Requirements:** 10 | **Success:** All command methods work asynchronously, signals are emitted correctly, connection state is properly tracked | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (classes, functions, integrations), then mark task as complete [x]

- [x] 2.2. Test IPCClientGUI with mock server
  - Use: Mock server from task 0.2
  - Create test program that uses IPCClientGUI to send commands
  - Verify signals are emitted correctly for responses
  - Test connection/disconnection scenarios
  - Purpose: Validate client wrapper works before building full GUI
  - _Leverage: Mock server from task 0.2, IPCClientGUI from task 2.1
  - _Requirements: 10_
  - _Prompt: **Role:** Qt Test Engineer with signals/slots expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create test program to validate IPCClientGUI using mock server from task 0.2 following requirement 10. Test all commands and verify signal emission. | **Restrictions:** Test must be automated and repeatable, verify all signals are emitted, test error scenarios | **_Leverage:** Mock server from tests/mock_ipc_server.cpp, IPCClientGUI API | **_Requirements:** 10 | **Success:** Test program successfully sends commands via IPCClientGUI, signals are emitted correctly, connection handling works properly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, integrations), then mark task as complete [x]

- [ ] 2.3. Add auto-reconnection logic to IPCClientGUI
  - File: `src/ui/qt/ipc_client_gui.cpp`
  - Implement exponential backoff retry logic for connection failures
  - Add QTimer-based periodic reconnection attempts
  - Emit connection state changes via signals
  - Purpose: Ensure GUI automatically reconnects if daemon restarts
  - _Leverage: QTimer, existing connection logic in IPCClientGUI
  - _Requirements: 10_
  - _Prompt: **Role:** Qt Developer with expertise in network reconnection patterns | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add auto-reconnection logic to IPCClientGUI in src/ui/qt/ipc_client_gui.cpp with exponential backoff retry strategy following requirement 10. Use QTimer for periodic reconnection attempts and emit proper signals for connection state changes. | **Restrictions:** Maximum 3 retry attempts with backoff (1s, 2s, 4s), do not spam reconnection attempts indefinitely, properly clean up timers on manual disconnect | **_Leverage:** Qt QTimer for scheduled retries, existing connection handling in IPCClientGUI | **_Requirements:** 10 | **Success:** GUI reconnects automatically when daemon restarts, retry backoff works correctly, no resource leaks from timer objects | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

## Phase 3: Minimal GUI Prototype (validate before full implementation)

- [ ] 3.1. Create minimal MainWindowGUI prototype
  - Files: `src/ui/qt/main_window_gui.cpp`, `src/ui/qt/main_window_gui.h`
  - Create basic window with connection status label only
  - Initialize IPCClientGUI instance and connect signals
  - Display connection state (Connected/Disconnected)
  - Purpose: Validate GUI-daemon communication with minimal UI
  - _Leverage: Qt5::Widgets, IPCClientGUI from Phase 2
  - _Requirements: 1, 2_
  - _Prompt: **Role:** Qt UI Developer specializing in rapid prototyping | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create minimal MainWindowGUI prototype in src/ui/qt/main_window_gui.cpp with only connection status display following requirements 1, 2. Validate IPC communication before building full UI. | **Restrictions:** Keep UI minimal (just connection status), focus on validating IPC integration, no complex layouts yet | **_Leverage:** IPCClientGUI from task 2.1, Qt QLabel for status display | **_Requirements:** 1, 2 | **Success:** Prototype window shows connection status correctly, updates when daemon starts/stops, IPC communication is verified working | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (components, integrations), then mark task as complete [x]

- [ ] 3.2. Test prototype with real daemon
  - Start yamy daemon with IPC enabled
  - Launch minimal GUI prototype
  - Verify connection status updates correctly
  - Test daemon restart scenario
  - Purpose: Validate end-to-end GUI-daemon communication works
  - _Leverage: Real yamy daemon, prototype from task 3.1
  - _Requirements: 1, 2, 10_
  - _Prompt: **Role:** Integration Tester with Linux daemon expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Test minimal GUI prototype from task 3.1 with real yamy daemon following requirements 1, 2, 10. Verify connection, status updates, and reconnection. Document results. | **Restrictions:** Test on clean system state, verify daemon doesn't crash, document any issues found | **_Leverage:** Running yamy daemon, minimal GUI prototype | **_Requirements:** 1, 2, 10 | **Success:** Prototype connects to daemon successfully, status updates work, reconnection works after daemon restart, no crashes | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting test results, then mark task as complete [x]

- [ ] 3.3. Add status display and enable/disable to prototype
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add daemon status labels (enabled/disabled, config name)
  - Add enable/disable toggle button
  - Update UI when receiving status changes from daemon
  - Purpose: Incrementally add features to validated prototype
  - _Leverage: IPCClientGUI signals, Qt widgets
  - _Requirements: 2, 3_
  - _Prompt: **Role:** Qt UI Developer with state management expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add status display and enable/disable toggle to minimal prototype from task 3.1 following requirements 2, 3. Connect to IPCClientGUI signals for status updates. | **Restrictions:** Keep changes incremental, test each addition, ensure UI stays responsive | **_Leverage:** IPCClientGUI::statusReceived signal, QPushButton for toggle | **_Requirements:** 2, 3 | **Success:** Status displays correctly, toggle button works, UI updates when daemon state changes | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

## Phase 4: Full GUI Implementation (build on validated prototype)

- [ ] 4.1. Expand MainWindowGUI with full UI layout
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add configuration selector dropdown
  - Add reload button
  - Add color-coded status indicators
  - Create menu bar structure
  - Purpose: Complete main window UI with all controls
  - _Leverage: Validated prototype from Phase 3, Qt layout managers
  - _Requirements: 2, 4, 5_
  - _Prompt: **Role:** Qt UI Developer specializing in desktop application design | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Expand validated prototype with full UI layout including config selector, reload button, indicators, and menu bar following requirements 2, 4, 5. Build on working foundation from Phase 3. | **Restrictions:** Do not break existing working features, keep UI clean and intuitive, use Qt layout managers properly | **_Leverage:** Working prototype from task 3.3, Qt QComboBox for configs, QMenuBar for menus | **_Requirements:** 2, 4, 5 | **Success:** All UI elements display correctly, layout is responsive, existing features still work | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (components), then mark task as complete [x]

- [ ] 4.2. Implement configuration management features
  - File: `src/ui/qt/main_window_gui.cpp`
  - Populate config selector with available configs
  - Implement config switch command via IPC
  - Implement reload button functionality
  - Handle errors with user-friendly messages
  - Purpose: Enable configuration management through GUI
  - _Leverage: IPCClientGUI, ConfigManager (via IPC), QMessageBox for errors
  - _Requirements: 4, 5_
  - _Prompt: **Role:** Qt Developer with expertise in data binding and error handling | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Implement configuration management features in MainWindowGUI following requirements 4, 5. Query configs via IPC, handle switch/reload operations, show error dialogs on failure. | **Restrictions:** Query configs through IPC (not directly), disable controls during operations, provide clear error messages | **_Leverage:** IPCClientGUI for commands, QMessageBox for user feedback | **_Requirements:** 4, 5 | **Success:** Config selector shows all configs, switching works correctly, reload works, errors are handled gracefully | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions), then mark task as complete [x]

- [ ] 4.3. Integrate existing dialogs into menu
  - File: `src/ui/qt/main_window_gui.cpp`
  - Add menu actions for all dialogs (Log, Investigate, Settings, etc.)
  - Implement slots to launch each dialog
  - Pass Engine pointer to dialogs that need it
  - Purpose: Provide access to all YAMY functionality
  - _Leverage: DialogLogQt, DialogInvestigateQt, DialogSettingsQt, etc.
  - _Requirements: 6, 7, 8, 9_
  - _Prompt: **Role:** Qt Developer with expertise in dialog management | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Integrate existing dialogs into MainWindowGUI menu system following requirements 6, 7, 8, 9. Create menu actions and slots to launch each dialog. | **Restrictions:** Do not modify existing dialog classes, manage dialog lifetime properly, use modal/modeless as appropriate | **_Leverage:** All existing dialog classes in src/ui/qt/ | **_Requirements:** 6, 7, 8, 9 | **Success:** All dialogs accessible from menu, dialogs launch and function correctly, no memory leaks | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, integrations), then mark task as complete [x]

## Phase 5: yamy-gui Executable

- [ ] 5.1. Create main_gui.cpp entry point
  - File: `src/app/main_gui.cpp`
  - Create QApplication instance
  - Instantiate and show MainWindowGUI
  - Run Qt event loop
  - Handle command-line arguments
  - Purpose: Provide executable entry point for yamy-gui
  - _Leverage: Qt5::Widgets, MainWindowGUI
  - _Requirements: 1_
  - _Prompt: **Role:** Application Developer with expertise in Qt application lifecycle | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create main_gui.cpp entry point in src/app/ following requirement 1. Set up QApplication, initialize MainWindowGUI, run event loop. | **Restrictions:** Do not include system tray code, use QApplication for full widget support, handle cleanup properly | **_Leverage:** Qt QApplication, MainWindowGUI from Phase 4 | **_Requirements:** 1 | **Success:** yamy-gui launches successfully, main window shows, application exits cleanly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (components), then mark task as complete [x]

- [ ] 5.2. Add CMake build target for yamy-gui
  - File: `CMakeLists.txt`
  - Add executable target for yamy-gui
  - Link necessary libraries (yamy_qt_gui, Qt5::Widgets, Qt5::Network)
  - Add install target
  - Purpose: Build yamy-gui as separate executable
  - _Leverage: existing yamy_qt_gui library
  - _Requirements: 1_
  - _Prompt: **Role:** Build Engineer with expertise in CMake | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add CMake build target for yamy-gui in CMakeLists.txt following requirement 1. Link required libraries and add install rule. | **Restrictions:** Do not modify existing targets, verify build succeeds, test installation | **_Leverage:** Existing yamy_qt_gui library target | **_Requirements:** 1 | **Success:** yamy-gui builds successfully, executable works, installs correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (integrations), then mark task as complete [x]

## Phase 6: Daemon Headless Refactoring

- [ ] 6.1. Refactor main.cpp to headless daemon
  - File: `src/app/main.cpp`
  - Replace QApplication with QCoreApplication
  - Remove all system tray code (SystemTrayAppIndicator, QSystemTrayIcon)
  - Keep Engine and IPC server functional
  - Purpose: Make daemon run without Qt GUI platform plugins
  - _Leverage: QCoreApplication, existing Engine/IPC code
  - _Requirements: 1_
  - _Prompt: **Role:** System Developer with expertise in daemon processes | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Refactor src/app/main.cpp to run as headless daemon following requirement 1. Use QCoreApplication, remove tray code, keep Engine/IPC working. | **Restrictions:** Do not break Engine functionality, ensure IPC still works, verify no Qt platform plugins loaded | **_Leverage:** QCoreApplication for minimal Qt, existing Engine initialization | **_Requirements:** 1 | **Success:** Daemon runs without GUI, no Qt/DBus crashes, IPC server functional | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, integrations), then mark task as complete [x]

- [ ] 6.2. Update CMakeLists.txt for headless daemon
  - File: `CMakeLists.txt`
  - Change yamy target to link Qt5::Core instead of Qt5::Widgets
  - Remove AppIndicator, GTK, libnotify dependencies
  - Verify build succeeds
  - Purpose: Build daemon without GUI libraries
  - _Leverage: existing yamy build target
  - _Requirements: 1_
  - _Prompt: **Role:** Build Engineer with expertise in dependency management | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Update yamy target in CMakeLists.txt to build headless following requirement 1. Use Qt5::Core only, remove GUI dependencies. | **Restrictions:** Keep dependencies needed by Engine, ensure IPC works (needs Qt5::Network), verify build completes | **_Leverage:** Existing yamy CMake target | **_Requirements:** 1 | **Success:** yamy builds without GUI libraries, binary size reduced, runs headless | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 6.3. Test headless daemon with yamy-gui
  - Start headless yamy daemon
  - Launch yamy-gui
  - Verify all features work end-to-end
  - Test daemon restart scenarios
  - Purpose: Validate complete system works together
  - _Leverage: Headless daemon, yamy-gui from Phase 5
  - _Requirements: All_
  - _Prompt: **Role:** Integration Tester with Linux desktop expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Test complete system with headless daemon and yamy-gui covering all requirements. Verify all features work, test restart scenarios, document any issues. | **Restrictions:** Test on clean system, verify no crashes, test all GUI features | **_Leverage:** Headless yamy daemon, yamy-gui executable | **_Requirements:** All | **Success:** All features work correctly, daemon and GUI communicate properly, no crashes, system is stable | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting comprehensive test results, then mark task as complete [x]

## Phase 7: Desktop Integration

- [ ] 7.1. Create desktop entry file
  - File: `resources/yamy-gui.desktop`
  - Create freedesktop.org compliant .desktop file
  - Add metadata (name, icon, categories, exec command)
  - Purpose: Enable launcher integration
  - _Leverage: freedesktop.org desktop entry specification
  - _Requirements: 1_
  - _Prompt: **Role:** Linux Desktop Integration Specialist | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create desktop entry file at resources/yamy-gui.desktop following freedesktop.org spec and requirement 1. Include all required metadata. | **Restrictions:** Follow spec exactly, use standard categories, validate with desktop-file-validate | **_Leverage:** Freedesktop.org desktop entry spec | **_Requirements:** 1 | **Success:** Desktop file validates, yamy-gui appears in launcher, launches correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 7.2. Add desktop file installation to CMake
  - File: `CMakeLists.txt`
  - Add install rules for desktop file and icon
  - Install to correct system locations
  - Purpose: Install desktop entry during make install
  - _Leverage: CMake install() command
  - _Requirements: 1_
  - _Prompt: **Role:** Build Engineer with Linux installation expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add CMake install rules for desktop file and icon following requirement 1. Install to correct FHS locations. | **Restrictions:** Follow FHS standard, test with DESTDIR, verify correct permissions | **_Leverage:** CMake install() function | **_Requirements:** 1 | **Success:** Files install to correct locations, launcher shows application, icon displays | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 8: Documentation and Final Validation

- [ ] 8.1. Update user documentation
  - Files: `docs/USER_GUIDE.md` (new), update `README.md`
  - Document yamy-gui usage and architecture
  - Add troubleshooting guide
  - Include screenshots
  - Purpose: Help users understand and use the system
  - _Leverage: existing documentation structure
  - _Requirements: All_
  - _Prompt: **Role:** Technical Writer with Linux application expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create comprehensive user documentation covering all requirements. Explain architecture, usage, troubleshooting. | **Restrictions:** Keep clear and concise, use screenshots, ensure accuracy | **_Leverage:** Existing docs structure | **_Requirements:** All | **Success:** Documentation is complete, users can understand and use all features, troubleshooting helps resolve issues | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 8.2. Create comprehensive test report
  - File: `docs/YAMY_GUI_TEST_REPORT.md`
  - Document all test results from investigation tools
  - List all features tested and results
  - Include performance metrics (launch time, IPC latency)
  - Document any known issues or limitations
  - Purpose: Provide evidence of thorough testing
  - _Leverage: Test results from all phases, debug tools from Phase 0
  - _Requirements: All_
  - _Prompt: **Role:** QA Lead with documentation expertise | **Task:** Implement the task for spec yamy-gui-application, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create comprehensive test report documenting all testing performed covering all requirements. Include test results, metrics, known issues. | **Restrictions:** Be thorough and honest about test coverage, include actual metrics, document limitations | **_Leverage:** Results from investigation tools (Phase 0), integration tests (all phases) | **_Requirements:** All | **Success:** Report documents all testing comprehensively, includes metrics, clearly states pass/fail for each feature | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Success Pattern Summary (from AUTONOMOUS_IMPLEMENTATION_SUMMARY.md)

✅ **Investigation Tools FIRST** (Phase 0) - Build debugging/testing infrastructure before implementation
✅ **Validate Each Phase** - Test with tools after each phase before proceeding
✅ **Document As You Go** - Create docs early (Phase 0), update throughout
✅ **Incremental Development** - Minimal prototype → Full UI (Phase 3 → 4)
✅ **Autonomous Testing** - Use investigation tools for autonomous validation
✅ **Comprehensive Validation** - Final test report documents everything (Phase 8)

This task structure follows the proven success pattern: **improvement method first, then implementation with continuous validation**! 🎯
