# Tasks Document: Investigate Window Feature

## Task Breakdown

### Phase 1: IPC Infrastructure (Critical Path)

- [x] 1.1 Create IPCChannelQt implementation for QLocalSocket communication
  - Files:
    - src/core/platform/linux/ipc_channel_qt.h (new)
    - src/core/platform/linux/ipc_channel_qt.cpp (new)
  - Description:
    - Implement `IIPCChannel` interface using `QLocalSocket` (client) and `QLocalServer` (server)
    - Add message framing (4-byte length prefix + data) to prevent partial reads
    - Implement serialization/deserialization using `QDataStream`
    - Emit `messageReceived()` signal when complete messages arrive
    - Handle connection states (disconnected, connecting, connected)
  - _Leverage:
    - src/core/platform/ipc_channel_interface.h
    - Qt5 QLocalSocket, QLocalServer, QDataStream
  - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5
  - _Prompt:
    ```
    Role: Qt/IPC Developer specializing in inter-process communication and Qt network programming

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Create a Qt-based IPC channel implementation (`IPCChannelQt`) that enables communication between the YAMY Qt GUI and the core engine process using Unix domain sockets.

    Requirements:
    - Implement `IIPCChannel` interface from src/core/platform/ipc_channel_interface.h
    - Use `QLocalSocket` for client mode (dialog connecting to engine)
    - Use `QLocalServer` for server mode (engine listening for connections)
    - Socket path: /tmp/yamy-{name}-{UID} (user-specific to avoid collisions)
    - Message framing: 4-byte length prefix (big-endian) + serialized data
    - Serialization: Use QDataStream to write/read ipc::Message structs
    - Error handling: Emit disconnected() signal on socket errors, reconnect logic in client
    - Thread safety: All operations should be thread-safe via Qt's queued connections

    Implementation Details:
    1. Header file (ipc_channel_qt.h):
       - Inherit from IIPCChannel (Qt QObject base)
       - Add private slots: onReadyRead(), onConnected(), onDisconnected()
       - Add private members: QLocalSocket* m_clientSocket, QLocalServer* m_server, QByteArray m_receiveBuffer
    2. Implementation file (ipc_channel_qt.cpp):
       - connect(): Create QLocalSocket, call connectToServer(socketPath)
       - listen(): Create QLocalServer, call QLocalServer::listen(socketPath)
       - send(): Serialize message to QByteArray (4-byte length + data), write to socket
       - onReadyRead(): Read from socket into buffer, parse length prefix, deserialize messages, emit messageReceived()
       - Handle partial reads (buffer accumulation until full message available)

    Restrictions:
    - Do not use blocking socket operations (no waitForReadyRead, waitForConnected)
    - Do not bypass Qt event loop (all I/O must be asynchronous via signals/slots)
    - Do not hardcode socket paths (use constructor parameter)
    - Do not leak memory (use smart pointers, Qt parent-child ownership)

    Success Criteria:
    - IPCChannelQt compiles without errors
    - Client can connect to server within <100ms
    - Messages sent via send() are received via messageReceived() signal
    - Round-trip latency <5ms (measured with QElapsedTimer)
    - No memory leaks after 1000 connect/disconnect cycles (validated with Valgrind)
    - Unit tests pass (send/receive, connection failure, large messages)

    Instructions:
    1. Before starting, search implementation logs:
       - grep -r "IPC.*Channel" .spec-workflow/specs/*/Implementation\ Logs/
       - grep -r "QLocalSocket" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark task as in progress: Edit tasks.md, change [ ] to [-]
    3. Implement the code following the technical design
    4. Write unit tests in tests/platform/ipc_channel_qt_test.cpp
    5. Run tests and verify all pass
    6. Log implementation using log-implementation tool with detailed artifacts:
       - apiEndpoints: N/A (not HTTP API)
       - components: N/A (backend only)
       - functions: List all public methods (connect, disconnect, listen, send, etc.)
       - classes: IPCChannelQt with full method list
       - integrations: Describe how dialog connects to engine via QLocalSocket
    7. Mark task as complete: Edit tasks.md, change [-] to [x]
    ```

- [ ] 1.2 Update IPC channel factory to return IPCChannelQt on Linux
  - Files:
    - src/core/platform/ipc_channel_factory.h (modify)
  - Description:
    - Replace `IPCChannelNull` with `IPCChannelQt` when `QT_CORE_LIB` is defined
    - Add conditional compilation for Qt dependency
  - _Leverage:
    - Existing factory pattern
    - ipc_channel_qt.h from task 1.1
  - _Requirements: 3.1
  - _Prompt:
    ```
    Role: Build System Engineer specializing in CMake and cross-platform compilation

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Update the IPC channel factory to return a real Qt-based IPC implementation instead of the null stub on Linux.

    Context:
    - Currently, createIPCChannel() always returns IPCChannelNull (stub, no-op)
    - Task 1.1 created IPCChannelQt with real QLocalSocket implementation
    - Need to conditionally compile: Linux+Qt → IPCChannelQt, otherwise → IPCChannelNull

    Implementation:
    1. Open src/core/platform/ipc_channel_factory.h
    2. Add #include "linux/ipc_channel_qt.h" at top (guarded by #if defined(__linux__) && defined(QT_CORE_LIB))
    3. Modify createIPCChannel() function:
       ```cpp
       inline std::unique_ptr<IIPCChannel> createIPCChannel(const std::string& name) {
           #if defined(__linux__) && defined(QT_CORE_LIB)
               return std::make_unique<IPCChannelQt>(name);
           #else
               return std::make_unique<IPCChannelNull>();
           #endif
       }
       ```

    Restrictions:
    - Do not break headless builds (when QT_CORE_LIB not defined)
    - Do not add Windows-specific code here (keep platform-neutral)
    - Ensure factory function remains inline (header-only, no .cpp file)

    Success Criteria:
    - Factory compiles on Linux with Qt (returns IPCChannelQt)
    - Factory compiles on Linux without Qt (returns IPCChannelNull)
    - Factory compiles on Windows (returns IPCChannelNull for now)
    - No linker errors (IPCChannelQt properly linked with Qt5::Network)

    Instructions:
    1. Search implementation logs: grep -r "factory.*IPC" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark task in progress: Edit tasks.md
    3. Modify ipc_channel_factory.h with conditional compilation
    4. Test compilation: mkdir build && cd build && cmake .. && make
    5. Verify with: nm yamy_stub | grep IPCChannelQt (should show symbols)
    6. Log implementation with artifacts (classes: createIPCChannel factory function)
    7. Mark task complete
    ```

