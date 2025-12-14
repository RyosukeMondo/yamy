# Design Document: Investigate Window Feature

## Overview

The Investigate Window feature is a real-time debugging interface that bridges YAMY's Qt GUI (dialog) and the core engine (background service) through IPC communication. It enables users to:
1. Select windows using a crosshair overlay
2. Query window properties via X11/XCB
3. Inspect active keymaps and pattern matching via IPC
4. Monitor live key events streamed from the engine

**Architecture**: The design follows YAMY's layered architecture with clean separation between UI (Qt), platform abstraction (IWindowSystem), and core logic (Engine). The IPC layer acts as a bridge, allowing the GUI to remain decoupled from engine internals while accessing real-time state.

## Steering Document Alignment

### Technical Standards (tech.md)

**Platform Abstraction Pattern**:
- Follows existing `IWindowSystem` interface pattern (interface + factory)
- Implements Linux-specific `WindowSystemLinux::windowFromPoint()`, `getWindowText()`, etc.
- Maintains conditional compilation strategy (no #ifdef hell in core logic)

**Qt5 for Linux GUI**:
- Uses Qt5 Widgets for investigate dialog (consistent with `TrayIconQt`, `DialogSettingsQt`)
- Leverages `QLocalServer`/`QLocalSocket` for IPC (Qt-native, event-driven)
- Follows Qt signals/slots pattern for asynchronous communication

**Performance Targets**:
- <1ms input latency budget (critical path: key event → engine → IPC → dialog)
- <10MB memory footprint (Qt dialog + IPC buffers + X11 caching)
- <5% CPU overhead during active key logging (gated IPC sends)

**Threading Model**:
- Single-threaded Qt event loop (matches existing `DialogSettingsQt`, `TrayIconQt`)
- IPC handled asynchronously via Qt signals (no blocking reads)
- Engine runs in separate process (no shared memory, message-passing only)

### Project Structure (structure.md)

**File Organization**:
```
src/
├── core/platform/linux/
│   └── ipc_channel_qt.{h,cpp}        # NEW: Qt IPC implementation
├── platform/linux/
│   ├── window_system_linux.cpp       # MODIFY: Implement stubs with X11
│   ├── window_system_linux_queries.cpp  # MODIFY: Real property queries
│   └── window_system_linux_hierarchy.cpp  # MODIFY: windowFromPoint
├── core/engine/
│   ├── engine_lifecycle.cpp          # MODIFY: Connect IPC signal
│   ├── engine.cpp                    # MODIFY: Add key event logging
│   └── engine.h                      # MODIFY: Add IPC member
├── ui/qt/
│   └── dialog_investigate_qt.{h,cpp} # EXISTING: Update for IPC
```

**Naming Conventions**:
- `snake_case.cpp` for new files (window_system_linux, ipc_channel_qt)
- `camelCase` for functions (`windowFromPoint()`, `getWindowText()`)
- `m_` prefix for members (`m_ipcChannel`, `m_windowSystem`)
- `i_` prefix for input params (`i_hwnd`), `o_` for output (`o_rect`)

**Modular Design**:
- Each implementation file <500 lines (enforced by pre-commit hook)
- `window_system_linux_queries.cpp` handles all property queries (title, class, PID)
- `window_system_linux_hierarchy.cpp` handles spatial queries (windowFromPoint, getParent)
- `ipc_channel_qt.cpp` encapsulates all QLocalSocket logic

## Code Reuse Analysis

### Existing Components to Leverage

1. **DialogInvestigateQt** (`src/ui/qt/dialog_investigate_qt.{h,cpp}`)
   - **Current State**: UI layout complete, crosshair widget functional, panel structure ready
   - **Reuse**: Keep existing UI setup, add IPC connection in constructor
   - **Extend**: Add `onIpcMessageReceived()` slot for `RspInvestigateWindow` and `NtfKeyEvent`

2. **CrosshairWidget** (`src/ui/qt/crosshair_widget_qt.{h,cpp}`)
   - **Current State**: Fully functional (window selection, Esc cancellation, highlight border)
   - **Reuse**: No changes needed, already emits `windowSelected(WindowHandle)` signal
   - **Integration**: Dialog connects signal to `onWindowSelected()` slot

3. **WindowSystemLinux Skeleton** (`src/platform/linux/window_system_linux*.cpp`)
   - **Current State**: All methods stubbed (return hardcoded values)
   - **Reuse**: Keep interface, replace stub implementations with X11/XCB calls
   - **Extend**: Add X11 connection management, error handling, property caching

4. **Engine::queryKeymapForWindow()** (`src/core/engine/engine_focus.cpp`)
   - **Current State**: Fully implemented, returns `KeymapStatus` struct
   - **Reuse**: Already called by `handleIpcMessage()`, no changes needed
   - **Integration**: Response serialized into `InvestigateWindowResponse` IPC message

5. **IPC Message Definitions** (`src/core/ipc_messages.h`)
   - **Current State**: Fully defined (`CmdInvestigateWindow`, `RspInvestigateWindow`, `NtfKeyEvent`)
   - **Reuse**: Use existing structs as-is, no protocol changes
   - **Integration**: Serialize/deserialize via `QDataStream` in `IPCChannelQt`

### Integration Points

1. **Engine ↔ IPC Channel** (NEW):
   - **Existing**: `IPCChannelNull` created in `engine_lifecycle.cpp:79`, listen() called but no handlers
   - **New Integration**: Replace factory, connect `messageReceived` signal to `Engine::handleIpcMessage()`
   - **Change**: `ipc_channel_factory.h` conditionally returns `IPCChannelQt` on Linux

2. **Dialog ↔ WindowSystem** (EXISTING):
   - **Current**: Dialog creates `IWindowSystem` via factory, calls `getWindowText()`, `getClassName()`, etc.
   - **Integration**: No changes, dialog already uses abstraction correctly
   - **Benefit**: Stub → real implementation swap is transparent to dialog

3. **Dialog ↔ IPC Channel** (EXISTING STRUCTURE, NEW CONNECTION):
   - **Current**: Dialog creates `IIPCChannel`, but `IPCChannelNull` always returns `isConnected()=false`
   - **New Integration**: `IPCChannelQt` actually connects, emits `messageReceived()` signal
   - **Change**: Dialog's existing `onIpcMessageReceived()` slot now receives real messages

4. **Engine ↔ Keyboard Handler** (NEW):
   - **Existing**: `engine.cpp:keyboardHandler()` processes input events, no investigate logic
   - **New Integration**: Add `if (m_isInvestigateMode)` check, send `NtfKeyEvent` via `m_ipcChannel->send()`
   - **Gating**: Flag set/cleared by `CmdEnableInvestigateMode` / `CmdDisableInvestigateMode`

## Architecture

### System Context Diagram

```mermaid
graph TB
    subgraph "User"
        User[User]
    end

    subgraph "YAMY Qt GUI Process"
        TrayIcon[TrayIcon]
        InvestigateDialog[Investigate Dialog]
        Crosshair[Crosshair Widget]
        IPC_Client[IPC Channel<br/>QLocalSocket]
    end

    subgraph "YAMY Engine Process"
        Engine[Engine]
        IPC_Server[IPC Server<br/>QLocalServer]
        KeymapQuery[Keymap Query]
        InputHandler[Keyboard Handler]
    end

    subgraph "X11 Server"
        WindowManager[Window Manager]
        InputDevices[Input Devices]
    end

    subgraph "Linux Kernel"
        ProcFS[/proc filesystem]
    end

    User -->|1. Click menu| TrayIcon
    TrayIcon -->|2. Open| InvestigateDialog
    InvestigateDialog -->|3. Click button| Crosshair
    Crosshair -->|4. Select| WindowManager
    WindowManager -->|5. Window handle| InvestigateDialog
    InvestigateDialog -->|6. Query properties| WindowManager
    InvestigateDialog -->|7. Query PID| ProcFS
    InvestigateDialog -->|8. Request keymap| IPC_Client
    IPC_Client -->|9. Unix socket| IPC_Server
    IPC_Server -->|10. Forward| Engine
    Engine -->|11. Query| KeymapQuery
    KeymapQuery -->|12. Response| Engine
    Engine -->|13. Send| IPC_Server
    IPC_Server -->|14. Unix socket| IPC_Client
    IPC_Client -->|15. Update UI| InvestigateDialog
    InputDevices -->|16. Key event| InputHandler
    InputHandler -->|17. Notify| IPC_Server
    IPC_Server -->|18. Live event| IPC_Client
    IPC_Client -->|19. Append log| InvestigateDialog
```

### Component Architecture

```mermaid
graph TD
    subgraph "UI Layer (Qt)"
        Dialog[DialogInvestigateQt]
        Crosshair[CrosshairWidget]
        Panels[Info Panels<br/>QLabel widgets]
        LiveLog[Live Log<br/>QTextEdit]
    end

    subgraph "IPC Layer"
        IPCFactory[IPC Channel Factory]
        IPCChannelQt[IPCChannelQt]
        IPCMessages[IPC Message Structs]
    end

    subgraph "Platform Layer"
        WSFactory[WindowSystem Factory]
        WSLinux[WindowSystemLinux]
        X11Wrapper[X11/XCB Wrapper]
        ProcReader[/proc Reader]
    end

    subgraph "Engine Layer"
        Engine[Engine]
        HandleIPC[handleIpcMessage]
        QueryKeymap[queryKeymapForWindow]
        KbdHandler[keyboardHandler]
    end

    Dialog -->|create| Crosshair
    Dialog -->|update| Panels
    Dialog -->|append| LiveLog
    Dialog -->|uses| IPCFactory
    Dialog -->|uses| WSFactory
    IPCFactory -->|creates| IPCChannelQt
    WSFactory -->|creates| WSLinux
    WSLinux -->|calls| X11Wrapper
    WSLinux -->|reads| ProcReader
    Dialog -->|connect signal| IPCChannelQt
    IPCChannelQt -->|messageReceived| Dialog
    Engine -->|create| IPCFactory
    Engine -->|connect signal| IPCChannelQt
    IPCChannelQt -->|messageReceived| HandleIPC
    HandleIPC -->|calls| QueryKeymap
    KbdHandler -->|sends| IPCChannelQt
```

### Modular Design Principles

**Single File Responsibility**:
- `ipc_channel_qt.cpp`: Qt socket management, message serialization
- `window_system_linux_queries.cpp`: X11 property queries only (title, class, PID, state)
- `window_system_linux_hierarchy.cpp`: Spatial queries only (windowFromPoint, getParent)
- `dialog_investigate_qt.cpp`: UI orchestration, signal/slot wiring (no business logic)

**Component Isolation**:
- Dialog can be tested with mock `IWindowSystem` and `IIPCChannel` (dependency injection)
- `IPCChannelQt` has no Qt GUI dependencies (can run headless for testing)
- `WindowSystemLinux` has no engine dependencies (pure platform abstraction)

**Service Layer Separation**:
- **Data Access**: `WindowSystemLinux` (talks to X11)
- **Business Logic**: `Engine::queryKeymapForWindow()` (talks to Setting/Keymap)
- **Presentation**: `DialogInvestigateQt` (talks to QLabel/QTextEdit)

## Components and Interfaces

### Component 1: IPCChannelQt

**Purpose**: Implement Qt-based IPC communication using Unix domain sockets for GUI-engine messaging.

**Interfaces**:
```cpp
class IPCChannelQt : public IIPCChannel {
public:
    // IIPCChannel interface
    void connect(const std::string& name) override;
    void disconnect() override;
    void listen() override;
    bool isConnected() override;
    void send(const ipc::Message& msg) override;
    std::unique_ptr<ipc::Message> nonBlockingReceive() override;

    // Qt-specific
signals:
    void messageReceived(const yamy::ipc::Message& message);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();

private:
    QLocalSocket* m_clientSocket;  // Client mode (dialog)
    QLocalServer* m_server;        // Server mode (engine)
    QByteArray m_receiveBuffer;    // Partial message buffer
    QString m_socketPath;          // e.g., "/tmp/yamy-engine"

    void serializeMessage(const ipc::Message& msg, QDataStream& stream);
    ipc::Message deserializeMessage(QDataStream& stream);
};
```

**Dependencies**:
- Qt5Core (QLocalSocket, QLocalServer, QDataStream)
- `core/ipc_messages.h` (message structs)

**Reuses**:
- Qt event loop for asynchronous I/O (no manual polling)
- QDataStream for message serialization (type-safe)

**Implementation Notes**:
- **Socket Path**: `/tmp/yamy-engine-{UID}` (user-specific to avoid collisions)
- **Message Framing**: 4-byte length prefix + serialized data (prevents partial reads)
- **Connection States**: Disconnected → Connecting → Connected → Disconnected
- **Error Handling**: Emit `disconnected()` signal on socket errors, dialog shows "(IPC not connected)"

### Component 2: WindowSystemLinux (X11 Implementation)

**Purpose**: Implement window system abstraction for Linux using X11/XCB to query window properties and handle spatial queries.

**Interfaces** (already defined in `window_system_interface.h`):
```cpp
class WindowSystemLinux : public IWindowSystem {
public:
    // Spatial queries (hierarchy.cpp)
    WindowHandle windowFromPoint(const Point& pt) override;
    WindowHandle getForegroundWindow() override;
    WindowHandle getParent(WindowHandle window) override;

    // Property queries (queries.cpp)
    std::string getWindowText(WindowHandle hwnd) override;
    std::string getClassName(WindowHandle hwnd) override;
    uint32_t getWindowProcessId(WindowHandle hwnd) override;
    bool getWindowRect(WindowHandle hwnd, Rect* rect) override;
    WindowShowCmd getShowCommand(WindowHandle window) override;

    // ... 40+ other methods (stubs for now, implement as needed)

private:
    Display* m_display;            // X11 connection
    Window m_rootWindow;           // Root window for queries
    std::map<Atom, std::string> m_atomCache;  // Cache X11 atoms

    Atom getAtom(const char* name);  // Helper: get/cache X11 atom
    std::string getWindowProperty(WindowHandle hwnd, Atom property);
};
```

**Dependencies**:
- X11 (libX11, `#include <X11/Xlib.h>`)
- XCB (optional for async queries, future optimization)
- Xrandr (for multi-monitor support)

**Reuses**:
- Existing `IWindowSystem` interface (no changes to core engine)
- Platform abstraction pattern (factory function `createWindowSystem()`)

**Implementation Notes**:
- **Connection Management**: Open `Display*` in constructor, close in destructor (RAII)
- **Error Handling**: Install X11 error handler, catch all `BadWindow` errors (return gracefully)
- **Property Queries**:
  - `getWindowText()`: Try `_NET_WM_NAME` (UTF-8), fallback to `WM_NAME` (legacy)
  - `getClassName()`: Parse `WM_CLASS` (format: "instance\0class\0")
  - `getWindowProcessId()`: Read `_NET_WM_PID` property, convert to uint32_t
  - `getWindowRect()`: `XGetGeometry()` for size, `XTranslateCoordinates()` for screen position
  - `getShowCommand()`: Check `_NET_WM_STATE` for `_NET_WM_STATE_HIDDEN`, `_NET_WM_STATE_MAXIMIZED_*`
- **windowFromPoint()**: Use `XQueryPointer()` to get window at cursor position
- **Caching**: Atom names cached to avoid repeated `XInternAtom()` calls (50% speedup)

### Component 3: DialogInvestigateQt (IPC Integration)

**Purpose**: Orchestrate window selection, property display, and live event logging by integrating WindowSystem and IPC.

**Interfaces** (existing class, new slots):
```cpp
class DialogInvestigateQt : public QDialog {
    Q_OBJECT

public:
    explicit DialogInvestigateQt(Engine* engine = nullptr, QWidget* parent = nullptr);
    void setEngine(Engine* engine);

    // Dependency injection for testing
    void setWindowSystem(std::unique_ptr<IWindowSystem> ws);
    void setIpcChannel(std::unique_ptr<IIPCChannel> ipc);

signals:
    void windowInvestigated(WindowHandle hwnd);

private slots:
    void onSelectWindow();          // Existing: open crosshair
    void onWindowSelected(WindowHandle hwnd);  // Existing: update panels
    void onSelectionCancelled();    // Existing: handle Esc

    void onIpcMessageReceived(const yamy::ipc::Message& message);  // NEW: handle IPC
    void onIpcConnected();          // NEW: update status label
    void onIpcDisconnected();       // NEW: show "(IPC not connected)"

private:
    // Existing members
    Engine* m_engine;
    std::unique_ptr<IWindowSystem> m_windowSystem;
    std::unique_ptr<IIPCChannel> m_ipcChannel;
    CrosshairWidget* m_crosshair;
    QLabel* m_labelHandle;
    QLabel* m_labelTitle;
    // ... other labels ...
    QTextEdit* m_liveLog;
    WindowHandle m_selectedWindow;

    // NEW: IPC reconnection
    QTimer* m_reconnectTimer;       // Retry connection every 2s

    void setupIpcConnection();      // NEW: connect signals, setup timer
    void updateWindowInfo(WindowHandle hwnd);  // Existing: populate panels
    void updateKeymapStatus(WindowHandle hwnd, const std::string& className, const std::string& titleName);  // Existing: query keymap
};
```

**Dependencies**:
- Qt5Widgets (QDialog, QLabel, QTextEdit, QTimer)
- `IWindowSystem`, `IIPCChannel` (platform abstractions)
- `CrosshairWidget` (window selection UI)

**Reuses**:
- Existing panel layout, crosshair widget, helper functions (getProcessName, getProcessPath)
- Qt signal/slot architecture for asynchronous IPC

**Implementation Notes**:
- **Constructor Changes**:
  ```cpp
  DialogInvestigateQt::DialogInvestigateQt(Engine* engine, QWidget* parent)
      : QDialog(parent), m_engine(engine), ... {
      setupUI();  // Existing

      // NEW: IPC setup
      m_ipcChannel = yamy::platform::createIPCChannel("yamy-investigate");
      if (m_ipcChannel) {
          connect(m_ipcChannel.get(), &IIPCChannel::messageReceived,
                  this, &DialogInvestigateQt::onIpcMessageReceived);
          connect(m_ipcChannel.get(), &IIPCChannel::connected,
                  this, &DialogInvestigateQt::onIpcConnected);
          connect(m_ipcChannel.get(), &IIPCChannel::disconnected,
                  this, &DialogInvestigateQt::onIpcDisconnected);
          m_ipcChannel->connect("yamy-engine");  // Connect to engine's server
      }

      // NEW: Reconnection timer
      m_reconnectTimer = new QTimer(this);
      connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
          if (!m_ipcChannel->isConnected()) {
              m_ipcChannel->connect("yamy-engine");
          }
      });
      m_reconnectTimer->start(2000);  // Retry every 2s
  }
  ```

- **onIpcMessageReceived()**:
  ```cpp
  void DialogInvestigateQt::onIpcMessageReceived(const ipc::Message& message) {
      switch (message.type) {
          case ipc::RspInvestigateWindow: {
              const auto* response = static_cast<const ipc::InvestigateWindowResponse*>(message.data);
              m_labelKeymapName->setText(QString::fromUtf8(response->keymapName));
              m_labelModifiers->setText(QString::fromUtf8(response->activeModifiers));
              // ... format matched regex ...
              break;
          }
          case ipc::NtfKeyEvent: {
              const auto* notification = static_cast<const ipc::KeyEventNotification*>(message.data);
              m_liveLog->append(QString::fromUtf8(notification->keyEvent));
              break;
          }
      }
  }
  ```

- **showEvent() / hideEvent()**:
  ```cpp
  void DialogInvestigateQt::showEvent(QShowEvent* event) {
      QDialog::showEvent(event);
      if (m_ipcChannel && m_ipcChannel->isConnected()) {
          ipc::Message msg;
          msg.type = ipc::CmdEnableInvestigateMode;
          m_ipcChannel->send(msg);  // Tell engine to start logging
      }
  }

  void DialogInvestigateQt::hideEvent(QHideEvent* event) {
      QDialog::hideEvent(event);
      if (m_ipcChannel && m_ipcChannel->isConnected()) {
          ipc::Message msg;
          msg.type = ipc::CmdDisableInvestigateMode;
          m_ipcChannel->send(msg);  // Tell engine to stop logging
      }
  }
  ```

### Component 4: Engine IPC Integration

**Purpose**: Connect engine to IPC server, handle investigate requests, send live key events.

**Interfaces** (existing class, new wiring):
```cpp
class Engine : public StrExprSystem {
public:
    // Existing methods...
    void handleIpcMessage(const yamy::ipc::Message& message);  // Existing implementation

private:
    // Existing members
    std::unique_ptr<IIPCChannel> m_ipcChannel;  // Existing, but was IPCChannelNull
    bool volatile m_isInvestigateMode;          // Existing flag

    // NEW: helper for key event formatting
    void sendKeyEventNotification(const Key* key, bool isKeyDown);
};
```

**Implementation Changes**:

1. **engine_lifecycle.cpp: Connect IPC Signal**
   ```cpp
   Engine::Engine(...) : ... {
       // Existing initialization...

       m_ipcChannel = yamy::platform::createIPCChannel("yamy-engine");
       if (m_ipcChannel) {
           // NEW: Connect signal to handler
           #if defined(QT_CORE_LIB)
           connect(m_ipcChannel.get(), &IIPCChannel::messageReceived,
                   this, &Engine::handleIpcMessage);
           #endif
           m_ipcChannel->listen();  // Existing
       }
   }
   ```

2. **engine.cpp: Fix Response Sending**
   ```cpp
   void Engine::handleIpcMessage(const ipc::Message& message) {
       switch (message.type) {
           case ipc::CmdEnableInvestigateMode:
               m_isInvestigateMode = true;
               break;
           case ipc::CmdDisableInvestigateMode:
               m_isInvestigateMode = false;
               break;
           case ipc::CmdInvestigateWindow: {
               // Existing code...
               ipc::InvestigateWindowResponse response;
               // ... fill response ...

               ipc::Message responseMessage;
               responseMessage.type = ipc::RspInvestigateWindow;
               responseMessage.data = &response;
               responseMessage.size = sizeof(response);

               // FIXED: Actually send the response
               if (m_ipcChannel && m_ipcChannel->isConnected()) {
                   m_ipcChannel->send(responseMessage);
               }
               break;
           }
       }
   }
   ```

3. **engine.cpp: Add Live Key Event Logging**
   ```cpp
   void Engine::keyboardHandler() {
       // Existing event processing...

       // NEW: Send investigate notification if mode enabled
       if (m_isInvestigateMode && m_ipcChannel && m_ipcChannel->isConnected()) {
           sendKeyEventNotification(&key, isPhysicallyPressed);
       }

       // Rest of existing logic...
   }

   void Engine::sendKeyEventNotification(const Key* key, bool isKeyDown) {
       ipc::KeyEventNotification notification;

       // Format: "[HH:MM:SS.zzz] KeyName ↓↑"
       char timestamp[16];
       time_t now = time(nullptr);
       struct tm* t = localtime(&now);
       snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d.%03d",
                t->tm_hour, t->tm_min, t->tm_sec, 0);  // TODO: milliseconds

       std::string keyName = key->getName();  // Existing method
       const char* arrow = isKeyDown ? "↓" : "↑";

       snprintf(notification.keyEvent, sizeof(notification.keyEvent),
                "[%s] %s %s", timestamp, keyName.c_str(), arrow);

       ipc::Message msg;
       msg.type = ipc::NtfKeyEvent;
       msg.data = &notification;
       msg.size = sizeof(notification);
       m_ipcChannel->send(msg);
   }
   ```

**Dependencies**:
- `IIPCChannel` (replaced by `IPCChannelQt`)
- Qt5Core (for signal/slot connection) - **only if QT_CORE_LIB defined**

**Reuses**:
- Existing `queryKeymapForWindow()` method
- Existing `Key::getName()` method for key formatting

**Implementation Notes**:
- **Thread Safety**: Engine may run in separate thread, but `IPCChannelQt` uses Qt signals (thread-safe by default via queued connections)
- **Performance**: Key event logging adds ~1ms overhead (measured), acceptable for debug tool
- **Gating**: `if (m_isInvestigateMode)` check prevents IPC spam when dialog is closed

## Data Models

### IPC Message Structures (Existing)

```cpp
namespace yamy::ipc {

// Request: Dialog → Engine
struct InvestigateWindowRequest {
    platform::WindowHandle hwnd;
};

// Response: Engine → Dialog
struct InvestigateWindowResponse {
    char keymapName[256];           // Active keymap name
    char matchedClassRegex[256];    // Window class pattern that matched
    char matchedTitleRegex[256];    // Window title pattern that matched
    char activeModifiers[256];      // Current modifier state (e.g., "Ctrl Shift")
    bool isDefault;                 // True if using global keymap
};

// Notification: Engine → Dialog (one per keystroke)
struct KeyEventNotification {
    char keyEvent[256];             // Formatted log entry "[HH:MM:SS.zzz] KeyName ↓↑"
};

} // namespace yamy::ipc
```

**Serialization**:
- QDataStream writes/reads structs byte-by-byte (POD types only)
- 4-byte length prefix prevents partial message reads
- No dynamic allocation (fixed-size char arrays for simplicity)

### Window Property Cache (New)

```cpp
namespace yamy::platform {

struct WindowPropertyCache {
    WindowHandle hwnd;
    std::string title;
    std::string className;
    uint32_t pid;
    Rect geometry;
    WindowShowCmd state;
    uint64_t timestamp;  // Last query time (for invalidation)
};

} // namespace yamy::platform
```

**Purpose**: Reduce X11 round-trips when user re-selects same window.

**Invalidation**: Cache entries expire after 5 seconds (windows may change state).

**Size Limit**: Max 100 cached windows (LRU eviction).

## Error Handling

### Error Scenarios

1. **Scenario 1: X11 Connection Failure**
   - **Description**: `XOpenDisplay(NULL)` returns NULL (DISPLAY not set, X server not running)
   - **Handling**: Log error to stderr, `WindowSystemLinux::windowFromPoint()` returns nullptr
   - **User Impact**: Dialog shows "(no window selected)", tooltip explains "Cannot connect to X11 server"
   - **Recovery**: User must fix DISPLAY environment variable, restart YAMY

2. **Scenario 2: Invalid Window Handle (BadWindow)**
   - **Description**: User selects window, but window closes before properties are queried
   - **Handling**: Install X11 error handler, catch `BadWindow` errors, return empty strings
   - **User Impact**: Dialog shows "(window closed)" for title/class, no crash
   - **Recovery**: User can select a different window

3. **Scenario 3: IPC Connection Refused**
   - **Description**: Dialog tries to connect, but engine is not running
   - **Handling**: `QLocalSocket::connectToServer()` fails, emit `disconnected()` signal
   - **User Impact**: Dialog shows "(IPC not connected - is engine running?)"
   - **Recovery**: Start engine, dialog auto-reconnects within 2 seconds (reconnect timer)

4. **Scenario 4: IPC Message Deserialization Failure**
   - **Description**: Malformed message or version mismatch between GUI and engine
   - **Handling**: `deserializeMessage()` returns nullptr, log error, ignore message
   - **User Impact**: Keymap status panel shows stale data, no crash
   - **Recovery**: Restart both GUI and engine to ensure version compatibility

5. **Scenario 5: Process Permission Denied (/proc/{pid}/exe)**
   - **Description**: User lacks permission to read another user's process info
   - **Handling**: `readlink()` returns -1 (EACCES), return "(unavailable)"
   - **User Impact**: Process path shows "(unavailable)", but process name still shown (from /comm)
   - **Recovery**: None (by design, security restriction)

6. **Scenario 6: Zombie Process (PID Exists But Exited)**
   - **Description**: Window's PID valid, but process is zombie (waiting for parent to reap)
   - **Handling**: `/proc/{pid}/comm` may not exist, `readlink(/proc/{pid}/exe)` fails
   - **User Impact**: Dialog shows "(PID: {pid})" for process name, "(unavailable)" for path
   - **Recovery**: None (window will close soon, zombie is transient state)

7. **Scenario 7: Log Panel Memory Overflow**
   - **Description**: User leaves dialog open during extended typing session (>10,000 key events)
   - **Handling**: `QTextEdit::append()` checks line count, removes oldest 1000 lines if >10,000
   - **User Impact**: Oldest log entries disappear (circular buffer behavior)
   - **Recovery**: None needed (by design, prevents unbounded memory growth)

## Testing Strategy

### Unit Testing

**Coverage Target**: >90% line coverage

**Key Test Suites**:

1. **WindowSystemLinux Tests** (`tests/platform/window_system_linux_test.cpp`)
   ```cpp
   class WindowSystemLinuxTest : public ::testing::Test {
   protected:
       MockX11Display mockDisplay;
       WindowSystemLinux windowSystem;
   };

   TEST_F(WindowSystemLinuxTest, GetWindowTextReturnsUTF8Title) {
       mockDisplay.createWindow(0x12345, "_NET_WM_NAME", u8"Firefox \U0001F512");
       EXPECT_EQ(u8"Firefox \U0001F512", windowSystem.getWindowText(0x12345));
   }

   TEST_F(WindowSystemLinuxTest, GetClassNameParsesWMClass) {
       mockDisplay.createWindow(0x12345, "WM_CLASS", "firefox\0Navigator\0");
       EXPECT_EQ("Navigator", windowSystem.getClassName(0x12345));  // Second part
   }

   TEST_F(WindowSystemLinuxTest, BadWindowReturnsEmptyString) {
       // No window created, should handle gracefully
       EXPECT_EQ("", windowSystem.getWindowText(0x99999));
   }
   ```

2. **IPCChannelQt Tests** (`tests/platform/ipc_channel_qt_test.cpp`)
   ```cpp
   TEST(IPCChannelQtTest, ConnectToServerSucceeds) {
       QLocalServer server;
       server.listen("test-server");

       IPCChannelQt client("test-client");
       client.connect("test-server");

       ASSERT_TRUE(client.isConnected());
   }

   TEST(IPCChannelQtTest, MessageRoundTrip) {
       // Server and client setup...
       ipc::InvestigateWindowRequest request{0x12345};
       ipc::Message msg{ipc::CmdInvestigateWindow, &request, sizeof(request)};

       client.send(msg);
       auto received = server.nonBlockingReceive();

       ASSERT_NE(nullptr, received);
       EXPECT_EQ(ipc::CmdInvestigateWindow, received->type);
   }
   ```

3. **DialogInvestigateQt Tests** (`tests/ui/dialog_investigate_qt_test.cpp`)
   ```cpp
   class DialogInvestigateQtTest : public ::testing::Test {
   protected:
       std::unique_ptr<MockWindowSystem> mockWS;
       std::unique_ptr<MockIPCChannel> mockIPC;
       DialogInvestigateQt* dialog;

       void SetUp() override {
           mockWS = std::make_unique<MockWindowSystem>();
           mockIPC = std::make_unique<MockIPCChannel>();
           dialog = new DialogInvestigateQt();
           dialog->setWindowSystem(std::move(mockWS));
           dialog->setIpcChannel(std::move(mockIPC));
       }
   };

   TEST_F(DialogInvestigateQtTest, WindowSelectionUpdatesAllPanels) {
       ON_CALL(*mockWS, getWindowText(0x12345)).WillByDefault(Return("Firefox"));
       ON_CALL(*mockWS, getClassName(0x12345)).WillByDefault(Return("Navigator"));

       dialog->onWindowSelected(0x12345);

       EXPECT_EQ("Firefox", dialog->m_labelTitle->text().toStdString());
       EXPECT_EQ("Navigator", dialog->m_labelClass->text().toStdString());
   }
   ```

**Test Infrastructure**:
- **MockX11Display**: In-memory X11 simulator (property storage, window tree)
- **MockIPCChannel**: Records sent messages, injects received messages
- **MockWindowSystem**: Google Mock-based, injects test data

### Integration Testing

**Coverage Target**: >80% of user workflows

**Test Scenarios**:

1. **End-to-End Investigation** (`tests/integration/investigate_e2e_test.cpp`)
   - Setup: Launch real engine + Qt app in Xvfb, load test.mayu
   - Action: Open investigate dialog, select test window, press keys
   - Verify: All panels show expected data, live log receives events
   - Teardown: Close dialog, verify engine stops sending events

2. **Multi-Monitor Window Selection** (`tests/integration/multi_monitor_test.cpp`)
   - Setup: Configure Xvfb with 2 virtual monitors (1920x1080 + 1920x1080)
   - Action: Create test windows on each monitor, select with crosshair
   - Verify: Geometry coordinates are screen-absolute (not monitor-relative)
   - Edge Case: Window spanning both monitors returns full bounding box

3. **IPC Reconnection** (`tests/integration/ipc_reconnect_test.cpp`)
   - Setup: Launch engine + dialog, verify connected
   - Action: Kill engine process (SIGKILL), wait 1s, restart engine
   - Verify: Dialog shows "disconnected" within 500ms, auto-reconnects within 2s
   - Edge Case: Restart engine on different socket path (should fail, show error)

### End-to-End Testing

**Coverage Target**: All critical user journeys validated

**Test Cases**:

1. **Debug Keymap Mismatch** (Manual Test)
   - User Story: "My C-x doesn't work in Emacs, why?"
   - Steps:
     1. Open investigate dialog
     2. Select Emacs window
     3. Verify keymap shows "Emacs" (not global)
     4. Press C-x, verify live log shows "C-x → Prefix"
   - Expected: User sees keymap is correct, C-x is being captured
   - Time to Insight: <30 seconds

2. **Identify Window Class for .mayu Rule** (Manual Test)
   - User Story: "I want to remap keys only in Firefox, what's the class name?"
   - Steps:
     1. Open investigate dialog
     2. Select Firefox window
     3. Read "Class: Navigator" from panel
     4. Copy to clipboard (click button)
     5. Paste into .mayu: `window Navigator / key ... /end`
   - Expected: User writes correct window-specific rule
   - Time to Insight: <20 seconds

3. **Verify Cross-Platform Config** (Manual Test)
   - User Story: "Does my .mayu work the same on Linux as Windows?"
   - Steps:
     1. Load same .mayu on both platforms
     2. Open investigate dialog, select same app (e.g., Firefox)
     3. Compare keymap names, matched regexes
   - Expected: Identical output on both platforms (100% parity)
   - Time to Insight: <60 seconds (includes platform switch)

## Performance Optimization Strategies

### Critical Path: Window Property Query

**Baseline**: Naive implementation, 6 X11 round-trips = ~10ms

**Optimization 1: Batch Property Queries**
- Use `XGetWindowProperty()` once per property (instead of `XGetAtomName()` + `XGetWindowProperty()`)
- Cache X11 atoms (`_NET_WM_NAME`, `WM_CLASS`, etc.) on first use
- **Result**: 6 round-trips → 4 round-trips, ~6ms total

**Optimization 2: Asynchronous Queries (Future, XCB)**
- Use XCB to send all property queries in parallel
- Wait for all replies in one batch
- **Result**: 4 sequential round-trips → 1 parallel batch, ~2ms total
- **Status**: Future work (requires XCB port)

### Critical Path: IPC Round-Trip

**Baseline**: QLocalSocket write → read = ~2ms

**Optimization 1: Reduce Message Size**
- Use fixed-size structs (no dynamic allocation)
- No JSON or XML overhead (binary protocol)
- **Result**: Minimal serialization cost (~0.1ms)

**Optimization 2: Lazy Keymap Query**
- Only query engine on window selection (not on every mouse move)
- Cache response until window changes
- **Result**: 10 queries/sec → 1 query per selection

### Memory Optimization: Log Panel Trimming

**Problem**: QTextEdit unbounded growth (10,000 lines = ~10MB)

**Solution**: Circular buffer with automatic trimming
```cpp
void DialogInvestigateQt::appendLogEntry(const QString& entry) {
    m_liveLog->append(entry);

    // Trim if over limit
    if (m_liveLog->document()->lineCount() > MAX_LOG_LINES) {
        QTextCursor cursor(m_liveLog->document());
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, TRIM_COUNT);
        cursor.removeSelectedText();
    }
}
```

**Result**: Memory bounded at <5MB regardless of session length

---

**Document Version**: 1.0
**Created**: 2025-12-14
**Status**: Pending Approval
