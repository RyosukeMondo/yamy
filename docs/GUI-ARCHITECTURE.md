# YAMY GUI Architecture

## Overview

YAMY uses a **minimal GUI approach** focused on system tray integration rather than a traditional windowed application. The GUI is currently **Windows-only** and uses Win32 API directly.

## Architecture Type

**System Tray Application (No Main Window)**
- Runs as a background service
- System tray icon for interaction
- Context menus for actions
- Modal dialogs for configuration

## Components

### 1. Main Application (mayu.cpp)

**Class:** `Mayu`

**Key Members:**
```cpp
HWND m_hwndTaskTray;          // Hidden window for tray icon
HWND m_hwndLog;               // Log dialog window
HWND m_hwndInvestigate;       // Investigate dialog window
HWND m_hwndVersion;           // Version dialog window
NOTIFYICONDATA m_ni;          // System tray icon data
HICON m_tasktrayIcon[2];      // Icons (enabled/disabled states)
HMENU m_hMenuTaskTray;        // Context menu
Engine m_engine;              // Keyboard remapping engine
```

### 2. GUI Technology

**Platform:** Win32 API (Windows-only)
- `Shell_NotifyIcon()` - System tray icon management
- `RegisterClass()` + `CreateWindow()` - Hidden window
- `TrackPopupMenu()` - Context menus
- `DialogBox()` - Modal dialogs

**No External GUI Framework:**
- ❌ No Qt
- ❌ No GTK
- ❌ No wxWidgets
- ✅ Pure Win32 API

### 3. System Tray Integration

**Icon Management:**
```cpp
// Add icon to system tray
Shell_NotifyIcon(NIM_ADD, &m_ni)

// Update icon (enabled/disabled state)
m_ni.hIcon = m_tasktrayIcon[m_engine.getIsEnabled() ? 1 : 0];
Shell_NotifyIcon(NIM_MODIFY, &m_ni)

// Show balloon tooltip
m_ni.uFlags = NIF_INFO;
m_ni.szInfo = "message";
m_ni.szInfoTitle = "title";
Shell_NotifyIcon(NIM_MODIFY, &m_ni)
```

**Icon States:**
- `m_tasktrayIcon[0]` - Disabled (gray icon)
- `m_tasktrayIcon[1]` - Enabled (color icon)

### 4. Window Procedure

**Hidden Window:** `mayuTasktray` class
- No visible window, just message pump
- Receives tray icon events
- Handles IPC messages
- Session change notifications

**Key Messages:**
```cpp
WM_APP_taskTrayNotify      // Tray icon clicked
WM_RBUTTONUP              // Right-click → show menu
WM_LBUTTONDBLCLK         // Double-click → default action
WM_COMMAND               // Menu item selected
WM_COPYDATA              // IPC from other apps
WM_WTSSESSION_CHANGE     // Lock/unlock events
```

### 5. Dialogs (Modal Windows)

**Dialog Files:**
- `dlgsetting.cpp` - Settings configuration
- `dlgeditsetting.cpp` - Edit keymap files
- `dlglog.cpp` - Log viewer
- `dlginvestigate.cpp` - Window investigation tool
- `dlgversion.cpp` - About dialog

**Technology:**
- Win32 dialog resources (`.rc` files)
- Modal dialogs via `DialogBox()`
- List views, edit controls, buttons
- Custom layout manager for resizing

### 6. Context Menu Structure

**Built from:**
- Static menu resources (`mayurc.h`)
- Dynamic menu items (loaded keymaps)
- Separator lines
- Checkmarks for enabled state

**Typical Menu:**
```
┌─────────────────────────┐
│ ✓ Enable                │ ← Toggle keyboard hook
│ ───────────────────────  │
│ Reload                  │ ← Reload keymap files
│ Keymap1.mayu            │ ← Switch keymap
│ Keymap2.mayu            │
│ ───────────────────────  │
│ Setting...              │ ← Open settings dialog
│ Log...                  │ ← Open log viewer
│ Investigate...          │ ← Window investigation
│ Version...              │ ← About dialog
│ ───────────────────────  │
│ Exit                    │ ← Quit application
└─────────────────────────┘
```

### 7. Event Flow

```
User Right-Clicks Tray Icon
         ↓
WM_APP_taskTrayNotify (WM_RBUTTONUP)
         ↓
GetCursorPos()
         ↓
SetForegroundWindow() ← Required for popup menu
         ↓
TrackPopupMenu()
         ↓
User Selects Menu Item
         ↓
WM_COMMAND
         ↓
Switch on menu ID
         ↓
Execute Action (enable/disable, reload, open dialog, etc.)
```

### 8. IPC Mechanism

**Mailslot for Notifications:**
```cpp
m_hNotifyMailslot = CreateMailslot("\\\\.\\mailslot\\mayuNotify")
ReadFileEx(..., mailslotProc)  // Async read
```