- [ ] 1.3 Connect engine IPC messageReceived signal to handleIpcMessage
  - Files:
    - src/core/engine/engine_lifecycle.cpp (modify)
    - src/core/engine/engine.h (modify if needed)
  - Description:
    - In Engine constructor, connect `m_ipcChannel->messageReceived` signal to `Engine::handleIpcMessage` slot
    - Ensure Qt signals/slots work (Engine may need Q_OBJECT macro if not already present)
  - _Leverage:
    - Existing m_ipcChannel member
    - Existing handleIpcMessage() method
    - Qt signal/slot system
  - _Requirements: 3.2
  - _Prompt:
    ```
    Role: Qt Backend Developer with expertise in signal/slot connections and engine integration

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Wire up the engine's IPC channel so that incoming messages trigger the existing handleIpcMessage() handler.

    Context:
    - Engine creates IPC channel in constructor (engine_lifecycle.cpp:79)
    - Engine has handleIpcMessage() method that processes requests (engine.cpp:499)
    - Currently: Channel created but messageReceived signal never connected (messages ignored)
    - Goal: Connect signal so RspInvestigateWindow responses are actually sent

    Implementation:
    1. Open src/core/engine/engine_lifecycle.cpp
    2. Find Engine::Engine() constructor (~line 37)
    3. Locate existing IPC channel creation (~line 79-84):
       ```cpp
       m_ipcChannel = yamy::platform::createIPCChannel("yamy-engine");
       if (m_ipcChannel) {
           m_ipcChannel->listen();
       }
       ```
    4. Add Qt signal/slot connection (only when Qt is available):
       ```cpp
       m_ipcChannel = yamy::platform::createIPCChannel("yamy-engine");
       if (m_ipcChannel) {
           #if defined(QT_CORE_LIB)
           // Connect IPC signal to handler
           QObject::connect(m_ipcChannel.get(), &yamy::platform::IIPCChannel::messageReceived,
                            [this](const yamy::ipc::Message& msg) {
                                this->handleIpcMessage(msg);
                            });
           #endif
           m_ipcChannel->listen();
       }
       ```
    5. Note: Engine class doesn't inherit QObject, so use lambda to capture 'this'

    Restrictions:
    - Do not modify Engine to inherit QObject (breaks existing architecture)
    - Do not connect signal if QT_CORE_LIB not defined (headless builds)
    - Do not block on IPC reads (connection is asynchronous)

    Success Criteria:
    - Code compiles with and without Qt
    - When dialog sends CmdInvestigateWindow, engine responds with RspInvestigateWindow
    - Integration test passes: dialog receives keymap status within 5ms
    - No deadlocks or blocking behavior

    Instructions:
    1. Search logs: grep -r "handleIpcMessage.*connect" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Modify engine_lifecycle.cpp with signal connection
    4. Test: Launch engine + dialog, select window, verify keymap panel updates
    5. Log with artifacts (integrations: "Engine IPC handler receives messages from IPCChannelQt")
    6. Mark complete
    ```

- [ ] 1.4 Uncomment and fix IPC response sending in handleIpcMessage
  - Files:
    - src/core/engine/engine.cpp (modify)
  - Description:
    - Find commented-out `m_ipcChannel->send(responseMessage)` line (~line 533)
    - Uncomment and wrap with null check: `if (m_ipcChannel && m_ipcChannel->isConnected())`
  - _Leverage:
    - Existing handleIpcMessage() implementation
    - Existing queryKeymapForWindow() method
  - _Requirements: 3.2, 4.1, 4.2, 4.3, 4.4
  - _Prompt:
    ```
    Role: Backend Developer with expertise in IPC protocols and engine internals

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Enable the engine to actually send IPC responses back to the dialog when it queries keymap status.

    Context:
    - Engine::handleIpcMessage() processes CmdInvestigateWindow requests (engine.cpp:499-540)
    - Code builds InvestigateWindowResponse struct but never sends it (line 533 commented out)
    - Comment says: "This is a simplification. A real implementation would need a way to send..."
    - Now we have real IPC (IPCChannelQt), so uncomment and fix the send

    Implementation:
    1. Open src/core/engine/engine.cpp
    2. Navigate to handleIpcMessage() method (~line 499)
    3. Find CmdInvestigateWindow case (~line 508-536)
    4. Locate commented line: // m_ipcChannel->send(responseMessage);
    5. Replace with:
       ```cpp
       // Send response back to dialog
       if (m_ipcChannel && m_ipcChannel->isConnected()) {
           m_ipcChannel->send(responseMessage);
       }
       ```
    6. Ensure responseMessage is still in scope (should be, it's right before this line)

    Restrictions:
    - Do not send if channel is null or disconnected (prevents crashes)
    - Do not block waiting for send to complete (IPC is asynchronous)
    - Do not modify queryKeymapForWindow() (already works correctly)

    Success Criteria:
    - Dialog sends CmdInvestigateWindow request → engine responds within 5ms
    - Keymap panel shows correct keymap name, matched regex, modifiers
    - No crashes when dialog disconnects mid-request
    - Integration test passes: round-trip request-response verified

    Instructions:
    1. Search logs: grep -r "RspInvestigateWindow" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Uncomment and fix send() line in engine.cpp
    4. Test: Use dialog to select window, verify panels populate (not "-" or "(IPC not connected)")
    5. Log with artifacts (integrations: "Engine sends RspInvestigateWindow to dialog via IPC")
    6. Mark complete
    ```

### Phase 2: Window System Implementation (X11/XCB)

- [ ] 2.1 Implement X11 connection management in WindowSystemLinux
  - Files:
    - src/platform/linux/window_system_linux.cpp (modify)
  - Description:
    - Open X11 display in constructor: `m_display = XOpenDisplay(NULL)`
    - Store root window: `m_rootWindow = DefaultRootWindow(m_display)`
    - Install X11 error handler to catch BadWindow errors gracefully
    - Close display in destructor
  - _Leverage:
    - Existing WindowSystemLinux skeleton
    - X11 Xlib API
  - _Requirements: 1.1, 1.2, 1.3, 1.4
  - _Prompt:
    ```
    Role: X11/Linux Systems Programmer with expertise in Xlib and window managers

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Set up X11 connection management so WindowSystemLinux can query window properties without crashing on errors.

    Context:
    - WindowSystemLinux is stubbed (all methods return hardcoded values)
    - Need to establish persistent X11 connection for property queries
    - Must handle errors gracefully (windows can close, disappear, etc.)

    Implementation:
    1. Open src/platform/linux/window_system_linux.cpp
    2. Add X11 includes at top:
       ```cpp
       #include <X11/Xlib.h>
       #include <X11/Xutil.h>
       #include <X11/Xatom.h>
       ```
    3. Add private members to WindowSystemLinux class (in .cpp or .h):
       ```cpp
       Display* m_display;
       Window m_rootWindow;
       std::map<Atom, std::string> m_atomCache;  // For caching atom lookups
       ```
    4. Implement constructor initialization:
       ```cpp
       WindowSystemLinux::WindowSystemLinux() {
           m_display = XOpenDisplay(NULL);
           if (!m_display) {
               std::cerr << "[WindowSystemLinux] ERROR: Cannot open X11 display" << std::endl;
               // Continue anyway, methods will handle null display
           } else {
               m_rootWindow = DefaultRootWindow(m_display);
               // Install error handler
               XSetErrorHandler(windowSystemErrorHandler);
           }
       }
       ```
    5. Implement error handler (static function):
       ```cpp
       static int windowSystemErrorHandler(Display* display, XErrorEvent* error) {
           char errorText[256];
           XGetErrorText(display, error->error_code, errorText, sizeof(errorText));
           std::cerr << "[WindowSystemLinux] X11 Error: " << errorText << std::endl;
           return 0;  // Don't abort, just log
       }
       ```
    6. Implement destructor:
       ```cpp
       WindowSystemLinux::~WindowSystemLinux() {
           if (m_display) {
               XCloseDisplay(m_display);
           }
       }
       ```
    7. Add helper method for atom caching:
       ```cpp
       Atom WindowSystemLinux::getAtom(const char* name) {
           auto it = m_atomCache.find(name);
           if (it != m_atomCache.end()) {
               return it->second;
           }
           Atom atom = XInternAtom(m_display, name, False);
           m_atomCache[name] = atom;
           return atom;
       }
       ```

    Restrictions:
    - Do not call XOpenDisplay in every method (expensive, connection pooling needed)
    - Do not crash on XOpenDisplay failure (graceful degradation)
    - Do not ignore X11 errors (log them for debugging)

    Success Criteria:
    - WindowSystemLinux constructor opens X11 display successfully
    - No crashes when DISPLAY env var is unset (error logged instead)
    - X11 error handler catches BadWindow, BadAtom, etc. without aborting
    - Destructor closes display without leaks (validated with Valgrind)

    Instructions:
    1. Search logs: grep -r "X11.*Display.*Open" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Implement X11 connection setup in constructor/destructor
    4. Test: DISPLAY=:0 ./yamy_stub (should not crash)
    5. Test: DISPLAY=invalid ./yamy_stub (should log error, continue)
    6. Log with artifacts (classes: WindowSystemLinux, functions: getAtom, windowSystemErrorHandler)
    7. Mark complete
    ```

