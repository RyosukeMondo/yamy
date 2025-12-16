# Linux Qt GUI - Remaining Implementation Plan

## Executive Summary

**Current Status:** Qt GUI framework complete (Phases 1-8) with stub engine
**Remaining Work:** Full feature parity with Windows GUI + engine integration
**Estimated Effort:** 200-400 development hours across 5 major tracks
**Timeline:** 8-12 weeks for complete implementation

---

## Gap Analysis Summary

### ✅ Completed (Qt GUI Phases 1-8)
- System tray icon with dual states
- Basic context menu (Enable, Reload, Settings, Log, About, Exit)
- Settings dialog (basic keymap file list)
- Log viewer dialog (basic display, save, auto-scroll)
- About dialog
- CMake build integration
- Documentation and setup automation

### ❌ Missing Features (Compared to Windows GUI)

**Critical (Blocks Basic Usage):**
- Dynamic reload submenu (configuration switching)
- Settings dialog: symbols field, reordering, edit dialog
- Configuration storage format (`.mayu0`-`.mayuN`, `.mayuIndex`)
- Actual reload functionality
- Engine integration (currently stub)

**Important (Core Functionality):**
- Investigate/debug dialog
- Engine notification handling
- Log detail level control
- Startup banner
- Shell execute support

**Advanced (Power User Features):**
- Target window selector
- Keyboard state checker
- IPC API for external control
- Session management

---

## Implementation Tracks

### Track 1: Core Refactoring (Foundation)
**Purpose:** Remove Windows dependencies from core YAMY
**Priority:** CRITICAL - Blocks all other work
**Estimated Effort:** 80-120 hours

#### 1.1 Platform Type Abstraction
**Files:** Core engine, settings, functions
**Work:**
- Replace `HWND` → `yamy::platform::WindowHandle` everywhere
- Replace `LPARAM`/`WPARAM` → Generic types
- Replace `tstring` → `std::string` or `std::u8string`
- Replace Win32 constants → Platform enums
  ```cpp
  // Before
  enum ShowCommandType {
      ShowCommandType_hide = SW_HIDE,
      ShowCommandType_maximize = SW_MAXIMIZE,
      // ...
  };

  // After
  enum ShowCommandType {
      ShowCommandType_hide = 0,
      ShowCommandType_maximize = 3,
      // Define constants ourselves
  };
  ```

#### 1.2 Message System Abstraction
**Files:** `engine.h`, `msgstream.h`, `mayu.h`
**Work:**
- Abstract `PostMessage()` → `yamy::platform::IPC::send()`
- Create IPC interface:
  ```cpp
  namespace yamy::platform {
      class IIPCChannel {
      public:
          virtual void send(const std::string& message) = 0;
          virtual void setCallback(std::function<void(const std::string&)> cb) = 0;
      };

      IIPCChannel* createIPCChannel();  // Platform-specific
  }
  ```
- Refactor `tomsgstream` to use callback instead of `PostMessage`

#### 1.3 Registry/ConfigStore Cleanup
**Files:** `config_store.h`, `registry.cpp`
**Work:**
- Remove `tstring` overloads (causes compilation errors on Linux)
- Keep only `std::string` versions
- Implement Linux backend (`config_store_linux.cpp`):
  ```cpp
  class ConfigStoreLinux : public ConfigStore {
      // Use QSettings or INI file
  };
  ```

#### 1.4 Setting Loader Cross-Platform
**Files:** `setting_loader.cpp`
**Work:**
- Abstract file I/O (already uses std::fstream, should be OK)
- Test .mayu file parsing on Linux
- Ensure UTF-8 handling works

**Deliverables:**
- ✅ Core engine compiles on Linux
- ✅ All Windows types abstracted
- ✅ IPC interface defined
- ✅ ConfigStore works on both platforms

---

### Track 2: Configuration Management
**Purpose:** Multi-configuration support with switching
**Priority:** HIGH - Required for basic usage
**Estimated Effort:** 40-60 hours
**Dependencies:** Track 1 complete

#### 2.1 Configuration Storage Format
**Files:** New `config_manager_qt.h/cpp`
**Work:**
- Define `MayuConfiguration` struct:
  ```cpp
  struct MayuConfiguration {
      std::string name;        // "Emacs 109"
      std::string path;        // "/home/user/.mayu/default.mayu"
      std::string symbols;     // "-DUSE109;-DUSEdefault"

      std::string toString() const;  // Serialize to "name;path;symbols"
      static MayuConfiguration fromString(const std::string& str);
  };
  ```
