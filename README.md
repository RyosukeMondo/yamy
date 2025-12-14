# Yamy (Yet Another Mado tsukai no Yuutsu)

**Yamy** is a key binding customization tool for Windows and Linux, derived from "Mado tsukai no Yuutsu" (Mayu).

The current architecture separates a **headless daemon (`yamy`)** from a **Qt Widgets front-end (`yamy-gui`)**. The daemon hosts the engine and IPC server; the GUI connects over the IPC channel to show status, toggle enable/disable, switch/reload configs, and launch existing dialogs.

## Overview

The original "Mado tsukai no Yuutsu" (Mayu) used a filter driver to replace key inputs. Yamy changes this approach to use a user-mode hook (`WH_KEYBOARD_LL`) and `SendInput()` API. This allows Yamy to work on modern Windows versions (Vista and later, including 64-bit) without requiring a signed driver, although some low-level replacement capabilities are traded off.

## Features

### Core Functionality
- **Key Remapping**: Remap any key to any other key using intuitive `.mayu` configuration syntax
- **Modal Layers**: Create Vim-style modal layers with custom key behaviors per layer
- **Substitution System**: Define key substitutions with support for modifiers and combinations
- **Cross-Platform**: Full support for both Windows and Linux with unified configuration

### Advanced Features

#### Number Keys as Custom Modifiers
Use number keys (0-9) as hardware modifier keys when held, perfect for small keyboards (60%, 65%, 75% layouts):

- **Hold-Tap Detection**: Hold number key ≥200ms → activates modifier (Shift, Ctrl, Alt, Win)
- **Tap Behavior**: Tap < 200ms → normal key behavior (or substitution if configured)
- **System-Wide**: Modifiers work in all applications, not just YAMY
- **Dual-Purpose**: Number keys serve double duty without sacrificing functionality

**Example Configuration:**
```mayu
# Hold number keys as modifiers
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt

# Tap for function keys
*_1 = *F1
*_2 = *F2
*_3 = *F3
```

**Usage:**
- Hold `1` + press `A` → Shift+A (capital A)
- Tap `1` quickly → F1 (if substitution configured)

See [Number Modifier User Guide](docs/NUMBER_MODIFIER_USER_GUIDE.md) for complete documentation.

## Status

This repository is a fork of the original Yamy project, reorganized for better maintainability and extended with **full Linux support**.