**Used for:**
- Focus change notifications (from hook DLL)
- Window title/class notifications
- Log messages from hook

**WM_COPYDATA for Commands:**
- Other apps can send commands to YAMY
- Used for external control/automation

## Linux Porting Considerations

### What Needs to Change

**1. System Tray → Desktop Environment Specific**
- **X11/freedesktop:** Use D-Bus + libappindicator
- **GNOME:** GtkStatusIcon or AppIndicator
- **KDE:** KStatusNotifierItem
- **Generic:** XEmbed tray protocol

**2. Dialogs → GUI Toolkit Required**
Options:
- **Qt** - Cross-platform, modern, comprehensive
- **GTK** - Native on GNOME/Linux
- **Dear ImGui** - Immediate mode, lightweight
- **Custom X11** - Too much work, not recommended

**3. Recommended Approach**

**Minimal Qt Implementation:**
```cpp
class TrayIcon : public QSystemTrayIcon {
    Engine* m_engine;
    QMenu* m_menu;

public:
    TrayIcon() {
        // Create tray icon
        setIcon(QIcon(":/icons/yamy.png"));

        // Create menu
        m_menu = new QMenu();
        m_menu->addAction("Enable", this, &TrayIcon::toggleEnable);
        m_menu->addSeparator();
        m_menu->addAction("Settings...", this, &TrayIcon::showSettings);
        m_menu->addAction("Exit", qApp, &QApplication::quit);

        setContextMenu(m_menu);
        show();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    TrayIcon tray;
    return app.exec();
}
```

**Benefits:**
- Cross-platform (Linux, Windows, macOS)
- Native look and feel
- Built-in event loop
- Easy to maintain

### Current Status

**Windows:**
- ✅ Full GUI implementation
- ✅ System tray icon
- ✅ Context menus
- ✅ Configuration dialogs
- ✅ Log viewer
- ✅ Window investigation tool

**Linux:**
- ❌ No GUI yet
- ✅ Core engine works (Track 1-12 complete)
- ✅ Input capture/injection functional
- ⏳ Need tray icon implementation
- ⏳ Need configuration UI

## Implementation Plan for Linux GUI

### Phase 1: Headless Mode (Done!)
- ✅ Engine runs without GUI
- ✅ Config file based configuration
- ✅ Log to stdout/file
- ✅ SIGUSR1 for reload

### Phase 2: Minimal Tray Icon (Future)
**Option A: Qt-based**
```bash
sudo apt install libqt5gui5 libqt5widgets5
```
- QSystemTrayIcon
- QMenu for context
- QSettings for config

**Option B: libappindicator (GNOME/Unity)**
```bash
sudo apt install libappindicator3-dev
```
- Native GNOME integration
- Limited to tray functionality

**Option C: Dear ImGui (Lightweight)**
```bash
# Header-only, no dependencies
```
- Minimal overhead
- Custom rendering
- No native integration

### Phase 3: Configuration UI (Future)
- Settings dialog (Qt/GTK)
- Log viewer
- Keymap editor
- About dialog

## File Structure

```
src/app/
  mayu.cpp               - Main application (Windows)
  main_linux.cpp         - Linux entry point (headless)
  yamy.cpp              - Launcher (Windows)
  yamyd.cpp             - Hook DLL helper (Windows)

src/ui/                  - Windows GUI dialogs
  dlgsetting.cpp         - Settings dialog
  dlgeditsetting.cpp     - Edit keymap dialog
  dlglog.cpp            - Log viewer
  dlginvestigate.cpp    - Window investigation
  dlgversion.cpp        - About dialog
  mayurc.h              - Resource IDs
  *.rc                  - Dialog resources

src/core/                - Platform-independent engine
  engine/               - Remapping engine
  commands/             - Keymap commands
  settings/             - Configuration

src/platform/
  windows/              - Win32 GUI code
  linux/                - Linux backend (no GUI yet)
```

## Summary

**Current Implementation:**
- Pure Win32 API
- System tray focused
- Minimal UI footprint
- Modal dialogs only

**Design Philosophy:**
- Background service, not foreground app
- Quick access via tray icon
- Configuration when needed
- Stays out of the way

**Linux Port:**
- Core engine ✅ Complete
- Input system ✅ Complete
- GUI ⏳ Not started
- Recommended: Qt for cross-platform consistency

## Resources

- [Shell_NotifyIcon Documentation](https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw)
- [Qt QSystemTrayIcon](https://doc.qt.io/qt-5/qsystemtrayicon.html)
- [libappindicator](https://lazka.github.io/pgi-docs/AppIndicator3-0.1/index.html)
- [Dear ImGui](https://github.com/ocornut/imgui)
