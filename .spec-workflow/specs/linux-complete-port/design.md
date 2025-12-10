# Design: Linux Complete Port

**Status**: Draft
**Created**: 2025-12-10
**Dependencies**: requirements.md
**Implements**: All user stories (US-1 through US-12)

---

## Design Overview

This document describes the technical design for achieving complete Windows feature parity on Linux. The design is organized into 6 tracks executed sequentially, with maximum parallelism within each track.

### Design Principles

1. **Platform Abstraction First** - No Linux code touches Windows types
2. **Interface Segregation** - Small, focused interfaces per subsystem
3. **Dependency Injection** - All platform dependencies injected via factory functions
4. **Zero Breaking Changes** - Existing .mayu files work without modification
5. **Performance Non-Negotiable** - <1ms latency maintained throughout refactoring

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    UI Layer (Qt5/Win32)                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Tray Icon    │  │ Settings Dlg │  │ Investigate  │  │
│  │ (Platform)   │  │ (Platform)   │  │ (Platform)   │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
└─────────┼──────────────────┼──────────────────┼─────────┘
          │                  │                  │
          v                  v                  v
┌─────────────────────────────────────────────────────────┐
│              Core Engine (Platform-Agnostic)            │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Engine Orchestrator                             │   │
│  │  - State machine (Stopped/Running/Error)         │   │
│  │  - Configuration loader                          │   │
│  │  - Notification dispatcher                       │   │
│  └────┬─────────────────────────────────────────┬───┘   │
│       │                                         │       │
│  ┌────v──────────┐  ┌─────────────┐  ┌─────────v────┐  │
│  │ Input Mapper  │  │ Window Mgr  │  │ Function Sys │  │
│  │ (60+ cmds)    │  │ (Focus)     │  │ (Actions)    │  │
│  └───────────────┘  └─────────────┘  └──────────────┘  │
└─────────┬─────────────────┬─────────────────┬───────────┘
          │                 │                 │
          v                 v                 v
┌─────────────────────────────────────────────────────────┐
│         Platform Abstraction Layer (Interfaces)         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ IInputHook   │  │ IWindowSys   │  │ IIPCChannel  │  │
│  │ IInputInject │  │ IInputDriver │  │ IFileSystem  │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
└─────────┼──────────────────┼──────────────────┼─────────┘
          │                  │                  │
    ┌─────┴─────┐      ┌─────┴─────┐      ┌────┴────┐
    │  Windows  │      │   Linux   │      │  macOS  │
    │ (Win32)   │      │(X11/evdev)│      │ (Future)│
    └───────────┘      └───────────┘      └─────────┘
```

---

## Track 1: Core Refactoring

**Goal**: Remove all Windows-specific types from core engine (FR-1)
**Duration**: 80 hours | **Tasks**: 60 | **Parallelism**: 3 batches

### 1.1 Type System Migration

#### 1.1.1 String Type Unification

**Problem**: Core engine uses `tstring` (typedef for `std::wstring` or `std::string`), causing link errors on Linux.

**Design Decision**: Standardize on UTF-8 `std::string` everywhere.

**Rationale**:
- Linux ecosystem is UTF-8 native (X11, evdev, Qt5)
- Modern C++ has `std::filesystem::u8path` for UTF-8
- Windows `WideCharToMultiByte` available when interfacing with Win32

**Changes**:
```cpp
// Before (Windows-centric)
typedef std::basic_string<TCHAR> tstring;
#define _T(x) L##x  // Wide strings on Windows

// After (Platform-agnostic)
using String = std::string;  // Always UTF-8
// No preprocessor macros
```

**Affected Files** (10 files):
- `src/utils/stringtool.h/cpp` - Remove all `tstring` typedefs
- `src/core/settings/config_store.h` - Remove `tstring` overloads
- `src/core/settings/errormessage.h` - Convert to `std::string`
- `src/core/input/keyboard.cpp` - String conversions
- `src/core/engine/engine.cpp` - Remove `_T()` macros
- `src/core/window/layoutmanager.cpp` - Window text handling
- `src/core/functions/function.cpp` - Command name strings
- `src/app/mayu.cpp` - Win32 bridge conversions
- `src/ui/dlgsetting.cpp` - ListView string conversions
- `src/ui/qt/dialog_settings_qt.cpp` - QSettings UTF-8 keys

**API Example**:
```cpp
// Old Windows-only API
class ConfigStore {
    bool read(const tstring& key, tstring* value);
    bool write(const tstring& key, const tstring& value);
};

// New platform-agnostic API
class ConfigStore {
    bool read(const std::string& key, std::string* value);
    bool write(const std::string& key, const std::string& value);

private:
    #ifdef _WIN32
        std::wstring toWide(const std::string& utf8);  // Win32 bridge
        std::string toUtf8(const std::wstring& wide);
    #endif
};
```

#### 1.1.2 Window Handle Abstraction

**Problem**: `HWND` (Windows handle) used throughout engine for window identification.

**Design Decision**: Platform-agnostic `WindowHandle` type.

**Implementation**:
```cpp
// src/platform/types.h
namespace yamy::platform {

#ifdef _WIN32
    using WindowHandle = HWND;
#elif __linux__
    struct WindowHandle {
        uint64_t xid;      // X11 Window ID
        uint32_t pid;      // Process ID (for Wayland)

        bool operator==(const WindowHandle& other) const {
            return xid == other.xid && pid == other.pid;
        }
        bool isValid() const { return xid != 0; }
    };
#endif

    constexpr WindowHandle NULL_WINDOW = {};

}  // namespace yamy::platform
```

**Affected Components**:
- `src/core/window/target.h/cpp` - 19 HWND references
- `src/core/window/focus.h/cpp` - 15 HWND references
- `src/core/window/layoutmanager.h/cpp` - 12 HWND references
- `src/core/engine/engine.h` - 8 HWND parameters
- `src/utils/msgstream.h` - 10 HWND references

**Migration Strategy**:
```cpp
// Step 1: Find all HWND usages
//   grep -r "HWND" src/core/ | wc -l  → 64 occurrences

// Step 2: Replace type declarations
//   HWND hwnd; → WindowHandle hwnd;

// Step 3: Replace null checks
//   if (hwnd == NULL) → if (!hwnd.isValid())

// Step 4: Replace API calls (next section)
```

### 1.2 Platform API Abstraction

#### 1.2.1 Window System Interface

**Problem**: Direct Win32 calls (`GetForegroundWindow`, `GetWindowRect`, `SetForegroundWindow`) in core engine.

**Design**: `IWindowSystem` interface with platform-specific implementations.

**Interface Design**:
```cpp
// src/platform/window_system.h
namespace yamy::platform {

struct Rect {
    int32_t left, top, right, bottom;
    int32_t width() const { return right - left; }
    int32_t height() const { return bottom - top; }
};

enum class ShowCommand {
    Hide = 0,
    Normal = 1,
    Minimized = 2,
    Maximized = 3,
    ShowNoActivate = 4,
    Show = 5,
    Minimize = 6,
    ShowMinNoActive = 7,
    ShowNA = 8,
    Restore = 9,
    ShowDefault = 10,
    ForceMinimize = 11
};

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    // Window queries
    virtual WindowHandle getForegroundWindow() = 0;
    virtual WindowHandle getDesktopWindow() = 0;
    virtual std::string getWindowText(WindowHandle hwnd) = 0;
    virtual std::string getClassName(WindowHandle hwnd) = 0;
    virtual Rect getWindowRect(WindowHandle hwnd) = 0;
    virtual bool isWindow(WindowHandle hwnd) = 0;
    virtual bool isWindowVisible(WindowHandle hwnd) = 0;

