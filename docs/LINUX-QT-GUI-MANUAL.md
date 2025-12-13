# YAMY Linux Qt GUI - Setup and User Manual

## Overview

YAMY now includes a **Qt5-based graphical user interface (GUI)** for Linux, providing the same system tray experience as the Windows version. The Qt GUI offers:

- **System tray icon** with enable/disable states
- **Context menu** with quick actions
- **Settings dialog** for keymap configuration
- **Log viewer** for debugging
- **About dialog** with version information

## Current Status

✅ **Implemented and Working:**
- Qt5 GUI framework integration
- System tray icon with visual state (enabled/disabled)
- Context menu (Enable, Reload, Settings, Log, About, Exit)
- Settings dialog (keymap file management)
- Log viewer dialog (with auto-scroll, save to file)
- About dialog (version, build info, license)
- CMake build system integration
- Platform-specific conditional compilation

⏳ **Pending (requires core refactoring):**
- Full keyboard remapping engine integration
- Configuration reload from .mayu files
- Dynamic keymap switching
- IPC integration

**Note:** The Qt GUI is currently running with a **stub engine** (demo mode). Full engine integration requires refactoring core YAMY code to remove Windows-specific dependencies (`HWND`, `PostMessage`, `SW_*`, `tstring`).

---

## Prerequisites

### Required Packages

#### Ubuntu/Debian:
```bash
sudo apt install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    qttools5-dev \
    libqt5x11extras5-dev \
    libx11-dev \
    libxrandr-dev \
    libudev-dev
```

#### Fedora/RHEL:
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    qt5-qtx11extras-devel \
    libX11-devel \
    libXrandr-devel \
    libudev-devel
```

#### Arch Linux:
```bash
sudo pacman -S \
    base-devel \
    cmake \
    qt5-base \
    qt5-tools \
    qt5-x11extras \
    libx11 \
    libxrandr \
    systemd-libs
```

### Verify Qt5 Installation

```bash
qmake --version
# Should output: QMake version 3.x
# Using Qt version 5.x.x
```

---

## Building YAMY with Qt GUI

### 1. Configure CMake

```bash
cd /path/to/yamy
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

**Expected output:**
```
-- Qt5 found - building with GUI support
-- Qt5 GUI configured for Linux
--   Qt5Widgets: 5.x.x
```

### 2. Build

```bash
cmake --build build
```

**Build targets:**
- `yamy_qt_gui` - Qt GUI library (static)
- `yamy` - Main executable with Qt GUI

### 3. Verify Build

```bash
./build/bin/yamy --version
# Or just run:
./build/bin/yamy
```

---

## Running YAMY Qt GUI

### Standard Launch

```bash
./build/bin/yamy
```

**Expected output:**
```
Starting YAMY on Linux (Qt GUI)
Note: Using stub engine (full integration pending core refactoring)
YAMY Qt GUI initialized. Running...
```

### System Tray Integration

1. **Look for the YAMY icon** in your system tray (usually top-right or bottom-right)
   - 🟢 Green icon = Enabled
   - ⚫ Gray icon = Disabled

2. **Right-click the icon** to open context menu:
   ```
   ┌─────────────────────────┐
   │ ✓ Enable                │
   ├─────────────────────────┤
   │ Reload                  │
   ├─────────────────────────┤
   │ Settings...             │
   │ Log...                  │
   │ About...                │
   ├─────────────────────────┤
   │ Exit                    │
   └─────────────────────────┘
   ```

3. **Double-click the icon** to toggle Enable/Disable

---

## GUI Features

### 1. Enable/Disable Toggle

**Purpose:** Enable or disable keyboard remapping

**How to use:**
- Right-click tray icon → **Enable** (toggle)
- Or double-click tray icon

**Visual feedback:**
- Icon color changes (green=enabled, gray=disabled)
- Desktop notification appears
- Tooltip updates

**Current status:** UI only (no actual remapping until engine integration)

---

### 2. Settings Dialog

**Purpose:** Manage keymap files and configuration

**How to open:**
- Right-click tray icon → **Settings...**

**Features:**
- **Keymap file list** - View all configured .mayu files
- **Add** - Browse and add keymap files
- **Edit** - Open keymap file in editor (placeholder)
- **Remove** - Remove keymap from list
- **Keymap directory** - Set default directory for .mayu files
- **Save/Cancel** - Persist or discard changes

**Settings storage:**
- Uses Qt Settings (stored in `~/.config/YAMY/YAMY.conf`)
- Cross-platform INI format

---

### 3. Log Viewer

**Purpose:** View runtime logs and debug output

**How to open:**
- Right-click tray icon → **Log...**

**Features:**
- **Timestamped entries** - Each log line has timestamp
- **Auto-scroll** - Checkbox to automatically scroll to latest
- **Clear** - Clear all log messages (with confirmation)
- **Save** - Export log to file (.log or .txt)
- **Monospace font** - Easy to read aligned output
- **10,000 line limit** - Automatic trimming of old entries

**Current status:** UI functional, waiting for engine log integration

---

### 4. About Dialog

**Purpose:** Display version and license information

**How to open:**
- Right-click tray icon → **About...**

**Displays:**
- Application name and subtitle
- Version number (0.04)
- Qt version used
- Build date
- Platform information (Qt5 GUI for Linux)
- License (MIT License)

---

## Configuration

### Qt Settings Location

```
~/.config/YAMY/YAMY.conf
```

**Stored settings:**
- Keymap file paths
- Keymap directory
- Window positions (future)
- UI preferences (future)