- QSettings storage:
  ```ini
  [keymaps]
  activeIndex=0
  configs\size=3
  configs\0="Emacs 109;/home/user/.mayu/default.mayu;-DUSE109"
  configs\1="Emacs 104;/home/user/.mayu/default.mayu;-DUSE104"
  configs\2="Minimal;/home/user/.mayu/minimal.mayu;"
  ```

#### 2.2 Configuration Manager Class
**Files:** `config_manager_qt.h/cpp`
**Interface:**
```cpp
class ConfigManagerQt {
public:
    // Load all configurations from QSettings
    std::vector<MayuConfiguration> loadConfigurations();

    // Save all configurations to QSettings
    void saveConfigurations(const std::vector<MayuConfiguration>& configs);

    // Get/set active configuration index
    int getActiveIndex() const;
    void setActiveIndex(int index);

    // Get active configuration
    MayuConfiguration getActiveConfiguration() const;

    // Add/remove/update configurations
    void addConfiguration(const MayuConfiguration& config);
    void removeConfiguration(int index);
    void updateConfiguration(int index, const MayuConfiguration& config);

    // Reorder
    void moveUp(int index);
    void moveDown(int index);
};
```

#### 2.3 Dynamic Reload Submenu
**Files:** `tray_icon_qt.cpp`
**Work:**
- Create dynamic submenu in `createMenu()`:
  ```cpp
  QMenu* reloadMenu = m_menu->addMenu("Reload");
  // Populate from ConfigManager
  for (int i = 0; i < configs.size(); ++i) {
      QAction* action = reloadMenu->addAction(configs[i].name);
      action->setCheckable(true);
      action->setChecked(i == activeIndex);
      action->setData(i);  // Store index
      connect(action, &QAction::triggered, this, &TrayIconQt::onReloadConfig);
  }
  ```
- Implement `onReloadConfig(int index)`:
  - Load configuration
  - Parse .mayu file with symbols
  - Create Setting object
  - Call `engine->setSetting()`

#### 2.4 Settings Dialog Enhancement
**Files:** `dialog_settings_qt.h/cpp`
**Work:**
- Add symbols column to QTableWidget (3 columns: Name, Path, Symbols)
- Add up/down buttons for reordering
- Add Edit button → Opens edit dialog
- Double-click on row → Opens edit dialog
- Save to ConfigManager on OK

**Deliverables:**
- ✅ ConfigManager class working
- ✅ QSettings storage format defined
- ✅ Reload submenu populates dynamically
- ✅ Configuration switching works
- ✅ Settings dialog has 3 columns + reordering

---

### Track 3: Edit Setting Dialog
**Purpose:** Full configuration editor
**Priority:** HIGH
**Estimated Effort:** 20-30 hours
**Dependencies:** Track 2 (ConfigManager)

#### 3.1 Dialog Implementation
**Files:** `dialog_edit_setting_qt.h/cpp`
**UI Layout:**
```
┌────────────────────────────────────────┐
│ Edit Configuration                      │
├────────────────────────────────────────┤
│                                         │
│ Name:    [Emacs 109             ]       │
│                                         │
│ Path:    [/home/user/.mayu/def..]  [📁] │
│                                         │
│ Symbols: [-DUSE109;-DUSEdefault ]       │
│          (Preprocessor flags, semicolon │
│           separated)                     │
│                                         │
│              [  OK  ]  [ Cancel ]       │
└────────────────────────────────────────┘
```

**Features:**
- Name field (required, validated)
- Path field + browse button (file dialog)
- Symbols field (QLineEdit, accepts `-DXXX;-DYYY`)
- OK button (disabled if name empty)
- Cancel button

#### 3.2 Integration
- Called from Settings dialog on:
  - "Edit" button click
  - Double-click on row
  - "Add" button (empty configuration)
- Returns `MayuConfiguration` on success
- Settings dialog updates its table

**Deliverables:**
- ✅ Edit dialog implemented
- ✅ Validation works
- ✅ Integrated with Settings dialog

---

### Track 4: Investigate Dialog
**Purpose:** Real-time debugging tool
**Priority:** MEDIUM
**Estimated Effort:** 60-80 hours
**Dependencies:** Track 1 (engine integration)