    // Window manipulation
    virtual void setForegroundWindow(WindowHandle hwnd) = 0;
    virtual void showWindow(WindowHandle hwnd, ShowCommand cmd) = 0;
    virtual void moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
    virtual void closeWindow(WindowHandle hwnd) = 0;

    // Caret/cursor (for &WindowCaret function)
    virtual bool getCaretPos(int32_t* x, int32_t* y) = 0;
    virtual WindowHandle getCaretWindow() = 0;
};

// Factory function (defined per-platform)
IWindowSystem* createWindowSystem();

}  // namespace yamy::platform
```

**Windows Implementation**:
```cpp
// src/platform/windows/window_system_win32.cpp
class WindowSystemWin32 : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override {
        return ::GetForegroundWindow();
    }

    std::string getWindowText(WindowHandle hwnd) override {
        wchar_t buf[256];
        ::GetWindowTextW(hwnd, buf, 256);
        return utf8::fromWide(buf);  // UTF-8 conversion
    }

    void moveWindow(WindowHandle hwnd, const Rect& rect) override {
        ::MoveWindow(hwnd, rect.left, rect.top, rect.width(), rect.height(), TRUE);
    }

    // ... other methods
};

IWindowSystem* createWindowSystem() {
    return new WindowSystemWin32();
}
```

**Linux Implementation**:
```cpp
// src/platform/linux/window_system_x11.cpp
#include <X11/Xlib.h>
#include <X11/Xatom.h>

class WindowSystemX11 : public IWindowSystem {
public:
    WindowSystemX11() {
        display_ = XOpenDisplay(nullptr);
        root_ = DefaultRootWindow(display_);
        // Cache atom IDs
        net_active_window_ = XInternAtom(display_, "_NET_ACTIVE_WINDOW", False);
        net_wm_name_ = XInternAtom(display_, "_NET_WM_NAME", False);
    }

    WindowHandle getForegroundWindow() override {
        Window focused;
        int revert;
        XGetInputFocus(display_, &focused, &revert);
        return WindowHandle{focused, 0};
    }

    std::string getWindowText(WindowHandle hwnd) override {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char* prop = nullptr;

        XGetWindowProperty(display_, hwnd.xid, net_wm_name_,
                          0, 1024, False, AnyPropertyType,
                          &actual_type, &actual_format, &nitems,
                          &bytes_after, &prop);

        std::string result((char*)prop);
        XFree(prop);
        return result;  // Already UTF-8 on X11
    }

    void moveWindow(WindowHandle hwnd, const Rect& rect) override {
        XMoveResizeWindow(display_, hwnd.xid,
                         rect.left, rect.top, rect.width(), rect.height());
        XFlush(display_);
    }

private:
    Display* display_;
    Window root_;
    Atom net_active_window_, net_wm_name_;
};

IWindowSystem* createWindowSystem() {
    return new WindowSystemX11();
}
```

**Integration**:
```cpp
// src/core/engine/engine.cpp
class Engine {
public:
    Engine(IWindowSystem* windowSys, IInputHook* inputHook)
        : m_windowSystem(windowSys), m_inputHook(inputHook) {}

    void handleWindowCommand(ShowCommand cmd) {
        WindowHandle hwnd = m_windowSystem->getForegroundWindow();
        m_windowSystem->showWindow(hwnd, cmd);
    }

private:
    IWindowSystem* m_windowSystem;
    IInputHook* m_inputHook;
};

// src/app/main_qt.cpp (Linux)
int main(int argc, char** argv) {
    IWindowSystem* windowSys = yamy::platform::createWindowSystem();
    IInputHook* inputHook = yamy::platform::createInputHook();

    Engine engine(windowSys, inputHook);
    engine.load("config.mayu");

    // ...
}
```

#### 1.2.2 IPC Abstraction

**Problem**: `PostMessage()` (Windows mailslots) used for engine→GUI communication.

**Design**: `IIPCChannel` interface with queue-based messaging.

**Interface**:
```cpp
// src/platform/ipc.h
namespace yamy::platform {

enum class MessageType : uint32_t {
    // Engine → GUI notifications
    EngineStarted = 0x1000,
    EngineStopped = 0x1001,
    EngineError = 0x1002,
    ConfigLoaded = 0x1003,
    ConfigError = 0x1004,
    LogMessage = 0x1005,
    KeyEvent = 0x1006,

    // GUI → Engine commands
    CmdReload = 0x2000,
    CmdStop = 0x2001,
    CmdInvestigate = 0x2002,
    CmdToggleEngine = 0x2003,

    // Custom range
    UserDefined = 0x3000
};

struct Message {
    MessageType type;
    uint32_t wparam;
    uint64_t lparam;
    std::string data;  // For complex payloads
};

class IIPCChannel {
public:
    virtual ~IIPCChannel() = default;

    // Sender interface
    virtual bool send(const Message& msg) = 0;
    virtual bool sendAsync(const Message& msg) = 0;

    // Receiver interface
    virtual bool poll(Message* outMsg) = 0;
    virtual void setCallback(std::function<void(const Message&)> callback) = 0;

    // Lifecycle
    virtual bool connect(const std::string& channelName) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
};

IIPCChannel* createIPCChannel();

}  // namespace yamy::platform
```

**Windows Implementation**:
```cpp
// src/platform/windows/ipc_mailslot.cpp
class IPCMailslot : public IIPCChannel {
public:
    bool send(const Message& msg) override {
        HWND hwnd = FindWindow(MAYU_WINDOW_CLASS, nullptr);
        if (!hwnd) return false;

        ::PostMessage(hwnd, static_cast<UINT>(msg.type),
                     msg.wparam, msg.lparam);
        return true;
    }

    bool poll(Message* outMsg) override {
        MSG msg;
        if (::PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
            outMsg->type = static_cast<MessageType>(msg.message);
            outMsg->wparam = msg.wParam;
            outMsg->lparam = msg.lParam;
            return true;
        }
        return false;
    }

private:
    HWND m_hwnd;
};
```

**Linux Implementation**:
```cpp
// src/platform/linux/ipc_socket.cpp
#include <sys/socket.h>
#include <sys/un.h>

