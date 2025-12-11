# YAMY Wayland Compatibility Guide

This document describes YAMY's compatibility with Wayland display servers and provides guidance for users running YAMY on Wayland-based desktop environments.

## Overview

YAMY v1.0 supports Linux systems running Wayland via **XWayland**. Native Wayland input capture is not supported in this release.

### How It Works

1. **Input Capture**: YAMY uses evdev for keyboard input capture, which works independently of X11/Wayland
2. **Key Injection**: YAMY uses uinput for key injection, which also works independently of display server
3. **Window Detection**: YAMY uses X11 for window detection and system tray, requiring XWayland on Wayland sessions
4. **GUI/System Tray**: Qt5 with X11 backend, runs via XWayland on Wayland sessions

## Supported Configurations

### Tested Environments

| Distribution | Desktop Environment | Session Type | Status |
|--------------|---------------------|--------------|--------|
| Ubuntu 24.04 LTS | GNOME 46 | X11 | Fully Supported |
| Ubuntu 24.04 LTS | GNOME 46 | Wayland (XWayland) | Supported with Limitations |
| Fedora 40 | GNOME 46 | Wayland (XWayland) | Supported with Limitations |
| Arch Linux | KDE Plasma 6 | X11 | Fully Supported |

### Requirements for Wayland Sessions

1. **XWayland must be running**: Most Wayland compositors start XWayland automatically when an X11 application is launched
2. **DISPLAY environment variable**: Must be set (typically `:0` or `:1`)
3. **Input group membership**: User must be in the `input` group for evdev access

## Known Limitations on Wayland

### 1. Native Wayland Applications

**Limitation**: Key remapping may not work for applications running natively under Wayland without XWayland.

**Affected Applications**:
- Firefox (when running in native Wayland mode)
- Some GTK4 applications
- Electron apps configured for native Wayland

**Workaround**: Force X11/XWayland mode for specific applications:

```bash
# For GTK applications
GDK_BACKEND=x11 firefox

# For Qt applications
QT_QPA_PLATFORM=xcb application-name

# For Electron applications (via environment)
ELECTRON_OZONE_PLATFORM_HINT=x11 electron-app
```

### 2. Window Focus Detection

**Limitation**: Window matching rules may not detect native Wayland windows correctly.

**Behavior**: YAMY can detect and match windows running through XWayland but may not see native Wayland windows.

**Workaround**: Configure applications to use XWayland, or use keyboard-only matching rules without window conditions.

### 3. Global Keyboard Shortcuts

**Limitation**: Wayland's security model restricts applications from capturing global keyboard input at the display server level.

**YAMY's Solution**: YAMY bypasses this limitation by capturing input directly from evdev devices at the kernel level, which works on both X11 and Wayland.

### 4. System Tray Compatibility

**Limitation**: Some Wayland compositors have limited system tray (StatusNotifierItem) support.

**Status by Desktop**:
- **GNOME**: Uses AppIndicator extension (may need to be installed)
- **KDE Plasma**: Native support via StatusNotifierItem
- **Sway/wlroots**: Requires waybar or similar with tray support

## Configuration for Wayland

### Environment Variables

Add these to your `~/.bashrc` or session startup:

```bash
# Ensure X11 display is available for YAMY
export DISPLAY=${DISPLAY:-:0}

# Force Qt to use X11 backend (already default for Qt5)
export QT_QPA_PLATFORM=xcb
```

### Desktop Entry Modification

To ensure YAMY starts correctly on Wayland sessions, the desktop entry includes:

```desktop
[Desktop Entry]
Name=YAMY
Exec=env QT_QPA_PLATFORM=xcb yamy
Type=Application
```

### Autostart Configuration

For GNOME on Wayland, YAMY can be set to autostart via:

1. **GNOME Tweaks**: Add YAMY to Startup Applications
2. **Manual**: Copy `.desktop` file to `~/.config/autostart/`

```bash
cp /usr/share/applications/yamy.desktop ~/.config/autostart/
```

## Troubleshooting

### YAMY Won't Start on Wayland

**Symptom**: YAMY fails to start or crashes immediately on Wayland session.

**Check**:
```bash
# Verify DISPLAY is set
echo $DISPLAY

# Verify XWayland is running
pgrep Xwayland

# Try starting manually with verbose output
QT_QPA_PLATFORM=xcb yamy --debug
```

**Solution**: Ensure XWayland is available. Start any X11 application first to launch XWayland.

### No System Tray Icon

**Symptom**: YAMY is running but no tray icon appears.

**For GNOME**:
```bash
# Install AppIndicator extension
sudo dnf install gnome-shell-extension-appindicator  # Fedora
sudo apt install gnome-shell-extension-appindicator  # Ubuntu

# Enable the extension
gnome-extensions enable appindicatorsupport@rgcjonas.gmail.com
```

**For other desktops**: Install a system tray that supports StatusNotifierItem/AppIndicator protocol.

### Key Remapping Not Working in Some Apps

**Symptom**: Remapping works in some applications but not others.

**Diagnosis**:
```bash
# Check if application is using XWayland
xlsclients  # Lists X11 clients

# Check application's Wayland status
xprop  # Then click on the window - if it works, it's XWayland
```

**Solution**: Force the application to use XWayland (see Workarounds above).

### Permission Denied for evdev

**Symptom**: YAMY cannot access keyboard devices.

**Check**:
```bash
# Verify input group membership
groups $USER | grep input

# Check device permissions
ls -la /dev/input/event*
```

**Solution**:
```bash
# Add user to input group
sudo usermod -aG input $USER

# Log out and log back in, or:
newgrp input
```

### SELinux Blocking evdev Access (Fedora)

**Symptom**: YAMY fails with permission errors despite correct group membership.

**Check**:
```bash
# Check for SELinux denials
sudo ausearch -m AVC -ts recent | grep yamy
```

**Solution**:
```bash
# Create local policy module
sudo audit2allow -a -M yamy_local
sudo semodule -i yamy_local.pp
```

## Future Plans

Native Wayland support is planned for a future release. This will include:

1. **libinput integration**: Alternative to evdev for input capture
2. **wlr-virtual-keyboard**: Native key injection for wlroots-based compositors
3. **ext-foreign-toplevel**: Window detection for Wayland
4. **Freedesktop portals**: For permissions and window access

Until then, XWayland provides a reliable compatibility layer for YAMY on Wayland sessions.

## Testing Wayland Compatibility

Run the Fedora 40 validation script to test YAMY on your Wayland system:

```bash
./tests/platform-validation/validate_fedora_40.sh
```

This script checks:
- Wayland session detection
- XWayland availability
- evdev/uinput access
- Package installation
- Functional tests

## References

- [Wayland Protocol Documentation](https://wayland.freedesktop.org/docs/html/)
- [XWayland Documentation](https://wayland.freedesktop.org/xserver.html)
- [evdev Input Subsystem](https://www.kernel.org/doc/html/latest/input/input.html)
- [uinput Virtual Device](https://www.kernel.org/doc/html/latest/input/uinput.html)