- [ ] 2.2 Implement window property queries (title, class, PID)
  - Files:
    - src/platform/linux/window_system_linux_queries.cpp (modify)
  - Description:
    - Implement `getWindowText()`: Query `_NET_WM_NAME` (UTF-8), fallback to `WM_NAME`
    - Implement `getClassName()`: Parse `WM_CLASS` property (format: "instance\0class\0")
    - Implement `getWindowProcessId()`: Read `_NET_WM_PID` property, return uint32_t
    - Handle errors (null properties, BadWindow, etc.)
  - _Leverage:
    - X11 Xlib XGetWindowProperty API
    - Atom caching from task 2.1
  - _Requirements: 1.3, 2.1, 2.2
  - _Prompt:
    ```
    Role: X11 Developer specializing in EWMH (Extended Window Manager Hints) and ICCCM protocols

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Implement real X11 property queries to extract window title, class name, and process ID.

    Context:
    - Task 2.1 set up X11 connection (m_display, m_rootWindow, error handler)
    - Currently: getWindowText() returns "Stub Window", getClassName() returns "StubClass", getWindowProcessId() returns 0
    - Need to query X11 properties using XGetWindowProperty

    Implementation:
    1. Open src/platform/linux/window_system_linux_queries.cpp
    2. Implement getWindowText() (~line 26-29):
       ```cpp
       std::string WindowSystemLinux::getWindowText(WindowHandle hwnd) {
           if (!m_display || !hwnd) return "";

           // Try _NET_WM_NAME first (UTF-8)
           Atom utf8String = getAtom("UTF8_STRING");
           Atom netWmName = getAtom("_NET_WM_NAME");
           Atom actualType;
           int actualFormat;
           unsigned long nItems, bytesAfter;
           unsigned char* prop = nullptr;

           if (XGetWindowProperty(m_display, hwnd, netWmName, 0, 1024, False,
                                  utf8String, &actualType, &actualFormat,
                                  &nItems, &bytesAfter, &prop) == Success && prop) {
               std::string result(reinterpret_cast<char*>(prop));
               XFree(prop);
               return result;
           }

           // Fallback to WM_NAME (legacy, may not be UTF-8)
           if (XFetchName(m_display, hwnd, reinterpret_cast<char**>(&prop)) && prop) {
               std::string result(reinterpret_cast<char*>(prop));
               XFree(prop);
               return result;
           }

           return "";  // No title
       }
       ```
    3. Implement getClassName() (~line 36-39):
       ```cpp
       std::string WindowSystemLinux::getClassName(WindowHandle hwnd) {
           if (!m_display || !hwnd) return "";

           XClassHint classHint;
           if (XGetClassHint(m_display, hwnd, &classHint) == 0) {
               return "";  // Failed
           }

           // WM_CLASS format: "instance\0class\0"
           // We want the class part (second string)
           std::string result = classHint.res_class ? classHint.res_class : "";
           if (classHint.res_name) XFree(classHint.res_name);
           if (classHint.res_class) XFree(classHint.res_class);
           return result;
       }
       ```
    4. Implement getWindowProcessId() (~line 46-49):
       ```cpp
       uint32_t WindowSystemLinux::getWindowProcessId(WindowHandle hwnd) {
           if (!m_display || !hwnd) return 0;

           Atom netWmPid = getAtom("_NET_WM_PID");
           Atom actualType;
           int actualFormat;
           unsigned long nItems, bytesAfter;
           unsigned char* prop = nullptr;

           if (XGetWindowProperty(m_display, hwnd, netWmPid, 0, 1, False,
                                  XA_CARDINAL, &actualType, &actualFormat,
                                  &nItems, &bytesAfter, &prop) == Success && prop) {
               uint32_t pid = *reinterpret_cast<uint32_t*>(prop);
               XFree(prop);
               return pid;
           }

           return 0;  // No PID
       }
       ```

    Restrictions:
    - Do not assume properties exist (check return values)
    - Do not leak memory (XFree all XGetWindowProperty results)
    - Do not crash on BadWindow (error handler catches it)

    Success Criteria:
    - getWindowText() returns actual window titles (tested with xterm, firefox)
    - getClassName() returns "XTerm", "Navigator", etc. (correct class, not instance)
    - getWindowProcessId() returns valid PIDs (verified via ps aux)
    - No memory leaks after 1000 queries (Valgrind clean)
    - Unit tests pass (mock X11 server)

    Instructions:
    1. Search logs: grep -r "XGetWindowProperty" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Implement property query methods
    4. Test: Select windows in dialog, verify title/class/PID show real values (not stubs)
    5. Log with artifacts (functions: getWindowText, getClassName, getWindowProcessId with X11 API details)
    6. Mark complete
    ```

