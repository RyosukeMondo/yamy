# Linux GUI Implementation Plan - Qt System Tray

## Objective

Add Qt-based GUI for Linux while keeping Windows Win32 GUI unchanged.

## Design Principle

**Same as Windows:**
- System tray icon (no main window)
- Context menu for actions
- Modal dialogs for configuration
- Enable/disable toggle
- Keymap reload
- Log viewer
- Background service philosophy

## Architecture Decision

```
┌─────────────────────────────────────────┐
│           YAMY Application              │
├─────────────────────────────────────────┤
│                                         │
│  Windows (unchanged)    Linux (new Qt) │
│  ┌──────────────┐      ┌─────────────┐ │
│  │ Win32 GUI    │      │ Qt GUI      │ │
│  │ mayu.cpp     │      │ main_qt.cpp │ │
│  │ System Tray  │      │ QSystemTray │ │
│  │ DialogBox    │      │ QDialog     │ │
│  └──────┬───────┘      └──────┬──────┘ │
│         │                     │         │
│         └──────────┬──────────┘         │
│                    │                    │
│         ┌──────────▼──────────┐         │
│         │   Engine (Core)     │         │
│         │   Platform Agnostic │         │
│         └─────────────────────┘         │
└─────────────────────────────────────────┘
```

## Implementation Phases

### Phase 1: Planning & Structure ✅
**Time:** 30 minutes
**Files:** Planning documents

- [x] Create implementation plan
- [x] Define file structure
- [x] Identify Qt components needed
- [x] Plan conditional compilation

### Phase 2: CMake Qt Integration
**Time:** 1 hour
**Files:** `CMakeLists.txt`, `src/ui/qt/CMakeLists.txt`

Tasks:
- [ ] Add `find_package(Qt5 COMPONENTS Widgets)`
- [ ] Linux-only Qt dependency
- [ ] Create `src/ui/qt/` directory
- [ ] Configure Qt MOC (Meta-Object Compiler)
- [ ] Set up Qt resources (icons)

### Phase 3: System Tray Icon Class
**Time:** 2 hours
**Files:** `tray_icon_qt.h`, `tray_icon_qt.cpp`

Tasks:
- [ ] `QSystemTrayIcon` wrapper class
- [ ] Icon state management (enabled/disabled)
- [ ] Tooltip updates
- [ ] Click/double-click handlers
- [ ] Message notifications (balloon tips)

### Phase 4: Context Menu & Actions
**Time:** 2 hours
**Files:** `tray_menu_qt.h`, `tray_menu_qt.cpp`

Tasks:
- [ ] Create `QMenu` with actions
- [ ] Enable/Disable toggle with checkmark
- [ ] Reload action
- [ ] Dynamic keymap submenu
- [ ] Settings/Log/About actions
- [ ] Exit action
- [ ] Connect actions to engine

### Phase 5: Dialogs
**Time:** 3 hours
**Files:** `dialog_*.h`, `dialog_*.cpp`, `*.ui` files

**5a. Settings Dialog:**
- [ ] Keymap file list (QListWidget)
- [ ] Add/Edit/Remove buttons
- [ ] Path configuration

**5b. Log Dialog:**
- [ ] QTextEdit for log display
- [ ] Auto-scroll
- [ ] Clear button
- [ ] Save to file

**5c. About Dialog:**
- [ ] Version info
- [ ] Build date
- [ ] License text

### Phase 6: Engine Integration
**Time:** 2 hours
**Files:** `main_qt.cpp`

Tasks:
- [ ] QApplication setup
- [ ] Create tray icon instance
- [ ] Connect to Engine signals/callbacks
- [ ] Handle engine state changes
- [ ] Configuration loading
- [ ] Signal handling (SIGUSR1 for reload)

### Phase 7: Build & Test
**Time:** 1 hour

Tasks:
- [ ] Install Qt5 development packages
- [ ] Build with CMake
- [ ] Test tray icon visibility
- [ ] Test menu actions
- [ ] Test dialogs
- [ ] Test engine integration
- [ ] Verify Windows build still works

## File Structure

```
src/
├── app/
│   ├── mayu.cpp                  # Windows GUI (unchanged)
│   ├── main_linux.cpp            # Linux headless (backup)
│   └── main_qt.cpp               # Linux Qt GUI (NEW)
│
├── ui/
│   ├── qt/                       # NEW directory
│   │   ├── CMakeLists.txt        # Qt build config
│   │   ├── tray_icon_qt.h        # System tray wrapper
│   │   ├── tray_icon_qt.cpp
│   │   ├── tray_menu_qt.h        # Context menu
│   │   ├── tray_menu_qt.cpp
│   │   ├── dialog_settings.h     # Settings dialog
│   │   ├── dialog_settings.cpp
│   │   ├── dialog_settings.ui    # Qt Designer file
│   │   ├── dialog_log.h          # Log viewer
│   │   ├── dialog_log.cpp
│   │   ├── dialog_log.ui
│   │   ├── dialog_about.h        # About dialog
│   │   ├── dialog_about.cpp
│   │   ├── dialog_about.ui
│   │   └── resources.qrc         # Qt resources (icons)
│   │
│   ├── dlgsetting.cpp            # Windows dialogs (unchanged)
│   ├── dlglog.cpp
│   └── ...
│
└── resources/
    └── icons/
        ├── yamy_enabled.png      # NEW
        └── yamy_disabled.png     # NEW
```

## Qt Components Used