class IPCUnixSocket : public IIPCChannel {
public:
    bool connect(const std::string& channelName) override {
        socket_fd_ = socket(AF_UNIX, SOCK_DGRAM, 0);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path),
                "/tmp/yamy-%d.sock", getuid());

        bind(socket_fd_, (sockaddr*)&addr, sizeof(addr));
        return true;
    }

    bool send(const Message& msg) override {
        // Serialize message to buffer
        std::vector<uint8_t> buf(16 + msg.data.size());
        memcpy(&buf[0], &msg.type, 4);
        memcpy(&buf[4], &msg.wparam, 4);
        memcpy(&buf[8], &msg.lparam, 8);
        memcpy(&buf[16], msg.data.data(), msg.data.size());

        // Send to peer socket
        sockaddr_un peer_addr{};
        peer_addr.sun_family = AF_UNIX;
        snprintf(peer_addr.sun_path, sizeof(peer_addr.sun_path),
                "/tmp/yamy-%d-gui.sock", getuid());

        sendto(socket_fd_, buf.data(), buf.size(), 0,
              (sockaddr*)&peer_addr, sizeof(peer_addr));
        return true;
    }

    bool poll(Message* outMsg) override {
        std::vector<uint8_t> buf(4096);
        ssize_t n = recv(socket_fd_, buf.data(), buf.size(), MSG_DONTWAIT);
        if (n < 16) return false;

        memcpy(&outMsg->type, &buf[0], 4);
        memcpy(&outMsg->wparam, &buf[4], 4);
        memcpy(&outMsg->lparam, &buf[8], 8);
        outMsg->data.assign((char*)&buf[16], n - 16);
        return true;
    }

private:
    int socket_fd_;
};

IIPCChannel* createIPCChannel() {
    return new IPCUnixSocket();
}
```

**Qt Integration** (Linux GUI):
```cpp
// src/ui/qt/tray_icon_qt.cpp
class TrayIconQt : public QSystemTrayIcon {
public:
    TrayIconQt() {
        ipc_ = yamy::platform::createIPCChannel();
        ipc_->connect("yamy-gui");

        // Use QSocketNotifier to integrate with Qt event loop
        int fd = ipc_->getFileDescriptor();
        notifier_ = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(notifier_, &QSocketNotifier::activated,
                this, &TrayIconQt::onIPCMessage);
    }

    void onIPCMessage() {
        yamy::platform::Message msg;
        while (ipc_->poll(&msg)) {
            handleEngineMessage(msg);
        }
    }

    void handleEngineMessage(const yamy::platform::Message& msg) {
        switch (msg.type) {
            case MessageType::EngineStarted:
                setToolTip("YAMY - Running");
                break;
            case MessageType::EngineStopped:
                setToolTip("YAMY - Stopped");
                break;
            case MessageType::LogMessage:
                emit logReceived(QString::fromStdString(msg.data));
                break;
        }
    }

private:
    IIPCChannel* ipc_;
    QSocketNotifier* notifier_;
};
```

### 1.3 Batched Execution Plan

**Batch 1: Foundation** (32 tasks, ALL PARALLEL)
- Group A: String cleanup (10 tasks)
- Group B: SW_* → ShowCommand (12 tasks)
- Group C: Message constants (10 tasks)

**Batch 2: Window System** (15 tasks, ALL PARALLEL after Batch 1)
- Group D: HWND → WindowHandle (8 tasks)
- Group E: IPC abstraction (7 tasks)

**Batch 3: Integration** (13 tasks, ALL PARALLEL after Batch 2)
- Group F: Window functions integration (13 tasks)

**Verification**: After each batch, run:
```bash
mkdir build-linux && cd build-linux
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)  # Should compile without Windows headers
```

---

## Track 2: Configuration Management

**Goal**: Multi-config support with GUI editor (FR-2)
**Duration**: 45 hours | **Tasks**: 15

### 2.1 Data Model

**Configuration Structure**:
```cpp
// src/core/settings/mayu_configuration.h
namespace yamy {

struct MayuConfiguration {
    std::string name;              // Display name
    std::string mayuPath;          // Absolute path to .mayu file
    std::vector<std::string> symbols;  // Preprocessor defines
    bool isActive;                 // Currently loaded

    // Metadata
    std::string description;
    time_t lastModified;
    time_t lastUsed;

    bool isValid() const {
        return !name.empty() && std::filesystem::exists(mayuPath);
    }
};

class ConfigurationManager {
public:
    // CRUD operations
    bool addConfiguration(const MayuConfiguration& config);
    bool removeConfiguration(const std::string& name);
    bool updateConfiguration(const std::string& name, const MayuConfiguration& newConfig);
    std::vector<MayuConfiguration> getAllConfigurations() const;

    // Activation
    bool setActiveConfiguration(const std::string& name);
    MayuConfiguration getActiveConfiguration() const;

    // Persistence
    bool save();
    bool load();

private:
    std::vector<MayuConfiguration> m_configs;
    std::string m_activeConfigName;
    platform::IFileSystem* m_fileSystem;
};

}  // namespace yamy
```

**Storage Format** (QSettings on Linux, Registry on Windows):
```ini
# ~/.config/yamy/configurations.ini
[general]
activeConfig=work

[configurations/work]
name=Work Layout
mayuPath=/home/user/.config/yamy/work.mayu
symbols=OFFICE_MODE,ENABLE_VIM
description=9-5 office configuration

