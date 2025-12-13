# Tasks: Qt Engine Integration

## Overview
Replace the stub Engine implementation in the Qt GUI application with the real keyboard remapping engine to enable actual key mapping functionality on Linux.

## Task List

- [x] 1. Create EngineAdapter header file
  - File: src/app/engine_adapter.h
  - Create adapter class that bridges Qt GUI and real Engine
  - Define interface matching stub Engine's methods
  - _Leverage: src/core/engine/engine.h (real Engine class)_
  - _Requirements: FR-4, NFR-4_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ systems programmer specializing in adapter pattern. Task: Create EngineAdapter.h with class declaration bridging Qt GUI (simple interface) and real Engine (complex API). Include all public methods from stub: getIsEnabled, isRunning, enable, disable, start, stop, loadConfig, getConfigPath, keyCount, getStatusJson, getConfigJson, getKeymapsJson, getMetricsJson. Private members: Engine* m_engine, std::string m_configPath, std::thread m_engineThread. Restrictions: Do NOT implement functionality yet (just declarations). Success: Header compiles, all methods declared, no warnings._

- [x] 2. Create EngineAdapter implementation file
  - File: src/app/engine_adapter.cpp
  - Implement constructor and stub method implementations
  - Add destructor with thread cleanup
  - _Leverage: src/core/engine/engine.h_
  - _Requirements: FR-4, NFR-4_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ developer. Task: Create EngineAdapter.cpp with constructor accepting Engine* and stubbed method implementations returning default values. Destructor should call stop() for thread cleanup. Restrictions: Do NOT add complex logic yet. Success: File compiles successfully with stubbed implementations._

- [x] 3. Remove stub Engine class from main_qt.cpp
  - File: src/app/main_qt.cpp
  - Delete stub Engine class definition (lines 21-172)
  - Add includes for real Engine and platform implementations
  - _Leverage: Completed tasks 1 and 2_
  - _Requirements: FR-1, NFR-4_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ refactoring specialist. Task: Delete stub Engine class (lines 21-172) from main_qt.cpp. Add includes: engine_adapter.h, core/engine/engine.h, platform/linux/input_hook_linux.h, platform/linux/input_injector_linux.h, platform/linux/window_system_linux.h. Restrictions: Do NOT modify other code yet, may not compile (references remain). Success: Stub class removed, new includes added._

- [x] 4. Replace stub Engine instantiation with real Engine
  - File: src/app/main_qt.cpp
  - Create Linux platform implementations
  - Instantiate real Engine with dependencies
  - Wrap Engine in EngineAdapter
  - _Leverage: Completed tasks 1-3, src/platform/linux/ implementations_
  - _Requirements: FR-1, FR-2_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ systems engineer specializing in dependency injection. Task: Find Engine* engine = new Engine() (around line 330), replace with: create InputHookLinux, InputInjectorLinux, WindowSystemLinux using make_unique; create real Engine with std::move of platform implementations; create EngineAdapter wrapping the Engine. Add cleanup in destructor. Restrictions: Do NOT modify EngineAdapter implementation yet. Success: Real Engine created with platform dependencies, EngineAdapter wraps it, application compiles and runs without crashes._

- [x] 5. Implement EngineAdapter lifecycle methods with thread management
  - File: src/app/engine_adapter.cpp
  - Implement start() to create thread and call Engine::start()
  - Implement stop() to signal engine and join thread
  - Implement enable()/disable() to update engine state
  - Add thread cleanup in destructor
  - _Leverage: Real Engine::start/stop methods, std::thread_
  - _Requirements: FR-1, US-3_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ concurrency expert. Task: Implement EngineAdapter::start() to create thread running m_engine->start(); stop() to call m_engine->stop() and join thread; enable()/disable() to call m_engine->enable/disable(); destructor to call stop(). Update getIsEnabled() to return m_engine->m_isEnabled and isRunning() to check thread.joinable(). Add sleep_for(100ms) after thread creation. Handle exceptions in engine thread. Restrictions: Use RAII for thread lifecycle, no mutex needed yet. Success: Engine runs in separate thread, stop() cleanly shuts down, no memory leaks or deadlocks._

- [x] 6. Implement EngineAdapter configuration loading
  - File: src/app/engine_adapter.cpp
  - Implement loadConfig() to parse .mayu files
  - Validate file existence
  - Parse using Setting/Parser and apply to Engine
  - Add comprehensive error handling
  - _Leverage: src/core/settings/setting_loader.h (loadSetting function), std::filesystem_
  - _Requirements: FR-3, US-2, US-5_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ configuration management specialist. Task: Implement loadConfig(path): validate file exists with std::filesystem; stop engine if running; call loadSetting(path.c_str()) to parse .mayu file; call m_engine->setSetting(setting); save m_configPath; restart engine if was running; return success/failure. Log errors to stderr. Implement getConfigPath() to return m_configPath. Restrictions: Do NOT modify Parser, handle all exceptions gracefully. Success: Valid .mayu files load successfully, invalid files return false with error log, no crashes on parse errors._