- **Original Mayu**: [http://mayu.sourceforge.net/](http://mayu.sourceforge.net/)
- **Original Yamy**: [http://yamy.sourceforge.jp/](http://yamy.sourceforge.jp/)

## Installation

### Linux (daemon + GUI, source build)

```bash
cmake -B build_release -DCMAKE_BUILD_TYPE=Release -DBUILD_LINUX_STUB=ON -DBUILD_QT_GUI=ON
cmake --build build_release -j$(nproc)

# 1) Start headless daemon (needs input permissions)
./build_release/bin/yamy --no-restore

# 2) Launch GUI (connects to IPC server name yamy-engine by default)
./build_release/bin/yamy-gui
```

See `docs/USER_GUIDE.md` for the full GUI walkthrough, troubleshooting, and screenshots.

### Linux (Ubuntu/Debian)

#### From Binary Release (Recommended)

```bash
# Install Qt5 dependencies
sudo apt update
sudo apt install -y libqt5core5a libqt5widgets5 libqt5gui5 libx11-6

# Download and install the latest release
wget https://github.com/RyosukeMondo/yamy/releases/download/v1.0.0/yamy-1.0.0-amd64.deb
sudo apt install ./yamy-1.0.0-amd64.deb

# Add your user to the input group (required for keyboard access)
sudo usermod -aG input $USER

# Log out and back in for group membership to take effect
```

See [RELEASE-NOTES-1.0.md](RELEASE-NOTES-1.0.md) for installation instructions for other distributions.

#### From Source

```bash
# 1. Install build dependencies
sudo apt update
sudo apt install -y build-essential cmake \
    libqt5core5a libqt5widgets5 libqt5gui5 libqt5x11extras5 \
    qtbase5-dev libqt5x11extras5-dev \
    libx11-dev libxrandr-dev libudev-dev

# 2. Build the project
./scripts/linux/build_linux_package.sh

# 3. Install binaries (Qt5 GUI application)
sudo cp build_release/bin/yamy /usr/local/bin/yamy
sudo cp build_release/bin/yamy-ctl /usr/local/bin/yamy-ctl
sudo chmod +x /usr/local/bin/yamy /usr/local/bin/yamy-ctl

# 4. Add your user to the input group (REQUIRED for keyboard capture)
sudo usermod -aG input $USER

# 5. Log out and log back in for the group change to take effect

# 6. Launch yamy
yamy
```

**Note**: The main application `yamy` is a full Qt5 GUI application with system tray icon and configuration dialogs.

### Windows

Download the latest release from the [Releases page](https://github.com/RyosukeMondo/yamy/releases).

## Directory Structure

The project structure has been reorganized for better clarity and maintainability.

- **`src/`**: Core source code.
  - **`core/`**: Main engine logic (key mapping, parser, settings).
  - **`platform/`**: Platform abstraction layer (Windows, Linux stubs).
  - **`ui/`**: User interface (dialogs, resources).
  - **`system/`**: Low-level system hooks, registry, and driver interaction.
  - **`utils/`**: General purpose utilities and helper classes.
  - **`app/`**: Application entry points (`yamy.cpp`, `mayu.cpp`).
- **`proj/`**: External library dependencies.
- **`driver/`**: Device driver source code and related files (formerly `d/`).
- **`keymaps/`**: Default and contributed keymap files (`.mayu`).
- **`resources/`**: Icons, cursors, and other resource files (formerly `r/`).
- **`scripts/`**: Build, utility, and tracking scripts.
- **`setup/`**: Installer and setup related files (formerly `s/`).
- **`docs/`**: Documentation files (`.html`, `.txt`, `.md`).

## Building

### Linux

Build script for creating distributable packages:

```bash
./scripts/linux/build_linux_package.sh [VERSION]
```

This will:
1. Configure CMake with Release build type
2. Enable Qt5 GUI and Linux platform support
3. Build all binaries (`yamy`, `yamy-ctl`)
4. Create a tarball in `dist/` directory

**Manual build:**

```bash
# Configure
cmake -B build_release \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_LINUX_STUB=ON \
    -DBUILD_QT_GUI=ON \
    -DBUILD_LINUX_TESTING=OFF \
    -DBUILD_REGRESSION_TESTS=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr

# Build
cmake --build build_release -j$(nproc)

# Binaries will be in build_release/bin/
```

### Visual Studio (MSVC)

To build the project using Visual Studio (or MSVC toolchain), run the packaging script from the `scripts` directory:

```powershell
scripts/windows/cmake_package.ps1
```

**Options:**
- **Default**: Incremental build. Uses existing `build/` directory if present for faster compilation.
- **Clean Build**: Use `-Clean` switch to force a full rebuild (e.g., `scripts/windows/cmake_package.ps1 -Clean`).

The script will:
1.  Run quality checks (anti-patterns, missing sources, encoding).
2.  Build both 64-bit and 32-bit binaries (if compilers are available).
3.  Package everything into `dist/yamy-dist.zip`.
4.  Save build logs to `logs/build_log_x64.txt` and `logs/build_log_x86.txt`.

### MinGW-w64 (MSYS2)

Yamy supports building with MinGW-w64, producing standalone artifacts without Visual Studio dependencies.

To build, run the packaging script:
```powershell
scripts/mingw_package.ps1
```

**Requirements:**
- **64-bit Build**: `mingw-w64-x86_64-toolchain`
- **32-bit Build**: `mingw-w64-i686-toolchain` (Optional, but required for `yamy32.exe`)
- **CMake**: Must be installed and in PATH (or `C:\Program Files\CMake\bin`).

The script automatically detects if the 32-bit toolchain is installed (e.g., at `C:\tools\msys64\mingw32`). If found, it will build the 32-bit binaries (`yamy32.exe`, `yamy32.dll`, `yamyd32.exe`) and include them in the final zip package. If not found, only 64-bit binaries will be built.

## Development & Progress Tracking

### Tracking Legacy Code Migration

To track the progress of modernization efforts (legacy string usage, Win32 type leakage), run:

```bash
bash scripts/linux/track_legacy_strings.sh
```

This script provides metrics on:
- Legacy string usage (tstring, _T(), _TCHAR, tstringi)
- Win32 type leakage in core code
- windows.h include verification
- Progress summary with status indicators

### Continuous Integration

The project uses GitHub Actions for automated builds and verification:
- **Linux Build**: Verifies architectural decoupling with Linux stub implementations
- **Windows Build**: Tests both MinGW and MSVC builds
- **Static Analysis**: Ensures no Windows dependencies leak into `src/core`

## License and Copyright

### Yamy
**Copyright (C) 2009, KOBAYASHI Yoshiaki <gimy@users.sourceforge.jp>**  
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

(See full license text in [docs/readme.txt](docs/readme.txt))

### Mado tsukai no Yuutsu (Mayu)
**Copyright (C) 1999-2005, TAGA Nayuta <nayuta@users.sourceforge.net>**
All rights reserved.

(See full license text in [docs/readme.txt](docs/readme.txt))

## Acknowledgments

We express our deepest gratitude to **TAGA Nayuta**, the author of "Mado tsukai no Yuutsu", and all contributors who made this software possible.