[configurations/gaming]
name=Gaming
mayuPath=/home/user/.config/yamy/gaming.mayu
symbols=GAMING_MODE
description=FPS-optimized
```

### 2.2 GUI Components

#### 2.2.1 Settings Dialog Enhancements

**Current State** (Phase 8):
- Basic file list with Add/Remove buttons
- No symbol editor
- No multi-select

**Enhanced Design**:
```
┌─ Edit Setting ─────────────────────────────────────┐
│ Configurations:                                    │
│ ┌────────────────────────────────────────────────┐ │
│ │ ● Work Layout          work.mayu     [Edit]    │ │
│ │   Gaming               gaming.mayu   [Edit]    │ │
│ │   Writing              writing.mayu  [Edit]    │ │
│ └────────────────────────────────────────────────┘ │
│ [Add New...]  [Duplicate]  [Remove]                │
│                                                    │
│ Selected Configuration: Work Layout                │
│ ┌────────────────────────────────────────────────┐ │
│ │ Name: [Work Layout___________________]          │ │
│ │ File: [/home/user/.mayu] [Browse...]            │ │
│ │                                                 │ │
│ │ Preprocessor Symbols:                           │ │
│ │ ┌─────────────────────────────────────────────┐ │ │
│ │ │ OFFICE_MODE                    [Remove]     │ │ │
│ │ │ ENABLE_VIM                     [Remove]     │ │ │
│ │ └─────────────────────────────────────────────┘ │ │
│ │ [Add Symbol...]                                 │ │
│ │                                                 │ │
│ │ Description:                                    │ │
│ │ ┌─────────────────────────────────────────────┐ │ │
│ │ │ 9-5 office configuration with Emacs         │ │ │
│ │ │ bindings and window management              │ │ │
│ │ └─────────────────────────────────────────────┘ │ │
│ └────────────────────────────────────────────────┘ │
│                                       [OK] [Cancel] │
└────────────────────────────────────────────────────┘
```

**Implementation**:
```cpp
// src/ui/qt/dialog_settings_qt.cpp
class DialogSettingsQt : public QDialog {
public:
    DialogSettingsQt(ConfigurationManager* configMgr, QWidget* parent = nullptr)
        : QDialog(parent), m_configMgr(configMgr) {
        setupUI();
        loadConfigurations();
    }

private:
    void setupUI() {
        auto* layout = new QVBoxLayout(this);

        // Configuration list
        m_configList = new QListWidget(this);
        connect(m_configList, &QListWidget::currentRowChanged,
                this, &DialogSettingsQt::onConfigSelected);

        // Buttons
        auto* btnLayout = new QHBoxLayout();
        auto* btnAdd = new QPushButton("Add New...", this);
        auto* btnDuplicate = new QPushButton("Duplicate", this);
        auto* btnRemove = new QPushButton("Remove", this);

        connect(btnAdd, &QPushButton::clicked, this, &DialogSettingsQt::onAddConfig);
        connect(btnDuplicate, &QPushButton::clicked, this, &DialogSettingsQt::onDuplicateConfig);
        connect(btnRemove, &QPushButton::clicked, this, &DialogSettingsQt::onRemoveConfig);

        btnLayout->addWidget(btnAdd);
        btnLayout->addWidget(btnDuplicate);
        btnLayout->addWidget(btnRemove);

        // Configuration editor
        m_editorGroup = new QGroupBox("Selected Configuration", this);
        auto* editorLayout = new QFormLayout(m_editorGroup);

        m_nameEdit = new QLineEdit(this);
        m_pathEdit = new QLineEdit(this);
        auto* btnBrowse = new QPushButton("Browse...", this);
        connect(btnBrowse, &QPushButton::clicked, this, &DialogSettingsQt::onBrowseMayu);

        auto* pathLayout = new QHBoxLayout();
        pathLayout->addWidget(m_pathEdit);
        pathLayout->addWidget(btnBrowse);

        // Symbol editor
        m_symbolList = new QListWidget(this);
        auto* btnAddSymbol = new QPushButton("Add Symbol...", this);
        auto* btnRemoveSymbol = new QPushButton("Remove", this);
        connect(btnAddSymbol, &QPushButton::clicked, this, &DialogSettingsQt::onAddSymbol);
        connect(btnRemoveSymbol, &QPushButton::clicked, this, &DialogSettingsQt::onRemoveSymbol);

        m_descEdit = new QTextEdit(this);
        m_descEdit->setMaximumHeight(80);

        editorLayout->addRow("Name:", m_nameEdit);
        editorLayout->addRow("File:", pathLayout);
        editorLayout->addRow("Symbols:", m_symbolList);
        editorLayout->addRow("", btnAddSymbol);
        editorLayout->addRow("Description:", m_descEdit);

        layout->addWidget(m_configList);
        layout->addLayout(btnLayout);
        layout->addWidget(m_editorGroup);

        auto* buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogSettingsQt::onOk);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }

    void loadConfigurations() {
        m_configList->clear();

        auto configs = m_configMgr->getAllConfigurations();
        for (const auto& config : configs) {
            auto* item = new QListWidgetItem(
                QString::fromStdString(config.name), m_configList);

            // Radio button for active config
            if (config.isActive) {
                item->setIcon(QIcon::fromTheme("emblem-default"));
            }

            item->setData(Qt::UserRole, QString::fromStdString(config.name));
            m_configList->addItem(item);
        }
    }

    void onConfigSelected(int row) {
        if (row < 0) return;

        auto* item = m_configList->item(row);
        std::string name = item->data(Qt::UserRole).toString().toStdString();

        auto configs = m_configMgr->getAllConfigurations();
        auto it = std::find_if(configs.begin(), configs.end(),
            [&](const MayuConfiguration& c) { return c.name == name; });

        if (it != configs.end()) {
            m_nameEdit->setText(QString::fromStdString(it->name));
            m_pathEdit->setText(QString::fromStdString(it->mayuPath));
            m_descEdit->setPlainText(QString::fromStdString(it->description));

            m_symbolList->clear();
            for (const auto& symbol : it->symbols) {
                m_symbolList->addItem(QString::fromStdString(symbol));
            }
        }
    }

    void onAddSymbol() {
        bool ok;
        QString symbol = QInputDialog::getText(this, "Add Symbol",
            "Symbol name:", QLineEdit::Normal, "", &ok);

        if (ok && !symbol.isEmpty()) {
            m_symbolList->addItem(symbol);
        }
    }

    void onOk() {
        // Save all changes back to ConfigurationManager
        // ... (update logic)
        accept();
    }

private:
    ConfigurationManager* m_configMgr;
    QListWidget* m_configList;
    QGroupBox* m_editorGroup;
    QLineEdit* m_nameEdit;
    QLineEdit* m_pathEdit;
    QListWidget* m_symbolList;
    QTextEdit* m_descEdit;
};
```

#### 2.2.2 Reload Submenu

**Design**:
```
Tray Icon Context Menu:
├─ Version
├─ Reload ▶
│  ├─ ● Work Layout
│  ├─   Gaming
│  ├─   Writing
│  ├─ ──────────
│  └─ Manage Configurations...
├─ Edit Setting...
├─ Investigate
├─ Task Tray
├─ Log
└─ Exit
```

**Implementation**:
```cpp
// src/ui/qt/tray_icon_qt.cpp
void TrayIconQt::buildReloadMenu() {
    if (!m_reloadMenu) {
        m_reloadMenu = new QMenu("Reload");
        m_contextMenu->insertMenu(m_contextMenu->actions()[1], m_reloadMenu);
    }

    m_reloadMenu->clear();

    // Get all configurations
    auto configs = m_configMgr->getAllConfigurations();
    auto activeConfig = m_configMgr->getActiveConfiguration();

    // Add configuration actions
    for (const auto& config : configs) {
        QAction* action = m_reloadMenu->addAction(
            QString::fromStdString(config.name));

        action->setCheckable(true);
        action->setChecked(config.name == activeConfig.name);

        connect(action, &QAction::triggered, [this, name = config.name]() {
            onReloadConfiguration(name);
        });
    }

    m_reloadMenu->addSeparator();

    QAction* manageAction = m_reloadMenu->addAction("Manage Configurations...");
    connect(manageAction, &QAction::triggered, this, &TrayIconQt::onEditSetting);
}

void TrayIconQt::onReloadConfiguration(const std::string& name) {
    // Send reload command to engine
    yamy::platform::Message msg;
    msg.type = yamy::platform::MessageType::CmdReload;
    msg.data = name;

    m_ipc->send(msg);

    // Update checkmarks
    buildReloadMenu();
}
```

### 2.3 Engine Integration

**Configuration Loading Flow**:
```
GUI: User selects config from Reload submenu
  ↓
GUI: Send IPC message {CmdReload, configName}
  ↓
Engine: Receive message
  ↓
Engine: configMgr->setActiveConfiguration(configName)
  ↓
Engine: config = configMgr->getActiveConfiguration()
  ↓
Engine: parser->parse(config.mayuPath, config.symbols)
  ↓
Engine: Apply new key mappings
  ↓
Engine: Send IPC message {ConfigLoaded, configName}
  ↓
GUI: Update tray tooltip "YAMY - Work Layout"
```

**Implementation**:
```cpp
// src/core/engine/engine.cpp
void Engine::handleIPCMessage(const platform::Message& msg) {
    switch (msg.type) {
        case platform::MessageType::CmdReload: {
            std::string configName = msg.data;
            if (m_configMgr->setActiveConfiguration(configName)) {
                auto config = m_configMgr->getActiveConfiguration();
                reload(config);

                // Notify GUI
                platform::Message response;
                response.type = platform::MessageType::ConfigLoaded;
                response.data = configName;
                m_ipc->send(response);
            } else {
                // Error
                platform::Message response;
                response.type = platform::MessageType::ConfigError;
                response.data = "Configuration not found: " + configName;
                m_ipc->send(response);
            }
            break;
        }
        // ... other commands
    }
}