#### 4.1 Window Information Panel
**Files:** `dialog_investigate_qt.h/cpp`
**UI Layout:**
```
┌────────────────────────────────────────────────────┐
│ Investigate - YAMY Debug Tool                      │
├────────────────────────────────────────────────────┤
│ Target Window:                [Crosshair Tool 🎯]   │
│                                                     │
│ Window Handle:   0x12345678                        │
│ Thread ID:       4567                              │
│ Class Name:      Qt5QWindowIcon                    │
│ Title:           Claude Code - Visual Studio Code  │
│                                                     │
│ Position:        (100, 200) - (1020, 850)          │
│ Size:            920 x 650                         │
│ MDI Window:      No                                │
│                                                     │
│ ┌─────────────────────────────────────────────┐   │
│ │ Keyboard Input Test (Focus to enable log)   │   │
│ │                                              │   │
│ │ [____________________________________]       │   │
│ │                                              │   │
│ └─────────────────────────────────────────────┘   │
│                                                     │
│ Virtual Key: VK_A (0x41)  Modifiers: E- U- D+      │
│ Scancode:    0x1E                                  │
│                                                     │
│                            [ Close ]                │
└────────────────────────────────────────────────────┘
```

#### 4.2 Crosshair Window Selector
**Implementation:**
- Press and hold on crosshair button
- Cursor changes to crosshair
- Mouse move updates target window (via `QCursor::pos()` + X11 queries)
- Release to lock selection
- Query window properties:
  ```cpp
  WindowHandle hwnd = windowSystem->getWindowFromPoint(x, y);
  std::string className = windowSystem->getClassName(hwnd);
  std::string title = windowSystem->getWindowText(hwnd);
  Rect rect = windowSystem->getWindowRect(hwnd);
  uint32_t threadId = windowSystem->getWindowThreadProcessId(hwnd);
  ```

#### 4.3 Keyboard Input Capture
- QLineEdit with focus event handling
- On focus: `engine->enableLogMode(true)`
- On blur: `engine->enableLogMode(false)`
- Display last key press with modifiers (E-/U-/D-)

#### 4.4 Positioning
- Opens near Log dialog if open
- Resizable, position persisted in QSettings

**Deliverables:**
- ✅ Investigate dialog working
- ✅ Crosshair selector functional
- ✅ Window info displayed
- ✅ Keyboard input capture works

---

### Track 5: Engine Integration & Notifications
**Purpose:** Connect engine to GUI for full functionality
**Priority:** CRITICAL
**Estimated Effort:** 40-60 hours
**Dependencies:** Track 1 complete

#### 5.1 Engine Notification System
**Files:** `engine_notifier_qt.h/cpp`, `main_qt.cpp`
**Design:**
```cpp
class EngineNotifierQt : public QObject {
    Q_OBJECT
signals:
    void shellExecuteRequested(const std::string& command);
    void loadSettingRequested();
    void helpMessageChanged(const std::string& title, const std::string& msg);
    void showDialogRequested(DialogType type, bool show);
    void setForegroundWindowRequested(WindowHandle hwnd);
    void clearLogRequested();

public slots:
    void handleEngineNotify(EngineNotify type, void* data);
};

// In main_qt.cpp:
EngineNotifierQt* notifier = new EngineNotifierQt();
engine->setNotificationCallback([=](EngineNotify type, void* data) {
    emit notifier->handleEngineNotify(type, data);
});

// Connect to GUI:
connect(notifier, &EngineNotifierQt::shellExecuteRequested,
        this, &MainWindow::executeShell);
connect(notifier, &EngineNotifierQt::helpMessageChanged,
        trayIcon, &TrayIconQt::showHelpBalloon);
// ... etc
```

#### 5.2 Shell Execute Support
**Implementation:**
```cpp
void MainWindow::executeShell(const std::string& command) {
    QProcess::startDetached(QString::fromStdString(command));
}
```

#### 5.3 Help Message Balloons
**Implementation:**
```cpp
void TrayIconQt::showHelpBalloon(const std::string& title, const std::string& msg) {
    if (!msg.empty()) {
        showMessage(QString::fromStdString(title),
                   QString::fromStdString(msg),
                   QSystemTrayIcon::Information);
    }
}
```

