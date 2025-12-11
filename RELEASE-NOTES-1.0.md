# YAMY v1.0.0 Release Notes

**Release Date:** December 2025

YAMY (Yet Another Mado tsukai no Yuutsu) v1.0.0 marks the first production-ready release with full Linux support. This release brings complete feature parity between Windows and Linux platforms, enabling users to use the same `.mayu` configuration files across operating systems.

## Highlights

- **Full Linux Support**: Complete port with X11 integration, evdev/uinput input handling, and Qt5 GUI
- **Cross-Platform Configurations**: Same `.mayu` files work identically on Windows and Linux
- **Modern Qt5 Interface**: System tray icon, configuration manager, log viewer, and preferences dialogs
- **Session Persistence**: Automatic state save/restore and XDG autostart integration
- **Plugin System**: Extensible architecture for custom functionality

## Installation

### Ubuntu / Debian

```bash
# Download the .deb package from GitHub Releases
sudo dpkg -i yamy-1.0.0-Linux-x86_64.deb

# Add your user to the input group (required for keyboard access)
sudo usermod -aG input $USER

# Log out and back in for group membership to take effect
```

### Fedora / openSUSE

```bash
# Download the .rpm package from GitHub Releases
sudo dnf install yamy-1.0.0-Linux-x86_64.rpm

# Add your user to the input group
sudo usermod -aG input $USER

# Log out and back in for group membership to take effect
```

### Arch Linux (AUR)

```bash
# Using yay
yay -S yamy

# Or using paru
paru -S yamy

# Or manually with PKGBUILD
git clone https://aur.archlinux.org/yamy.git
cd yamy
makepkg -si
```

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake g++ qt5-default libx11-dev libudev-dev

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

## New Features

### Configuration Management
- Manage multiple `.mayu` configuration profiles
- Quick-switch hotkey for rapid profile changes
- Automatic reload when configuration files change
- Configuration validation with error highlighting

### System Tray Integration
- Enable/disable key remapping with visual indicator
- Quick access to reload, settings, and log viewer
- Desktop notifications for status changes
- Notification history accessible from tray

### Log Viewer
- Real-time log display with auto-scroll
- Filter by log level and category
- Search functionality
- Export logs to file
- Syntax highlighting for key events

### Command-Line Control
Use `yamy-ctl` to control the daemon from scripts:
```bash
yamy-ctl status          # Show daemon status
yamy-ctl reload          # Reload configuration
yamy-ctl config list     # List configurations
yamy-ctl stop            # Stop daemon
```

## System Requirements

- **OS**: Linux with X11 (Wayland via XWayland supported)
- **Desktop**: GNOME, KDE Plasma, XFCE, or other XDG-compliant environment
- **Dependencies**: Qt5, libX11, libudev
- **Permissions**: User must be member of `input` group

### Tested Distributions
- Ubuntu 24.04 LTS (GNOME)
- Fedora 40 (GNOME Wayland via XWayland)
- Arch Linux (KDE Plasma)

## Known Limitations

- **Wayland**: Native Wayland support is not available. YAMY works via XWayland on Wayland sessions.
- **TTY/Console**: Keyboard remapping only works in graphical X11 sessions.
- **Secure Input Fields**: Some applications (password managers, certain browsers) may block input injection.

## Upgrade Notes

This is the first Linux release, so no upgrade path is required. Windows users can continue using existing Windows builds. Configuration files are fully compatible between platforms.

## Performance

- Key processing latency: <1ms (p99)
- Memory usage: <10MB typical
- CPU usage: <1% at idle

## Documentation

- [User Guide](docs/user-guide.md) - Installation, basic usage, and preferences
- [Configuration Guide](docs/configuration-guide.md) - Writing `.mayu` files
- [Troubleshooting Guide](docs/troubleshooting.md) - Common issues and solutions
- [Building on Linux](docs/building-linux.md) - Build instructions

## Contributors

We thank everyone who contributed to this release through code, testing, documentation, and feedback.

## License

YAMY is released under the BSD 3-Clause License. See [LICENSE](LICENSE) for details.

Original Mayu Copyright (C) 1999-2005 TAGA Nayuta
YAMY Copyright (C) 2009 KOBAYASHI Yoshiaki

## Links

- [GitHub Repository](https://github.com/user/yamy)
- [Issue Tracker](https://github.com/user/yamy/issues)
- [Full Changelog](CHANGELOG.md)