void Engine::reload(const MayuConfiguration& config) {
    // Stop current engine
    stop();

    // Parse new configuration
    m_parser = std::make_unique<MayuParser>();
    for (const auto& symbol : config.symbols) {
        m_parser->define(symbol, "1");
    }

    if (!m_parser->parse(config.mayuPath)) {
        throw std::runtime_error("Failed to parse: " + config.mayuPath);
    }

    // Rebuild keymaps
    m_keymaps = m_parser->getKeymaps();

    // Restart engine
    start();
}
```

---

## Track 3: Investigate Dialog

**Goal**: Full window inspector with crosshair (FR-3)
**Duration**: 60 hours | **Tasks**: 18

### 3.1 Component Architecture

**Dialog Structure**:
```
InvestigateDialog (QDialog)
├─ CrosshairWidget (Custom QWidget for drag interaction)
├─ WindowInfoPanel (QGroupBox)
│  ├─ Window handle (hex)
│  ├─ Window text (title)
│  ├─ Class name
│  ├─ Process name
│  ├─ Geometry (x, y, w, h)
│  └─ State (visible, focused, topmost)
├─ KeymapStatusPanel (QGroupBox)
│  ├─ Matched window regex
│  ├─ Active keymap name
│  └─ Effective modifiers
├─ LiveLogPanel (QTextEdit)
│  └─ Real-time key events from engine
└─ Buttons (QDialogButtonBox)
   └─ Close
```

### 3.2 Crosshair Implementation

**Approach**: Transparent overlay window with custom cursor.

**Linux X11 Implementation**:
```cpp
// src/ui/qt/crosshair_widget_qt.cpp
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

class CrosshairWidget : public QWidget {
public:
    CrosshairWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground);
        setMouseTracking(true);

        // Load crosshair cursor
        Display* display = QX11Info::display();
        Cursor cursor = XCreateFontCursor(display, XC_crosshair);
        setCursor(QCursor(Qt::CrossCursor));

        // Make window click-through but still receive mouse events
        XSetWindowAttributes attrs;
        attrs.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
        XChangeWindowAttributes(display, winId(), CWEventMask, &attrs);
    }

    void activate() {
        showFullScreen();
        grabMouse();
        setFocus();
    }

    void deactivate() {
        releaseMouse();
        hide();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        // Draw crosshair lines
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QPoint center = mapFromGlobal(QCursor::pos());

        // Vertical line
        painter.setPen(QPen(QColor(255, 0, 0, 200), 2));
        painter.drawLine(center.x(), 0, center.x(), height());

        // Horizontal line
        painter.drawLine(0, center.y(), width(), center.y());

        // Center dot
        painter.setBrush(QBrush(QColor(255, 0, 0)));
        painter.drawEllipse(center, 4, 4);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            // Get window under cursor
            WindowHandle hwnd = getWindowAtCursor();
            emit windowSelected(hwnd);
            deactivate();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        update();  // Redraw crosshair
    }

signals:
    void windowSelected(WindowHandle hwnd);

private:
    WindowHandle getWindowAtCursor() {
        Display* display = QX11Info::display();
        Window root = DefaultRootWindow(display);

        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        Window child;

        XQueryPointer(display, root, &root, &child,
                     &root_x, &root_y, &win_x, &win_y, &mask);

        // Traverse to leaf window
        Window target = child;
        while (child != None) {
            target = child;
            XQueryPointer(display, target, &root, &child,
                         &root_x, &root_y, &win_x, &win_y, &mask);
        }

        return WindowHandle{target, 0};
    }
};
```

### 3.3 Window Information Panel

**Data Source**: `IWindowSystem` interface + `/proc` filesystem.

**Implementation**:
```cpp
// src/ui/qt/dialog_investigate_qt.cpp
class DialogInvestigateQt : public QDialog {
public:
    void updateWindowInfo(WindowHandle hwnd) {
        auto* windowSys = yamy::platform::createWindowSystem();

        // Basic window properties
        std::string title = windowSys->getWindowText(hwnd);
        std::string className = windowSys->getClassName(hwnd);
        auto rect = windowSys->getWindowRect(hwnd);
        bool visible = windowSys->isWindowVisible(hwnd);

        // Process information (Linux-specific)
        std::string processName = getProcessName(hwnd.pid);
        std::string processPath = getProcessPath(hwnd.pid);

        // Update UI
        m_handleLabel->setText(QString("0x%1").arg(hwnd.xid, 0, 16));
        m_titleLabel->setText(QString::fromStdString(title));
        m_classLabel->setText(QString::fromStdString(className));
        m_processLabel->setText(QString::fromStdString(processName));
        m_geometryLabel->setText(QString("(%1, %2) %3x%4")
            .arg(rect.left).arg(rect.top).arg(rect.width()).arg(rect.height()));
        m_stateLabel->setText(visible ? "Visible" : "Hidden");

        // Query engine for keymap status
        queryKeymapStatus(hwnd);

        delete windowSys;
    }

private:
    std::string getProcessName(uint32_t pid) {
        std::ifstream comm("/proc/" + std::to_string(pid) + "/comm");
        std::string name;
        std::getline(comm, name);
        return name;
    }

    std::string getProcessPath(uint32_t pid) {
        char path[PATH_MAX];
        std::string exePath = "/proc/" + std::to_string(pid) + "/exe";
        ssize_t len = readlink(exePath.c_str(), path, sizeof(path) - 1);
        if (len != -1) {
            path[len] = '\0';
            return std::string(path);
        }
        return "";
    }

    void queryKeymapStatus(WindowHandle hwnd) {
        // Send IPC request to engine
        yamy::platform::Message msg;
        msg.type = yamy::platform::MessageType::CmdInvestigate;
        msg.lparam = hwnd.xid;

        m_ipc->send(msg);

        // Engine will respond with keymap info
    }
};
```

### 3.4 Engine Investigation Support

**Engine Side**:
```cpp
// src/core/engine/engine.cpp
void Engine::handleInvestigateRequest(WindowHandle hwnd) {
    // Find matching keymap
    std::string windowText = m_windowSystem->getWindowText(hwnd);
    std::string className = m_windowSystem->getClassName(hwnd);

    Keymap* matchedKeymap = nullptr;
    std::string matchedRegex;

    for (auto& keymap : m_keymaps) {
        if (keymap->matchesWindow(windowText, className)) {
            matchedKeymap = keymap;
            matchedRegex = keymap->getWindowRegex();
            break;
        }
    }

    // Build response
    yamy::platform::Message response;
    response.type = yamy::platform::MessageType::KeyEvent;  // Reuse

    if (matchedKeymap) {
        response.data = "Keymap: " + matchedKeymap->getName() + "\n";
        response.data += "Matched: " + matchedRegex + "\n";
        response.data += "Modifiers: " + formatModifiers(matchedKeymap->getModifiers());
    } else {
        response.data = "No matching keymap (using default)";
    }

    m_ipc->send(response);
}
```

### 3.5 Live Log Integration

**Approach**: Engine sends log events to Investigate dialog when open.

```cpp
// src/core/engine/engine.cpp
void Engine::processKey(const KeyEvent& event) {
    // Normal processing
    // ...

    // If investigate mode active, send log
    if (m_investigateMode) {
        yamy::platform::Message msg;
        msg.type = yamy::platform::MessageType::LogMessage;
        msg.data = formatKeyEvent(event);
        m_ipc->send(msg);
    }
}

