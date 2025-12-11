# YAMY User Guide

A comprehensive guide to using YAMY (Yet Another Mado tsukai no Yuutsu) for keyboard remapping on Linux.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
   - [Dependencies](#dependencies)
   - [Building from Source](#building-from-source)
   - [Binary Installation](#binary-installation)
3. [First Run and Setup](#first-run-and-setup)
   - [Permission Setup](#permission-setup)
   - [Starting YAMY](#starting-yamy)
4. [Basic Configuration](#basic-configuration)
   - [Configuration File Location](#configuration-file-location)
   - [Understanding .mayu Files](#understanding-mayu-files)
   - [Using Configuration Templates](#using-configuration-templates)
5. [System Tray Usage](#system-tray-usage)
   - [Tray Icon States](#tray-icon-states)
   - [Context Menu](#context-menu)
6. [Dialogs and Windows](#dialogs-and-windows)
   - [Investigate Dialog](#investigate-dialog)
   - [Log Dialog](#log-dialog)
   - [Settings Dialog](#settings-dialog)
   - [Preferences Dialog](#preferences-dialog)
   - [Configuration Manager](#configuration-manager)
7. [Common Use Cases](#common-use-cases)
8. [Quick Reference](#quick-reference)

---

## Introduction

YAMY is a powerful keyboard remapping tool that lets you customize your keyboard layout and create shortcuts across all applications. Originally developed for Windows, this Linux port provides the same powerful configuration language with native Linux integration.

### What Can YAMY Do?

- **Remap keys globally**: Change any key to any other key across all applications
- **Create keyboard shortcuts**: Define custom key combinations for actions
- **Window management**: Control windows with keyboard shortcuts (move, resize, maximize)
- **Application-specific keymaps**: Different keymaps for different applications
- **Emacs/Vim keybindings**: Use your preferred editing keybindings everywhere
- **Modifier key remapping**: Make CapsLock act as Control, etc.

---

## Installation

### Dependencies

YAMY on Linux requires the following dependencies:

**Build Dependencies:**
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake qt6-base-dev libx11-dev libxi-dev libudev-dev

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel libX11-devel libXi-devel systemd-devel

# Arch Linux
sudo pacman -S base-devel cmake qt6-base libx11 libxi
```

**Runtime Dependencies:**
- Qt 6 libraries
- X11 libraries (for window management)
- uinput kernel module (for key injection)

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/your-repo/yamy.git
   cd yamy
   ```

2. **Create build directory:**
   ```bash
   mkdir -p build_linux
   cd build_linux
   ```

3. **Configure with CMake:**
   ```bash
   cmake ..
   ```

4. **Build:**
   ```bash
   make -j$(nproc)
   ```

5. **Install (optional):**
   ```bash
   sudo make install
   ```

The built binary will be at `build_linux/bin/yamy`.

### Binary Installation

If pre-built binaries are available:

1. Download the appropriate package for your distribution
2. Install using your package manager:
   ```bash
   # Debian/Ubuntu
   sudo dpkg -i yamy_*.deb

   # Fedora/RHEL
   sudo rpm -i yamy-*.rpm
   ```

---

## First Run and Setup

### Permission Setup

YAMY needs special permissions to capture keyboard input and inject key events. Run the setup script:

```bash
sudo ./linux_setup.sh
```

This script:
1. Adds your user to the `input` group
2. Creates udev rules for input device access
3. Loads the `uinput` kernel module

**Important:** Log out and log back in for group changes to take effect.

#### Manual Permission Setup

If you prefer manual setup:

```bash
# Add user to input group
sudo usermod -a -G input $USER

# Create udev rule
sudo tee /etc/udev/rules.d/99-input.rules > /dev/null <<'EOF'
KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Load uinput module
sudo modprobe uinput
echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf
```

### Starting YAMY

After setup, start YAMY:

```bash
./yamy
```

Or if installed system-wide:

```bash
yamy
```

YAMY will:
1. Load your configuration file
2. Start capturing keyboard events
3. Display a system tray icon

---

## Basic Configuration

### Configuration File Location

YAMY looks for configuration files in these locations (in order):

1. `~/.config/yamy/config.mayu` (recommended)
2. `~/.yamy/config.mayu`
3. `./config.mayu` (current directory)

Create your configuration directory:

```bash
mkdir -p ~/.config/yamy
```

### Understanding .mayu Files

YAMY uses `.mayu` configuration files with a simple but powerful syntax. Here's a basic example:

```mayu
# Comments start with #

# Define a global keymap
keymap Global

# Remap CapsLock to Control
mod control += CapsLock
key *CapsLock = *LControl

# Create a shortcut: Ctrl+Shift+Q closes window
key C-S-Q = A-F4
```

Key concepts:

| Concept | Description | Example |
|---------|-------------|---------|
| `keymap` | Defines a set of key bindings | `keymap Global` |
| `key` | Maps one key/combo to another | `key C-A = Home` |
| `mod` | Modifies modifier key behavior | `mod control += CapsLock` |
| `window` | Creates app-specific keymaps | `window Firefox /:firefox:/` |

**Modifier Prefixes:**
- `C-` = Control
- `S-` = Shift
- `A-` or `M-` = Alt (Meta)
- `W-` = Super (Windows key)

For detailed syntax, see the [Configuration Guide](configuration-guide.md).

### Using Configuration Templates

YAMY includes pre-built templates for common setups:

1. **Default Template** (`default.mayu`): Basic remappings including CapsLock to Control
2. **Emacs Template** (`emacs.mayu`): Emacs-style keybindings system-wide
3. **Vim Template** (`vim.mayu`): Vim-style modal editing

To use a template, copy it to your config directory:

```bash
cp /path/to/yamy/src/resources/templates/emacs.mayu ~/.config/yamy/config.mayu
```

---

## System Tray Usage

### Tray Icon States

The system tray icon indicates YAMY's current state:

| Icon | State | Description |
|------|-------|-------------|
| Green/Normal | Running | YAMY is active and remapping keys |
| Gray/Dimmed | Disabled | YAMY is paused, keys pass through normally |
| Yellow | Loading | Configuration is being loaded |
| Red | Error | An error occurred (check log) |

### Context Menu

Right-click the tray icon for the context menu:

| Menu Item | Description |
|-----------|-------------|
| **Enable/Disable** | Toggle key remapping on/off |
| **Reload** | Reload configuration file |
| **Configurations** | Switch between config files |
| **Investigate** | Open window investigation dialog |
| **Log** | View YAMY log messages |
| **Preferences** | Open preferences dialog |
| **Help** | Access documentation and examples |
| **About** | Version and license information |
| **Exit** | Quit YAMY |

**Quick Actions:**
- **Left-click**: Toggle enable/disable
- **Double-click**: Open settings dialog

---

## Dialogs and Windows

### Investigate Dialog

The Investigate dialog helps you identify windows for creating application-specific keymaps.

**How to use:**

1. Open from tray menu: **Investigate**
2. Click **Select Window**
3. Click on any window to investigate
4. View window information:
   - Window title and class name
   - Process name and path
   - Window geometry
   - Active keymap

**Copying information:**

Click **Copy to Clipboard** to copy window information for use in your configuration.

**Generating conditions:**

Click **Generate Condition** to automatically create a `window` directive for your config file.

### Log Dialog

The Log dialog shows real-time YAMY activity.

**Features:**

- **Level filtering**: Show only errors, warnings, info, or all messages
- **Category filtering**: Filter by Engine, Parser, Input, Window, or Config
- **Search**: Find specific messages with Ctrl+F
- **Export**: Save logs to a file for troubleshooting
- **Pause/Resume**: Stop scrolling to read messages
- **Statistics**: View counts by log level

**Keyboard shortcuts:**
- `Ctrl+F` - Find in log
- `F3` / `Shift+F3` - Find next/previous
- `Ctrl+S` - Save log to file

### Settings Dialog

The Settings dialog configures the active configuration file:

- Select which `.mayu` file to load
- View current configuration status
- Access configuration templates

### Preferences Dialog

The Preferences dialog contains application-wide settings organized in tabs:

**General Tab:**
- Start on login
- Quick-switch hotkey for cycling configurations
- Default configuration file

**Notifications Tab:**
- Desktop notification settings
- Sound notification settings
- Volume control

**Logging Tab:**
- Log level (Error, Warning, Info, Debug, Trace)
- Log buffer size
- Log file output

**Advanced Tab:**
- Performance metrics interval
- Debug mode
- Performance overlay

### Configuration Manager

The Configuration Manager lets you:

- View all available configurations
- Create new configurations
- Edit existing configurations
- Delete configurations
- Set the active configuration
- Add metadata (name, description, tags)

---

## Common Use Cases

### CapsLock as Control

The most popular remapping. Add to your config:

```mayu
keymap Global
mod control += CapsLock
key *CapsLock = *LControl
```

### Emacs-Style Navigation

Navigate with Ctrl+N/P/F/B in all applications:

```mayu
keymap Global
key C-F = Right
key C-B = Left
key C-N = Down
key C-P = Up
key C-A = Home
key C-E = End
```

### Window Management Shortcuts

Control windows without the mouse:

```mayu
keymap Global
key C-S-Left   = &WindowMove(-16, 0)   # Move window left
key C-S-Right  = &WindowMove(16, 0)    # Move window right
key C-S-Up     = &WindowMove(0, -16)   # Move window up
key C-S-Down   = &WindowMove(0, 16)    # Move window down
key C-S-Z      = &WindowMaximize       # Maximize
key C-S-I      = &WindowMinimize       # Minimize
```

### Application-Specific Keymaps

Different keybindings for specific applications:

```mayu
# Firefox-specific keymaps
window Firefox /:firefox:/ : Global
key C-L = F6  # Focus address bar

# Terminal exceptions - don't remap in terminal
window Terminal /:gnome-terminal:/ : Global
key C-A = &Default  # Let terminal handle Ctrl+A
```

---

## Quick Reference

### Essential Shortcuts (Default Template)

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+S` | Reload configuration |
| `Ctrl+Shift+D` | Investigate current window |
| `Ctrl+Shift+Z` | Maximize window |
| `Ctrl+Shift+I` | Minimize window |
| `CapsLock` | Acts as Control |

### Configuration Syntax Quick Reference

```mayu
# Key remapping
key OldKey = NewKey

# With modifiers
key C-A = Home           # Ctrl+A -> Home
key C-S-K = A-F4         # Ctrl+Shift+K -> Alt+F4

# Modifier assignment
mod control += CapsLock  # Add CapsLock as Control

# Functions
key C-S-Q = &WindowClose
key W-Left = &MouseMove(-16, 0)

# Window-specific
window AppName /regex-pattern/ : ParentKeymap
```

### Getting Help

- **Documentation**: Right-click tray icon > Help > Documentation
- **Keyboard Shortcuts**: Right-click tray icon > Help > Keyboard Shortcuts
- **Examples**: Right-click tray icon > Help > Configuration Examples
- **Report Bug**: Right-click tray icon > Help > Report Bug

---

## Next Steps

- Read the [Configuration Guide](configuration-guide.md) for complete syntax reference
- Check [Troubleshooting](troubleshooting.md) if you encounter issues
- Explore the example configurations in `keymaps/` directory