### Minimal Qt Modules
```cmake
find_package(Qt5 REQUIRED COMPONENTS
    Core        # QObject, signals/slots
    Widgets     # QApplication, QDialog
    Gui         # QIcon, QPixmap
)
```

**NOT needed:**
- ❌ QtNetwork
- ❌ QtMultimedia
- ❌ QtWebEngine
- ❌ QtQuick/QML

### Key Qt Classes

| Class | Purpose |
|-------|---------|
| `QApplication` | Main event loop |
| `QSystemTrayIcon` | System tray icon |
| `QMenu` | Context menu |
| `QAction` | Menu items |
| `QDialog` | Modal dialogs |
| `QListWidget` | Keymap file list |
| `QTextEdit` | Log viewer |
| `QIcon` | Icons |
| `QSettings` | Configuration |

## Code Example: Minimal Structure

### main_qt.cpp
```cpp
#include <QApplication>
#include "tray_icon_qt.h"
#include "engine.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // Run in background

    // Create engine
    Engine engine;
    engine.initialize();

    // Create tray icon
    TrayIconQt tray(&engine);
    tray.show();

    return app.exec();
}
```

### tray_icon_qt.h
```cpp
#pragma once
#include <QSystemTrayIcon>
#include <QMenu>

class Engine;

class TrayIconQt : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit TrayIconQt(Engine* engine, QObject* parent = nullptr);
    ~TrayIconQt() override;

public slots:
    void updateIcon(bool enabled);
    void showMessage(const QString& title, const QString& message);

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void onToggleEnable();
    void onReload();
    void onSettings();
    void onShowLog();
    void onAbout();
    void onExit();

private:
    void createMenu();

    Engine* m_engine;
    QMenu* m_menu;
    QAction* m_actionEnable;
    QIcon m_iconEnabled;
    QIcon m_iconDisabled;
};
```

## Conditional Compilation

### CMakeLists.txt
```cmake
if(UNIX AND NOT APPLE)
    # Linux-specific GUI
    option(BUILD_QT_GUI "Build Qt GUI for Linux" ON)

    if(BUILD_QT_GUI)
        find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
        add_subdirectory(src/ui/qt)

        set(GUI_SOURCES
            src/app/main_qt.cpp
        )
        set(GUI_LIBS Qt5::Widgets Qt5::Gui)
    else()
        # Headless fallback
        set(GUI_SOURCES src/app/main_linux.cpp)
        set(GUI_LIBS)
    endif()
elseif(WIN32)
    # Windows Win32 GUI
    set(GUI_SOURCES
        src/app/mayu.cpp
        src/ui/dlgsetting.cpp
        src/ui/dlglog.cpp
        # ... other Win32 GUI files
    )
    set(GUI_LIBS comctl32 shell32)
endif()

add_executable(yamy ${GUI_SOURCES} ${CORE_SOURCES})
target_link_libraries(yamy ${GUI_LIBS} ${PLATFORM_LIBS})
```

## Dependencies

### Ubuntu/Debian
```bash
sudo apt install qtbase5-dev qttools5-dev
```

### Fedora/RHEL
```bash
sudo dnf install qt5-qtbase-devel qt5-qttools-devel
```

### Arch
```bash
sudo pacman -S qt5-base qt5-tools
```

## Timeline

| Phase | Time | Cumulative |
|-------|------|------------|
| 1. Planning | 0.5h | 0.5h |
| 2. CMake | 1h | 1.5h |
| 3. Tray Icon | 2h | 3.5h |
| 4. Menu | 2h | 5.5h |
| 5. Dialogs | 3h | 8.5h |
| 6. Integration | 2h | 10.5h |
| 7. Testing | 1h | 11.5h |
| **Total** | **~12 hours** | |

## Success Criteria

- [ ] Qt GUI builds on Linux
- [ ] Win32 GUI still builds on Windows (unchanged)
- [ ] System tray icon appears
- [ ] Enable/Disable toggle works
- [ ] Reload config works
- [ ] Settings dialog functional
- [ ] Log viewer functional
- [ ] About dialog shows
- [ ] Engine integration works
- [ ] No regressions on Windows

## Testing Checklist

### Visual Tests
- [ ] Tray icon visible in system tray
- [ ] Icon changes color (enabled/disabled)
- [ ] Tooltip shows correct state
- [ ] Right-click shows menu
- [ ] Menu has all items

### Functional Tests
- [ ] Enable/Disable toggles engine
- [ ] Reload reloads configuration
- [ ] Settings dialog opens
- [ ] Settings saves/loads
- [ ] Log viewer shows logs
- [ ] About dialog shows info
- [ ] Exit quits cleanly

### Integration Tests
- [ ] Engine callbacks update GUI
- [ ] GUI actions control engine
- [ ] Config changes take effect
- [ ] Logs appear in log dialog

### Build Tests
- [ ] Linux build with Qt succeeds
- [ ] Linux build without Qt falls back to headless
- [ ] Windows build unchanged/working

## Risk Mitigation

**Risk:** Qt not available
**Mitigation:** Make Qt optional, fallback to headless

**Risk:** Qt version incompatibility
**Mitigation:** Support Qt5 (widely available)

**Risk:** Different Linux DEs
**Mitigation:** Use Qt's abstraction (works on all)

**Risk:** Break Windows build
**Mitigation:** Strict conditional compilation, separate files

## Next Steps

1. ✅ Complete this plan
2. Install Qt5 development packages
3. Create directory structure
4. Implement Phase 2 (CMake)
5. Implement Phase 3 (Tray Icon)
6. Continue through phases
7. Test thoroughly

---

Ready to implement! Starting with Phase 2...