std::string Engine::formatKeyEvent(const KeyEvent& event) {
    std::ostringstream oss;
    oss << "[" << event.timestamp << "] ";
    oss << (event.isPressed ? "DOWN" : "UP") << " ";
    oss << "Key: " << event.keyCode << " ";
    oss << "Mods: " << formatModifiers(event.modifiers) << " ";
    oss << "→ " << (event.handled ? "HANDLED" : "PASSED");
    return oss.str();
}
```

---

## Track 4: Log Dialog Enhancement

**Goal**: Feature parity with Windows log viewer (FR-4)
**Duration**: 20 hours | **Tasks**: 6

### 4.1 Log Levels and Filtering

**Enhanced Log Structure**:
```cpp
// src/core/logging/log_entry.h
namespace yamy {

enum class LogLevel {
    Trace = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

struct LogEntry {
    time_t timestamp;
    LogLevel level;
    std::string category;  // "Engine", "Parser", "Input", "Window"
    std::string message;

    std::string format() const {
        std::ostringstream oss;
        oss << "[" << formatTime(timestamp) << "] ";
        oss << levelToString(level) << " ";
        oss << "[" << category << "] ";
        oss << message;
        return oss.str();
    }
};

class Logger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::string& category, const std::string& msg);
    void setMinLevel(LogLevel level);
    void addListener(std::function<void(const LogEntry&)> listener);

private:
    LogLevel m_minLevel = LogLevel::Info;
    std::vector<std::function<void(const LogEntry&)>> m_listeners;
    std::mutex m_mutex;
};

}  // namespace yamy
```

**GUI Integration**:
```cpp
// src/ui/qt/dialog_log_qt.cpp
class DialogLogQt : public QDialog {
public:
    DialogLogQt(QWidget* parent = nullptr) : QDialog(parent) {
        setupUI();

        // Subscribe to logger
        yamy::Logger::instance().addListener(
            [this](const yamy::LogEntry& entry) {
                QMetaObject::invokeMethod(this, "appendLog",
                    Qt::QueuedConnection,
                    Q_ARG(QString, QString::fromStdString(entry.format())));
            });
    }

private:
    void setupUI() {
        auto* layout = new QVBoxLayout(this);

        // Filter controls
        auto* filterLayout = new QHBoxLayout();
        filterLayout->addWidget(new QLabel("Detail Level:"));

        m_levelCombo = new QComboBox(this);
        m_levelCombo->addItem("Trace", static_cast<int>(LogLevel::Trace));
        m_levelCombo->addItem("Info", static_cast<int>(LogLevel::Info));
        m_levelCombo->addItem("Warning", static_cast<int>(LogLevel::Warning));
        m_levelCombo->addItem("Error", static_cast<int>(LogLevel::Error));
        m_levelCombo->setCurrentIndex(1);  // Default: Info

        connect(m_levelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &DialogLogQt::onLevelChanged);

        filterLayout->addWidget(m_levelCombo);
        filterLayout->addStretch();

        // Category filters
        m_chkEngine = new QCheckBox("Engine", this);
        m_chkParser = new QCheckBox("Parser", this);
        m_chkInput = new QCheckBox("Input", this);
        m_chkWindow = new QCheckBox("Window", this);

        m_chkEngine->setChecked(true);
        m_chkParser->setChecked(true);
        m_chkInput->setChecked(true);
        m_chkWindow->setChecked(true);

        connect(m_chkEngine, &QCheckBox::toggled, this, &DialogLogQt::onFilterChanged);
        connect(m_chkParser, &QCheckBox::toggled, this, &DialogLogQt::onFilterChanged);
        connect(m_chkInput, &QCheckBox::toggled, this, &DialogLogQt::onFilterChanged);
        connect(m_chkWindow, &QCheckBox::toggled, this, &DialogLogQt::onFilterChanged);

        filterLayout->addWidget(m_chkEngine);
        filterLayout->addWidget(m_chkParser);
        filterLayout->addWidget(m_chkInput);
        filterLayout->addWidget(m_chkWindow);

        layout->addLayout(filterLayout);

        // Log display
        m_logEdit = new QTextEdit(this);
        m_logEdit->setReadOnly(true);
        m_logEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        layout->addWidget(m_logEdit);

        // Font selector
        auto* fontLayout = new QHBoxLayout();
        fontLayout->addWidget(new QLabel("Font:"));

        m_fontCombo = new QFontComboBox(this);
        m_fontCombo->setCurrentFont(m_logEdit->font());
        connect(m_fontCombo, &QFontComboBox::currentFontChanged,
                this, &DialogLogQt::onFontChanged);

        m_fontSizeSpin = new QSpinBox(this);
        m_fontSizeSpin->setRange(6, 24);
        m_fontSizeSpin->setValue(10);
        connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &DialogLogQt::onFontSizeChanged);

        fontLayout->addWidget(m_fontCombo);
        fontLayout->addWidget(m_fontSizeSpin);
        fontLayout->addStretch();

        auto* btnClear = new QPushButton("Clear", this);
        connect(btnClear, &QPushButton::clicked, m_logEdit, &QTextEdit::clear);
        fontLayout->addWidget(btnClear);

        layout->addLayout(fontLayout);

        resize(800, 600);
    }

    Q_SLOT void appendLog(const QString& text) {
        // Apply filters
        if (!shouldDisplay(text)) return;

        // Syntax highlighting
        QString formatted = text;
        if (formatted.contains("ERROR"))
            formatted = "<span style='color: red;'>" + formatted + "</span>";
        else if (formatted.contains("WARNING"))
            formatted = "<span style='color: orange;'>" + formatted + "</span>";

        m_logEdit->append(formatted);

        // Auto-scroll to bottom
        QTextCursor cursor = m_logEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logEdit->setTextCursor(cursor);
    }

    void onLevelChanged(int index) {
        LogLevel level = static_cast<LogLevel>(m_levelCombo->currentData().toInt());
        yamy::Logger::instance().setMinLevel(level);
    }

    bool shouldDisplay(const QString& text) {
        if (!m_chkEngine->isChecked() && text.contains("[Engine]")) return false;
        if (!m_chkParser->isChecked() && text.contains("[Parser]")) return false;
        if (!m_chkInput->isChecked() && text.contains("[Input]")) return false;
        if (!m_chkWindow->isChecked() && text.contains("[Window]")) return false;
        return true;
    }

private:
    QComboBox* m_levelCombo;
    QCheckBox* m_chkEngine;
    QCheckBox* m_chkParser;
    QCheckBox* m_chkInput;
    QCheckBox* m_chkWindow;
    QTextEdit* m_logEdit;
    QFontComboBox* m_fontCombo;
    QSpinBox* m_fontSizeSpin;
};
```

---

## Track 5: Engine Notification System

**Goal**: Real-time engine status updates to GUI (FR-5)
**Duration**: 35 hours | **Tasks**: 10

### 5.1 Notification Types

**Message Types**:
```cpp
// src/platform/ipc.h (extension)
enum class MessageType : uint32_t {
    // ... existing messages

