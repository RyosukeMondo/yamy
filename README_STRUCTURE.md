# Yamy Project Structure

This repository has been reorganized for better clarity and maintainability.

## Directory Structure

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
- **`scripts/`**: Build and utility scripts (e.g., `build_yamy.bat`).
- **`setup/`**: Installer and setup related files (formerly `s/`).
- **`docs/`**: Documentation files (`.html`, `.txt`).
- **`tools/`**: Helper tools and utilities.

## Building

To build the project, run the build script from the `scripts` directory:

```bat
scripts\build_yamy.bat
```

This script will set up the environment, build the Release configurations (x64 and Win32), and handle artifact renaming.
