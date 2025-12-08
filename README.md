# Yamy (Yet Another Mado tsukai no Yuutsu)

**Yamy** is a key binding customization tool for Windows, derived from "Mado tsukai no Yuutsu" (Mayu).

## Overview

The original "Mado tsukai no Yuutsu" (Mayu) used a filter driver to replace key inputs. Yamy changes this approach to use a user-mode hook (`WH_KEYBOARD_LL`) and `SendInput()` API. This allows Yamy to work on modern Windows versions (Vista and later, including 64-bit) without requiring a signed driver, although some low-level replacement capabilities are traded off.

## Status

This repository is a fork of the original Yamy project, reorganized for better maintainability.

- **Original Mayu**: [http://mayu.sourceforge.net/](http://mayu.sourceforge.net/)
- **Original Yamy**: [http://yamy.sourceforge.jp/](http://yamy.sourceforge.jp/)

## Directory Structure

The project structure has been reorganized for better clarity and maintainability.

- **`src/`**: Core source code.
  - **`core/`**: Main engine logic (key mapping, parser, settings).
  - **`ui/`**: User interface (dialogs, resources).
  - **`system/`**: Low-level system hooks, registry, and driver interaction.
  - **`utils/`**: General purpose utilities and helper classes.
  - **`app/`**: Application entry points (`yamy.cpp`, `mayu.cpp`).
- **`proj/`**: Visual Studio project and solution files (`.sln`, `.vcxproj`).
- **`driver/`**: Device driver source code and related files (formerly `d/`).
- **`keymaps/`**: Default and contributed keymap files (`.mayu`).
- **`resources/`**: Icons, cursors, and other resource files (formerly `r/`).
- **`scripts/`**: Build and utility scripts.
- **`setup/`**: Installer and setup related files (formerly `s/`).
- **`docs/`**: Documentation files (`.html`, `.txt`).

## Building

### Visual Studio (MSVC)

To build the project using Visual Studio, run the build script from the `scripts` directory:

```bat
scripts\build_yamy.bat
```

This script will set up the environment, build the Release configurations (x64 and Win32), and handle artifact renaming.

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

### Boost C++ Libraries
This software uses Boost C++ Libraries.
**Boost Software License - Version 1.0**

## Acknowledgments

We express our deepest gratitude to **TAGA Nayuta**, the author of "Mado tsukai no Yuutsu", and all contributors who made this software possible.