### Manual Configuration

You can edit the Qt settings file directly:

```ini
[keymaps]
directory=/home/user/.yamy
files=/home/user/.yamy/default.mayu, /home/user/.yamy/custom.mayu
```

---

## Troubleshooting

### Qt GUI doesn't appear

**Problem:** System tray icon doesn't show up

**Solutions:**
1. Check if system tray is available:
   ```bash
   ps aux | grep tray
   ```

2. Try different desktop environment or panel:
   - GNOME: Install `gnome-shell-extension-appindicator`
   - KDE: Works out of the box
   - XFCE: Works out of the box

3. Check console output for errors

### Build fails with "Qt5 not found"

**Problem:** CMake can't find Qt5

**Solutions:**
1. Install Qt5 development packages (see Prerequisites)

2. Set `CMAKE_PREFIX_PATH`:
   ```bash
   export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake
   cmake -B build
   ```

3. Verify qmake is in PATH:
   ```bash
   which qmake
   ```

### Build fails with "libudev.h not found"

**Problem:** Missing libudev development headers

**Solution:**
```bash
sudo apt install libudev-dev  # Ubuntu/Debian
sudo dnf install libudev-devel  # Fedora/RHEL
sudo pacman -S systemd-libs  # Arch
```

### Application crashes on startup

**Problem:** Missing runtime libraries

**Solutions:**
1. Check Qt5 runtime libraries:
   ```bash
   ldd ./build/bin/yamy | grep Qt
   ```

2. Install Qt5 runtime (usually automatic with qtbase5-dev)

3. Check console output for specific error

---

## Development

### File Structure

```
src/
├── ui/qt/                      # Qt GUI implementation
│   ├── tray_icon_qt.h/cpp      # System tray icon
│   ├── dialog_settings_qt.h/cpp # Settings dialog
│   ├── dialog_log_qt.h/cpp     # Log viewer
│   ├── dialog_about_qt.h/cpp   # About dialog
│   ├── resources.qrc            # Qt resources (icons)
│   └── CMakeLists.txt           # Qt build config
│
├── app/
│   └── main_qt.cpp              # Qt GUI entry point
│
└── resources/icons/
    ├── yamy_enabled.png         # Green icon (32x32)
    └── yamy_disabled.png        # Gray icon (32x32)
```

### Build System

**Qt-specific CMake:**
- `CMAKE_AUTOMOC=ON` - Automatic Meta-Object Compiler
- `CMAKE_AUTORCC=ON` - Automatic Resource Compiler
- `CMAKE_AUTOUIC=ON` - Automatic UI Compiler

**Conditional compilation:**
```cmake
if(UNIX AND NOT APPLE)
    option(BUILD_QT_GUI "Build Qt5 GUI for Linux" ON)
    # ...
endif()
```

### Adding New Features

1. **Add new dialog:**
   - Create `dialog_foo_qt.h` and `dialog_foo_qt.cpp`
   - Add to `src/ui/qt/CMakeLists.txt`
   - Connect from tray icon menu

2. **Add menu action:**
   - Edit `tray_icon_qt.cpp::createMenu()`
   - Add slot method `onFoo()`
   - Connect signal to slot

3. **Add resources:**
   - Add files to `src/resources/`
   - Register in `resources.qrc`
   - Access via `:/prefix/filename`

---

## Future Work

### Full Engine Integration

**Requires:**
1. Refactor core YAMY to remove Windows types:
   - Replace `HWND` → `yamy::platform::WindowHandle`
   - Replace `tstring` → `std::string`
   - Replace Win32 constants → Platform-agnostic enums
   - Abstract `PostMessage` → IPC interface

2. Implement Setting loader for Linux:
   - Parse `.mayu` files
   - Create `Setting` objects
   - Call `engine->setSetting()`

3. Connect engine callbacks to GUI:
   - Engine log → Log viewer
   - State changes → Tray icon updates
   - IPC messages → Action handlers

### Additional Features

- **Keymap hot reload** - Reload without restart
- **Dynamic keymap menu** - Show loaded keymaps in menu
- **Investigate window dialog** - X11 window inspection tool
- **Autostart integration** - XDG autostart file
- **Tray notifications** - More informative messages
- **Theme support** - Follow system theme

---

## FAQ

**Q: Does the Qt GUI work on Wayland?**
A: Partial support. Qt5 has X11/Wayland compatibility layer, but input capture requires X11 currently. Future Wayland native support planned.

**Q: Can I run YAMY without GUI (headless)?**
A: Yes, build with `-DBUILD_QT_GUI=OFF` to use headless mode.

**Q: Why is the engine stubbed?**
A: Core YAMY has Windows-specific code (`HWND`, `PostMessage`, etc.) that needs refactoring for Linux compatibility. The Qt GUI is ready and waiting for core refactoring.

**Q: Can I contribute?**
A: Yes! See `CONTRIBUTING.md` for guidelines. Priority areas:
- Core platform abstraction
- Setting loader for Linux
- Engine integration
- Testing and bug reports

**Q: Will this break Windows builds?**
A: No. Windows continues to use Win32 GUI. Qt is Linux-only via conditional compilation.

---

## Support

- **Issues:** https://github.com/yourusername/yamy/issues
- **Discussions:** https://github.com/yourusername/yamy/discussions
- **Documentation:** `docs/` directory

---

**Last updated:** 2025-12-10
**Version:** 0.04 (Qt GUI Phase 1-8 complete)