- [ ] 2.3 Implement window geometry and state queries
  - Files:
    - src/platform/linux/window_system_linux_queries.cpp (modify)
  - Description:
    - Implement `getWindowRect()`: Use `XGetGeometry()` + `XTranslateCoordinates()` for screen-absolute position
    - Implement `getShowCommand()`: Check `_NET_WM_STATE` for `_NET_WM_STATE_HIDDEN`, `_NET_WM_STATE_MAXIMIZED_*`
    - Map X11 states to `WindowShowCmd` enum (Normal, Minimized, Maximized)
  - _Leverage:
    - X11 Xlib geometry API
    - EWMH _NET_WM_STATE property
  - _Requirements: 1.3, 1.6
  - _Prompt:
    ```
    Role: X11 Window Manager Expert with knowledge of EWMH state management

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Implement geometry and window state queries to support the investigate dialog's window info panel.

    Context:
    - Dialog needs to show window position (x,y) and size (width, height)
    - Dialog needs to show window state (Normal, Minimized, Maximized)
    - X11 coordinates are tricky: XGetGeometry returns parent-relative, need screen-absolute

    Implementation:
    1. Open src/platform/linux/window_system_linux_queries.cpp
    2. Implement getWindowRect() (~line 18-24):
       ```cpp
       bool WindowSystemLinux::getWindowRect(WindowHandle hwnd, Rect* rect) {
           if (!m_display || !hwnd || !rect) return false;

           Window root, parent, *children = nullptr;
           unsigned int nChildren;
           XWindowAttributes attrs;

           // Get window attributes (size)
           if (XGetWindowAttributes(m_display, hwnd, &attrs) == 0) {
               return false;
           }

           // Get absolute screen position
           int x, y;
           Window child;
           XTranslateCoordinates(m_display, hwnd, m_rootWindow, 0, 0, &x, &y, &child);

           rect->left = x;
           rect->top = y;
           rect->right = x + attrs.width;
           rect->bottom = y + attrs.height;

           return true;
       }
       ```
    3. Implement getShowCommand() (~line 87-90):
       ```cpp
       WindowShowCmd WindowSystemLinux::getShowCommand(WindowHandle window) {
           if (!m_display || !window) return WindowShowCmd::Normal;

           Atom netWmState = getAtom("_NET_WM_STATE");
           Atom actualType;
           int actualFormat;
           unsigned long nItems, bytesAfter;
           unsigned char* prop = nullptr;

           if (XGetWindowProperty(m_display, window, netWmState, 0, 1024, False,
                                  XA_ATOM, &actualType, &actualFormat,
                                  &nItems, &bytesAfter, &prop) != Success || !prop) {
               return WindowShowCmd::Normal;
           }

           Atom* states = reinterpret_cast<Atom*>(prop);
           Atom hiddenAtom = getAtom("_NET_WM_STATE_HIDDEN");
           Atom maxVert = getAtom("_NET_WM_STATE_MAXIMIZED_VERT");
           Atom maxHorz = getAtom("_NET_WM_STATE_MAXIMIZED_HORZ");

           bool isHidden = false;
           bool isMaximized = false;

           for (unsigned long i = 0; i < nItems; i++) {
               if (states[i] == hiddenAtom) isHidden = true;
               if (states[i] == maxVert || states[i] == maxHorz) isMaximized = true;
           }

           XFree(prop);

           if (isHidden) return WindowShowCmd::Minimized;
           if (isMaximized) return WindowShowCmd::Maximized;
           return WindowShowCmd::Normal;
       }
       ```

    Restrictions:
    - Do not use relative coordinates (must be screen-absolute for multi-monitor)
    - Do not assume all windows have _NET_WM_STATE (legacy windows may not)
    - Handle maximized windows correctly (both vert and horz atoms set)

    Success Criteria:
    - getWindowRect() returns correct screen-absolute coordinates (tested on multi-monitor setup)
    - getShowCommand() correctly identifies minimized, maximized, normal states
    - Minimized windows return Minimized (even if not visible)
    - Fullscreen windows return Maximized (X11 doesn't distinguish fullscreen state)

    Instructions:
    1. Search logs: grep -r "getWindowRect\|getShowCommand" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Implement geometry and state query methods
    4. Test: Select windows in different states (normal, minimized, maximized), verify dialog shows correct values
    5. Log with artifacts (functions: getWindowRect, getShowCommand, X11 coordinate translation)
    6. Mark complete
    ```

- [ ] 2.4 Implement windowFromPoint for crosshair selection
  - Files:
    - src/platform/linux/window_system_linux_hierarchy.cpp (modify)
  - Description:
    - Implement `windowFromPoint()`: Use `XQueryPointer()` to get window at cursor position
    - Handle nested windows (return deepest child window)
    - Handle unmapped windows, desktops, etc.
  - _Leverage:
    - X11 XQueryPointer API
    - Existing crosshair widget (calls this method)
  - _Requirements: 1.1, 1.2
  - _Prompt:
    ```
    Role: X11 Windowing Expert with deep knowledge of window hierarchies and pointer queries

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Implement windowFromPoint to support the crosshair window selection feature.

    Context:
    - CrosshairWidget calls windowFromPoint(cursor position) when user drags cursor
    - Currently returns nullptr (stub)
    - Need to query X11 for window at specific screen coordinates
    - Must return the deepest visible window (not root, not decorations)

    Implementation:
    1. Open src/platform/linux/window_system_linux_hierarchy.cpp
    2. Implement windowFromPoint() (~line 14-16):
       ```cpp
       WindowHandle WindowSystemLinux::windowFromPoint(const Point& pt) {
           if (!m_display) return nullptr;

           Window root, child;
           int rootX, rootY, winX, winY;
           unsigned int mask;

           // Query pointer to get window at cursor
           if (!XQueryPointer(m_display, m_rootWindow, &root, &child,
                              &rootX, &rootY, &winX, &winY, &mask)) {
               return nullptr;  // Pointer not on this screen
           }

           // If child is None, pointer is on root window
           if (child == None) {
               return m_rootWindow;
           }

           // Descend to deepest child window
           Window target = child;
           while (true) {
               Window nextRoot, nextChild;
               int nextX, nextY;
               unsigned int nextMask;

               if (!XQueryPointer(m_display, target, &nextRoot, &nextChild,
                                  &rootX, &rootY, &nextX, &nextY, &nextMask)) {
                   break;  // Can't query further
               }

               if (nextChild == None) {
                   break;  // Reached leaf window
               }

               target = nextChild;
           }

           return target;
       }
       ```

    Restrictions:
    - Do not return root window (user wants application window, not desktop)
    - Do not infinite loop if window hierarchy is circular (add max depth limit)
    - Handle unmapped/invisible windows gracefully

    Success Criteria:
    - windowFromPoint() returns correct window when cursor over app window
    - Returns deepest child (e.g., Firefox canvas, not Firefox frame)
    - Handles multi-monitor setups (cursor on monitor 2 returns windows on monitor 2)
    - No crashes when cursor over desktop, panel, or invalid windows

    Instructions:
    1. Search logs: grep -r "windowFromPoint\|XQueryPointer" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Implement windowFromPoint with deep traversal
    4. Test: Use crosshair to select windows, verify correct window highlighted
    5. Log with artifacts (functions: windowFromPoint, X11 pointer query logic)
    6. Mark complete
    ```

### Phase 3: Live Key Event Logging