    // Engine lifecycle
    EngineStarting = 0x1007,
    EngineStarted = 0x1000,
    EngineStopping = 0x1008,
    EngineStopped = 0x1001,
    EngineError = 0x1002,

    // Configuration
    ConfigLoading = 0x1009,
    ConfigLoaded = 0x1003,
    ConfigError = 0x1004,
    ConfigValidating = 0x100A,

    // Runtime events
    KeymapSwitched = 0x100B,
    FocusChanged = 0x100C,
    ModifierChanged = 0x100D,

    // Performance
    LatencyReport = 0x100E,
    CpuUsageReport = 0x100F,
};
```

### 5.2 Engine State Machine

**States**:
```
┌─────────┐ start()  ┌─────────┐ validate() ┌─────────┐
│ Stopped ├─────────→│ Loading ├───────────→│ Running │
└────┬────┘          └────┬────┘            └────┬────┘
     ↑                    │                      │
     │ stop()             │ error()              │ error()
     │                    ↓                      ↓
     │               ┌────────┐                  │
     └───────────────┤ Error  │←─────────────────┘
                     └────────┘
```

**Implementation**:
```cpp
// src/core/engine/engine_state.h
namespace yamy {

enum class EngineState {
    Stopped,
    Loading,
    Running,
    Error
};

class Engine {
public:
    void start() {
        if (m_state == EngineState::Running) return;

        setState(EngineState::Loading);
        notifyGUI(MessageType::EngineStarting);

        try {
            // Load configuration
            loadConfiguration();

            // Initialize input hooks
            m_inputHook->install();

            setState(EngineState::Running);
            notifyGUI(MessageType::EngineStarted);

        } catch (const std::exception& e) {
            setState(EngineState::Error);

            Message msg;
            msg.type = MessageType::EngineError;
            msg.data = e.what();
            m_ipc->send(msg);
        }
    }

    void stop() {
        if (m_state == EngineState::Stopped) return;

        setState(EngineState::Stopped);
        notifyGUI(MessageType::EngineStopping);

        m_inputHook->uninstall();

        notifyGUI(MessageType::EngineStopped);
    }

private:
    void setState(EngineState newState) {
        m_state = newState;
        Logger::instance().log(LogLevel::Info, "Engine",
            "State: " + stateToString(newState));
    }

    void notifyGUI(MessageType type) {
        Message msg;
        msg.type = type;
        msg.wparam = static_cast<uint32_t>(m_state);
        m_ipc->send(msg);
    }

    EngineState m_state = EngineState::Stopped;
};

}  // namespace yamy
```

### 5.3 GUI Status Display

**Tray Icon Updates**:
```cpp
// src/ui/qt/tray_icon_qt.cpp
void TrayIconQt::handleEngineMessage(const Message& msg) {
    switch (msg.type) {
        case MessageType::EngineStarting:
            setIcon(QIcon(":/icons/tray_loading.png"));
            setToolTip("YAMY - Starting...");
            break;

        case MessageType::EngineStarted:
            setIcon(QIcon(":/icons/tray_running.png"));
            setToolTip("YAMY - Running");
            m_actionToggle->setText("Stop Engine");
            break;

        case MessageType::EngineStopped:
            setIcon(QIcon(":/icons/tray_stopped.png"));
            setToolTip("YAMY - Stopped");
            m_actionToggle->setText("Start Engine");
            break;

        case MessageType::EngineError:
            setIcon(QIcon(":/icons/tray_error.png"));
            setToolTip("YAMY - Error: " + QString::fromStdString(msg.data));
            showMessage("Engine Error", QString::fromStdString(msg.data),
                       QSystemTrayIcon::Critical, 5000);
            break;

        case MessageType::ConfigLoaded: {
            std::string configName = msg.data;
            setToolTip("YAMY - " + QString::fromStdString(configName));
            buildReloadMenu();  // Update checkmarks
            break;
        }

        case MessageType::KeymapSwitched:
            // Update status (if status bar exists)
            break;
    }
}
```

---

## Track 6: Advanced Features

**Goal**: Session management, IPC API, help menu (FR-6, FR-7, FR-9)
**Duration**: 45 hours | **Tasks**: 12

### 6.1 Session Management

**Auto-Start Integration**:
```cpp
// src/core/settings/session_manager.h
namespace yamy {

class SessionManager {
public:
    bool enableAutoStart();
    bool disableAutoStart();
    bool isAutoStartEnabled();

    // Session restoration
    void saveSession();
    void restoreSession();

private:
    #ifdef __linux__
        void createDesktopEntry();
        void removeDesktopEntry();
        std::filesystem::path getAutoStartPath();
    #endif
};

// Linux implementation
#ifdef __linux__
std::filesystem::path SessionManager::getAutoStartPath() {
    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    std::filesystem::path config_dir = xdg_config
        ? std::filesystem::path(xdg_config)
        : std::filesystem::path(getenv("HOME")) / ".config";

    return config_dir / "autostart";
}

void SessionManager::createDesktopEntry() {
    auto autostart_dir = getAutoStartPath();
    std::filesystem::create_directories(autostart_dir);

    std::ofstream desktop_file(autostart_dir / "yamy.desktop");
    desktop_file << "[Desktop Entry]\n"
                 << "Type=Application\n"
                 << "Name=YAMY\n"
                 << "Exec=" << std::filesystem::current_path() / "yamy" << "\n"
                 << "Icon=yamy\n"
                 << "Comment=Keyboard remapping utility\n"
                 << "X-GNOME-Autostart-enabled=true\n";

    Logger::instance().log(LogLevel::Info, "Session",
        "Created autostart entry");
}

bool SessionManager::enableAutoStart() {
    try {
        createDesktopEntry();
        return true;
    } catch (const std::exception& e) {
        Logger::instance().log(LogLevel::Error, "Session",
            "Failed to enable autostart: " + std::string(e.what()));
        return false;
    }
}
#endif

}  // namespace yamy
```

### 6.2 External IPC API

**Command-Line Tool**:
```bash
# Reload current configuration
$ yamy-ctl reload

# Switch to specific configuration
$ yamy-ctl reload --config gaming

# Get engine status
$ yamy-ctl status
Engine: Running
Config: work.mayu (Work Layout)
Uptime: 2h 15m
Keys processed: 12,453

# Stop/start engine
$ yamy-ctl stop
$ yamy-ctl start

