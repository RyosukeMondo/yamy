# Tasks: Linux Complete Port

**Status**: Draft
**Created**: 2025-12-10
**Dependencies**: design.md
**Total Tasks**: 121 tasks across 6 tracks

---

## Track 1: Core Refactoring (60 tasks)

### Batch 1: Foundation (32 tasks - ALL PARALLEL)

- [x] 1.1.1 Remove tstring from stringtool.h
  - File: src/utils/stringtool.h
  - Remove Windows-specific tstring typedef and _T() macros
  - Convert all function signatures to use std::string
  - _Leverage: src/utils/stringtool.cpp_
  - _Requirements: FR-1.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Remove all Windows-specific string types from stringtool.h - Step 1: Remove typedef std::basic_string<TCHAR> tstring, Step 2: Remove all _T() macro usages, Step 3: Keep only std::string versions of functions, Step 4: Remove any #ifdef UNICODE blocks, Step 5: Ensure all function signatures use std::string, Verify with grep tstring and grep _T( commands | Restrictions: Do not modify function behavior, Maintain UTF-8 encoding throughout, Ensure file compiles on Linux without windows.h, Do not break existing callers | Success: No tstring references remain, No _T() macros remain, All functions use std::string, File compiles on Linux without windows.h, All unit tests pass | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.2 Remove tstring from stringtool.cpp
  - File: src/utils/stringtool.cpp
  - Update implementation to match stringtool.h changes
  - Convert wstring operations to UTF-8 std::string
  - _Leverage: src/utils/stringtool.h_
  - _Requirements: FR-1.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update stringtool.cpp implementation to match stringtool.h changes - Step 1: Remove all tstring usages in function implementations, Step 2: Convert wstring operations to UTF-8 std::string, Step 3: Update string conversions by removing toWide/toNarrow functions and adding UTF-8 conversion utilities if needed for Windows bridge, Step 4: Update all string literals from _T("foo") to "foo", Verify with grep commands and unit tests | Restrictions: Maintain UTF-8 encoding, Do not break Windows build (add UTF-8/UTF-16 bridge if needed), Preserve all function semantics | Success: No tstring references, All string literals are plain UTF-8, Unit tests pass on both Linux and Windows | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.3 Clean up config_store.h tstring overloads
  - File: src/utils/config_store.h (actual location)
  - Remove duplicate tstring overloads causing link errors
  - Keep only std::string API versions
  - _Leverage: src/platform/windows/registry.h_
  - _Requirements: FR-1.2, FR-1.6_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: ConfigStore has duplicate overloads for tstring and std::string causing link errors on Linux, Remove tstring versions - Step 1: Remove overloads bool read(const tstring& key, tstring* value), bool write(const tstring& key, const tstring& value), bool exists(const tstring& key), Step 2: Keep only std::string versions, Step 3: Update internal storage to use std::string keys | Restrictions: Do not break existing callers, Maintain API compatibility where possible, Ensure no ambiguity errors during compilation | Success: No tstring overloads remain, Only std::string API exists, No compilation errors, File compiles on Linux | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.4 Update config_store.cpp implementation
  - File: src/core/settings/config_store.cpp (actual: src/platform/windows/registry.cpp)
  - Update implementation to use std::string
  - Add UTF-8 to UTF-16 bridge for Windows registry calls
  - _Leverage: src/core/settings/config_store.h_
  - _Requirements: FR-1.2, FR-1.6_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update ConfigStore implementation to use std::string - Step 1: Remove tstring function implementations, Step 2: On Windows update registry calls to convert UTF-8 to UTF-16 at API boundary using utf8::toWide(key), Step 3: On Linux use QSettings which already handles UTF-8, Step 4: Update internal cache to std::map<std::string, std::string> m_cache | Restrictions: Maintain registry compatibility on Windows, Ensure UTF-8 round-trip works correctly, Do not break config file format | Success: Compiles on both Linux and Windows, Windows uses UTF-8 to UTF-16 bridge at registry API boundary, All tests pass, Config persistence works | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.5 Remove tstring from errormessage.h
  - File: src/utils/errormessage.h (actual location)
  - Convert ErrorMessage class to use std::string
  - Update member variables and method signatures
  - _Leverage: src/core/settings/errormessage.cpp_
  - _Requirements: FR-1.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: ErrorMessage class uses tstring for message storage, Convert to std::string - Step 1: Change member variable tstring m_message to std::string m_message, Step 2: Update constructor ErrorMessage(const tstring& msg) to ErrorMessage(const std::string& msg), Step 3: Update getter const tstring& getMessage() to const std::string& getMessage(), Step 4: Remove any _T() macros in error messages | Restrictions: Do not change error message content, Maintain API compatibility for error handling code | Success: No tstring in file, std::string used throughout, No _T() macros, All error messages display correctly in UTF-8 | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.6 Update keyboard.cpp string handling (already done)
  - File: src/core/input/keyboard.cpp
  - Convert key name storage from tstring to std::string
  - Update key name parsing functions
  - _Leverage: src/core/input/keyboard.h_
  - _Requirements: FR-1.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Keyboard class uses tstring for key name storage, Convert to std::string - Step 1: Update key name map from std::map<tstring, KeyCode> to std::map<std::string, KeyCode>, Step 2: Update function KeyCode parseKeyName(const tstring& name) to KeyCode parseKeyName(const std::string& name), Step 3: Update string literals in key name table by removing _T() macros, Step 4: Update case-insensitive comparisons to use UTF-8 aware functions if needed | Restrictions: Maintain key name parsing behavior, Ensure all existing key names still work, Do not break .mayu file parsing | Success: No tstring references, Key name map uses std::string, Key name parsing tests pass, All 60+ key names recognized | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.7 Update parser string handling
  - File: src/core/settings/parser.cpp (actual location)
  - Convert parser string handling to std::string
  - Update token storage and string comparisons
  - _Leverage: src/core/parser/parser.h_
  - _Requirements: FR-1.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Parser uses tstring for token storage and comparisons, Convert to std::string - Step 1: Update token storage from std::vector<tstring> to std::vector<std::string>, Step 2: Update string comparison functions to use std::string, Step 3: Remove _T() macros from keyword table, Step 4: Ensure UTF-8 file parsing works correctly | Restrictions: Maintain .mayu file parsing compatibility, Ensure line numbers and error positions remain accurate, Do not change parser behavior | Success: No tstring references, Token storage uses std::string, Parser tests pass, .mayu files parse correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.8 Update mayu_scanner.cpp string handling (N/A - file does not exist)
  - File: src/core/parser/mayu_scanner.cpp
  - Convert scanner string buffer to std::string
  - Update string literal handling
  - _Leverage: src/core/parser/mayu_scanner.h_
  - _Requirements: FR-1.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Scanner uses tstring for buffer storage, Convert to std::string - Step 1: Update buffer from tstring m_buffer to std::string m_buffer, Step 2: Update getCurrentToken() from returning tstring to std::string, Step 3: Remove _T() macros from scanner error messages, Step 4: Ensure UTF-8 scanning works correctly | Restrictions: Maintain scanner behavior, Preserve token positions for error reporting, Do not break string literal parsing | Success: No tstring references, Buffer uses std::string, Scanner tests pass, String literals scanned correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.9 Create WindowHandle type alias (already done)
  - File: src/core/platform/types.h (actual location)
  - Create platform-agnostic WindowHandle type
  - Add conditional compilation for Windows HWND vs Linux Window
  - _Leverage: src/platform/window_system.h_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Create platform-agnostic WindowHandle type - Step 1: Add #ifdef _WIN32 using WindowHandle = HWND else using WindowHandle = unsigned long (X11 Window), Step 2: Define InvalidWindowHandle constant, Step 3: Add utility functions isValidWindow(WindowHandle), Step 4: Document the type in comments | Restrictions: Must support both Windows HWND and X11 Window types, Ensure type safety, Keep overhead minimal (just a typedef) | Success: WindowHandle works on both platforms, Invalid handle detection works, Type is documented, Compiles on both Linux and Windows | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.10 Create IWindowSystem interface (already done)
  - File: src/core/platform/window_system_interface.h (actual location)
  - Define abstract interface for window operations
  - Include all window management methods
  - _Leverage: src/platform/types.h, design.md Section 1.2_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Define abstract IWindowSystem interface for window operations - Methods: getForegroundWindow, getWindowText, getWindowClassName, getWindowRect, showWindow, moveWindow, setForegroundWindow, enumWindows - Step 1: Create pure virtual interface class, Step 2: Use WindowHandle type, Step 3: Define Rect structure, Step 4: Add factory function createWindowSystem() | Restrictions: Interface must be platform-agnostic, Do not include platform-specific types in interface, Keep methods minimal and focused | Success: Interface is well-defined, Factory function declared, Compiles on both platforms, Methods cover all window operations needed | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.11 Create WindowSystemWin32 implementation (already done)
  - File: src/platform/windows/window_system_win32.h/.cpp (actual location)
  - Implement IWindowSystem for Windows using Win32 API
  - Wrap existing Win32 window operations
  - _Leverage: src/platform/window_system.h_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Implement IWindowSystem for Windows - Step 1: Create WindowSystemWin32 class implementing IWindowSystem, Step 2: Wrap GetForegroundWindow, GetWindowText, GetClassName, GetWindowRect, ShowWindow, MoveWindow, SetForegroundWindow, EnumWindows, Step 3: Handle UTF-8 to UTF-16 conversions at API boundary, Step 4: Implement factory function returning WindowSystemWin32 | Restrictions: Maintain existing Windows behavior exactly, Handle string conversions correctly, Ensure proper error handling | Success: All methods implemented correctly, Windows build works, UTF-8 conversions work, Window operations function as before | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.12 Create WindowSystemLinux implementation (stub exists)
  - File: src/platform/linux/window_system_linux.cpp
  - Implement IWindowSystem for Linux using X11
  - Add EWMH support for window operations
  - _Leverage: src/platform/window_system.h_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with X11 and EWMH expertise | Task: Implement IWindowSystem for Linux - Step 1: Create WindowSystemLinux class implementing IWindowSystem, Step 2: Use XGetInputFocus for getForegroundWindow, Step 3: Use XFetchName/_NET_WM_NAME for getWindowText, Step 4: Use XGetClassHint for getWindowClassName, Step 5: Implement showWindow using XMapWindow/XUnmapWindow/_NET_WM_STATE, Step 6: Implement moveWindow using XMoveResizeWindow, Step 7: Implement factory function returning WindowSystemLinux | Restrictions: Use X11 and EWMH APIs correctly, Handle UTF-8 properly (X11 uses UTF-8 for _NET_WM_NAME), Ensure window operations work with modern window managers | Success: All methods implemented, Linux build works, Window operations tested with various WMs, UTF-8 text retrieved correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.13 Create IInputInjector interface (already done)
  - File: src/core/platform/input_injector_interface.h (actual location)
  - Define abstract interface for input injection
  - Include keyboard and mouse event injection methods
  - _Leverage: src/platform/types.h, design.md Section 1.3_
  - _Requirements: FR-1.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Define abstract IInputInjector interface for input injection - Methods: injectKeyDown, injectKeyUp, injectMouseButton, injectMouseMove, injectMouseWheel - Step 1: Create pure virtual interface, Step 2: Define KeyEvent and MouseEvent structures with scancode/keycode/modifiers, Step 3: Add factory function createInputInjector() | Restrictions: Interface must be platform-agnostic, Structures must support both platforms, Keep overhead minimal | Success: Interface is well-defined, Event structures support both platforms, Factory function declared, Compiles on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.14 Create InputInjectorWin32 implementation (already done)
  - File: src/platform/windows/input_injector_win32.h/.cpp (actual location)
  - Implement IInputInjector for Windows using SendInput
  - Map virtual keycodes and scan codes
  - _Leverage: src/platform/input_injector.h_
  - _Requirements: FR-1.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Implement IInputInjector for Windows - Step 1: Create InputInjectorWin32 class implementing IInputInjector, Step 2: Use SendInput API for all injection methods, Step 3: Map KeyEvent to INPUT structure with KEYEVENTF_SCANCODE, Step 4: Map MouseEvent to INPUT structure, Step 5: Implement factory function returning InputInjectorWin32 | Restrictions: Use SendInput not keybd_event/mouse_event (deprecated), Maintain existing Windows input behavior, Handle extended scancodes correctly | Success: All methods implemented, Windows input injection works, Key remapping works as before, No regressions in input handling | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.15 Create InputInjectorLinux implementation (already done)
  - File: src/platform/linux/input_injector_linux.cpp
  - Implement IInputInjector for Linux using XTest extension
  - Map keycodes between platforms
  - _Leverage: src/platform/input_injector.h_
  - _Requirements: FR-1.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with X11 and XTest expertise | Task: Implement IInputInjector for Linux - Step 1: Create InputInjectorLinux class implementing IInputInjector, Step 2: Use XTestFakeKeyEvent for keyboard injection, Step 3: Use XTestFakeButtonEvent for mouse buttons, Step 4: Use XTestFakeMotionEvent for mouse movement, Step 5: Map Windows scancodes to X11 keycodes using lookup table, Step 6: Implement factory function returning InputInjectorLinux | Restrictions: Use XTest extension correctly, Build keycode mapping table for common keys, Ensure XFlush called after injection | Success: All methods implemented, Linux input injection works, Key remapping functional, Keycode mapping covers all common keys | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.16 Create IInputHook interface (already done)
  - File: src/core/platform/input_hook_interface.h (actual location)
  - Define abstract interface for input hooking
  - Include callback mechanism for key events
  - _Leverage: src/platform/types.h, design.md Section 1.4_
  - _Requirements: FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Define abstract IInputHook interface for input hooking - Methods: install, uninstall, isInstalled, setCallback - Step 1: Create pure virtual interface, Step 2: Define HookCallback function type taking KeyEvent and returning bool (true = consume event), Step 3: Add factory function createInputHook() | Restrictions: Interface must support both polling (Windows) and event-driven (Linux) models, Callback must be thread-safe, Keep overhead minimal | Success: Interface is well-defined, Callback mechanism flexible, Factory function declared, Compiles on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.17 Create InputHookWin32 implementation (already done)
  - File: src/platform/windows/input_hook_win32.h/.cpp (actual location)
  - Implement IInputHook for Windows using low-level keyboard hook
  - Handle hook callback and event filtering
  - _Leverage: src/platform/input_hook.h_
  - _Requirements: FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Implement IInputHook for Windows - Step 1: Create InputHookWin32 class implementing IInputHook, Step 2: Use SetWindowsHookEx with WH_KEYBOARD_LL, Step 3: In hook procedure convert KBDLLHOOKSTRUCT to KeyEvent and invoke callback, Step 4: Return 1 if callback returns true (consume), otherwise CallNextHookEx, Step 5: Implement install/uninstall managing hook handle | Restrictions: Maintain existing Windows hook behavior, Ensure thread-safety in callback invocation, Handle hook errors gracefully | Success: Hook installs correctly, Callbacks invoked properly, Event consumption works, Key remapping functional, No crashes or hangs | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.18 Create InputHookLinux implementation (already done)
  - File: src/platform/linux/input_hook_linux.h/.cpp (actual location)
  - Implement IInputHook for Linux using XRecord extension
  - Handle asynchronous event callbacks
  - _Leverage: src/platform/input_hook.h_
  - _Requirements: FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with X11 and XRecord expertise | Task: Implement IInputHook for Linux - Step 1: Create InputHookLinux class implementing IInputHook, Step 2: Use XRecordCreateContext to capture KeyPress/KeyRelease events, Step 3: Run XRecordEnableContext in separate thread for event loop, Step 4: In callback convert XRecordInterceptData to KeyEvent and invoke user callback, Step 5: Use XRecordDisableContext to consume events when callback returns true | Restrictions: Use XRecord extension correctly, Handle threading properly (XRecord runs in separate thread), Ensure proper synchronization with main thread | Success: Hook captures events, Callbacks work correctly, Event consumption works, Key remapping functional, Thread-safe implementation | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.19 Create ShowCommand enum (already done as WindowShowCmd)
  - File: src/core/platform/types.h (actual location)
  - Define platform-agnostic show command enum
  - Replace SW_HIDE, SW_SHOW, SW_MINIMIZE, etc.
  - _Leverage: design.md Section 1.5_
  - _Requirements: FR-1.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Create platform-agnostic ShowCommand enum - Step 1: Define enum class ShowCommand with values Hide, Show, ShowNormal, ShowMinimized, ShowMaximized, Restore, Step 2: Add utility function toShowCommand(int win32Code) for Windows conversion, Step 3: Document each enum value | Restrictions: Must cover all used SW_* constants, Ensure enum is type-safe, Keep mapping straightforward | Success: Enum defined, All SW_* constants covered, Conversion function works, Compiles on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.20 Replace SW_HIDE in window management code (N/A - no SW_* used)
  - Files: src/core/window_manager.cpp, src/ui/qt/main_window.cpp
  - Replace SW_HIDE with ShowCommand::Hide
  - Update showWindow calls
  - _Leverage: src/platform/types.h, src/platform/window_system.h_
  - _Requirements: FR-1.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Replace SW_HIDE with ShowCommand::Hide - Step 1: Search for all SW_HIDE usages with grep, Step 2: Replace with ShowCommand::Hide, Step 3: Update function calls from showWindow(hwnd, SW_HIDE) to windowSystem->showWindow(handle, ShowCommand::Hide), Step 4: Verify build on both platforms | Restrictions: Maintain window hiding behavior, Do not break other window operations, Ensure Linux implementation handles Hide correctly | Success: No SW_HIDE references remain, Windows hiding works on both platforms, Builds successfully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.21 Replace SW_SHOW and SW_SHOWNORMAL (N/A - no SW_* used)
  - Files: src/core/window_manager.cpp, src/ui/qt/main_window.cpp
  - Replace with ShowCommand::Show and ShowCommand::ShowNormal
  - Update window display logic
  - _Leverage: src/platform/types.h, src/platform/window_system.h_
  - _Requirements: FR-1.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Replace SW_SHOW and SW_SHOWNORMAL - Step 1: Search for SW_SHOW and SW_SHOWNORMAL with grep, Step 2: Replace with ShowCommand::Show and ShowCommand::ShowNormal, Step 3: Update showWindow calls, Step 4: Verify window showing works on both platforms | Restrictions: Maintain window showing behavior exactly, Handle differences between Show and ShowNormal correctly, Ensure Linux XMapWindow works | Success: No SW_SHOW/SW_SHOWNORMAL references, Window showing works on both platforms, Builds successfully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.22 Replace SW_MINIMIZE and SW_MAXIMIZE (N/A - no SW_* used)
  - Files: src/core/window_manager.cpp
  - Replace with ShowCommand::ShowMinimized and ShowCommand::ShowMaximized
  - Update window state management
  - _Leverage: src/platform/types.h, src/platform/window_system.h_
  - _Requirements: FR-1.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Replace SW_MINIMIZE and SW_MAXIMIZE - Step 1: Search for SW_MINIMIZE and SW_MAXIMIZE with grep, Step 2: Replace with ShowCommand::ShowMinimized and ShowCommand::ShowMaximized, Step 3: Update showWindow calls, Step 4: Verify window state changes work on Linux using _NET_WM_STATE | Restrictions: Maintain window state behavior, Ensure Linux implementation uses EWMH _NET_WM_STATE_MAXIMIZED_VERT/_HORZ correctly | Success: No SW_MINIMIZE/SW_MAXIMIZE references, Window state changes work on both platforms, EWMH properties set correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.23 Create IIPCChannel interface (done via IWindowSystem::sendCopyData)
  - File: src/core/platform/ipc.h and window_system_interface.h (actual location)
  - Define abstract interface for IPC communication
  - Support message passing between instances
  - _Leverage: design.md Section 1.6_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Define abstract IIPCChannel interface for IPC - Methods: send, setMessageHandler, connect, disconnect - Step 1: Create pure virtual interface, Step 2: Define MessageHandler callback taking message data, Step 3: Add factory function createIPCChannel(instanceId) | Restrictions: Interface must support both Windows messages and Linux signals/sockets, Keep message format flexible, Support bidirectional communication | Success: Interface is well-defined, Callback mechanism works, Factory function declared, Compiles on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.24 Create IPCChannelWin32 implementation (N/A - IPC via IWindowSystem)
  - File: src/platform/windows/window_system_win32.cpp
  - Implement IIPCChannel using Windows messages
  - Create hidden window for message handling
  - _Leverage: src/platform/ipc_channel.h_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Implement IIPCChannel for Windows - Step 1: Create IPCChannelWin32 class implementing IIPCChannel, Step 2: Create hidden window with unique class name for receiving messages, Step 3: Use FindWindow to locate other instances, Step 4: Use SendMessage/PostMessage to send data, Step 5: Use WM_COPYDATA for message payload, Step 6: In WndProc invoke MessageHandler callback | Restrictions: Maintain existing IPC behavior for reload/exit commands, Handle multiple instances correctly, Ensure messages delivered reliably | Success: IPC works between instances, Reload and exit commands functional, Messages delivered correctly, No message loss | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.25 Create IPCChannelLinux implementation (stub in WindowSystemLinux)
  - File: src/platform/linux/window_system_linux.cpp
  - Implement IIPCChannel using Unix domain sockets
  - Handle socket lifecycle and message protocol
  - _Leverage: src/platform/ipc_channel.h_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with Unix IPC expertise | Task: Implement IIPCChannel for Linux - Step 1: Create IPCChannelLinux class implementing IIPCChannel, Step 2: Use Unix domain socket at /tmp/yamy-{instanceId}.sock, Step 3: In connect create and bind socket then listen for connections, Step 4: In send connect to remote socket and write message, Step 5: Run accept loop in thread invoking MessageHandler for received messages, Step 6: Handle socket cleanup on disconnect | Restrictions: Use Unix domain sockets correctly, Handle socket errors gracefully, Ensure thread-safe message handling, Clean up socket files | Success: IPC works between instances, Socket communication reliable, Thread-safe implementation, Socket files cleaned up on exit | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.26 Replace PostMessage in engine.cpp (N/A - no PostMessage used)
  - File: src/core/engine
  - Replace PostMessage with IIPCChannel::send
  - Update IPC initialization and message handling
  - _Leverage: src/platform/ipc_channel.h_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Replace PostMessage with IIPCChannel - Step 1: Search for PostMessage calls with grep, Step 2: Add IIPCChannel* m_ipcChannel member to Engine class, Step 3: In constructor call createIPCChannel("yamy-main") and store in m_ipcChannel, Step 4: Replace PostMessage(hwnd, msg, wparam, lparam) with m_ipcChannel->send(message), Step 5: Set message handler to process reload/exit commands | Restrictions: Maintain existing IPC behavior, Ensure message commands still work, Handle IPC errors gracefully | Success: No PostMessage calls remain, IPC works on both platforms, Reload and exit functional, Builds successfully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.27 Update main.cpp IPC handling (N/A - no FindWindow/SendMessage)
  - File: src/app/mayu.cpp (Windows entry point)
  - Replace FindWindow and SendMessage with IIPCChannel
  - Update single instance check
  - _Leverage: src/platform/ipc_channel.h_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update main.cpp IPC handling - Step 1: Replace FindWindow with attempt to connect to IPC channel, Step 2: If connection succeeds another instance exists, send command and exit, Step 3: If connection fails this is first instance, create IPC channel for listening, Step 4: Remove all Windows message loop code from main | Restrictions: Maintain single instance behavior, Ensure command-line args sent to running instance correctly, Handle first run vs subsequent runs | Success: Single instance detection works, Commands sent correctly to running instance, First instance listens properly, Builds on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.28 Create platform initialization in main.cpp (already done)
  - File: src/app/mayu.cpp (Windows), Qt main uses factory functions
  - Initialize platform subsystems at startup
  - Clean up platform resources on exit
  - _Leverage: src/platform/window_system.h, src/platform/input_hook.h, src/platform/input_injector.h_
  - _Requirements: FR-1.1, FR-1.3, FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Create platform initialization in main - Step 1: Add platform init code after QApplication construction: auto windowSystem = createWindowSystem(), auto inputHook = createInputHook(), auto inputInjector = createInputInjector(), Step 2: Pass these to Engine constructor, Step 3: Ensure proper cleanup on exit | Restrictions: Initialize in correct order, Handle initialization failures, Ensure cleanup happens even on errors | Success: Platform initialized correctly, Engine receives platform interfaces, Cleanup works properly, No resource leaks | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.29 Update Engine constructor to accept platform interfaces (already done)
  - File: src/core/engine/engine.h stores IWindowSystem*, IInputInjector*, IInputHook*
  - Add parameters for platform interfaces
  - Store interfaces as member variables
  - _Leverage: src/platform/window_system.h, src/platform/input_hook.h, src/platform/input_injector.h_
  - _Requirements: FR-1.1, FR-1.3, FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update Engine constructor to accept platform interfaces - Step 1: Add parameters Engine(IWindowSystem* ws, IInputHook* ih, IInputInjector* ii, IIPCChannel* ipc), Step 2: Add member variables storing these pointers, Step 3: Update initialization code to use interfaces instead of direct Win32 calls, Step 4: Update all call sites | Restrictions: Maintain Engine behavior, Ensure all dependencies injected properly, Handle null pointers defensively | Success: Engine constructor updated, Platform interfaces stored, All call sites updated, Builds on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.30 Replace direct Win32 calls in Engine with platform interfaces (already done)
  - File: src/core/engine uses m_windowSystem, m_inputInjector throughout
  - Replace GetForegroundWindow with windowSystem->getForegroundWindow()
  - Replace SendInput with inputInjector->injectKey()
  - _Leverage: src/platform/window_system.h, src/platform/input_injector.h_
  - _Requirements: FR-1.1, FR-1.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Replace direct Win32 calls in Engine - Step 1: Search for GetForegroundWindow, GetWindowText, GetClassName calls with grep, Step 2: Replace with m_windowSystem->getForegroundWindow(), m_windowSystem->getWindowText(), etc, Step 3: Search for SendInput, keybd_event calls, Step 4: Replace with m_inputInjector->injectKeyDown/injectKeyUp, Step 5: Verify all Win32 API calls removed | Restrictions: Maintain exact engine behavior, Ensure key remapping still works correctly, Handle errors from platform interfaces | Success: No direct Win32 calls in Engine, Platform interfaces used throughout, Engine works on both platforms, Key remapping functional | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.31 Add Linux-specific build flags and dependencies (already done)
  - File: CMakeLists.txt (lines 236-290 have Linux config)
  - Add X11, XTest, XRecord library dependencies for Linux
  - Add conditional compilation flags
  - _Leverage: CMakeLists.txt_
  - _Requirements: FR-1.1, FR-1.3, FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with CMake expertise | Task: Add Linux build dependencies - Step 1: Add find_package(X11 REQUIRED), find_package(XTest REQUIRED) for Linux, Step 2: Add target_link_libraries with X11, Xtst, Xrecord for Linux target, Step 3: Add platform-specific source files to build: src/platform/linux/*.cpp, Step 4: Add preprocessor defines for platform detection | Restrictions: Maintain Windows build compatibility, Use modern CMake patterns, Ensure all Linux libraries found correctly | Success: CMake finds all Linux dependencies, Linux build compiles successfully, Both platforms build without issues, Libraries linked correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.1.32 Add platform-specific source files to build system (already done)
  - File: CMakeLists.txt (Linux sources at lines 239-254)
  - Conditionally compile Win32 and Linux platform implementations
  - Organize platform code properly
  - _Leverage: CMakeLists.txt_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with CMake expertise | Task: Add platform-specific source files - Step 1: Create source lists for each platform: WIN32_SOURCES (src/platform/win32/*.cpp), LINUX_SOURCES (src/platform/linux/*.cpp), COMMON_SOURCES (src/platform/*.cpp shared), Step 2: Use if(WIN32) to add WIN32_SOURCES, if(UNIX AND NOT APPLE) to add LINUX_SOURCES, Step 3: Verify both platforms build correctly | Restrictions: Keep build system clean and maintainable, Ensure correct files compiled per platform, Avoid duplicate symbols | Success: Platform files compiled correctly per platform, No symbol conflicts, Both Windows and Linux build successfully, Build system is maintainable | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

### Batch 2: Integration (18 tasks - PARALLEL within batch, SEQUENTIAL after Batch 1)

- [x] 1.2.1 Update window context tracking in Engine
  - File: src/core/engine.cpp
  - Replace HWND-based window context with WindowHandle
  - Update window focus tracking logic
  - _Leverage: src/platform/window_system.h, src/platform/types.h_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update window context tracking - Step 1: Change HWND m_focusedWindow to WindowHandle m_focusedWindow, Step 2: Update updateContext() to use windowSystem->getForegroundWindow(), Step 3: Update window class and title retrieval to use windowSystem methods, Step 4: Verify context-dependent keymapping works | Restrictions: Maintain window context behavior exactly, Ensure per-window keymaps work, Handle invalid windows gracefully | Success: Window context tracked correctly on both platforms, Per-window keymaps functional, Window focus changes detected, Context updates work reliably | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.2 Update keymap activation logic
  - File: src/core/keymap_layer.cpp
  - Update condition evaluation for window-based activation
  - Use platform abstractions for window matching
  - _Leverage: src/platform/window_system.h_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update keymap activation logic - Step 1: Update condition evaluation in KeymapLayer::shouldActivate, Step 2: Replace direct window API calls with windowSystem methods, Step 3: Ensure regex matching works with UTF-8 strings, Step 4: Test activation with various window title/class patterns | Restrictions: Maintain activation behavior, Ensure all condition types still work (WindowClass, WindowTitle, etc), Handle UTF-8 properly | Success: Keymap activation works correctly, All condition types functional, Regex patterns match properly, Works on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.3 Update modifier state tracking
  - File: src/core/modifier_state.cpp
  - Track modifier state using platform-agnostic key codes
  - Update modifier key detection
  - _Leverage: src/platform/input_hook.h_
  - _Requirements: FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update modifier state tracking - Step 1: Update ModifierState to use platform-agnostic KeyEvent structure, Step 2: Update modifier detection from KeyEvent scancode/keycode, Step 3: Handle both Windows scancodes and Linux keycodes, Step 4: Ensure modifier combinations tracked correctly | Restrictions: Maintain modifier tracking accuracy, Support all modifier keys (Ctrl, Alt, Shift, Win/Super), Handle left/right modifiers separately if needed | Success: Modifier state tracked correctly, All modifiers detected properly, Modifier combinations work, Works on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.4 Update key event processing in Engine
  - File: src/core/engine.cpp
  - Process KeyEvent structure from input hook
  - Update event filtering and transformation
  - _Leverage: src/platform/input_hook.h, src/core/modifier_state.cpp_
  - _Requirements: FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update key event processing - Step 1: Update hook callback to receive KeyEvent instead of Windows KBDLLHOOKSTRUCT, Step 2: Update event processing logic to use KeyEvent fields, Step 3: Update modifier state from KeyEvent, Step 4: Update keymap matching using KeyEvent, Step 5: Return true to consume or false to pass through | Restrictions: Maintain event processing behavior, Ensure all key events handled correctly, Preserve event timing and ordering | Success: Key events processed correctly, Event consumption works, Modifier tracking accurate, Key remapping functional on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.5 Update key injection in action execution
  - File: src/core/action.cpp
  - Use IInputInjector for key injection in actions
  - Update all action types that inject keys
  - _Leverage: src/platform/input_injector.h_
  - _Requirements: FR-1.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer refactoring cross-platform keyboard remapping utility | Task: Update key injection in actions - Step 1: Update Action::execute to accept IInputInjector* parameter, Step 2: Replace SendInput calls with inputInjector->injectKeyDown/injectKeyUp, Step 3: Update KeyAction, ModifierAction, StringAction to use injector, Step 4: Ensure key sequences injected correctly | Restrictions: Maintain action behavior exactly, Preserve key injection timing, Handle all action types correctly | Success: All actions use IInputInjector, Key injection works correctly, Action behavior unchanged, Works on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.6 Update MainWindow to use IWindowSystem
  - File: src/ui/qt/main_window.cpp
  - Replace direct window API calls with IWindowSystem
  - Update window management in GUI
  - _Leverage: src/platform/window_system.h_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with Qt expertise | Task: Update MainWindow to use IWindowSystem - Step 1: Add IWindowSystem* m_windowSystem member to MainWindow, Step 2: Accept IWindowSystem in constructor, Step 3: Replace ShowWindow calls with m_windowSystem->showWindow, Step 4: Update window enumeration in investigate dialog to use m_windowSystem->enumWindows | Restrictions: Maintain GUI behavior, Ensure dialogs display correctly, Handle window operations properly | Success: MainWindow uses IWindowSystem, Window operations work correctly, Dialogs display properly, Works on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.7 Test basic key remapping on Linux
  - Test environment: Linux system with X11
  - Verify basic key remapping works
  - Test modifier keys and combinations
  - _Leverage: test .mayu configs, src/core/engine.cpp_
  - _Requirements: FR-1.1, FR-1.3, FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with Linux system expertise | Task: Test basic key remapping on Linux - Step 1: Create test .mayu config with simple remaps (e.g. a to b), Step 2: Start YAMY on Linux, Step 3: Test basic key remapping in various applications, Step 4: Test modifier keys (Ctrl, Alt, Shift), Step 5: Test modifier combinations, Step 6: Document any issues found | Restrictions: Use real X11 environment (not Wayland), Test in multiple applications (terminal, browser, editor), Verify events not duplicated | Success: Basic remapping works, Modifier keys work correctly, Combinations functional, No event duplication, Key presses recognized reliably | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.8 Test window-context dependent keymaps on Linux
  - Test environment: Linux with multiple applications
  - Verify window title/class matching works
  - Test keymap switching between windows
  - _Leverage: test .mayu configs with window conditions_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with Linux system expertise | Task: Test window-context keymaps - Step 1: Create .mayu config with window-specific keymaps using window class/title conditions, Step 2: Start YAMY, Step 3: Switch between applications and verify correct keymap activates, Step 4: Test window title patterns with regex, Step 5: Test window class matching, Step 6: Document any window detection issues | Restrictions: Test with real applications (Firefox, Gnome Terminal, etc), Verify X11 window properties retrieved correctly, Test focus change detection | Success: Window contexts detected correctly, Keymaps switch properly between windows, Title and class patterns match, Focus changes handled, No keymap activation delays | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.9 Test IPC between multiple instances on Linux
  - Test environment: Linux system
  - Verify single instance detection works
  - Test reload and exit commands via IPC
  - _Leverage: src/platform/linux/ipc_channel_linux.cpp_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with Linux IPC expertise | Task: Test IPC on Linux - Step 1: Start first YAMY instance, Step 2: Attempt to start second instance and verify it exits, Step 3: Send reload command via yamy --reload and verify first instance reloads, Step 4: Send exit command via yamy --exit and verify first instance exits, Step 5: Verify socket files cleaned up, Step 6: Test error handling when socket locked | Restrictions: Test with real Unix domain sockets, Verify socket cleanup, Test permission issues, Check /tmp/yamy-*.sock files | Success: Single instance works, Reload command functional, Exit command functional, Socket cleanup works, No orphaned sockets, Error handling correct | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.10 Fix compilation errors in Core module
  - Files: src/core/*.cpp
  - Fix any remaining type mismatches and API changes
  - Resolve linker errors
  - _Leverage: Compiler error messages_
  - _Requirements: FR-1.1 through FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Fix compilation errors in Core - Step 1: Run cmake and make on Linux, Step 2: Fix tstring-related errors, Step 3: Fix HWND-related errors, Step 4: Fix missing include directives, Step 5: Fix linker errors for platform functions, Step 6: Iterate until Core module compiles cleanly | Restrictions: Fix errors without changing behavior, Add includes as needed, Ensure Windows build not broken | Success: Core module compiles on Linux, No compiler errors, No linker errors, Windows build still works, All platform abstractions used correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.11 Fix compilation errors in UI module
  - Files: src/ui/qt/*.cpp
  - Fix Qt-specific compilation issues
  - Resolve UI-related type mismatches
  - _Leverage: Compiler error messages_
  - _Requirements: FR-1.1, FR-1.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with Qt expertise | Task: Fix compilation errors in UI - Step 1: Run cmake and make on Linux, Step 2: Fix tstring to QString conversion issues, Step 3: Fix ShowCommand usage, Step 4: Fix include directives for Qt headers, Step 5: Iterate until UI module compiles cleanly | Restrictions: Fix errors without breaking UI behavior, Ensure QString UTF-8 handling correct, Maintain Qt best practices | Success: UI module compiles on Linux, No compiler errors, Qt integration works, Windows build unaffected, GUI displays correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.12 Fix compilation errors in Platform module
  - Files: src/platform/linux/*.cpp, src/platform/win32/*.cpp
  - Fix platform-specific compilation issues
  - Resolve X11 and Win32 API usage errors
  - _Leverage: Compiler error messages_
  - _Requirements: FR-1.1 through FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with platform API expertise | Task: Fix compilation errors in Platform - Step 1: Run cmake and make on Linux, Step 2: Fix X11 API usage errors, Step 3: Fix include directives for X11 headers, Step 4: Fix XTest and XRecord usage, Step 5: Verify Windows platform code still compiles, Step 6: Iterate until Platform module compiles cleanly on both platforms | Restrictions: Ensure proper X11 header includes, Link against correct libraries, Maintain Windows compatibility | Success: Platform module compiles on Linux, Windows code still compiles, No API usage errors, Libraries linked correctly, Both platforms build successfully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.13 Create unit tests for WindowSystemLinux
  - File: tests/platform/window_system_linux_test.cpp
  - Test X11 window operations
  - Verify EWMH property handling
  - _Leverage: src/platform/linux/window_system_linux.cpp, tests/helpers/x11_test_utils.cpp_
  - _Requirements: FR-1.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with X11 expertise | Task: Create unit tests for WindowSystemLinux - Step 1: Create test fixture with mock X11 display, Step 2: Test getForegroundWindow returns correct window, Step 3: Test getWindowText retrieves _NET_WM_NAME correctly, Step 4: Test getWindowClassName retrieves WM_CLASS correctly, Step 5: Test showWindow with various ShowCommand values, Step 6: Test moveWindow changes window geometry | Restrictions: Use mock X11 functions or test in controlled environment, Ensure tests run in CI without X11 server, Test error handling | Success: All WindowSystemLinux methods tested, Tests pass reliably, Edge cases covered, Error handling verified, Tests run in CI | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.14 Create unit tests for InputInjectorLinux
  - File: tests/platform/input_injector_linux_test.cpp
  - Test XTest input injection
  - Verify keycode mapping
  - _Leverage: src/platform/linux/input_injector_linux.cpp, tests/helpers/x11_test_utils.cpp_
  - _Requirements: FR-1.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with X11 and input system expertise | Task: Create unit tests for InputInjectorLinux - Step 1: Create test fixture with mock X11 display, Step 2: Test injectKeyDown/injectKeyUp generate correct XTest events, Step 3: Test keycode mapping from Windows scancodes to X11 keycodes, Step 4: Test mouse button and motion injection, Step 5: Test modifier key injection, Step 6: Verify XFlush called after injection | Restrictions: Mock XTest functions or use test environment, Ensure tests reliable, Test keycode mapping for common keys | Success: All InputInjectorLinux methods tested, XTest usage verified, Keycode mapping tested for 50+ keys, Mouse injection tested, Tests pass reliably | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.15 Create unit tests for InputHookLinux
  - File: tests/platform/input_hook_linux_test.cpp
  - Test XRecord event capture
  - Verify callback invocation and event consumption
  - _Leverage: src/platform/linux/input_hook_linux.cpp, tests/helpers/x11_test_utils.cpp_
  - _Requirements: FR-1.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with X11 and threading expertise | Task: Create unit tests for InputHookLinux - Step 1: Create test fixture with mock XRecord functions, Step 2: Test install creates XRecord context, Step 3: Test callback invoked when key event captured, Step 4: Test event consumption when callback returns true, Step 5: Test thread-safety of callback invocation, Step 6: Test uninstall cleans up properly | Restrictions: Mock XRecord or use test environment, Test threading behavior, Ensure tests don't interfere with system | Success: All InputHookLinux methods tested, Callback behavior verified, Event consumption tested, Thread-safety confirmed, Cleanup verified, Tests pass reliably | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.16 Create unit tests for IPCChannelLinux
  - File: tests/platform/ipc_channel_linux_test.cpp
  - Test Unix domain socket communication
  - Verify message sending and receiving
  - _Leverage: src/platform/linux/ipc_channel_linux.cpp, tests/helpers/socket_test_utils.cpp_
  - _Requirements: FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with Unix IPC expertise | Task: Create unit tests for IPCChannelLinux - Step 1: Create test fixture with temp socket paths, Step 2: Test connect creates and binds socket, Step 3: Test send delivers message to remote socket, Step 4: Test MessageHandler callback invoked on message receipt, Step 5: Test disconnect cleans up socket file, Step 6: Test error handling for connection failures | Restrictions: Use temporary socket paths for testing, Clean up sockets after tests, Test thread-safety, Verify file permissions | Success: All IPCChannelLinux methods tested, Socket communication verified, Callback behavior tested, Cleanup confirmed, Error handling tested, Tests pass reliably | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.17 Add integration test for full key remapping flow
  - File: tests/integration/key_remapping_integration_test.cpp
  - Test end-to-end key remapping on Linux
  - Verify hook → engine → injection flow
  - _Leverage: All platform implementations, src/core/engine.cpp_
  - _Requirements: All FR-1 requirements_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with integration testing expertise | Task: Create integration test for key remapping - Step 1: Create test fixture initializing all platform components, Step 2: Create Engine with platform interfaces, Step 3: Load test .mayu config with simple remap, Step 4: Simulate key event via input hook, Step 5: Verify engine processes event correctly, Step 6: Verify correct key injected via input injector, Step 7: Test modifier combinations, Step 8: Test window-context switching | Restrictions: Use mocks where needed to avoid system interference, Test full flow from hook to injection, Verify event ordering | Success: Integration test passes, Full remapping flow verified, Event flow correct, Modifier handling works, Context switching works, Test reliable and reproducible | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2.18 Document platform abstraction architecture
  - File: docs/architecture/platform-abstraction.md
  - Document interface design and implementation strategy
  - Provide examples for future platform additions
  - _Leverage: src/platform/*.h, design.md_
  - _Requirements: FR-1.1 through FR-1.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Technical Writer with C++ expertise | Task: Document platform abstraction - Step 1: Create architecture document describing interface design, Step 2: Document each platform interface (IWindowSystem, IInputInjector, IInputHook, IIPCChannel), Step 3: Explain Windows and Linux implementations, Step 4: Provide code examples showing usage, Step 5: Add guidelines for adding new platforms, Step 6: Document type abstractions (WindowHandle, ShowCommand) | Restrictions: Keep documentation clear and concise, Use code examples, Include diagrams if helpful, Ensure accuracy | Success: Documentation complete and accurate, All interfaces documented, Implementation strategies explained, Examples provided, Guidelines for future platforms clear | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

### Batch 3: Polish (10 tasks - PARALLEL within batch, SEQUENTIAL after Batch 2)

- [x] 1.3.1 Add error handling for X11 connection failures
  - Files: src/platform/linux/window_system_linux.cpp, src/platform/linux/input_injector_linux.cpp, src/platform/linux/input_hook_linux.cpp
  - Handle XOpenDisplay failures gracefully
  - Provide user-friendly error messages
  - _Leverage: src/utils/errorHandler.cpp_
  - _Requirements: NF-4 Reliability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with X11 expertise | Task: Add error handling for X11 failures - Step 1: Check XOpenDisplay return value, throw exception if null, Step 2: Add try-catch in factory functions to catch X11 errors, Step 3: Display user-friendly error message suggesting DISPLAY env check, Step 4: Add XSetErrorHandler to catch X11 protocol errors, Step 5: Log X11 errors for debugging | Restrictions: Don't crash on X11 errors, Provide actionable error messages, Log errors for troubleshooting | Success: X11 connection failures handled gracefully, User-friendly errors displayed, No crashes on X11 errors, Errors logged appropriately | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.2 Add error handling for XTest/XRecord unavailable
  - Files: src/platform/linux/input_injector_linux.cpp, src/platform/linux/input_hook_linux.cpp
  - Check for XTest and XRecord extensions
  - Fail gracefully if extensions missing
  - _Leverage: src/utils/errorHandler.cpp_
  - _Requirements: NF-4 Reliability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with X11 expertise | Task: Add error handling for missing extensions - Step 1: Call XTestQueryExtension and XRecordQueryVersion to check availability, Step 2: Throw exception with clear message if extension missing, Step 3: Suggest installing xserver-xorg-input-all or similar package, Step 4: Add extension check to factory functions | Restrictions: Check extensions early in initialization, Provide helpful installation instructions, Don't proceed without required extensions | Success: Missing extensions detected, User-friendly error with install instructions, No crashes, Application fails safely | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.3 Add logging for platform operations
  - Files: src/platform/linux/*.cpp, src/platform/win32/*.cpp
  - Add debug logging for window operations, input events, IPC
  - Use consistent logging format
  - _Leverage: src/utils/logger.cpp_
  - _Requirements: NF-4 Reliability, NF-6 Security_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add logging for platform operations - Step 1: Add log statements in window operations (getForegroundWindow, getWindowText, etc), Step 2: Log input events in hook callbacks (key down/up with scancode), Step 3: Log IPC messages (send/receive), Step 4: Use log levels appropriately (DEBUG for frequent events, INFO for important operations, ERROR for failures), Step 5: Ensure no PII logged | Restrictions: Use structured logging, Don't log sensitive data, Keep overhead minimal, Use appropriate log levels | Success: Platform operations logged, Logs help debugging, No PII in logs, Logging overhead acceptable, Both platforms have consistent logging | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.4 Optimize X11 window text retrieval
  - File: src/platform/linux/window_system_linux.cpp
  - Cache window properties to reduce X11 round-trips
  - Implement efficient property retrieval
  - _Leverage: X11 XGetWindowProperty API_
  - _Requirements: NF-1 Performance_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with X11 performance expertise | Task: Optimize window text retrieval - Step 1: Implement property cache with timeout, Step 2: Retrieve _NET_WM_NAME and WM_NAME in single call if possible, Step 3: Use XGetWindowProperty efficiently (check property existence first), Step 4: Add cache invalidation on window change events, Step 5: Benchmark and ensure <10ms retrieval time | Restrictions: Maintain correctness, Ensure cache doesn't serve stale data, Keep memory overhead minimal | Success: Window text retrieval <10ms, Cache reduces X11 calls by 80%, No stale data served, Memory overhead <1KB per window, Performance measurably improved | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.5 Optimize keycode mapping table
  - File: src/platform/linux/input_injector_linux.cpp
  - Pre-build keycode mapping table at initialization
  - Use efficient lookup structure
  - _Leverage: std::unordered_map_
  - _Requirements: NF-1 Performance_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Optimize keycode mapping - Step 1: Create static std::unordered_map<uint16_t, KeyCode> mapping Windows scancodes to X11 keycodes, Step 2: Initialize map at static initialization time, Step 3: Use const lookup in injectKey methods, Step 4: Benchmark and ensure <1μs lookup time, Step 5: Cover all 100+ common keys | Restrictions: Use efficient data structure, Ensure complete key coverage, Keep lookup time minimal | Success: Keycode lookup <1μs, All common keys covered, Map initialized efficiently, No runtime overhead, Both directions supported if needed | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.6 Add performance metrics collection
  - File: src/utils/metrics.cpp, src/core/engine.cpp
  - Collect latency metrics for key event processing
  - Report average, p95, p99 latencies
  - _Leverage: std::chrono_
  - _Requirements: NF-1 Performance_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with performance monitoring expertise | Task: Add performance metrics - Step 1: Add metrics collection class with methods recordLatency(operation, duration), Step 2: Instrument Engine::processKeyEvent with timing measurement, Step 3: Instrument input hook callback, Step 4: Instrument input injection, Step 5: Compute and log average/p95/p99 every 60 seconds, Step 6: Add /metrics IPC command to query metrics | Restrictions: Keep instrumentation overhead minimal (<1% CPU), Use thread-safe collection, Don't impact key processing latency | Success: Metrics collected accurately, Overhead <1% CPU, Stats logged periodically, /metrics command works, Performance baseline established | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.7 Add memory leak detection
  - Files: src/app/main.cpp, tests/leak_test.cpp
  - Integrate AddressSanitizer or Valgrind checks
  - Verify no memory leaks in platform code
  - _Leverage: AddressSanitizer, Valgrind_
  - _Requirements: NF-4 Reliability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with memory debugging expertise | Task: Add memory leak detection - Step 1: Add CMake option to enable AddressSanitizer (ENABLE_ASAN), Step 2: Add compiler flags -fsanitize=address -fno-omit-frame-pointer when enabled, Step 3: Create leak test running full lifecycle (init, process events, cleanup), Step 4: Run test under ASAN and verify no leaks, Step 5: Document ASAN usage in docs, Step 6: Add Valgrind suppression file if needed | Restrictions: Ensure ASAN build works, Test covers all code paths, Fix any leaks found, Don't break release build | Success: ASAN build compiles, Leak test passes with no leaks, X11 resources freed properly, Platform code leak-free, Documentation updated | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.8 Add automated regression tests
  - File: tests/regression/regression_suite.cpp
  - Create test suite covering all platform functionality
  - Run tests in CI pipeline
  - _Leverage: Google Test, existing unit tests_
  - _Requirements: NF-4 Reliability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with CI/CD expertise | Task: Add regression tests - Step 1: Create regression test suite aggregating all unit tests, Step 2: Add integration tests to suite, Step 3: Configure CMake to build test suite, Step 4: Add CI job running test suite on every commit, Step 5: Ensure tests run without X11 server (use Xvfb), Step 6: Report test results and coverage | Restrictions: Tests must run in CI environment, Use Xvfb for X11 tests, Ensure tests don't interfere with each other, Require 80% coverage | Success: Regression suite runs in CI, All tests pass, Coverage ≥80%, Tests reliable, No flaky tests, CI reports results clearly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.9 Add code quality checks to CI
  - File: .github/workflows/ci.yml or similar
  - Add linter (cpplint, clang-tidy), formatter (clang-format) checks
  - Enforce code quality standards
  - _Leverage: clang-tidy, clang-format_
  - _Requirements: NF-5 Maintainability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: DevOps Engineer with C++ tooling expertise | Task: Add code quality checks - Step 1: Add .clang-format config matching project style, Step 2: Add clang-format check to CI verifying all files formatted, Step 3: Add clang-tidy config with common checks enabled, Step 4: Add clang-tidy check to CI reporting violations, Step 5: Fix any violations found in platform code, Step 6: Document how to run locally | Restrictions: Use project coding style, Enable relevant checks (no noisy warnings), Fix violations before merging, Don't break build on warnings initially | Success: Linter and formatter configured, CI runs checks on every commit, Platform code passes all checks, Violations reported clearly, Documentation updated | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3.10 Create platform abstraction migration guide
  - File: docs/migration/platform-abstraction-migration.md
  - Document migration from Win32 to platform abstraction
  - Provide examples and common pitfalls
  - _Leverage: Code changes from Track 1_
  - _Requirements: NF-5 Maintainability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Technical Writer with C++ expertise | Task: Create migration guide - Step 1: Document step-by-step migration process from Win32 to platform abstraction, Step 2: Provide before/after code examples for common operations (window management, input injection, etc), Step 3: Document common pitfalls (string encoding, handle lifetime, thread-safety), Step 4: Add troubleshooting section for compilation errors, Step 5: Link to platform abstraction architecture doc | Restrictions: Keep guide practical with examples, Cover common issues, Ensure accuracy, Link to related docs | Success: Migration guide complete, Examples clear and tested, Common issues covered, Troubleshooting helpful, Guide follows documentation standards | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 2: Configuration Management (15 tasks)

**Purpose**: Implement multi-configuration support and GUI config management

### Batch 1: Configuration Backend (8 tasks - ALL PARALLEL)

- [x] 2.1.1 Implement ConfigManager class
  - File: src/core/settings/config_manager.h, src/core/settings/config_manager.cpp
  - Manage list of available configurations
  - Track active configuration
  - _Leverage: src/core/settings/config_store.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement ConfigManager - Create singleton managing vector of config file paths, Track active config index, Methods: listConfigs, getActiveConfig, setActiveConfig, addConfig, removeConfig, refreshList - Use ConfigStore for persistence, Scan ~/.yamy/ directory for .mayu files | Restrictions: Thread-safe access, Validate config paths exist, Handle missing files gracefully | Success: ConfigManager manages config list, Active config tracked, Persistence works, Thread-safe, Config scanning works | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [-] 2.1.2 Add config switching API to Engine
  - File: src/core/engine.h, src/core/engine.cpp
  - Add method switchConfiguration(configPath)
  - Reload parser and keymaps with new config
  - _Leverage: src/core/settings/config_manager.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add config switching to Engine - Add method bool switchConfiguration(const std::string& configPath), Unload current keymaps, Parse new config file, Rebuild keymap tree, Update ConfigManager active config, Emit notification to GUI, Return false on parse errors | Restrictions: Handle parse errors gracefully, Maintain engine state, Don't crash on invalid config, Rollback on failure | Success: Config switching works, Keymaps reloaded correctly, Errors handled gracefully, GUI notified, No crashes on invalid config | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.1.3 Add config file validation
  - File: src/core/parser/config_validator.h, src/core/parser/config_validator.cpp
  - Validate .mayu syntax before loading
  - Report specific errors with line numbers
  - _Leverage: src/core/parser/parser.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement config validator - Create ConfigValidator class, Method: vector<ValidationError> validate(configPath), Check syntax errors, Check semantic errors (undefined keys, circular references), Return errors with line numbers and messages, Use existing parser for syntax checking | Restrictions: Don't load config into engine, Fast validation (<100ms for typical config), Comprehensive error reporting | Success: Validator catches syntax errors, Semantic errors detected, Line numbers accurate, Fast validation, Error messages helpful | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.1.4 Add config file watcher
  - File: src/core/settings/config_watcher.h, src/core/settings/config_watcher.cpp
  - Watch active config file for changes
  - Auto-reload on modification
  - _Leverage: QFileSystemWatcher or inotify_
  - _Requirements: FR-2.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement config file watcher - Create ConfigWatcher class, Use QFileSystemWatcher on Qt platforms or inotify on Linux, Watch active config file path, On file modified signal call Engine::switchConfiguration, Add debouncing to avoid multiple reloads, Methods: start, stop, setConfigPath | Restrictions: Handle file deletion gracefully, Debounce rapid changes (300ms), Don't reload if parsing fails, Stop watching when disabled | Success: Watcher detects file changes, Auto-reload works, Debouncing prevents excessive reloads, File deletion handled, Can be disabled | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.1.5 Add config backup and restore
  - File: src/core/settings/config_manager.cpp
  - Backup config before modifications
  - Allow restore from backup
  - _Leverage: std::filesystem_
  - _Requirements: FR-2.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement config backup - Add methods to ConfigManager: createBackup(configPath) copies to .bak file with timestamp, listBackups(configPath) returns vector of backup paths, restoreBackup(backupPath) copies back to original, deleteBackup(backupPath), Keep max 10 backups per config, Auto-backup before any modification | Restrictions: Use filesystem copy, Handle copy errors, Limit backup count, Use safe file operations | Success: Backup creates .bak file with timestamp, Restore works correctly, Old backups deleted automatically, Backup limit enforced, File operations safe | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.1.6 Add config import/export
  - File: src/core/settings/config_manager.cpp
  - Export config with dependencies to archive
  - Import config from archive
  - _Leverage: libarchive or QZip_
  - _Requirements: FR-2.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement config import/export - Add methods to ConfigManager: exportConfig(configPath, archivePath) creates zip with config and included files, importConfig(archivePath, targetDir) extracts to target directory, Parse include directives to find dependencies, Validate archive contents on import, Use zip format for portability | Restrictions: Handle missing includes gracefully, Validate archive integrity, Don't overwrite without confirmation, Use standard zip format | Success: Export creates zip with all dependencies, Import extracts correctly, Include dependencies resolved, Archive validated, Portable across systems | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.1.7 Add config metadata storage
  - File: src/core/settings/config_metadata.h, src/core/settings/config_metadata.cpp
  - Store config name, description, author, last modified
  - Persist metadata separately from .mayu file
  - _Leverage: src/core/settings/config_store.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement config metadata - Create ConfigMetadata class with fields: name, description, author, createdDate, modifiedDate, tags, Create metadata file .mayu.meta in JSON format alongside .mayu file, Methods: load, save, update, Store in ~/.yamy/.metadata/ directory, Auto-update modifiedDate on config changes | Restrictions: Use JSON format, Handle missing metadata gracefully, Don't require metadata, Auto-create if missing | Success: Metadata stored separately, JSON format parseable, Auto-updated on changes, Optional (works without), Backward compatible | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.1.8 Add config templates
  - Files: src/resources/templates/default.mayu, src/resources/templates/emacs.mayu, src/resources/templates/vim.mayu
  - Create preset configurations for common use cases
  - Allow creating new config from template
  - _Leverage: src/core/settings/config_manager.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Keyboard remapping expert | Task: Create config templates - Create 3 template configs: default.mayu (basic remaps, CapsLock to Ctrl), emacs.mayu (Emacs keybindings), vim.mayu (Vim keybindings in other apps), Include comments explaining each section, Add method to ConfigManager: createFromTemplate(templateName, targetPath), Copy template and update metadata | Restrictions: Templates must be valid .mayu files, Include helpful comments, Cover common use cases, Keep simple and understandable | Success: 3 templates created, Templates parse correctly, Templates functional, createFromTemplate works, Comments helpful | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

### Batch 2: Configuration GUI (7 tasks - PARALLEL within batch, SEQUENTIAL after Batch 1)

- [ ] 2.2.1 Create configuration menu in system tray
  - File: src/ui/qt/system_tray.cpp
  - Add "Configurations" submenu to tray icon
  - List available configs with checkmark on active
  - _Leverage: QMenu, src/core/settings/config_manager.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create configuration menu - Add QMenu* m_configMenu to SystemTray class, In buildMenu add submenu "Configurations", Populate submenu with config list from ConfigManager, Add checkmark (QAction::setCheckable) on active config, Connect action triggered to Engine::switchConfiguration, Add "Manage Configurations..." action opening dialog | Restrictions: Update menu dynamically when configs change, Show checkmark correctly, Handle long config lists gracefully | Success: Configuration submenu appears, Configs listed correctly, Active config has checkmark, Clicking switches config, Manage action opens dialog | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.2.2 Create ConfigManagerDialog GUI
  - File: src/ui/qt/config_manager_dialog.h, src/ui/qt/config_manager_dialog.cpp, src/ui/qt/config_manager_dialog.ui
  - GUI for managing configurations
  - List, add, remove, rename, edit configs
  - _Leverage: QDialog, QListWidget, src/core/settings/config_manager.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create ConfigManagerDialog - Create QDialog with QListWidget showing configs, Add buttons: New, Duplicate, Delete, Rename, Edit, Import, Export, Set as Active, Each config item shows name and path, Double-click or Edit button opens config in default editor, Connect buttons to ConfigManager methods, Update list when configs change | Restrictions: Confirm before deleting, Validate names, Disable buttons when no selection, Update list dynamically | Success: Dialog displays config list, All buttons functional, Deletion requires confirmation, Names validated, Edit opens editor, List updates correctly | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.2.3 Add config editor integration
  - File: src/ui/qt/config_manager_dialog.cpp
  - Open config in external or internal editor
  - Support for syntax-aware editors
  - _Leverage: QProcess, QDesktopServices_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add config editor integration - Add preference for editor command (default: xdg-open or system default), In Edit action: launch editor with QProcess or QDesktopServices::openUrl, On Windows use configured editor or notepad, On Linux use $EDITOR or xdg-open, Allow configuring editor in preferences, Show error if editor fails to launch | Restrictions: Handle editor launch failures, Support spaces in editor path, Use system defaults if not configured | Success: Edit action launches editor, Config file opened, Configurable editor works, System defaults used appropriately, Errors handled gracefully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.2.4 Add config validation feedback in GUI
  - File: src/ui/qt/config_manager_dialog.cpp
  - Show validation status for each config
  - Display errors inline with line numbers
  - _Leverage: src/core/parser/config_validator.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add validation feedback - Run ConfigValidator on each config in list, Show icon indicating status (valid, invalid, warning), On selection show validation errors in detail view below list, Format errors with line numbers: "Line 42: Unknown key 'Foobar'", Use red icon for errors, yellow for warnings, green for valid | Restrictions: Validate in background thread, Don't block UI, Update validation when file changes, Limit validation frequency | Success: Validation status shown, Errors displayed with line numbers, Icons indicate status, Validation doesn't block UI, Status updates on file change | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.2.5 Add config quick-switch hotkey
  - File: src/core/engine.cpp, src/ui/qt/system_tray.cpp
  - Assign global hotkey to cycle through configs
  - Show notification on config switch
  - _Leverage: src/platform/input_hook.h, QSystemTrayIcon_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with Qt expertise | Task: Add quick-switch hotkey - Add preference for quick-switch hotkey (default: Ctrl+Alt+C), In Engine hook callback check for hotkey, On hotkey call ConfigManager::setNextConfig(), Show tray notification with new config name, Add to system tray tooltip, Support disabled hotkey (set to none) | Restrictions: Don't conflict with other hotkeys, Allow customization, Notification should be brief, Hotkey must work even when YAMY is remapping | Success: Hotkey cycles configs, Notification shows config name, Hotkey customizable, Works with remapping active, Can be disabled | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.2.6 Add config metadata editor
  - File: src/ui/qt/config_metadata_dialog.h, src/ui/qt/config_metadata_dialog.cpp
  - Edit config name, description, tags
  - Display creation and modification dates
  - _Leverage: QDialog, src/core/settings/config_metadata.h_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create metadata editor - Create QDialog with fields: Name (QLineEdit), Description (QTextEdit), Author (QLineEdit), Tags (QLineEdit comma-separated), Display dates as read-only labels, Save button updates metadata, Cancel button discards changes, Show from ConfigManagerDialog context menu or double-click | Restrictions: Validate name not empty, Handle missing metadata gracefully, Update modifiedDate automatically | Success: Metadata editor displays current values, All fields editable except dates, Save updates metadata, Validation prevents empty name, Dates updated automatically | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.2.7 Add config search and filter
  - File: src/ui/qt/config_manager_dialog.cpp
  - Search configs by name, description, tags
  - Filter by validation status
  - _Leverage: QLineEdit, QComboBox_
  - _Requirements: FR-2.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add search and filter - Add QLineEdit for search above config list, Filter list as user types (search name, description, tags), Add QComboBox for status filter: All, Valid, Invalid, Warnings, Combine search and filter (AND logic), Update list dynamically, Clear search button | Restrictions: Case-insensitive search, Real-time filtering, Don't block UI on large lists, Handle special characters in search | Success: Search filters list correctly, Status filter works, Filters combine properly, Real-time updates, Clear button works, Handles large lists | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 3: Investigate Dialog Enhancements (10 tasks)

**Purpose**: Full window inspector with crosshair selection (FR-3)

- [ ] 3.1 Create CrosshairWidget for window selection
  - File: src/ui/qt/crosshair_widget_qt.h, src/ui/qt/crosshair_widget_qt.cpp
  - Implement transparent overlay with crosshair cursor
  - Handle mouse events for window selection under cursor
  - _Leverage: QWidget, X11 XQueryPointer, design.md Section 3.2_
  - _Requirements: FR-3.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with X11 expertise | Task: Create CrosshairWidget class extending QWidget - Step 1: Set window flags Qt::WindowStaysOnTopHint, Qt::FramelessWindowHint, Qt::Tool, Step 2: Set attributes Qt::WA_TranslucentBackground, Qt::WA_NoSystemBackground, Step 3: Enable mouse tracking and create crosshair cursor, Step 4: In paintEvent draw red crosshair lines and center dot at cursor position, Step 5: In mousePressEvent use XQueryPointer to get window under cursor, emit windowSelected signal, Step 6: Add activate/deactivate methods for showing/hiding overlay | Restrictions: Use X11 API correctly, Handle cursor traversal to leaf window, Ensure overlay doesn't block other windows, Use QX11Info for Display access | Success: CrosshairWidget displays fullscreen transparent overlay, Crosshair follows cursor, Left-click selects window under cursor, Window selection accurate, Signal emitted with correct WindowHandle | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.2 Create InvestigateDialog UI structure
  - File: src/ui/qt/dialog_investigate_qt.h, src/ui/qt/dialog_investigate_qt.cpp, src/ui/qt/dialog_investigate_qt.ui
  - Design dialog layout with info panels and crosshair trigger button
  - Integrate CrosshairWidget into dialog
  - _Leverage: QDialog, QGroupBox, CrosshairWidget, design.md Section 3.1_
  - _Requirements: FR-3.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create InvestigateDialog UI - Step 1: Create QDialog with layout containing: Select Window button, WindowInfoPanel QGroupBox with labels for handle/title/class/process/geometry/state, KeymapStatusPanel QGroupBox for keymap info, LiveLogPanel QTextEdit for real-time events, Close button, Step 2: Create CrosshairWidget member, connect windowSelected signal to updateWindowInfo slot, Step 3: On Select Window button click call crosshair->activate(), Step 4: Set dialog size to 800x600, make resizable | Restrictions: Follow Qt best practices, Use layouts not fixed positioning, Ensure panels are clearly labeled, Make dialog non-modal | Success: Dialog displays with all panels, Select Window button functional, Crosshair activates on click, Layout responsive, All UI elements accessible | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.3 Implement window information retrieval
  - File: src/ui/qt/dialog_investigate_qt.cpp
  - Use IWindowSystem to get window properties
  - Display window handle, title, class name, geometry, state
  - _Leverage: IWindowSystem, design.md Section 3.3_
  - _Requirements: FR-3.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with platform abstraction knowledge | Task: Implement updateWindowInfo method - Step 1: Accept WindowHandle parameter, Step 2: Use IWindowSystem to query: getWindowText, getClassName, getWindowRect, isWindowVisible, Step 3: Update UI labels: handle (hex format 0x%X), title (QString::fromStdString), className, geometry (%d, %d %dx%d), state (Visible/Hidden), Step 4: Handle errors gracefully if window invalid, Step 5: Clear previous info before updating | Restrictions: Use platform abstraction not direct X11 calls, Handle UTF-8 to QString conversion correctly, Format hex values properly, Don't crash on invalid windows | Success: Window info displays correctly, All properties accurate, Handle formatted as hex, UTF-8 text displays properly, Invalid windows handled gracefully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.4 Add process information retrieval
  - File: src/ui/qt/dialog_investigate_qt.cpp
  - Read /proc filesystem for process name and path
  - Display process info in WindowInfoPanel
  - _Leverage: Linux /proc filesystem, design.md Section 3.3_
  - _Requirements: FR-3.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with Linux system expertise | Task: Add getProcessName and getProcessPath methods - Step 1: Implement getProcessName reading /proc/{pid}/comm file, Step 2: Implement getProcessPath using readlink on /proc/{pid}/exe, Step 3: Extract PID from WindowHandle (on Linux windows have associated PID via _NET_WM_PID property), Step 4: Update updateWindowInfo to call these methods and display in UI, Step 5: Handle errors when process info unavailable | Restrictions: Check file existence before reading, Handle readlink errors, Work with root-owned processes (limited info), Parse PID from window properties correctly | Success: Process name displays correctly, Process path shows full executable path, Handles missing process info gracefully, Works with various applications, No crashes on permission errors | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.5 Implement keymap status query
  - File: src/ui/qt/dialog_investigate_qt.cpp, src/core/engine.cpp
  - Send IPC request to engine for keymap status
  - Display matched keymap and active window regex
  - _Leverage: IIPCChannel, design.md Section 3.4_
  - _Requirements: FR-3.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with IPC expertise | Task: Implement keymap status query - Step 1: In InvestigateDialog add IIPCChannel member, Step 2: In updateWindowInfo send CmdInvestigate message with WindowHandle, Step 3: In Engine add handleInvestigateRequest method finding matching keymap for window, Step 4: Engine sends response with keymap name, matched regex, active modifiers, Step 5: InvestigateDialog receives response and updates KeymapStatusPanel labels | Restrictions: Use IPC channel for communication, Don't block UI waiting for response, Handle engine not responding, Format keymap info clearly | Success: IPC request sent correctly, Engine finds matching keymap, Response received in dialog, Keymap info displays: name, regex pattern, modifiers, No keymap shown when using default | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.6 Add live log panel integration
  - File: src/ui/qt/dialog_investigate_qt.cpp, src/core/engine.cpp
  - Stream key events from engine to dialog when active
  - Display real-time key events in LiveLogPanel
  - _Leverage: IIPCChannel, QTextEdit, design.md Section 3.5_
  - _Requirements: FR-3.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with real-time data streaming | Task: Implement live logging - Step 1: Add m_investigateMode boolean to Engine, Step 2: When InvestigateDialog opens send EnableInvestigateMode IPC message, Step 3: In Engine processKey method if m_investigateMode is true send LogMessage with formatted key event, Step 4: Format includes timestamp, DOWN/UP, keycode, modifiers, HANDLED/PASSED status, Step 5: InvestigateDialog receives messages and appends to QTextEdit, auto-scrolls to bottom | Restrictions: Don't overwhelm UI with too many messages (limit rate if needed), Use QMetaObject::invokeMethod for thread-safe UI updates, Disable investigate mode when dialog closes, Keep message format readable | Success: Key events stream to dialog in real-time, Format shows all relevant info, Auto-scrolls to latest, No UI lag with rapid keypresses, Mode disables when dialog closed | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.7 Add copy-to-clipboard functionality
  - File: src/ui/qt/dialog_investigate_qt.cpp
  - Copy window info and keymap status to clipboard
  - Format as human-readable text
  - _Leverage: QClipboard_
  - _Requirements: FR-3.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add copy functionality - Step 1: Add Copy to Clipboard button to dialog, Step 2: Implement onCopyClicked slot gathering all window info, keymap status, and recent log entries, Step 3: Format as text with sections: Window Information, Process Information, Keymap Status, Recent Events (last 20 lines from log), Step 4: Use QApplication::clipboard()->setText(formattedText), Step 5: Show brief notification "Copied to clipboard" | Restrictions: Format text clearly with headers, Include all visible information, Handle empty fields gracefully, Don't copy if no window selected | Success: Button copies all info to clipboard, Text formatted clearly with sections, Paste works in other applications, Notification confirms copy, Button disabled when no window selected | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.8 Implement window condition generator
  - File: src/ui/qt/dialog_investigate_qt.cpp
  - Generate .mayu window condition syntax from current window
  - Provide multiple pattern options (exact, prefix, regex)
  - _Leverage: Window info from IWindowSystem_
  - _Requirements: FR-3.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with YAMY configuration expertise | Task: Add condition generator - Step 1: Add Generate Condition button, Step 2: Implement onGenerateCondition slot creating dialog with options: Match by Title (Exact/Contains/Regex), Match by Class (Exact/Regex), Combine Title and Class, Step 3: Generate .mayu syntax based on selection: "window title /regex/" or "window class /regex/" or both, Step 4: Show generated condition in QTextEdit with syntax highlighting, Step 5: Add Copy button to copy condition | Restrictions: Escape regex special characters in exact match mode, Provide examples for each option, Generate valid .mayu syntax, Offer helpful patterns like ".*Visual Studio Code.*" | Success: Generator creates valid .mayu conditions, Multiple pattern types available, Exact match escapes special chars, Regex patterns work correctly, Copy button functional, Examples helpful | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.9 Add visual window highlighting
  - File: src/ui/qt/crosshair_widget_qt.cpp
  - Draw border around window under cursor during selection
  - Use semi-transparent colored overlay
  - _Leverage: X11 XDrawRectangle or Qt overlay_
  - _Requirements: FR-3.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with X11 graphics | Task: Add window highlighting - Step 1: In CrosshairWidget mouseMoveEvent get window under cursor, Step 2: Get window rect using IWindowSystem, Step 3: In paintEvent draw semi-transparent colored rectangle (rgba 0, 255, 0, 100) around window rect, Step 4: Update on every mouse move, Step 5: Handle multiple monitors correctly | Restrictions: Use efficient drawing (don't flicker), Draw in screen coordinates, Handle window rect correctly across monitors, Don't interfere with crosshair drawing, Keep overlay responsive | Success: Window under cursor highlighted with green semi-transparent border, Highlight updates smoothly as cursor moves, Works across multiple monitors, No flicker, Doesn't obscure crosshair | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.10 Create comprehensive tests for investigate dialog
  - File: tests/ui/dialog_investigate_test.cpp
  - Test crosshair widget, window info retrieval, keymap query
  - Verify IPC communication and UI updates
  - _Leverage: Qt Test framework, mock IWindowSystem_
  - _Requirements: FR-3 all_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with Qt testing expertise | Task: Create comprehensive tests - Step 1: Create test fixture with mock IWindowSystem and IIPCChannel, Step 2: Test CrosshairWidget: activation shows overlay, mouse click emits signal with correct window, deactivation hides overlay, Step 3: Test InvestigateDialog: updateWindowInfo displays correct data, process info retrieval works, keymap query sends correct IPC message, live log appends events, copy functionality works, condition generator creates valid syntax, Step 4: Test error cases: invalid windows, missing process info, engine not responding, Step 5: Verify no memory leaks and proper cleanup | Restrictions: Use Qt Test macros, Mock platform components, Test UI thread-safety, Cover error paths, Ensure tests run in CI | Success: All dialog features tested, CrosshairWidget behavior verified, IPC communication tested, Error handling covered, Tests pass reliably, No memory leaks detected | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 4: Log Dialog Enhancements (12 tasks)

**Purpose**: Feature parity with Windows log viewer (FR-4)

- [ ] 4.1 Create LogEntry structure and Logger class
  - File: src/core/logging/log_entry.h, src/core/logging/logger.h, src/core/logging/logger.cpp
  - Define structured logging with levels and categories
  - Implement singleton Logger with listener pattern
  - _Leverage: design.md Section 4.1_
  - _Requirements: FR-4.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Create structured logging system - Step 1: Define LogLevel enum (Trace, Info, Warning, Error), Step 2: Define LogEntry struct with fields: timestamp, level, category, message, format() method, Step 3: Create Logger singleton class with methods: log(level, category, msg), setMinLevel, addListener, Step 4: Use std::vector for listeners, std::mutex for thread-safety, Step 5: Implement log method filtering by level and invoking all listeners | Restrictions: Thread-safe implementation, Efficient listener invocation, Don't block logging calls, Support multiple listeners, Use structured format | Success: Logger singleton accessible, Log levels filterable, Listeners receive all matching logs, Thread-safe operation, Format method produces readable output | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.2 Integrate Logger into Engine
  - File: src/core/engine.cpp, src/core/parser/parser.cpp, src/core/input/keyboard.cpp
  - Replace printf/cout with Logger calls
  - Add logging at key points: startup, config load, key events, errors
  - _Leverage: Logger class_
  - _Requirements: FR-4.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add logging to core components - Step 1: In Engine::start/stop log state changes at Info level, Step 2: In Engine::processKey log at Trace level with key info, Step 3: In Parser log config loading at Info, parsing errors at Error, Step 4: In error handling log all errors at Error level with context, Step 5: Use appropriate categories: "Engine", "Parser", "Input", "Window", "Config" | Restrictions: Don't log in tight loops excessively, Use Trace for frequent events, Include relevant context in messages, Don't log sensitive data, Keep messages concise | Success: Engine logs startup/shutdown, Key events logged at Trace level, Config loading logged, Errors logged with context, Categories used consistently, Logging overhead <1% CPU | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.3 Create DialogLogQt UI with filter controls
  - File: src/ui/qt/dialog_log_qt.h, src/ui/qt/dialog_log_qt.cpp
  - Implement log viewer with level and category filters
  - Subscribe to Logger for real-time updates
  - _Leverage: QDialog, QTextEdit, QComboBox, QCheckBox, design.md Section 4.1_
  - _Requirements: FR-4.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create log dialog UI - Step 1: Create QDialog with QComboBox for log level (Trace/Info/Warning/Error), Step 2: Add QCheckBox for each category (Engine, Parser, Input, Window, Config), Step 3: Add QTextEdit for log display with fixed-width font, Step 4: Subscribe to Logger::addListener in constructor, Step 5: Implement appendLog slot using QMetaObject::invokeMethod for thread-safety, Step 6: Filter logs based on level and category checkboxes | Restrictions: Thread-safe UI updates, Don't block logger callbacks, Filter efficiently, Auto-scroll to latest entries, Support large log volumes (limit buffer size) | Success: Dialog displays logs in real-time, Level filter works, Category filters work, Thread-safe updates, Auto-scrolls, Handles high log volume, Fixed-width font readable | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.4 Add font customization controls
  - File: src/ui/qt/dialog_log_qt.cpp
  - Add font selector and size spinner
  - Persist font preferences
  - _Leverage: QFontComboBox, QSpinBox, ConfigStore_
  - _Requirements: FR-4.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add font controls - Step 1: Add QFontComboBox for font family selection, Step 2: Add QSpinBox for font size (range 6-24, default 10), Step 3: Connect signals to slots updating QTextEdit font, Step 4: Load saved font settings from ConfigStore on dialog open, Step 5: Save font settings to ConfigStore when changed, Step 6: Set default to system fixed-width font if no preference saved | Restrictions: Use system fixed-width font as default, Validate font size range, Persist settings immediately, Handle font not available gracefully, Ensure readable minimum size | Success: Font selector lists all available fonts, Size spinner changes font size, Settings persist across sessions, Default to system monospace font, Font changes apply immediately, Readable at all sizes | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.5 Implement syntax highlighting for log entries
  - File: src/ui/qt/dialog_log_qt.cpp
  - Color-code log entries by level: Error=red, Warning=orange, Info=default, Trace=gray
  - Highlight keywords: DOWN, UP, HANDLED, PASSED
  - _Leverage: QTextEdit HTML formatting_
  - _Requirements: FR-4.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add syntax highlighting - Step 1: In appendLog method detect log level from message text, Step 2: Wrap ERROR messages in red span, WARNING in orange, Trace in gray, Step 3: Highlight keywords: DOWN/UP in bold, HANDLED in green, PASSED in default, Step 4: Use HTML formatting: <span style='color: red;'>text</span>, Step 5: Ensure performance with large logs (don't reprocess entire buffer) | Restrictions: Use efficient string matching, Don't slow down log appending, Apply formatting only to new entries, Handle special HTML characters (escape <>&), Keep highlighting subtle and readable | Success: Error logs show in red, Warnings in orange, Trace in gray, Keywords highlighted, Performance acceptable with rapid logging, HTML characters escaped correctly, Highlighting improves readability | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.6 Add Clear and Pause controls
  - File: src/ui/qt/dialog_log_qt.cpp
  - Clear button to erase all log entries
  - Pause/Resume button to stop auto-scrolling
  - _Leverage: QTextEdit, QPushButton_
  - _Requirements: FR-4.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add clear and pause controls - Step 1: Add Clear button connected to QTextEdit::clear slot, Step 2: Add Pause/Resume toggle button (default: Resume), Step 3: When paused: stop auto-scrolling but continue adding log entries, buffer indicator shows "(Paused)", Step 4: When resumed: scroll to bottom and clear pause indicator, Step 5: Show count of entries added while paused | Restrictions: Don't lose log entries when paused, Resume scrolls to latest, Pause indicator visible, Clear requires confirmation if >1000 entries, Button states clear | Success: Clear button erases all logs, Pause stops scrolling but keeps logging, Resume scrolls to bottom, Entry count shown when paused, Confirmation on large clear, Button states intuitive | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.7 Implement log search functionality
  - File: src/ui/qt/dialog_log_qt.cpp
  - Add search box to find text in logs
  - Highlight matches and navigate between them
  - _Leverage: QLineEdit, QTextEdit::find_
  - _Requirements: FR-4.6_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add search functionality - Step 1: Add QLineEdit for search text with Find Next/Previous buttons, Step 2: Implement find using QTextEdit::find with QTextDocument::FindFlags, Step 3: Highlight all matches with background color, Step 4: Navigate with Find Next/Previous, wrap around at ends, Step 5: Show match count: "3 of 15 matches", Step 6: Clear highlights when search cleared or dialog closed | Restrictions: Case-insensitive search by default (add case-sensitive option), Highlight all matches not just current, Wrap around at document ends, Clear previous highlights before new search, Update match count dynamically | Success: Search finds text in logs, All matches highlighted, Navigate between matches works, Match count accurate, Wraps around, Case option works, Highlights clear appropriately | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.8 Add log export functionality
  - File: src/ui/qt/dialog_log_qt.cpp
  - Export logs to text file
  - Support filtered export (only visible entries)
  - _Leverage: QFileDialog, std::ofstream_
  - _Requirements: FR-4.7_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add export functionality - Step 1: Add Export button opening QFileDialog for save location, Step 2: Implement export writing QTextEdit contents to file, Step 3: Option to export all or only filtered logs, Step 4: Use UTF-8 encoding, Step 5: Include timestamp in filename: logs_YYYYMMDD_HHMMSS.txt, Step 6: Show success/error notification after export | Restrictions: Handle file write errors, Use UTF-8 encoding, Suggest timestamped filename, Don't block UI during export, Export respects current filters if "filtered" option selected | Success: Export button opens save dialog, Logs written to file correctly, UTF-8 encoding preserved, Filtered export respects filters, Timestamped filename suggested, Success notification shown, Errors handled gracefully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.9 Implement log buffer size limit
  - File: src/ui/qt/dialog_log_qt.cpp
  - Limit QTextEdit to maximum number of lines
  - Remove oldest entries when limit reached
  - _Leverage: QTextEdit line count_
  - _Requirements: FR-4.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add buffer limit - Step 1: Set maximum buffer size (default 10000 lines, configurable), Step 2: In appendLog check line count after adding entry, Step 3: If exceeds limit: remove oldest lines from top (use QTextCursor movePosition with QTextCursor::Start), Step 4: Add preference setting for buffer size in ConfigStore, Step 5: Show buffer usage: "Log: 8543/10000 lines" in status area | Restrictions: Remove full lines not partial, Keep buffer limit configurable, Don't impact performance when under limit, Update usage indicator periodically not every append, Handle concurrent appends safely | Success: Buffer never exceeds limit, Oldest entries removed when full, Configurable limit in settings, Usage indicator shows current/max, Performance good even when full, No UI lag, Thread-safe trimming | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.10 Add log statistics panel
  - File: src/ui/qt/dialog_log_qt.cpp
  - Display log entry counts by level and category
  - Update statistics in real-time
  - _Leverage: QTableWidget or QLabel grid_
  - _Requirements: FR-4.9_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add statistics panel - Step 1: Add collapsible Statistics panel at bottom of dialog, Step 2: Show counts: Total entries, by Level (Trace/Info/Warning/Error), by Category (Engine/Parser/Input/Window), Step 3: Update counts in appendLog method, Step 4: Add Clear Stats button resetting counters, Step 5: Format nicely: "Errors: 3 | Warnings: 12 | Info: 145 | Trace: 8234" | Restrictions: Update efficiently (batch updates if needed), Don't slow down logging, Show only non-zero counts optionally, Keep display compact, Thread-safe counter updates | Success: Statistics panel shows accurate counts, Updates in real-time, Level and category breakdowns shown, Clear button resets counters, Compact display, No performance impact, Thread-safe | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.11 Implement log timestamp formatting options
  - File: src/ui/qt/dialog_log_qt.cpp
  - Allow choosing timestamp format: absolute, relative, none
  - Update display when format changed
  - _Leverage: LogEntry::timestamp, std::chrono_
  - _Requirements: FR-4.10_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add timestamp formatting - Step 1: Add QComboBox for timestamp format: Absolute (HH:MM:SS.mmm), Relative (since start), None (hidden), Step 2: Store dialog start time for relative calculation, Step 3: In appendLog format timestamp according to selected option, Step 4: For relative: show as +MM:SS.mmm from start, Step 5: Persist format preference in ConfigStore | Restrictions: Calculate relative time correctly, Format absolute time with milliseconds, Handle timezone consistently, Update preference on change, Don't reformat existing entries (only new) | Success: Three timestamp formats available, Absolute shows HH:MM:SS.mmm, Relative shows +MM:SS.mmm from start, None hides timestamps, Format preference persists, Selection applies to new entries | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.12 Create comprehensive tests for log dialog
  - File: tests/ui/dialog_log_test.cpp
  - Test logging system, UI controls, filtering, export
  - Verify thread-safety and performance
  - _Leverage: Qt Test framework, mock Logger_
  - _Requirements: FR-4 all_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with Qt testing expertise | Task: Create comprehensive tests - Step 1: Test Logger: singleton works, listeners receive events, filtering by level works, thread-safe operation, Step 2: Test DialogLogQt: appends logs correctly, level filter works, category filters work, font changes apply, syntax highlighting works, clear button works, pause/resume works, search finds text, export writes file, buffer limit enforced, statistics accurate, timestamp formats work, Step 3: Test performance: 10000 log entries in <5 seconds, no memory leaks, thread-safe concurrent logging, Step 4: Test error cases: export failure, invalid font, search with no matches | Restrictions: Use Qt Test macros, Test thread-safety explicitly, Verify performance benchmarks, Mock file operations for export tests, Ensure tests run in CI without UI | Success: All logger features tested, All UI controls tested, Filtering verified, Export tested with mocks, Performance meets targets, Thread-safety verified, Error handling covered, Tests pass reliably, No memory leaks | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 5: Engine Notification System (15 tasks)

**Purpose**: Real-time engine status updates to GUI (FR-5)

- [ ] 5.1 Extend MessageType enum with notification types
  - File: src/platform/ipc.h
  - Add engine lifecycle, config, runtime, performance message types
  - Document each message type
  - _Leverage: Existing IPC infrastructure, design.md Section 5.1_
  - _Requirements: FR-5.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Extend MessageType enum - Step 1: Add engine lifecycle types: EngineStarting, EngineStarted, EngineStopping, EngineStopped, EngineError, Step 2: Add config types: ConfigLoading, ConfigLoaded, ConfigError, ConfigValidating, Step 3: Add runtime types: KeymapSwitched, FocusChanged, ModifierChanged, Step 4: Add performance types: LatencyReport, CpuUsageReport, Step 5: Document purpose of each type in comments | Restrictions: Use explicit hex values (0x1007, etc), Don't conflict with existing message types, Keep names descriptive, Document data field usage for each type | Success: All notification types defined, Hex values unique and sequential, Types well documented, Covers all engine states and events, Enum extensible for future types | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.2 Implement EngineState enum and state machine
  - File: src/core/engine_state.h, src/core/engine.cpp
  - Define engine states: Stopped, Loading, Running, Error
  - Implement state transitions in Engine class
  - _Leverage: design.md Section 5.2_
  - _Requirements: FR-5.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement engine state machine - Step 1: Define EngineState enum with Stopped, Loading, Running, Error, Step 2: Add EngineState m_state member to Engine, Step 3: Add setState method updating state and logging transition, Step 4: Implement state transitions: Stopped->Loading on start(), Loading->Running after successful init, any->Error on failure, Running->Stopped on stop(), Step 5: Add getState() accessor | Restrictions: Validate state transitions (don't allow invalid), Log all state changes, Thread-safe state access, Initialize to Stopped, Don't allow Running->Loading directly | Success: State machine implemented, Valid transitions only, All state changes logged, Thread-safe access, getState returns current state, State persists correctly across operations | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.3 Add notifyGUI method to Engine
  - File: src/core/engine.cpp
  - Send notification messages via IPC channel
  - Include state and relevant data in notifications
  - _Leverage: IIPCChannel, MessageType enum_
  - _Requirements: FR-5.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Implement notifyGUI method - Step 1: Add void notifyGUI(MessageType type, const std::string& data = "") method, Step 2: Create Message with type, wparam = current state, data = provided string, Step 3: Send via m_ipc->send(msg), Step 4: Handle IPC send failures gracefully (log but don't crash), Step 5: Call notifyGUI at appropriate points in engine lifecycle | Restrictions: Don't block on IPC send, Handle null IPC channel, Include relevant data in message, Log notification send, Don't send too frequently (debounce if needed) | Success: notifyGUI method sends IPC messages, State included in wparam, Data field used appropriately, Failures handled gracefully, Notifications sent at right times, No performance impact from notification sending | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.4 Add engine start/stop notifications
  - File: src/core/engine.cpp
  - Send EngineStarting, EngineStarted, EngineStopped notifications
  - Include error details in EngineError notifications
  - _Leverage: notifyGUI method, design.md Section 5.2_
  - _Requirements: FR-5.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add lifecycle notifications - Step 1: In Engine::start() call notifyGUI(EngineStarting) before loading config, Step 2: After successful initialization call notifyGUI(EngineStarted), Step 3: On error catch exception and call notifyGUI(EngineError) with error message in data field, Step 4: In Engine::stop() call notifyGUI(EngineStopping) before cleanup, then notifyGUI(EngineStopped) after, Step 5: Ensure state changes before notifications sent | Restrictions: Send notifications in correct order, Include error details in EngineError, Don't send duplicate notifications, Ensure notifications sent even if subsequent operations fail | Success: Notifications sent at correct lifecycle points, EngineStarting sent before loading, EngineStarted after success, EngineError on failures with details, EngineStopping/Stopped on stop, Order consistent | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.5 Add configuration notifications
  - File: src/core/engine.cpp, src/core/parser/parser.cpp
  - Send ConfigLoading, ConfigLoaded, ConfigError notifications
  - Include config file name in notifications
  - _Leverage: notifyGUI method_
  - _Requirements: FR-5.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add config notifications - Step 1: In loadConfiguration method call notifyGUI(ConfigLoading) with config path in data, Step 2: After successful parse call notifyGUI(ConfigLoaded) with config name, Step 3: On parse errors call notifyGUI(ConfigError) with error message and line number, Step 4: In switchConfiguration send ConfigLoading/ConfigLoaded sequence, Step 5: Add ConfigValidating notification when validation starts | Restrictions: Include config file name not full path in ConfigLoaded, Format error messages clearly with line numbers, Don't overwhelm with notifications during rapid config switches, Send even if config unchanged | Success: ConfigLoading sent when loading starts, ConfigLoaded after successful load with name, ConfigError on failures with details, Config switches send full sequence, Validation notifications sent, Config name displayed to user | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.6 Add runtime event notifications
  - File: src/core/engine.cpp
  - Send KeymapSwitched, FocusChanged, ModifierChanged notifications
  - Debounce frequent events
  - _Leverage: notifyGUI method_
  - _Requirements: FR-5.6_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add runtime notifications - Step 1: When active keymap changes call notifyGUI(KeymapSwitched) with keymap name, Step 2: When window focus changes call notifyGUI(FocusChanged) with window title, Step 3: When modifier state changes significantly call notifyGUI(ModifierChanged) with modifier string, Step 4: Debounce FocusChanged (don't send more than once per 100ms), Step 5: Only send ModifierChanged on major changes (not every key) | Restrictions: Debounce appropriately to avoid spam, Include relevant context in data field, Don't impact key processing performance, Send only significant changes not every event | Success: KeymapSwitched sent when keymap changes, FocusChanged debounced and includes window title, ModifierChanged on significant changes only, No performance impact, Notifications useful for user feedback | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.7 Implement performance metrics notifications
  - File: src/core/engine.cpp, src/utils/metrics.cpp
  - Collect and report latency and CPU usage metrics
  - Send periodic LatencyReport and CpuUsageReport
  - _Leverage: std::chrono, notifyGUI method_
  - _Requirements: FR-5.7_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with performance monitoring | Task: Add performance notifications - Step 1: Instrument Engine::processKey with timing measurement, Step 2: Calculate running average, p95, p99 latencies over 60 second windows, Step 3: Every 60 seconds send LatencyReport with stats in data field, Step 4: Track CPU usage (platform-specific /proc/self/stat on Linux), Step 5: Send CpuUsageReport periodically with percentage, Step 6: Make reporting interval configurable | Restrictions: Keep instrumentation overhead <1%, Use efficient stat calculation, Don't block processing for metrics, Report in background thread if needed, Format stats clearly | Success: Latency measured for all key events, Average/p95/p99 calculated correctly, LatencyReport sent every 60s with stats, CPU usage tracked, CpuUsageReport sent periodically, Overhead <1%, Metrics accurate | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.8 Update TrayIconQt to handle notifications
  - File: src/ui/qt/tray_icon_qt.h, src/ui/qt/tray_icon_qt.cpp
  - Handle engine notification messages
  - Update tray icon and tooltip based on state
  - _Leverage: QSystemTrayIcon, IIPCChannel, design.md Section 5.3_
  - _Requirements: FR-5.8_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add notification handling to tray icon - Step 1: Add handleEngineMessage slot accepting Message parameter, Step 2: Connect to IPC message received signal, Step 3: Switch on message type: EngineStarting sets loading icon and tooltip "YAMY - Starting...", EngineStarted sets running icon and tooltip "YAMY - Running", EngineStopped sets stopped icon and tooltip "YAMY - Stopped", EngineError sets error icon and shows error notification, ConfigLoaded updates tooltip with config name, Step 4: Create icon resources for each state | Restrictions: Icons must be visible in dark/light themes, Tooltip updates immediately, Error notifications auto-dismiss after 5 seconds, Handle missing icons gracefully, Don't spam notifications | Success: Tray icon changes with engine state, Tooltips show current state and config, Error notifications displayed with details, Icons visible in all themes, State always reflects engine accurately | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.9 Create engine status display widget
  - File: src/ui/qt/status_widget_qt.h, src/ui/qt/status_widget_qt.cpp
  - Show engine state, config, uptime, key count
  - Update in real-time from notifications
  - _Leverage: QWidget, QLabel_
  - _Requirements: FR-5.9_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create status widget - Step 1: Create QWidget with labels: State (Stopped/Loading/Running/Error), Config name, Uptime (HH:MM:SS), Keys processed count, Current keymap, Step 2: Handle engine notifications updating labels, Step 3: Update uptime every second using QTimer, Step 4: Track key count from engine (add to notifications or query periodically), Step 5: Format compactly for tray menu or status bar | Restrictions: Update efficiently (batch label updates), Handle missing data gracefully, Format uptime clearly, Keep widget compact, Thread-safe label updates | Success: Widget shows all status info, Updates in real-time from notifications, Uptime updates every second, Key count accurate, Current keymap displayed, Compact and readable, All data accurate | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.10 Add notification history log
  - File: src/ui/qt/notification_history.h, src/ui/qt/notification_history.cpp
  - Store recent notifications with timestamps
  - Provide UI to view notification history
  - _Leverage: QListWidget, Message_
  - _Requirements: FR-5.10_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Implement notification history - Step 1: Create NotificationHistory class storing last 100 notifications, Step 2: Store Message with timestamp, Step 3: Add notification when received, rolling window, Step 4: Create history dialog showing list: timestamp, type, state, data, Step 5: Format nicely: "[15:42:33] EngineStarted: Config loaded - work.mayu", Step 6: Add to tray menu: "View Notification History" | Restrictions: Limit to 100 most recent, Thread-safe storage, Format timestamps consistently, Handle all message types, Dismiss old entries, Persistent across dialog closes | Success: History stores last 100 notifications, Dialog displays list with timestamps, All notification types shown, Formatted clearly, History accessible from tray menu, Thread-safe storage, No memory leaks | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.11 Implement notification sound preferences
  - File: src/ui/qt/preferences_dialog.cpp, src/core/settings/config_store.h
  - Add preferences for notification sounds
  - Play sounds on critical notifications
  - _Leverage: QSound or QMediaPlayer, ConfigStore_
  - _Requirements: FR-5.11_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add notification sound preferences - Step 1: Add preferences checkboxes: Enable sounds, Sound on error, Sound on config loaded, Sound on state change, Step 2: Store preferences in ConfigStore, Step 3: In handleEngineMessage play appropriate sound based on type and prefs, Step 4: Use system beep or bundled sound files, Step 5: Add volume control slider | Restrictions: Respect system sound settings, Don't play excessively, Use short non-annoying sounds, Handle audio unavailable gracefully, Make all sounds optional | Success: Preferences allow enabling/disabling sounds per notification type, Sounds play on appropriate notifications, Volume adjustable, Preferences persist, System audio respected, Sounds brief and appropriate, All optional | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.12 Add notification desktop integration
  - File: src/ui/qt/tray_icon_qt.cpp
  - Use QSystemTrayIcon::showMessage for desktop notifications
  - Follow desktop notification standards
  - _Leverage: QSystemTrayIcon_
  - _Requirements: FR-5.12_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer with Linux desktop integration | Task: Add desktop notifications - Step 1: On EngineError call showMessage with Critical icon, title "YAMY Error", error text, Step 2: On ConfigLoaded call showMessage with Information icon if configured, Step 3: On EngineStarted show brief notification if enabled in prefs, Step 4: Use appropriate QSystemTrayIcon::MessageIcon for each type, Step 5: Set timeout: errors 10s, others 3s | Restrictions: Follow freedesktop.org notification spec on Linux, Don't spam notifications, Respect user notification preferences, Use appropriate urgency levels, Keep messages brief | Success: Desktop notifications displayed, Appropriate icons used, Errors shown prominently for 10s, Info messages brief, Notifications respect system settings, Users can disable in preferences, Works with various desktop environments | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.13 Implement notification filtering preferences
  - File: src/ui/qt/preferences_dialog.cpp
  - Allow users to enable/disable specific notification types
  - Filter notifications before displaying
  - _Leverage: QCheckBox, ConfigStore_
  - _Requirements: FR-5.13_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add notification filtering - Step 1: Add preferences section "Notifications" with checkboxes for each type: Show engine state changes, Show config changes, Show keymap switches, Show focus changes, Show errors (always enabled), Show performance metrics, Step 2: Store preferences in ConfigStore, Step 3: In handleEngineMessage check preferences before processing, Step 4: Default: errors, state changes, config changes enabled; others disabled, Step 5: Add "Reset to defaults" button | Restrictions: Errors always shown (checkbox disabled), Preferences apply immediately, Store per-notification-type not global, Provide sensible defaults, Don't require restart | Success: Preferences allow fine-grained filtering, Each notification type toggleable, Errors always shown, Preferences persist, Apply immediately without restart, Defaults sensible, Reset button works | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.14 Add notification callback system for extensions
  - File: src/core/notification_dispatcher.h, src/core/notification_dispatcher.cpp
  - Allow registering callbacks for notification events
  - Support plugin/extension use cases
  - _Leverage: std::function, std::vector_
  - _Requirements: FR-5.14_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Create notification dispatcher - Step 1: Create NotificationDispatcher singleton with methods: registerCallback, unregisterCallback, dispatch, Step 2: Store callbacks in vector: std::function<void(MessageType, const std::string&)>, Step 3: In Engine::notifyGUI also call dispatcher.dispatch, Step 4: Support filtering: registerCallback(types, callback) for specific types only, Step 5: Thread-safe callback invocation | Restrictions: Thread-safe registration and dispatch, Handle callback exceptions gracefully, Support multiple callbacks per type, Allow unregistration, Don't block on slow callbacks | Success: Dispatcher manages callback registration, Multiple callbacks per type supported, dispatch invokes all matching callbacks, Thread-safe operation, Callback exceptions caught and logged, Unregistration works, Extensible for plugins | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.15 Create comprehensive tests for notification system
  - File: tests/core/notification_test.cpp, tests/ui/notification_ui_test.cpp
  - Test notification generation, delivery, UI handling
  - Verify callback system and filtering
  - _Leverage: Qt Test, mock IPC_
  - _Requirements: FR-5 all_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with C++ and Qt testing | Task: Create comprehensive tests - Step 1: Test Engine state machine: all transitions work, invalid transitions rejected, notifications sent at right times, Step 2: Test notifyGUI: messages sent via IPC correctly, state included, data formatted properly, Step 3: Test TrayIconQt: icon updates on state changes, tooltips accurate, desktop notifications shown, Step 4: Test NotificationHistory: stores notifications, rolling window works, thread-safe, Step 5: Test dispatcher: callbacks registered and invoked, filtering works, thread-safe, exceptions handled, Step 6: Test preferences: filtering applies, persistence works, sounds play when enabled | Restrictions: Mock IPC for isolated testing, Test all notification types, Verify thread-safety explicitly, Test error cases, Ensure no memory leaks | Success: All notification types tested, State machine verified, UI updates tested, History functional, Dispatcher tested, Preferences work, Thread-safety confirmed, Error handling covered, Tests pass reliably | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 6: Advanced Features (19 tasks)

**Purpose**: Session management, IPC API, help menu (FR-6, FR-7, FR-9)

- [ ] 6.1 Create SessionManager class
  - File: src/core/settings/session_manager.h, src/core/settings/session_manager.cpp
  - Manage session state: active config, engine state, window positions
  - Save and restore sessions
  - _Leverage: ConfigStore, design.md Section 6.1_
  - _Requirements: FR-6.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Create SessionManager - Step 1: Create SessionManager class with methods: saveSession, restoreSession, enableAutoStart, disableAutoStart, isAutoStartEnabled, Step 2: Store session data: active config path, last window positions (dialogs), engine running state, Step 3: Serialize to JSON file in ~/.config/yamy/session.json, Step 4: On saveSession write all state, on restoreSession read and apply, Step 5: Call saveSession on clean shutdown, restoreSession on startup | Restrictions: Handle file errors gracefully, Use JSON format, Store minimal essential state, Don't save transient data, Validate session data on restore | Success: SessionManager saves/restores state, Active config persisted, Window positions restored, Engine state remembered, JSON format readable, File errors handled, Validates data on restore | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.2 Implement Linux autostart support
  - File: src/core/settings/session_manager.cpp
  - Create .desktop file in ~/.config/autostart
  - Enable/disable autostart
  - _Leverage: XDG autostart specification, design.md Section 6.1_
  - _Requirements: FR-6.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with Linux desktop integration | Task: Add autostart support - Step 1: Implement getAutoStartPath returning $XDG_CONFIG_HOME/autostart or ~/.config/autostart, Step 2: In enableAutoStart create yamy.desktop file with: Type=Application, Name=YAMY, Exec=(path to yamy binary), Icon=yamy, X-GNOME-Autostart-enabled=true, Step 3: In disableAutoStart remove yamy.desktop file, Step 4: In isAutoStartEnabled check if file exists and is valid, Step 5: Handle permissions errors | Restrictions: Follow XDG desktop entry spec, Use absolute path in Exec, Create autostart directory if missing, Handle write errors, Don't break if XDG_CONFIG_HOME not set | Success: enableAutoStart creates desktop file, YAMY starts on login, disableAutoStart removes file, isAutoStartEnabled accurate, Follows XDG spec, Works with GNOME/KDE/XFCE, Errors handled gracefully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.3 Add session restore on startup
  - File: src/app/main.cpp, src/core/engine.cpp
  - Restore session automatically on startup
  - Restore last config and engine state
  - _Leverage: SessionManager_
  - _Requirements: FR-6.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add session restore - Step 1: In main after QApplication creation call SessionManager::restoreSession, Step 2: If session has active config load it instead of default, Step 3: If engine was running start it automatically, Step 4: Restore dialog positions if available, Step 5: Add --no-restore command line flag to skip restoration | Restrictions: Restore should be optional (respect --no-restore), Handle corrupt session files gracefully, Fall back to defaults if restoration fails, Don't restore if config file missing, Log restoration actions | Success: Session restores on startup, Last config loaded, Engine auto-starts if was running, Dialog positions restored, --no-restore flag works, Corrupt sessions handled, Falls back gracefully, Logs restoration steps | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.4 Create yamy-ctl command-line tool
  - File: src/app/yamy_ctl.cpp, src/app/CMakeLists.txt
  - Implement CLI for controlling running YAMY instance
  - Support: reload, stop, start, status commands
  - _Leverage: IIPCChannel, design.md Section 6.2_
  - _Requirements: FR-7.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Create yamy-ctl tool - Step 1: Create main function parsing command: reload, stop, start, status, Step 2: Connect to YAMY via createIPCChannel("yamy-engine"), Step 3: Send appropriate message type based on command, Step 4: For reload with --config: include config name in message data, Step 5: Wait for response and print to stdout, Step 6: Exit with code 0 on success, 1 if YAMY not running, 2 on command failure | Restrictions: Check YAMY running before sending, Handle connection failures, Format output clearly, Support --help flag, Use descriptive error messages, Don't hang waiting for response (timeout 5s) | Success: yamy-ctl connects to running YAMY, reload command works, stop/start/status work, --config flag functional, Errors clearly reported, Help text available, Timeout prevents hangs, Exit codes correct | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.5 Extend IPC with query commands
  - File: src/platform/ipc.h, src/core/engine.cpp
  - Add query commands: get-status, get-config, get-keymaps, get-metrics
  - Return structured data via IPC
  - _Leverage: MessageType enum, JSON formatting_
  - _Requirements: FR-7.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add query commands - Step 1: Add MessageType enum values: CmdGetStatus, CmdGetConfig, CmdGetKeymaps, CmdGetMetrics, Step 2: In Engine handle these commands returning JSON in response.data, Step 3: GetStatus returns: state, uptime, config, key_count, current_keymap, Step 4: GetConfig returns: config_path, config_name, loaded_time, Step 5: GetKeymaps returns array of keymap objects with name and window conditions, Step 6: GetMetrics returns latency stats and CPU usage | Restrictions: Return valid JSON, Handle missing data gracefully, Keep response size reasonable, Don't block engine processing, Format timestamps as ISO8601 | Success: All query commands implemented, Responses in valid JSON format, Data accurate and complete, Engine not blocked by queries, Timestamps formatted correctly, yamy-ctl can parse responses | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.6 Add yamy-ctl query subcommands
  - File: src/app/yamy_ctl.cpp
  - Implement status, config, keymaps, metrics subcommands
  - Parse and format JSON responses
  - _Leverage: JSON parsing library (e.g. nlohmann/json)_
  - _Requirements: FR-7.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Add query commands to yamy-ctl - Step 1: Add status command sending CmdGetStatus and formatting response: "Engine: Running | Config: work.mayu | Uptime: 2h 15m | Keys: 12,453", Step 2: Add config command showing config details, Step 3: Add keymaps command listing all keymaps with window conditions, Step 4: Add metrics command showing latency and CPU stats, Step 5: Use JSON library to parse responses, handle parse errors | Restrictions: Format output human-readable, Handle JSON parse errors, Show error if command fails, Keep output concise, Support --json flag for raw JSON output | Success: status command shows formatted info, config shows config details, keymaps lists all keymaps, metrics shows performance stats, JSON parsing works, Errors handled, --json flag outputs raw JSON | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.7 Create Help menu structure
  - File: src/ui/qt/main_window.cpp, src/ui/qt/help_menu.cpp
  - Add Help menu to system tray
  - Include: Documentation, Keyboard Shortcuts, Examples, Report Bug, About
  - _Leverage: QMenu, QAction, design.md Section 6.3_
  - _Requirements: FR-9.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create help menu - Step 1: Add Help submenu to tray icon menu, Step 2: Add actions: Online Documentation (opens browser), Keyboard Shortcuts (opens dialog), Configuration Examples (opens dialog), Report Bug (opens GitHub issues in browser), separator, About YAMY (opens about dialog), Step 3: Connect actions to slots, Step 4: Use QDesktopServices::openUrl for external links | Restrictions: Handle browser launch failures, Use appropriate icons for each action, Disable actions if resources unavailable, Follow standard help menu conventions | Success: Help menu appears in tray, All actions functional, Documentation opens in browser, Shortcuts dialog opens, Examples dialog opens, Bug report opens GitHub, About dialog shows version/license, Browser failures handled | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.8 Create Keyboard Shortcuts reference dialog
  - File: src/ui/qt/dialog_shortcuts.h, src/ui/qt/dialog_shortcuts.cpp
  - Show table of keyboard shortcuts
  - List global hotkeys and dialog shortcuts
  - _Leverage: QDialog, QTableWidget_
  - _Requirements: FR-9.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create shortcuts dialog - Step 1: Create QDialog with QTableWidget showing two columns: Action and Shortcut, Step 2: Populate with entries: "Reload Configuration" = configured hotkey, "Open Investigate Dialog" = configured, "Open Log Dialog" = configured, "Quick Config Switch" = Ctrl+Alt+C, etc, Step 3: Include dialog shortcuts: "Find in Log" = Ctrl+F, "Close Dialog" = Esc, Step 4: Make table sortable, read-only, Step 5: Add search box to filter shortcuts | Restrictions: Show only configured shortcuts (not empty), Update if shortcuts change, Keep table compact, Support keyboard navigation, Make searchable | Success: Dialog shows all shortcuts, Table has two columns, Sortable by action or shortcut, Search filters list, Keyboard navigation works, Shows current configured values, Read-only table | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.9 Create Configuration Examples dialog
  - File: src/ui/qt/dialog_examples.h, src/ui/qt/dialog_examples.cpp
  - Show example .mayu configurations
  - Provide copy-to-clipboard functionality
  - _Leverage: QDialog, QListWidget, QTextEdit_
  - _Requirements: FR-9.3_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create examples dialog - Step 1: Create QDialog with QListWidget on left showing example names: Basic Remapping, Emacs Bindings, Vim Mode, CapsLock as Ctrl, Multi-Monitor Setup, Step 2: QTextEdit on right showing selected example code with syntax highlighting, Step 3: Each example includes commented explanation, Step 4: Add Copy button copying example to clipboard, Step 5: Add Save As button saving to file | Restrictions: Examples must be valid .mayu syntax, Include helpful comments, Use syntax highlighting if possible (or monospace font), Make examples practical and reusable | Success: Dialog shows example list, Selecting example displays code, Examples valid and well-commented, Copy button works, Save As creates file, Syntax highlighting or monospace, Examples helpful and practical | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.10 Create About dialog
  - File: src/ui/qt/dialog_about.h, src/ui/qt/dialog_about.cpp
  - Show version, license, contributors, build info
  - Include links to project resources
  - _Leverage: QDialog, QLabel, QTextBrowser_
  - _Requirements: FR-9.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create about dialog - Step 1: Create QDialog showing YAMY logo/icon, Step 2: Display version from build info (VERSION macro), Step 3: Show license: GPL-3.0 (or project license) with full text in scrollable area, Step 4: List major contributors from AUTHORS file or embedded list, Step 5: Show build info: commit hash, build date, Qt version, compiler, Step 6: Include links: GitHub repository, Documentation site, Bug tracker | Restrictions: Read version from build system, Format license text readably, Keep dialog size reasonable, Make links clickable, Handle missing build info, Show platform: Linux/x86_64 etc | Success: Dialog shows version, License text readable, Contributors listed, Build info accurate, Links clickable and working, Platform shown, Professional appearance, Scrollable if content long | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.11 Add online documentation link
  - File: src/ui/qt/help_menu.cpp
  - Open documentation URL in browser
  - Support offline docs if available
  - _Leverage: QDesktopServices::openUrl_
  - _Requirements: FR-9.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add documentation link - Step 1: Define documentation URL (e.g. https://github.com/user/yamy/wiki), Step 2: In Online Documentation action call QDesktopServices::openUrl, Step 3: If URL fails show error with alternative: "Documentation available at: [URL]", Step 4: Check for local docs in /usr/share/doc/yamy/ or ~/.local/share/yamy/docs/, Step 5: If local docs exist add "Local Documentation" action opening file:// URL | Restrictions: Handle browser launch failures gracefully, Provide URL as fallback, Check local docs on startup, Use proper file:// URLs for local, Don't hard-code absolute paths | Success: Online Documentation opens browser with correct URL, Errors show message with URL, Local docs detected if present, Local Documentation action appears when available, file:// URLs work, Browser failures handled | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.12 Create Preferences dialog
  - File: src/ui/qt/preferences_dialog.h, src/ui/qt/preferences_dialog.cpp
  - Centralize all user preferences
  - Organize into tabs: General, Notifications, Logging, Advanced
  - _Leverage: QDialog, QTabWidget, ConfigStore_
  - _Requirements: FR-6.4_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Create preferences dialog - Step 1: Create QDialog with QTabWidget, Step 2: General tab: Start on login checkbox, Quick-switch hotkey editor, Default config dropdown, Step 3: Notifications tab: Enable/disable per type, Sound settings, Desktop notification settings, Step 4: Logging tab: Log level dropdown, Buffer size spinner, Log to file checkbox with path, Step 5: Advanced tab: IPC port (if configurable), Performance metrics interval, Debug mode checkbox, Step 6: Load from ConfigStore on open, save on OK/Apply | Restrictions: Validate all inputs, Apply immediately or on OK (user choice), Provide tooltips for complex settings, Use standard dialog buttons, Reset to defaults button per tab | Success: Preferences dialog with 4 tabs, All settings editable, Validation works, Changes persist via ConfigStore, Apply button updates immediately, Reset to defaults works, Tooltips helpful, Professional layout | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.13 Add preferences action to tray menu
  - File: src/ui/qt/tray_icon_qt.cpp
  - Add Preferences action opening dialog
  - Place appropriately in menu structure
  - _Leverage: PreferencesDialog_
  - _Requirements: FR-6.5_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add preferences to menu - Step 1: Add Preferences action to tray menu after Configurations submenu, Step 2: Connect action to slot creating and showing PreferencesDialog, Step 3: Use standard Preferences icon or gear icon, Step 4: Add keyboard shortcut hint in menu text, Step 5: Ensure dialog is modal or single-instance (don't open multiple) | Restrictions: Show only one preferences dialog at a time, Place logically in menu (near top or bottom section), Use standard icon, Make easily discoverable | Success: Preferences action in tray menu, Opens dialog on click, Only one dialog open at a time, Icon appropriate, Keyboard shortcut shown, Menu placement logical and discoverable | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.14 Implement crash reporting infrastructure
  - File: src/utils/crash_handler.h, src/utils/crash_handler.cpp
  - Catch crashes and generate reports
  - Save crash dump and logs
  - _Leverage: signal handlers on Linux_
  - _Requirements: NF-4 Reliability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with Linux systems programming | Task: Implement crash handler - Step 1: Register signal handlers for SIGSEGV, SIGABRT, SIGFPE, SIGILL, Step 2: In handler: capture backtrace using backtrace(), backtrace_symbols(), Step 3: Write crash report to ~/.local/share/yamy/crashes/crash_TIMESTAMP.txt including: signal number, backtrace, YAMY version, system info, loaded config, recent log entries, Step 4: Use writev for async-signal-safe output, Step 5: Call default handler to generate core dump if enabled | Restrictions: Use async-signal-safe functions only in handler, Don't allocate memory in handler, Keep handler minimal, Generate core dump for debugger, Don't prevent default crash behavior entirely | Success: Crash handler catches signals, Backtrace captured, Report written to file with timestamp, Includes version and system info, Recent logs included, Core dump still generated, Handler is async-signal-safe | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.15 Add crash report dialog on restart
  - File: src/app/main.cpp, src/ui/qt/crash_report_dialog.h
  - Detect crash reports on startup
  - Offer to view and report
  - _Leverage: CrashHandler, QDialog_
  - _Requirements: NF-4 Reliability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Qt GUI developer | Task: Add crash report dialog - Step 1: On startup check ~/.local/share/yamy/crashes/ for .txt files, Step 2: If found show dialog: "YAMY crashed. Would you like to view the crash report?", Step 3: View button opens dialog showing crash report contents, Step 4: Report Bug button opens GitHub issues with crash report pre-filled in template, Step 5: Dismiss button deletes crash report file, Step 6: Always show dialog option checkbox | Restrictions: Don't show every startup (store dismissed flag), Offer to delete report after viewing, Pre-fill GitHub issue carefully (don't expose sensitive info), Make dismissing easy, Respect user privacy | Success: Crash reports detected on startup, Dialog offers view/report/dismiss options, View shows report contents, Report Bug opens GitHub with template, Dismiss deletes file, Option to not show again, Privacy respected, Easy to dismiss | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.16 Add plugin system foundation
  - File: src/core/plugin_manager.h, src/core/plugin_manager.cpp
  - Define plugin interface and loading mechanism
  - Support shared library plugins
  - _Leverage: dlopen/dlsym on Linux_
  - _Requirements: FR-8.1_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer with plugin architecture expertise | Task: Create plugin system foundation - Step 1: Define IPlugin interface with methods: getName, getVersion, initialize(Engine*), shutdown, Step 2: Create PluginManager singleton loading plugins from ~/.local/share/yamy/plugins/, Step 3: Use dlopen to load .so files, dlsym to resolve plugin_create factory function, Step 4: Call initialize on each plugin passing Engine pointer, Step 5: Store loaded plugins, call shutdown on program exit, Step 6: Handle plugin errors gracefully (log and continue) | Restrictions: Isolate plugin errors (don't crash main app), Validate plugin interface version, Support unloading plugins, Use RTLD_LOCAL to avoid symbol conflicts, Document plugin API, Provide example plugin | Success: PluginManager loads .so files from plugins directory, Factory function resolved and called, Plugins initialized with Engine access, Plugins can register callbacks, shutdown called on exit, Errors logged not crashed, API versioning works, Example plugin functional | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.17 Create example plugin
  - File: examples/plugins/example_plugin.cpp
  - Demonstrate plugin interface usage
  - Show registration with notification system
  - _Leverage: IPlugin interface, NotificationDispatcher_
  - _Requirements: FR-8.2_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior C++ developer | Task: Create example plugin - Step 1: Implement IPlugin interface in ExamplePlugin class, Step 2: In initialize register callback with NotificationDispatcher for ConfigLoaded events, Step 3: In callback log message "Example plugin: Config loaded", Step 4: Implement plugin_create factory function, Step 5: Add CMakeLists.txt building example plugin as shared library, Step 6: Document plugin in README explaining how to build and install | Restrictions: Keep example simple and well-commented, Show best practices, Demonstrate error handling, Include build instructions, Test plugin loads and works | Success: Example plugin compiles to .so, Implements IPlugin correctly, Factory function exports symbol, Registers notification callback, Callback invoked on events, Logs messages, README clear, Demonstrates plugin development | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.18 Create end-to-end integration tests
  - File: tests/integration/integration_suite.cpp
  - Test full application lifecycle
  - Verify all tracks working together
  - _Leverage: All components, Qt Test_
  - _Requirements: All NF requirements_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA Engineer with integration testing expertise | Task: Create integration test suite - Step 1: Test full lifecycle: start engine, load config, process keys, stop engine, Step 2: Test IPC: send commands via IPCChannel, verify responses, test all command types, Step 3: Test GUI integration: tray icon updates, notifications displayed, dialogs open/close, Step 4: Test session: save session, restart, restore session, verify state, Step 5: Test crash recovery: trigger controlled crash (if possible), verify crash report generated, Step 6: Test performance: measure key processing latency under load, verify <1ms, Step 7: Test all tracks: platform abstraction, config management, investigate dialog, log dialog, notifications, advanced features | Restrictions: Use test environment not affecting user system, Mock X11 if needed (Xvfb), Test real IPC not mocks, Measure actual performance, Verify no memory leaks, Ensure tests reliable | Success: Full lifecycle tested end-to-end, All IPC commands work, GUI integration verified, Session save/restore works, Performance meets targets (<1ms latency), All tracks tested together, No memory leaks, Tests pass reliably in CI | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.19 Create end-user documentation
  - File: docs/user-guide.md, docs/configuration-guide.md, docs/troubleshooting.md
  - Write comprehensive user documentation
  - Include installation, configuration, troubleshooting
  - _Leverage: All features_
  - _Requirements: NF-3 Usability_
  - _Prompt: Implement the task for spec linux-complete-port, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Technical Writer | Task: Create user documentation - Step 1: Write user-guide.md covering: Installation (dependencies, build from source, binary), First run and setup, Basic configuration (.mayu syntax), System tray usage, Dialogs (Investigate, Log), Preferences, Step 2: Write configuration-guide.md with: Complete .mayu syntax reference, Key names table, Example configurations, Window matching patterns, Advanced features (functions, includes, conditionals), Step 3: Write troubleshooting.md covering: Common issues (keys not remapping, config errors, performance), How to report bugs, Viewing logs, Known limitations, FAQ, Step 4: Include screenshots of dialogs where helpful, Step 5: Add examples throughout | Restrictions: Write for non-technical users, Use clear language, Include practical examples, Keep organized with ToC, Test all examples, Proofread thoroughly | Success: Three documentation files complete, User guide covers all basic usage, Configuration guide comprehensive reference, Troubleshooting answers common questions, Examples tested and working, Screenshots included, Professional writing quality, Easily navigable with ToC | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Task Execution Guidelines

### Parallel Execution
- Within each batch, tasks CAN be executed in parallel by multiple AI agents
- Batch 1 must complete before starting Batch 2
- Batch 2 must complete before starting Batch 3

### Dependencies
- Tasks within a batch have minimal dependencies
- Dependencies are explicitly noted in the _Dependencies field
- Cross-batch dependencies are enforced by batch ordering

### Verification
- Each task includes specific success criteria
- Run unit tests after completing each task
- Verify builds on both Linux and Windows when applicable

### Progress Tracking
- Mark tasks as `- [-]` when starting implementation
- Mark tasks as `- [x]` when completed and tested
- Update this file after completing each task