- [ ] 3.1 Add live key event notifications in engine keyboard handler
  - Files:
    - src/core/engine/engine.cpp (modify)
    - src/core/engine/engine.h (modify, add helper method)
  - Description:
    - In `keyboardHandler()` method, add check for `m_isInvestigateMode` flag
    - If enabled, format key event string and send `NtfKeyEvent` via IPC
    - Format: "[HH:MM:SS.zzz] KeyName ↓↑"
  - _Leverage:
    - Existing keyboardHandler() in engine.cpp
    - Existing Key::getName() method
    - IPC channel from task 1.3
  - _Requirements: 5.1, 5.2, 5.4, 5.5, 5.6
  - _Prompt:
    ```
    Role: Engine Developer with expertise in input event processing and performance optimization

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Add live key event logging to the engine so the investigate dialog can display a real-time stream of keystrokes.

    Context:
    - Engine processes key events in keyboardHandler() (engine.cpp, Windows: ~line 25, Linux: similar)
    - Investigate mode flag `m_isInvestigateMode` is set/cleared by IPC commands (task 1.4)
    - Need to send IPC notification for every keystroke when investigate mode is active
    - Must not impact normal remapping latency (<1ms overhead target)

    Implementation:
    1. Open src/core/engine/engine.h
    2. Add private helper method declaration:
       ```cpp
       private:
           void sendKeyEventNotification(const Key* key, bool isKeyDown);
       ```
    3. Open src/core/engine/engine.cpp
    4. Find keyboardHandler() method (Windows: ~line 25, Linux: check #ifdef guards)
    5. Locate input processing loop (where `Key` object is created/processed)
    6. Add investigate mode check after key is identified but before remapping:
       ```cpp
       // Existing code: Key identified, state known
       bool isPhysicallyPressed = event.isKeyDown;  // Or similar

       // NEW: Send investigate notification if mode enabled
       if (m_isInvestigateMode && m_ipcChannel && m_ipcChannel->isConnected()) {
           sendKeyEventNotification(&key, isPhysicallyPressed);
       }

       // Existing code: Continue with remapping logic...
       ```
    7. Implement sendKeyEventNotification() method:
       ```cpp
       void Engine::sendKeyEventNotification(const Key* key, bool isKeyDown) {
           ipc::KeyEventNotification notification;

           // Format timestamp
           auto now = std::chrono::system_clock::now();
           auto time_t_now = std::chrono::system_clock::to_time_t(now);
           auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
               now.time_since_epoch()) % 1000;
           struct tm* t = localtime(&time_t_now);

           char timestamp[16];
           snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d.%03d",
                    t->tm_hour, t->tm_min, t->tm_sec, static_cast<int>(ms.count()));

           // Get key name
           std::string keyName = key->getName();  // Existing method

           // Format log entry
           const char* arrow = isKeyDown ? "↓" : "↑";
           snprintf(notification.keyEvent, sizeof(notification.keyEvent),
                    "[%s] %s %s", timestamp, keyName.c_str(), arrow);

           // Send IPC notification
           ipc::Message msg;
           msg.type = ipc::NtfKeyEvent;
           msg.data = &notification;
           msg.size = sizeof(notification);
           m_ipcChannel->send(msg);
       }
       ```

    Restrictions:
    - Do not send events when m_isInvestigateMode is false (overhead should be zero when dialog closed)
    - Do not block or wait for IPC send to complete (asynchronous send only)
    - Do not exceed 256 bytes for event string (notification.keyEvent buffer size)
    - Keep overhead <1ms (measure with QElapsedTimer if needed)

    Success Criteria:
    - Key events appear in dialog's live log panel within <10ms of keystroke
    - No noticeable input lag when investigate mode is active
    - Zero IPC traffic when dialog is closed (verified via strace)
    - Log format matches spec: "[HH:MM:SS.zzz] KeyName ↓↑"
    - Rapid typing (50 keys/sec) doesn't drop events or freeze UI

    Instructions:
    1. Search logs: grep -r "NtfKeyEvent\|sendKeyEventNotification" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Implement key event notification in engine.cpp
    4. Test: Open dialog, press keys, verify live log updates in real-time
    5. Log with artifacts (functions: sendKeyEventNotification, integrations: "Engine sends NtfKeyEvent to dialog for each keystroke")
    6. Mark complete
    ```

### Phase 4: Testing and Validation

- [ ] 4.1 Write unit tests for WindowSystemLinux property queries
  - Files:
    - tests/platform/window_system_linux_test.cpp (new)
  - Description:
    - Create mock X11 server for testing
    - Test getWindowText(), getClassName(), getWindowProcessId(), getWindowRect(), getShowCommand()
    - Test edge cases: null handles, missing properties, BadWindow errors
  - _Leverage:
    - Google Test framework
    - Mock X11 display (or real Xvfb)
  - _Requirements: All window query requirements (1.1-1.6, 2.1-2.5)
  - _Prompt:
    ```
    Role: QA Engineer specializing in unit testing and X11 system testing

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Create comprehensive unit tests for all WindowSystemLinux property query methods to ensure correctness and edge case handling.

    Context:
    - WindowSystemLinux methods query X11 for window properties (implemented in phase 2)
    - Need to verify correct behavior with various window states and error conditions
    - Tests should be deterministic and fast (no manual window creation)

    Implementation:
    1. Create tests/platform/window_system_linux_test.cpp
    2. Set up test fixture with Xvfb or mock X11 display:
       ```cpp
       #include <gtest/gtest.h>
       #include "platform/linux/window_system_linux.cpp"
       #include <X11/Xlib.h>

       class WindowSystemLinuxTest : public ::testing::Test {
       protected:
           Display* display;
           Window testWindow;
           WindowSystemLinux* windowSystem;

           void SetUp() override {
               display = XOpenDisplay(nullptr);
               ASSERT_NE(nullptr, display);

               // Create test window
               testWindow = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                                0, 0, 100, 100, 0, 0, 0);

               windowSystem = new WindowSystemLinux();
           }

           void TearDown() override {
               XDestroyWindow(display, testWindow);
               delete windowSystem;
               XCloseDisplay(display);
           }

           void setWindowProperty(Window w, const char* propName, const char* value) {
               Atom prop = XInternAtom(display, propName, False);
               Atom type = XInternAtom(display, "UTF8_STRING", False);
               XChangeProperty(display, w, prop, type, 8, PropModeReplace,
                               reinterpret_cast<const unsigned char*>(value),
                               strlen(value));
           }
       };

       TEST_F(WindowSystemLinuxTest, GetWindowTextReturnsUTF8Title) {
           setWindowProperty(testWindow, "_NET_WM_NAME", "Test Window");
           EXPECT_EQ("Test Window", windowSystem->getWindowText(testWindow));
       }

       TEST_F(WindowSystemLinuxTest, GetWindowTextHandlesUnicode) {
           setWindowProperty(testWindow, "_NET_WM_NAME", u8"日本語タイトル");
           EXPECT_EQ(u8"日本語タイトル", windowSystem->getWindowText(testWindow));
       }

       TEST_F(WindowSystemLinuxTest, GetClassNameReturnsCorrectClass) {
           XClassHint classHint;
           classHint.res_name = const_cast<char*>("firefox");
           classHint.res_class = const_cast<char*>("Navigator");
           XSetClassHint(display, testWindow, &classHint);

           EXPECT_EQ("Navigator", windowSystem->getClassName(testWindow));
       }

       TEST_F(WindowSystemLinuxTest, GetWindowProcessIdReturnsValidPID) {
           uint32_t myPid = getpid();
           Atom netWmPid = XInternAtom(display, "_NET_WM_PID", False);
           XChangeProperty(display, testWindow, netWmPid, XA_CARDINAL, 32,
                           PropModeReplace, reinterpret_cast<unsigned char*>(&myPid), 1);

           EXPECT_EQ(myPid, windowSystem->getWindowProcessId(testWindow));
       }

       TEST_F(WindowSystemLinuxTest, GetWindowRectReturnsCorrectGeometry) {
           XMoveResizeWindow(display, testWindow, 100, 200, 300, 400);
           XMapWindow(display, testWindow);
           XFlush(display);

           yamy::platform::Rect rect;
           ASSERT_TRUE(windowSystem->getWindowRect(testWindow, &rect));
           EXPECT_EQ(300, rect.width());
           EXPECT_EQ(400, rect.height());
           // Note: Exact x,y depends on window manager decorations
       }

       TEST_F(WindowSystemLinuxTest, BadWindowHandlesGracefully) {
           Window invalidWindow = 0x99999999;
           EXPECT_EQ("", windowSystem->getWindowText(invalidWindow));
           EXPECT_EQ("", windowSystem->getClassName(invalidWindow));
           EXPECT_EQ(0u, windowSystem->getWindowProcessId(invalidWindow));
       }
       ```
    3. Add more tests: null handles, missing properties, maximized/minimized states

    Restrictions:
    - Tests must run in CI (use Xvfb, not real X server)
    - Tests must be deterministic (no race conditions)
    - Do not assume specific window manager behavior

    Success Criteria:
    - All tests pass on Ubuntu 22.04 + Fedora 39
    - Code coverage >90% for window_system_linux_queries.cpp
    - Tests run in <5 seconds total
    - No X11 resource leaks (Valgrind clean)

    Instructions:
    1. Search logs: grep -r "window_system.*test" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Create test file and implement test cases
    4. Run: ctest --verbose
    5. Log with artifacts (functions: List all TEST_F cases)
    6. Mark complete
    ```