# Query active keymap for current window
$ yamy-ctl query-keymap
Window: "Visual Studio Code"
Keymap: emacs-mode
```

**Implementation**:
```cpp
// src/app/yamy_ctl.cpp
#include "platform/ipc.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: yamy-ctl <command> [args]\n";
        return 1;
    }

    std::string command = argv[1];

    // Connect to running engine
    auto* ipc = yamy::platform::createIPCChannel();
    if (!ipc->connect("yamy-engine")) {
        std::cerr << "Error: YAMY engine is not running\n";
        return 1;
    }

    // Send command
    yamy::platform::Message msg;

    if (command == "reload") {
        msg.type = yamy::platform::MessageType::CmdReload;
        if (argc >= 4 && std::string(argv[2]) == "--config") {
            msg.data = argv[3];
        }
    } else if (command == "stop") {
        msg.type = yamy::platform::MessageType::CmdStop;
    } else if (command == "start") {
        msg.type = yamy::platform::MessageType::CmdToggleEngine;
    } else if (command == "status") {
        msg.type = yamy::platform::MessageType::CmdInvestigate;
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        return 1;
    }

    ipc->send(msg);

    // Wait for response
    yamy::platform::Message response;
    if (ipc->poll(&response)) {
        std::cout << response.data << "\n";
    }

    delete ipc;
    return 0;
}
```

### 6.3 Help Menu

**Help System Design**:
```
Help Menu:
├─ Online Documentation (opens browser)
├─ Keyboard Shortcuts Reference (QDialog)
├─ Configuration Examples (QDialog with code samples)
├─ Report Bug (opens GitHub issues)
├─ ──────────
└─ About (version, license, contributors)
```

**Implementation**:
```cpp
// src/ui/qt/dialog_help_qt.cpp
class DialogHelpQt : public QDialog {
public:
    DialogHelpQt(QWidget* parent = nullptr) : QDialog(parent) {
        setupUI();
        loadShortcuts();
    }

private:
    void setupUI() {
        auto* layout = new QVBoxLayout(this);

        auto* tabs = new QTabWidget(this);

        // Shortcuts tab
        auto* shortcutsTab = new QWidget();
        auto* shortcutsLayout = new QVBoxLayout(shortcutsTab);

        m_shortcutTable = new QTableWidget(this);
        m_shortcutTable->setColumnCount(2);
        m_shortcutTable->setHorizontalHeaderLabels({"Action", "Shortcut"});
        m_shortcutTable->horizontalHeader()->setStretchLastSection(true);

        shortcutsLayout->addWidget(m_shortcutTable);
        tabs->addTab(shortcutsTab, "Keyboard Shortcuts");

        // Examples tab
        auto* examplesTab = new QWidget();
        auto* examplesLayout = new QVBoxLayout(examplesTab);

        auto* exampleList = new QListWidget(this);
        exampleList->addItem("Basic Remapping");
        exampleList->addItem("Emacs Bindings");
        exampleList->addItem("Vim Modal Editing");
        exampleList->addItem("Window Management");

        auto* exampleCode = new QTextEdit(this);
        exampleCode->setReadOnly(true);
        exampleCode->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

        connect(exampleList, &QListWidget::currentRowChanged,
                [this, exampleCode](int row) {
                    exampleCode->setPlainText(getExample(row));
                });

        examplesLayout->addWidget(exampleList, 1);
        examplesLayout->addWidget(exampleCode, 3);
        tabs->addTab(examplesTab, "Examples");

        layout->addWidget(tabs);

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
        layout->addWidget(buttonBox);

        resize(700, 500);
    }

    void loadShortcuts() {
        QStringList shortcuts = {
            "Open Settings|Ctrl+Shift+S",
            "Reload Configuration|Ctrl+Shift+R",
            "Open Log|Ctrl+Shift+L",
            "Investigate Window|Ctrl+Shift+I",
            "Toggle Engine|Ctrl+Shift+E",
            "Show Help|F1"
        };

        m_shortcutTable->setRowCount(shortcuts.size());

        for (int i = 0; i < shortcuts.size(); ++i) {
            QStringList parts = shortcuts[i].split('|');
            m_shortcutTable->setItem(i, 0, new QTableWidgetItem(parts[0]));
            m_shortcutTable->setItem(i, 1, new QTableWidgetItem(parts[1]));
        }
    }

    QString getExample(int index) {
        switch (index) {
            case 0:  // Basic Remapping
                return R"(# Basic key remapping
keymap Global
    # Swap Caps Lock and Control
    key CapsLock = Control
    key Control = CapsLock

    # Arrow keys on right hand
    mod Control += {
        key H = Left
        key J = Down
        key K = Up
        key L = Right
    }
)";
            case 1:  // Emacs Bindings
                return R"(# Emacs-style bindings
keymap Global
    mod Control += {
        key P = Up
        key N = Down
        key B = Left
        key F = Right
        key A = Home
        key E = End
        key K = S-End Delete
        key W = C-X
    }
)";
            // ... more examples
        }
        return "";
    }

    QTableWidget* m_shortcutTable;
};
```

---

## Integration Testing Strategy

### Test Plan

**Unit Tests** (Per-track):
- Track 1: 60 unit tests (one per file refactored)
- Track 2: 15 tests (ConfigurationManager CRUD)
- Track 3: 18 tests (Window inspection, crosshair)
- Track 4: 6 tests (Log filtering, formatting)
- Track 5: 10 tests (Notification delivery)
- Track 6: 12 tests (Session mgmt, IPC)

**Integration Tests**:
1. **End-to-End Configuration Flow**
   - Add config in GUI → Engine reloads → Verify keymaps active
2. **Cross-Platform .mayu Compatibility**
   - Load same .mayu on Windows and Linux → Verify identical behavior
3. **IPC Stress Test**
   - Send 1000 messages/sec → Verify no dropped messages
4. **Performance Benchmarks**
   - Key latency < 1ms (99th percentile)
   - CPU usage < 1% idle
   - Memory < 10MB RSS

**Test Automation**:
```bash
# CMakeLists.txt
enable_testing()
add_subdirectory(tests)

# Run all tests
ctest --output-on-failure

# Run with coverage
cmake -DCOVERAGE=ON ..
make test
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## Rollout Strategy

### Phase 1: Alpha (Internal Testing)
- Deploy Track 1-3 to core maintainers
- Validate basic functionality
- Fix P0 bugs

### Phase 2: Beta (Community Testing)
- Deploy Track 1-6 to 20 beta testers
- Collect feedback via GitHub discussions
- Performance tuning

### Phase 3: Release Candidate
- All features complete
- Zero P0 bugs
- Documentation finalized
- Pass integration test suite

### Phase 4: v1.0 Launch
- AUR, PPA, Copr packages
- GitHub release with binaries
- Announcement on HN, Reddit
- Tutorial video series

---

## Risk Mitigation

### Technical Risks

**Risk**: X11 window APIs behave differently than Win32
**Mitigation**: Extensive integration testing, fallback strategies

**Risk**: Performance degradation on older hardware
**Mitigation**: Profiling on Raspberry Pi, optimization passes

**Risk**: Wayland incomplete support
**Mitigation**: Clear documentation of X11 requirement, Wayland planned for v1.1

### Schedule Risks

**Risk**: 60 parallel tasks may have hidden dependencies
**Mitigation**: 3-batch structure, strict acceptance criteria per task

**Risk**: Qt GUI complexity underestimated
**Mitigation**: Track 3 (Investigate) has buffer time (60h allocated)

---

## Success Criteria

**Track 1**: ✅ Engine compiles on Linux without Windows headers
**Track 2**: ✅ Can manage 3+ configurations via GUI
**Track 3**: ✅ Investigate dialog shows all window info + keymap status
**Track 4**: ✅ Log filtering works, 4 detail levels
**Track 5**: ✅ Tray icon updates in <100ms after engine state change
**Track 6**: ✅ `yamy-ctl` controls running engine, autostart works

**Overall**: ✅ Pass 100% of Windows integration tests on Linux

---

**Document Version**: 1.0
**Last Updated**: 2025-12-10
**Implements**: requirements.md (all FR-1 through FR-9, NFR-1 through NFR-6)
**Ready for**: Approval → Task Breakdown → Implementation