- [x] 7. Implement EngineAdapter JSON status methods
  - File: src/app/engine_adapter.cpp
  - Implement getStatusJson() with real engine state
  - Implement getConfigJson() with actual config info
  - Implement getKeymapsJson() with loaded keymaps
  - Implement getMetricsJson() with real performance metrics
  - _Leverage: Qt JSON classes (QJsonObject, QJsonDocument), Engine query methods_
  - _Requirements: FR-4, US-4_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ API developer specializing in JSON serialization. Task: Implement getStatusJson() returning JSON with state, uptime, config, key_count, current_keymap from real Engine; getConfigJson() with config_path, config_name, loaded_time; getKeymapsJson() iterating m_engine->getKeymaps() with name/window_class/window_title; getMetricsJson() from m_engine->getMetrics(); keyCount() returning m_engine->getKeyCount(). Use QJsonObject/QJsonDocument for serialization. Restrictions: Match exact JSON format of stub, handle null/empty cases. Success: All methods return valid JSON from real Engine, yamy-ctl commands show real data._

- [ ] 8. Update IPC reload command handler
  - File: src/app/main_qt.cpp
  - Update reload command to call adapter->loadConfig()
  - Add error handling with proper result status
  - Handle both specific config and current config reload
  - _Leverage: EngineAdapter::loadConfig() from task 6_
  - _Requirements: FR-4, US-2_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ IPC developer. Task: In main_qt.cpp IPC handler lambda (around line 380), update ControlCommand::Reload case: if data not empty, call engine->loadConfig(data); else reload current config with engine->loadConfig(engine->getConfigPath()); set result.success and result.message based on return value; wrap in try-catch for exceptions. Restrictions: Do NOT change IPC protocol or CommandResult structure. Success: yamy-ctl reload works with real config loading, errors returned with clear messages._

- [ ] 9. Update IPC start/stop command handlers
  - File: src/app/main_qt.cpp
  - Update start/stop commands to use adapter methods
  - Return proper success/error status
  - _Leverage: EngineAdapter::start/stop from task 5_
  - _Requirements: FR-4, US-3_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ IPC developer. Task: In main_qt.cpp IPC handler, update ControlCommand::Stop case to call engine->stop() and set result.success=true, result.message="Engine stopped"; update ControlCommand::Start case to call engine->start() and set result.success=true, result.message="Engine started". Restrictions: Keep backward compatibility with yamy-ctl. Success: yamy-ctl start/stop control the real engine correctly._

- [ ] 10. Enhance session state persistence
  - File: src/app/main_qt.cpp
  - Save session state after successful config load
  - Restore session on startup with validation
  - Handle missing config files gracefully
  - _Leverage: Existing SessionManager, EngineAdapter::loadConfig_
  - _Requirements: FR-5, US-2_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ persistence specialist. Task: In EngineAdapter::loadConfig(), after success, call SessionManager::instance().saveSession() with activeConfigPath and engineWasRunning. In main() restoreSessionState(), load session, validate config file exists, call engine->loadConfig(path), start engine if was running. Add --no-restore check. Restrictions: Do NOT modify SessionManager class. Success: Config path saved when loaded, restored on startup, missing files handled gracefully._

- [ ] 11. Add engine notification callbacks
  - File: src/app/engine_adapter.cpp and src/app/engine_adapter.h
  - Add notification callback mechanism to EngineAdapter
  - Emit notifications on config load/error
  - Connect to TrayIconQt notification system
  - _Leverage: TrayIconQt::handleMessage(), yamy::MessageType enum_
  - _Requirements: US-2, US-5_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: Qt GUI developer. Task: In engine_adapter.h add NotificationCallback typedef and setNotificationCallback() method; in loadConfig() call callback with ConfigLoaded or ConfigLoadError and data; in main_qt.cpp after creating trayIcon, call engine->setNotificationCallback([&trayIcon](type, data) { trayIcon->handleMessage(type, qdata); }). Restrictions: Use existing notification infrastructure. Success: Config load success shows notification, errors show with details, tray icon updates._

- [ ] 12. Update CMakeLists.txt for EngineAdapter
  - File: CMakeLists.txt
  - Add engine_adapter.cpp to yamy target sources
  - Verify build configuration
  - _Leverage: Existing CMake configuration_
  - _Requirements: All_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: Build engineer. Task: In CMakeLists.txt, find yamy target definition (around line 460), add src/app/engine_adapter.cpp to STUB_SOURCES or directly to target_sources. Verify build compiles successfully. Restrictions: Do NOT change other build settings. Success: Project builds successfully with no warnings._

- [ ] 13. Integration testing and verification
  - Files: All modified files
  - Test configuration loading with real .mayu files
  - Verify keyboard remapping works
  - Test all yamy-ctl commands
  - Run memory leak checks with valgrind
  - _Leverage: All previous tasks, keymaps/master.mayu_
  - _Requirements: All FR and US requirements_
  - _Prompt: Implement the task for spec qt-engine-integration. Role: C++ QA engineer and integration specialist. Task: Run comprehensive tests: 1) Build and start yamy; 2) Load keymaps/master.mayu in Settings; 3) Verify tray icon shows "Running"; 4) Press remapped keys to verify they work; 5) Test yamy-ctl status/reload/metrics commands; 6) Restart yamy to test session restore; 7) Test invalid .mayu error handling; 8) Run valgrind for memory leaks; 9) Remove debug cout statements; 10) Run clang-format on modified files. Restrictions: Do NOT skip verification tests, fix any issues found. Success: All tests pass, real keyboard remapping works, no memory leaks, code is clean._

## Progress Tracking

- Total Tasks: 13
- Completed: 0
- In Progress: 0
- Pending: 13

## Notes

- Tasks are ordered sequentially with dependencies
- Each task should take 30-60 minutes
- Test after each task completion
- Mark tasks with `[-]` when starting, `[x]` when complete
- Log implementation details using log-implementation tool after each task