- [ ] 4.2 Write unit tests for IPCChannelQt message serialization
  - Files:
    - tests/platform/ipc_channel_qt_test.cpp (new)
  - Description:
    - Test send/receive round-trip for all message types
    - Test connection/disconnection handling
    - Test partial message buffering (message framing)
    - Test large messages, connection refused, timeout
  - _Leverage:
    - Qt Test framework (QTest)
    - Google Test framework
  - _Requirements: All IPC requirements (3.1-3.5)
  - _Prompt:
    ```
    Role: Qt Testing Specialist with expertise in IPC and network testing

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Create comprehensive unit tests for IPCChannelQt to verify message serialization, connection handling, and error scenarios.

    Context:
    - IPCChannelQt uses QLocalSocket/QLocalServer (implemented in task 1.1)
    - Need to test both client and server modes
    - Need to verify message framing (4-byte length + data) works correctly

    Implementation:
    1. Create tests/platform/ipc_channel_qt_test.cpp
    2. Set up test fixture:
       ```cpp
       #include <gtest/gtest.h>
       #include <QCoreApplication>
       #include <QLocalServer>
       #include <QLocalSocket>
       #include "core/platform/linux/ipc_channel_qt.h"
       #include "core/ipc_messages.h"

       class IPCChannelQtTest : public ::testing::Test {
       protected:
           QCoreApplication* app;
           IPCChannelQt* server;
           IPCChannelQt* client;

           void SetUp() override {
               int argc = 0;
               char* argv[] = {nullptr};
               app = new QCoreApplication(argc, argv);

               server = new IPCChannelQt("test-server");
               client = new IPCChannelQt("test-client");

               server->listen();
               client->connect("test-server");

               // Wait for connection
               QTest::qWait(100);
           }

           void TearDown() override {
               delete client;
               delete server;
               delete app;
           }
       };

       TEST_F(IPCChannelQtTest, ClientConnectsToServer) {
           ASSERT_TRUE(client->isConnected());
       }

       TEST_F(IPCChannelQtTest, MessageRoundTrip_InvestigateWindowRequest) {
           yamy::ipc::InvestigateWindowRequest request;
           request.hwnd = reinterpret_cast<void*>(0x12345);

           yamy::ipc::Message msg;
           msg.type = yamy::ipc::CmdInvestigateWindow;
           msg.data = &request;
           msg.size = sizeof(request);

           bool received = false;
           QObject::connect(server, &IPCChannelQt::messageReceived,
                            [&](const yamy::ipc::Message& receivedMsg) {
               EXPECT_EQ(yamy::ipc::CmdInvestigateWindow, receivedMsg.type);
               EXPECT_EQ(sizeof(request), receivedMsg.size);
               auto* req = static_cast<const yamy::ipc::InvestigateWindowRequest*>(receivedMsg.data);
               EXPECT_EQ(request.hwnd, req->hwnd);
               received = true;
           });

           client->send(msg);
           QTest::qWait(50);  // Process event loop

           EXPECT_TRUE(received);
       }

       TEST_F(IPCChannelQtTest, LargeMessageHandling) {
           // Test message larger than typical socket buffer
           char largeData[65536] = {0};
           yamy::ipc::Message msg;
           msg.type = yamy::ipc::NtfKeyEvent;
           msg.data = largeData;
           msg.size = sizeof(largeData);

           bool received = false;
           QObject::connect(server, &IPCChannelQt::messageReceived,
                            [&](const yamy::ipc::Message& receivedMsg) {
               EXPECT_EQ(sizeof(largeData), receivedMsg.size);
               received = true;
           });

           client->send(msg);
           QTest::qWait(100);

           EXPECT_TRUE(received);
       }

       TEST_F(IPCChannelQtTest, ConnectionRefusedHandling) {
           IPCChannelQt* failClient = new IPCChannelQt("fail-client");
           failClient->connect("nonexistent-server");

           QTest::qWait(100);
           EXPECT_FALSE(failClient->isConnected());

           delete failClient;
       }
       ```

    Restrictions:
    - Tests must use QTest::qWait() to process Qt event loop
    - Do not use blocking waits (no QEventLoop::exec())
    - Clean up all QObjects to avoid leaks

    Success Criteria:
    - All tests pass on Qt 5.12+
    - Message framing correctly handles partial reads (tested with fragmented sends)
    - No memory leaks (Valgrind clean)
    - Tests run in <3 seconds

    Instructions:
    1. Search logs: grep -r "IPCChannelQt.*test" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Create test file and implement test cases
    4. Run: ctest -R ipc_channel_qt_test -V
    5. Log with artifacts (functions: List all TEST_F cases)
    6. Mark complete
    ```