#### 5.4 Dialog Show/Hide Commands
**Implementation:**
- Keep dialog instances (don't delete on close)
- Show/hide on command:
  ```cpp
  void MainWindow::showDialog(DialogType type, bool show) {
      switch (type) {
      case DialogType::Investigate:
          if (show) investigateDialog->show();
          else investigateDialog->hide();
          break;
      case DialogType::Log:
          if (show) logDialog->show();
          else logDialog->hide();
          break;
      }
  }
  ```

#### 5.5 Log Integration
**Files:** `dialog_log_qt.cpp`
**Work:**
- Connect engine's `tomsgstream` to log dialog
- Real-time log streaming via Qt signals
- Detail level control (checkbox):
  ```cpp
  void onDetailLevelChanged(bool checked) {
      int level = checked ? 1 : 0;
      engine->setLogLevel(level);
  }
  ```

#### 5.6 Startup Banner
**Implementation:**
- Display in log dialog on startup:
  ```cpp
  log->appendLog("YAMY " + version + " (" + build + ")");
  log->appendLog("Compiled: " + date + " " + time);
  log->appendLog("Platform: Linux Qt " + Qt::version());
  ```

**Deliverables:**
- ✅ EngineNotifierQt working
- ✅ All 6 notification types handled
- ✅ Shell execute works
- ✅ Help balloons work
- ✅ Dialog show/hide works
- ✅ Log streaming integrated
- ✅ Banner displays on startup

---

### Track 6: Advanced Features
**Purpose:** Power user tools and polish
**Priority:** LOW
**Estimated Effort:** 30-50 hours
**Dependencies:** Tracks 1-5 complete

#### 6.1 Keyboard State Checker
**Files:** Add to Investigate dialog or separate dialog
**Features:**
- Display all pressed keys (from engine)
- Show lock key states (Caps, Num, Scroll)
- Show modifier states (Shift, Ctrl, Alt, Meta)
- Refresh on timer

#### 6.2 Font Selection (Log Dialog)
**Files:** `dialog_log_qt.cpp`
**Work:**
- Add "Change Font..." button
- Open QFontDialog
- Apply font to QTextEdit
- Save to QSettings:
  ```cpp
  void onChangeFont() {
      bool ok;
      QFont font = QFontDialog::getFont(&ok, m_logView->font(), this);
      if (ok) {
          m_logView->setFont(font);
          QSettings settings;
          settings.setValue("ui/logFont", font);
      }
  }
  ```
- Load font on startup

#### 6.3 IPC API for External Control
**Files:** New `ipc_server_qt.h/cpp`
**Implementation:**
- D-Bus service on Linux:
  ```xml
  <interface name="net.gimy.yamy">
      <method name="Enable">
          <arg type="b" direction="in"/>
      </method>
      <method name="Reload"/>
      <method name="Quit"/>
  </interface>
  ```
- Or Unix domain socket
- Or QLocalServer

#### 6.4 Session Management (Linux Equivalent)
**Purpose:** Handle screen lock, suspend events
**Implementation:**
- Listen to D-Bus signals:
  - `org.freedesktop.login1.SessionRemoved` - Logout
  - `org.freedesktop.ScreenSaver.ActiveChanged` - Screen lock
  - `org.freedesktop.login1.PrepareForSleep` - Suspend
- Pause engine on lock/suspend
- Resume on unlock/wake

#### 6.5 Help File Support
**Files:** `tray_icon_qt.cpp`
**Work:**
- Add "Help" menu item
- Open default browser with help URL:
  ```cpp
  void onHelp() {
      QDesktopServices::openUrl(
          QUrl("https://github.com/yourusername/yamy/wiki"));
  }
  ```

**Deliverables:**
- ✅ Keyboard state checker
- ✅ Font selection working
- ✅ IPC API implemented
- ✅ Session management working
- ✅ Help menu functional

---

## Implementation Phases & Timeline

### Phase A: Foundation (Weeks 1-4)
**Focus:** Core refactoring to enable Linux builds
- Track 1: Core Refactoring (complete)
- **Milestone:** Engine compiles and links on Linux

### Phase B: Essential Features (Weeks 5-7)
**Focus:** Configuration management and reload
- Track 2: Configuration Management
- Track 3: Edit Setting Dialog
- **Milestone:** Multi-config support with switching works

### Phase C: Engine Integration (Weeks 8-9)
**Focus:** Connect real engine to GUI
- Track 5: Engine Integration & Notifications
- **Milestone:** Full engine functionality working

### Phase D: Debugging Tools (Weeks 10-11)
**Focus:** Developer experience
- Track 4: Investigate Dialog
- **Milestone:** Debug tools functional

### Phase E: Polish (Week 12)
**Focus:** Advanced features and UX
- Track 6: Advanced Features
- Final testing and bug fixes
- **Milestone:** Feature parity with Windows GUI

---

## Detailed Task Breakdown

### Track 1 Tasks (Core Refactoring)
```
├─ T1.1: Platform type abstraction [30h]
│  ├─ Replace HWND everywhere [8h]
│  ├─ Replace tstring everywhere [8h]
│  ├─ Replace Win32 constants [8h]
│  └─ Test compilation on Linux [6h]
├─ T1.2: Message system abstraction [25h]
│  ├─ Design IPC interface [4h]
│  ├─ Implement Linux IPC backend [10h]
│  ├─ Refactor PostMessage calls [8h]
│  └─ Test IPC on both platforms [3h]
├─ T1.3: ConfigStore cleanup [15h]
│  ├─ Remove tstring overloads [3h]
│  ├─ Implement Linux backend [8h]
│  └─ Test on both platforms [4h]
└─ T1.4: SettingLoader cross-platform [10h]
   ├─ Test .mayu parsing on Linux [4h]
   ├─ Fix any UTF-8 issues [4h]
   └─ Integration test [2h]
Total: 80h
```

### Track 2 Tasks (Configuration Management)
```
├─ T2.1: Configuration storage [12h]
│  ├─ Define MayuConfiguration struct [2h]
│  ├─ Implement serialization [4h]
│  ├─ QSettings storage format [4h]
│  └─ Unit tests [2h]
├─ T2.2: ConfigManager class [15h]
│  ├─ Implement all methods [8h]
│  ├─ QSettings integration [4h]
│  └─ Unit tests [3h]
├─ T2.3: Dynamic reload submenu [8h]
│  ├─ Menu creation logic [4h]
│  ├─ onReloadConfig slot [3h]
│  └─ Testing [1h]
└─ T2.4: Settings dialog enhancement [10h]
   ├─ Add symbols column [3h]
   ├─ Up/down buttons [3h]
   ├─ Edit button integration [2h]
   └─ Testing [2h]
Total: 45h
```

### Track 3 Tasks (Edit Setting Dialog)
```
├─ T3.1: Dialog UI implementation [15h]
│  ├─ Layout design [3h]
│  ├─ Widgets and connections [6h]
│  ├─ Validation logic [4h]
│  └─ Polish [2h]
└─ T3.2: Integration [5h]
   ├─ Settings dialog hooks [2h]
   ├─ Data flow [2h]
   └─ Testing [1h]
Total: 20h
```

### Track 4 Tasks (Investigate Dialog)
```
├─ T4.1: Window info panel [20h]
│  ├─ UI layout [5h]
│  ├─ Window property queries [8h]
│  ├─ Display updates [5h]
│  └─ Testing [2h]
├─ T4.2: Crosshair selector [25h]
│  ├─ Crosshair cursor logic [8h]
│  ├─ X11 window-from-point [10h]
│  ├─ Visual feedback [5h]
│  └─ Testing [2h]
├─ T4.3: Keyboard input capture [10h]
│  ├─ QLineEdit focus handling [4h]
│  ├─ Log mode toggling [3h]
│  ├─ Display last key [2h]
│  └─ Testing [1h]
└─ T4.4: Positioning & persistence [5h]
   ├─ QSettings save/load [2h]
   ├─ Relative positioning [2h]
   └─ Testing [1h]
Total: 60h
```

### Track 5 Tasks (Engine Integration)
```
├─ T5.1: Notification system [12h]
│  ├─ EngineNotifierQt class [4h]
│  ├─ Signal/slot connections [4h]
│  ├─ Engine callback registration [3h]
│  └─ Testing [1h]
├─ T5.2: Shell execute [3h]
├─ T5.3: Help balloons [4h]
├─ T5.4: Dialog show/hide [6h]
│  ├─ Keep dialog instances [2h]
│  ├─ Show/hide logic [3h]
│  └─ Testing [1h]
├─ T5.5: Log integration [8h]
│  ├─ Stream redirection [4h]
│  ├─ Detail level control [2h]
│  └─ Testing [2h]
└─ T5.6: Startup banner [2h]
Total: 35h
```

### Track 6 Tasks (Advanced Features)
```
├─ T6.1: Keyboard state checker [12h]
│  ├─ UI implementation [6h]
│  ├─ State queries [4h]
│  └─ Testing [2h]
├─ T6.2: Font selection [6h]
│  ├─ Font dialog integration [3h]
│  ├─ QSettings persistence [2h]
│  └─ Testing [1h]
├─ T6.3: IPC API [15h]
│  ├─ D-Bus service definition [4h]
│  ├─ Implementation [8h]
│  ├─ Testing [3h]
├─ T6.4: Session management [10h]
│  ├─ D-Bus signal listeners [6h]
│  ├─ Pause/resume logic [3h]
│  └─ Testing [1h]
└─ T6.5: Help menu [2h]
Total: 45h
```

**Grand Total: 285 hours**

---

## Dependencies Graph

```
Track 1 (Core Refactoring)
    │
    ├─→ Track 2 (Configuration) ──┐
    │       │                      │
    │       └─→ Track 3 (Edit)     │
    │                              │
    └─→ Track 5 (Engine) ──────────┤
            │                      │
            └─→ Track 4 (Investigate)
                                   │
                                   └─→ Track 6 (Advanced)
```

**Critical Path:** Track 1 → Track 5 → Track 4 → Track 6 (155h minimum)

---

## Risk Assessment

### High Risk
1. **Core refactoring complexity**
   - Many Windows types deeply embedded
   - **Mitigation:** Incremental refactoring, one type at a time
   - **Fallback:** Keep Windows ifdef blocks temporarily

2. **Engine notification callback changes**
   - Requires modifying Engine class API
   - **Mitigation:** Use observer pattern, minimize Engine changes
   - **Fallback:** Polling instead of callbacks

### Medium Risk
3. **X11 window selector reliability**
   - Different WMs behave differently
   - **Mitigation:** Test on multiple DEs (GNOME, KDE, XFCE)
   - **Fallback:** Simplified selector without live preview

4. **.mayu file parsing edge cases**
   - May have Windows-specific assumptions
   - **Mitigation:** Comprehensive test suite with real .mayu files
   - **Fallback:** Document unsupported features

### Low Risk
5. **QSettings format differences**
   - Different from Windows registry
   - **Mitigation:** Migration tool for Windows users
   - **Fallback:** Manual configuration entry

---

## Testing Strategy

### Unit Tests
- ConfigManager: All methods, edge cases
- MayuConfiguration: Serialization round-trip
- EngineNotifierQt: Signal emission

### Integration Tests
- Configuration load → reload → switch
- Engine notifications → GUI updates
- Dialog interactions → Config changes

### Manual Testing Checklist
```
[ ] Install fresh YAMY on Linux
[ ] Add 3 configurations
[ ] Switch between configurations via menu
[ ] Edit configuration symbols
[ ] Reload configuration after .mayu edit
[ ] Open Investigate dialog
[ ] Select window with crosshair
[ ] Type in keyboard input field (log mode)
[ ] Change log font
[ ] Clear log
[ ] Save log to file
[ ] Enable/disable via tray icon
[ ] Enable/disable via double-click
[ ] Check keyboard state
[ ] Lock screen → verify engine pauses
[ ] Unlock screen → verify engine resumes
[ ] External IPC command (if implemented)
[ ] Help menu opens browser
```

---

## Acceptance Criteria

### Minimum Viable (Release 1.0)
- ✅ Track 1 complete (engine compiles)
- ✅ Track 2 complete (multi-config works)
- ✅ Track 3 complete (edit dialog works)
- ✅ Track 5 (partial) - Basic engine integration
- ✅ All dialogs functional
- ✅ Configuration persists across restarts

### Feature Parity (Release 2.0)
- ✅ All 6 tracks complete
- ✅ Investigate dialog fully functional
- ✅ All Windows features working on Linux
- ✅ IPC API for external control
- ✅ Session management working
- ✅ Comprehensive documentation

---

## Next Steps

1. **Review and approve this plan** with stakeholders
2. **Set up development environment** for Linux
3. **Create test .mayu files** for integration testing
4. **Begin Track 1** (Core Refactoring)
5. **Set up CI/CD** for Linux builds
6. **Create milestones** in issue tracker

---

**Document Version:** 1.0
**Last Updated:** 2025-12-10
**Author:** Claude Code
**Status:** Pending Approval