- [ ] 4.3 Write integration test for full investigate workflow
  - Files:
    - tests/integration/investigate_workflow_test.cpp (new)
  - Description:
    - Launch real engine + dialog processes
    - Simulate window selection via crosshair
    - Verify all panels populated with correct data
    - Simulate key presses, verify live log updates
    - Test IPC disconnection/reconnection
  - _Leverage:
    - Xvfb for headless X server
    - xdotool for simulating input
    - QTest for Qt event processing
  - _Requirements: All requirements (end-to-end validation)
  - _Prompt:
    ```
    Role: Integration Test Engineer with expertise in end-to-end system testing and automation

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Create an integration test that validates the complete investigate workflow from window selection to live event logging.

    Context:
    - Need to test full stack: CrosshairWidget → WindowSystem → IPC → Engine → IPC → Dialog
    - Test should be automated (no manual intervention)
    - Should run in CI with Xvfb (headless X server)

    Implementation:
    1. Create tests/integration/investigate_workflow_test.cpp
    2. Set up test environment:
       ```cpp
       #include <gtest/gtest.h>
       #include <QApplication>
       #include <QProcess>
       #include "ui/qt/dialog_investigate_qt.h"
       #include "core/engine/engine.h"

       class InvestigateWorkflowTest : public ::testing::Test {
       protected:
           QApplication* app;
           Engine* engine;
           DialogInvestigateQt* dialog;
           Window testWindow;

           void SetUp() override {
               // Set up Xvfb environment (CI)
               qputenv("DISPLAY", ":99");

               int argc = 1;
               char* argv[] = {const_cast<char*>("test"), nullptr};
               app = new QApplication(argc, argv);

               // Create test window with known properties
               Display* display = XOpenDisplay(nullptr);
               testWindow = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                                100, 100, 200, 200, 1, 0, 0);
               Atom netWmName = XInternAtom(display, "_NET_WM_NAME", False);
               const char* title = "Test Integration Window";
               XChangeProperty(display, testWindow, netWmName,
                               XInternAtom(display, "UTF8_STRING", False),
                               8, PropModeReplace,
                               reinterpret_cast<const unsigned char*>(title),
                               strlen(title));
               XMapWindow(display, testWindow);
               XFlush(display);

               // Start engine
               engine = new Engine(...);  // Initialize with test config
               engine->start();

               // Create dialog
               dialog = new DialogInvestigateQt(engine);
               dialog->show();

               QTest::qWait(500);  // Wait for IPC connection
           }

           void TearDown() override {
               delete dialog;
               engine->stop();
               delete engine;
               delete app;
           }
       };

       TEST_F(InvestigateWorkflowTest, WindowSelectionPopulatesAllPanels) {
           // Simulate window selection
           dialog->onWindowSelected(testWindow);
           QTest::qWait(100);  // Wait for IPC round-trip

           // Verify window info panel
           EXPECT_EQ("Test Integration Window", dialog->m_labelTitle->text().toStdString());
           EXPECT_NE("(unavailable)", dialog->m_labelClass->text().toStdString());
           EXPECT_NE("0", dialog->m_labelProcessPath->text().toStdString());

           // Verify keymap status panel
           EXPECT_NE("(IPC not connected)", dialog->m_labelKeymapName->text().toStdString());
           EXPECT_NE("-", dialog->m_labelMatchedRegex->text().toStdString());
       }

       TEST_F(InvestigateWorkflowTest, LiveKeyEventsAppearInLog) {
           dialog->onWindowSelected(testWindow);
           QTest::qWait(100);

           int initialLogLines = dialog->m_liveLog->document()->lineCount();

           // Simulate key press (via xdotool or direct engine injection)
           // For simplicity, directly inject event into engine
           yamy::platform::KeyEvent event{65, true, false, 0};  // 'A' key down
           engine->pushInputEvent(event);

           QTest::qWait(50);  // Wait for IPC notification

           int newLogLines = dialog->m_liveLog->document()->lineCount();
           EXPECT_GT(newLogLines, initialLogLines);

           QString logText = dialog->m_liveLog->toPlainText();
           EXPECT_TRUE(logText.contains("↓") || logText.contains("↑"));
       }

       TEST_F(InvestigateWorkflowTest, IPCReconnectionAfterEngineCrash) {
           dialog->onWindowSelected(testWindow);
           QTest::qWait(100);

           EXPECT_TRUE(dialog->m_ipcChannel->isConnected());

           // Simulate engine crash
           engine->stop();
           delete engine;
           engine = nullptr;

           QTest::qWait(600);  // Wait for disconnection detection

           EXPECT_FALSE(dialog->m_ipcChannel->isConnected());
           EXPECT_EQ("(IPC not connected)", dialog->m_labelKeymapName->text().toStdString());

           // Restart engine
           engine = new Engine(...);
           engine->start();

           QTest::qWait(2500);  // Wait for auto-reconnect (2s timer)

           EXPECT_TRUE(dialog->m_ipcChannel->isConnected());
       }
       ```

    Restrictions:
    - Test must run in CI (use Xvfb, not real X server)
    - Do not rely on specific window manager behavior
    - Test must be deterministic (no random failures)

    Success Criteria:
    - Test passes on Ubuntu 22.04 + Fedora 39 (CI)
    - All assertions pass consistently (10 consecutive runs)
    - Test completes in <10 seconds
    - No resource leaks (windows, processes, sockets)

    Instructions:
    1. Search logs: grep -r "integration.*test.*investigate" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Create integration test with full workflow
    4. Run: ctest -R investigate_workflow_test -V
    5. Log with artifacts (integrations: "Full E2E workflow from crosshair to live logging")
    6. Mark complete
    ```

- [ ] 4.4 Performance benchmarking and optimization
  - Files:
    - tests/benchmarks/investigate_performance_test.cpp (new)
  - Description:
    - Measure window property query latency (<10ms target)
    - Measure IPC round-trip latency (<5ms target)
    - Measure live event notification latency (<10ms target)
    - Stress test: 50 keys/sec, verify no dropped events and <5% CPU
  - _Leverage:
    - QElapsedTimer for precise timing
    - perf/ftrace for profiling
  - _Requirements: All performance requirements (NFR section)
  - _Prompt:
    ```
    Role: Performance Engineer specializing in latency optimization and profiling

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Create performance benchmarks to validate all latency and throughput requirements are met.

    Context:
    - Requirements specify strict latency targets (<10ms window query, <5ms IPC, <10ms E2E)
    - Need to measure actual performance and identify bottlenecks
    - Benchmarks should fail if performance degrades below targets

    Implementation:
    1. Create tests/benchmarks/investigate_performance_test.cpp
    2. Implement latency benchmarks:
       ```cpp
       #include <gtest/gtest.h>
       #include <QElapsedTimer>
       #include "platform/linux/window_system_linux.cpp"
       #include "core/platform/linux/ipc_channel_qt.h"

       class InvestigatePerformanceTest : public ::testing::Test {
       protected:
           WindowSystemLinux* windowSystem;
           Display* display;
           Window testWindow;

           void SetUp() override {
               display = XOpenDisplay(nullptr);
               testWindow = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                                0, 0, 100, 100, 0, 0, 0);
               XMapWindow(display, testWindow);
               XFlush(display);

               windowSystem = new WindowSystemLinux();
           }
       };

       TEST_F(InvestigatePerformanceTest, WindowPropertyQueryLatency) {
           QElapsedTimer timer;
           const int iterations = 100;
           qint64 totalNs = 0;

           for (int i = 0; i < iterations; i++) {
               timer.start();
               std::string title = windowSystem->getWindowText(testWindow);
               std::string className = windowSystem->getClassName(testWindow);
               uint32_t pid = windowSystem->getWindowProcessId(testWindow);
               yamy::platform::Rect rect;
               windowSystem->getWindowRect(testWindow, &rect);
               auto state = windowSystem->getShowCommand(testWindow);
               totalNs += timer.nsecsElapsed();
           }

           qint64 avgNs = totalNs / iterations;
           double avgMs = avgNs / 1000000.0;

           std::cout << "Average window query latency: " << avgMs << " ms" << std::endl;
           EXPECT_LT(avgMs, 10.0);  // Must be <10ms
       }

       TEST_F(InvestigatePerformanceTest, IPCRoundTripLatency) {
           IPCChannelQt server("perf-server");
           IPCChannelQt client("perf-client");

           server.listen();
           client.connect("perf-server");
           QTest::qWait(100);

           QElapsedTimer timer;
           const int iterations = 100;
           qint64 totalNs = 0;

           for (int i = 0; i < iterations; i++) {
               timer.start();

               yamy::ipc::InvestigateWindowRequest request{testWindow};
               yamy::ipc::Message msg{yamy::ipc::CmdInvestigateWindow, &request, sizeof(request)};
               client.send(msg);

               // Simulate server response
               yamy::ipc::InvestigateWindowResponse response;
               yamy::ipc::Message responsMsg{yamy::ipc::RspInvestigateWindow, &response, sizeof(response)};
               server.send(responsMsg);

               // Wait for round-trip
               QTest::qWait(1);
               totalNs += timer.nsecsElapsed();
           }

           qint64 avgNs = totalNs / iterations;
           double avgMs = avgNs / 1000000.0;

           std::cout << "Average IPC round-trip latency: " << avgMs << " ms" << std::endl;
           EXPECT_LT(avgMs, 5.0);  // Must be <5ms
       }

       TEST_F(InvestigatePerformanceTest, StressTest_50KeysPerSecond) {
           // Simulate rapid key presses
           // Measure CPU usage and event drop rate
           // EXPECT <5% CPU, 0 dropped events
           GTEST_SKIP() << "Stress test requires real engine process";
       }
       ```

    Restrictions:
    - Benchmarks must be repeatable (run 100+ iterations)
    - Do not use sleeps to simulate latency (measure actual operations)
    - Report P99 latency, not just average

    Success Criteria:
    - Window query latency <10ms (P99)
    - IPC round-trip <5ms (P99)
    - All benchmarks pass consistently
    - Results logged to build artifacts for tracking

    Instructions:
    1. Search logs: grep -r "benchmark.*performance" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Create benchmark tests
    4. Run: ctest -R investigate_performance_test -V
    5. Log with artifacts (functions: List benchmark test cases, include performance results)
    6. Mark complete
    ```

### Phase 5: Documentation and Cleanup

- [ ] 5.1 Update code documentation (Doxygen comments)
  - Files:
    - All new/modified files (add /** */ comments to public APIs)
  - Description:
    - Add Doxygen comments to all public methods (IPCChannelQt, WindowSystemLinux)
    - Document parameters, return values, error conditions
    - Update class-level documentation
  - _Leverage:
    - Existing Doxygen comment style
  - _Requirements: Code quality gates (documentation requirement)
  - _Prompt:
    ```
    Role: Documentation Specialist with C++ and Doxygen expertise

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Add comprehensive Doxygen documentation to all public APIs created/modified in this spec.

    Context:
    - New classes: IPCChannelQt, expanded WindowSystemLinux methods
    - Modified classes: Engine (IPC connection, key event logging)
    - Need to document parameters, return values, error handling, thread safety

    Implementation:
    1. For each public method, add Doxygen comment:
       ```cpp
       /**
        * @brief Connects to a named IPC server using Unix domain socket
        *
        * Initiates asynchronous connection to the specified IPC server.
        * The connection is non-blocking; use isConnected() or wait for
        * the connected() signal to verify successful connection.
        *
        * @param name Server name (e.g., "yamy-engine")
        *             Socket path will be /tmp/yamy-{name}-{UID}
        *
        * @note This method does not block. Connection happens asynchronously.
        * @note If already connected, this method disconnects first.
        *
        * @see isConnected(), disconnected() signal
        */
       void IPCChannelQt::connect(const std::string& name);
       ```
    2. Document all new classes:
       ```cpp
       /**
        * @class IPCChannelQt
        * @brief Qt-based IPC channel using Unix domain sockets
        *
        * Implements IIPCChannel interface for Linux using QLocalSocket (client)
        * and QLocalServer (server) for inter-process communication between
        * the Qt GUI and the engine process.
        *
        * Thread Safety: All operations are thread-safe via Qt's queued connections.
        *
        * Example Usage:
        * @code
        * IPCChannelQt channel("my-client");
        * connect(&channel, &IPCChannelQt::messageReceived, this, &MyClass::onMessage);
        * channel.connect("yamy-engine");
        * @endcode
        */
       class IPCChannelQt : public IIPCChannel { ... };
       ```
    3. Update modified methods with @note for behavior changes

    Restrictions:
    - Follow existing Doxygen style (see window_system_interface.h)
    - Document error conditions (what happens if display is null, etc.)
    - Do not document private methods (internal implementation)

    Success Criteria:
    - Doxygen generates HTML without warnings
    - All public APIs have /** */ comments
    - Examples included for complex APIs (IPC, window queries)
    - Thread safety documented where relevant

    Instructions:
    1. Search logs: grep -r "Doxygen\|documentation" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Add Doxygen comments to all modified files
    4. Run: doxygen Doxyfile (check for warnings)
    5. Log with artifacts (files: List all files with updated documentation)
    6. Mark complete
    ```

- [ ] 5.2 Update CHANGELOG.md and user documentation
  - Files:
    - CHANGELOG.md (add entry for investigate window feature)
    - docs/LINUX-QT-GUI-MANUAL.md (update with investigate dialog usage)
  - Description:
    - Document new functionality in CHANGELOG
    - Add user guide section on how to use investigate window feature
    - Include screenshots (optional, describe UI panels)
  - _Leverage:
    - Existing CHANGELOG format
    - Existing user guide structure
  - _Requirements: Usability (documentation for users)
  - _Prompt:
    ```
    Role: Technical Writer specializing in user documentation and release notes

    Task: Implement the task for spec investigate-window, first run spec-workflow-guide to get the workflow guide then implement the task:

    Update user-facing documentation to describe the investigate window feature and how to use it for debugging.

    Context:
    - CHANGELOG.md tracks all user-visible changes
    - docs/LINUX-QT-GUI-MANUAL.md is the main user guide for Qt GUI
    - Need to explain what the feature does and how to use it

    Implementation:
    1. Update CHANGELOG.md:
       ```markdown
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
       - Window property queries now return real data (not stubs) on Linux
       - IPC channel now functional (was stub returning "not connected")
       - Engine now sends keymap status responses to investigate dialog
       ```
    2. Update docs/LINUX-QT-GUI-MANUAL.md:
       ```markdown
       ## Investigate Window (Debug Tool)

       The Investigate Window feature helps you debug your .mayu configuration by showing:
       - Which window YAMY sees when you select it
       - Which keymap is active for that window
       - Why a particular regex matched (or didn't match)
       - Real-time stream of key events as you type

       ### How to Use

       1. **Open the Investigate Dialog**:
          - Right-click the YAMY tray icon
          - Select "Investigate Window" from menu
          - Dialog window opens

       2. **Select a Window**:
          - Click "Select Window" button
          - Crosshair cursor appears
          - Drag crosshair over target window
          - Release mouse button to select

       3. **View Window Information**:
          - **Window Information Panel** (left):
            - Handle: Unique window ID (hex)
            - Title: Window title as seen by YAMY
            - Class: Window class name (for .mayu window rules)
            - Process: Application process name
            - Path: Full path to executable
            - Geometry: Screen position and size
            - State: Normal, Minimized, or Maximized

       4. **View Keymap Status** (right):
          - **Keymap**: Name of active keymap (or "(global keymap)")
          - **Matched Regex**: Class/title patterns that matched
          - **Modifiers**: Current modifier state (Shift, Ctrl, Alt, etc.)

       5. **Monitor Live Key Events**:
          - Press keys while window is selected
          - Events appear in log panel: `[HH:MM:SS.zzz] KeyName ↓↑`
          - Verify your bindings are being captured correctly

       ### Troubleshooting

       - **"(IPC not connected)"**: Engine is not running. Start `yamy_stub`.
       - **Panels show "-"**: No window selected yet. Click "Select Window".
       - **No live events**: Make sure dialog is still open (closing disables logging).

       ### Example: Debug Emacs Binding

       Problem: "My C-x doesn't work in Emacs, why?"

       1. Open investigate dialog
       2. Select Emacs window
       3. Check "Keymap" field: Should show "Emacs" (not "Global")
          - If shows "Global", your window rule didn't match
          - Check "Matched Regex" to see what YAMY sees
       4. Press C-x
       5. Check live log: Should show "[HH:MM:SS] C-x ↓"
          - If shows different key, your binding has a typo in .mayu
       ```

    Restrictions:
    - Use clear, non-technical language (user guide, not developer docs)
    - Include examples (debugging scenarios)
    - Do not document internal implementation details

    Success Criteria:
    - CHANGELOG entry describes user-visible features
    - User guide explains how to use all panels
    - Troubleshooting section addresses common issues
    - No technical jargon (class names, IPC, etc.)

    Instructions:
    1. Search logs: grep -r "CHANGELOG\|user.*doc" .spec-workflow/specs/*/Implementation\ Logs/
    2. Mark in progress
    3. Update CHANGELOG.md and user guide
    4. Review: Have another developer read for clarity
    5. Log with artifacts (files: CHANGELOG.md, docs/LINUX-QT-GUI-MANUAL.md)
    6. Mark complete
    ```

---

**Document Version**: 1.0
**Created**: 2025-12-14
**Status**: Pending Approval
**Total Tasks**: 18 (5 phases)
**Estimated Effort**: 40-60 hours
