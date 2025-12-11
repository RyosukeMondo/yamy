# YAMY Troubleshooting Guide

Solutions to common issues, debugging tips, and frequently asked questions.

---

## Table of Contents

1. [Quick Diagnostics](#quick-diagnostics)
2. [Common Issues](#common-issues)
   - [Keys Not Remapping](#keys-not-remapping)
   - [Configuration Errors](#configuration-errors)
   - [Permission Issues](#permission-issues)
   - [Performance Problems](#performance-problems)
3. [Viewing Logs](#viewing-logs)
4. [Debugging Tips](#debugging-tips)
5. [Known Limitations](#known-limitations)
6. [How to Report Bugs](#how-to-report-bugs)
7. [FAQ](#faq)

---

## Quick Diagnostics

Run through this checklist when YAMY isn't working:

1. **Is YAMY running?**
   - Check for the tray icon
   - Run `pgrep yamy` to verify the process

2. **Is YAMY enabled?**
   - The tray icon should be green/normal (not gray)
   - Right-click tray icon and check "Enable" status

3. **Was your configuration loaded?**
   - Check tray tooltip for config name
   - Open Log dialog and look for "Configuration loaded" message

4. **Are you in the right application?**
   - Use Investigate dialog to check active keymap
   - Your keybindings may be application-specific

5. **Do you have permissions?**
   - Run `groups | grep input` to verify input group membership
   - Check device permissions: `ls -la /dev/input/event*`

---

## Common Issues

### Keys Not Remapping

#### Symptom
Keys are not being remapped as configured.

#### Possible Causes and Solutions

**1. Configuration not loaded**

Check the Log dialog for errors:
```
[ERROR] Failed to parse config: ...
```

Solution: Fix syntax errors in your `.mayu` file.

**2. Wrong keymap active**

Your keybindings might be in an application-specific keymap.

Solution:
1. Open Investigate dialog (right-click tray > Investigate)
2. Click "Select Window" and click the problem application
3. Check which keymap is active
4. Verify your key definitions are in that keymap or `Global`

**3. YAMY disabled**

Solution: Click the tray icon or right-click > Enable

**4. Key defined in wrong scope**

```mayu
# WRONG - key outside any keymap
key C-A = Home

# CORRECT - key inside keymap
keymap Global
key C-A = Home
```

**5. Application captures keys before YAMY**

Some applications grab keyboard input directly.

Solution: Check Known Limitations section below.

---

### Configuration Errors

#### Syntax Error

**Symptom:**
```
[ERROR] Parse error at line 15: unexpected token
```

**Common causes:**

1. **Missing quotes in regex:**
   ```mayu
   # WRONG
   window Firefox /firefox/ : Global

   # CORRECT
   window Firefox /:firefox:/ : Global
   ```

2. **Typo in key name:**
   ```mayu
   # WRONG - 'Contrl' is not valid
   key Contrl-A = Home

   # CORRECT
   key C-A = Home
   ```

3. **Missing colon in window directive:**
   ```mayu
   # WRONG
   window Firefox /pattern/ Global

   # CORRECT
   window Firefox /pattern/ : Global
   ```

4. **Unbalanced parentheses in conditionals:**
   ```mayu
   # WRONG
   if ( KBD104 ) and KBD109 )

   # CORRECT
   if ( KBD104 ) and ( KBD109 )
   ```

#### Configuration Not Found

**Symptom:**
```
[WARNING] Configuration file not found
```

**Solution:**
Create the configuration file in one of these locations:
- `~/.config/yamy/config.mayu` (recommended)
- `~/.yamy/config.mayu`

```bash
mkdir -p ~/.config/yamy
cp /path/to/yamy/src/resources/templates/default.mayu ~/.config/yamy/config.mayu
```

---

### Permission Issues

#### "Permission denied" on /dev/input/event*

**Symptom:**
```
[ERROR] Failed to open input device: Permission denied
```

**Solution:**
1. Add user to input group:
   ```bash
   sudo usermod -a -G input $USER
   ```

2. Log out and log back in

3. Verify:
   ```bash
   groups | grep input
   ```

#### uinput not available

**Symptom:**
```
[ERROR] Failed to create uinput device
```

**Solution:**
1. Load the uinput module:
   ```bash
   sudo modprobe uinput
   ```

2. Make it persistent:
   ```bash
   echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf
   ```

3. Set permissions:
   ```bash
   sudo tee /etc/udev/rules.d/99-uinput.rules > /dev/null <<'EOF'
   KERNEL=="uinput", MODE="0660", GROUP="input"
   EOF
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

---

### Performance Problems

#### High Input Latency

**Symptom:** Noticeable delay when typing.

**Possible causes:**

1. **Too many keymaps** - Each keymap adds processing overhead
   - Solution: Consolidate keymaps where possible

2. **Complex regex patterns** - Slow window matching
   - Solution: Simplify regex patterns

3. **Debug mode enabled**
   - Solution: Disable debug mode in Preferences > Advanced

**Diagnosis:**
1. Open Preferences > Advanced
2. Enable "Show Performance Overlay"
3. Check key processing latency (should be <1ms)

#### High CPU Usage

**Symptom:** YAMY using excessive CPU.

**Solutions:**
1. Check for configuration loops (keys that trigger each other)
2. Reduce performance metrics interval in Preferences
3. Disable verbose logging

---

## Viewing Logs

### Log Dialog

1. Right-click tray icon > Log
2. Filter by level (Error, Warning, Info, Debug, Trace)
3. Filter by category (Engine, Parser, Input, Window, Config)
4. Use Search (Ctrl+F) to find specific messages

### Log Levels

| Level | When to Use |
|-------|-------------|
| Error | Something failed |
| Warning | Potential problem |
| Info | Normal operation |
| Debug | Detailed information |
| Trace | Very detailed (performance impact) |

### Exporting Logs

1. Open Log dialog
2. Click "Export" button
3. Save to file for sharing in bug reports

---

## Debugging Tips

### Check Active Keymap

Use the Investigate dialog:
1. Right-click tray > Investigate
2. Select a window
3. View "Active Keymap" field

### Test Key Processing

Enable trace logging to see all key events:
1. Open Preferences > Logging
2. Set Log Level to "Trace"
3. Watch keys being processed in Log dialog

### Verify Configuration Syntax

Test configuration without applying:
```bash
yamy --check-config ~/.config/yamy/config.mayu
```

### Use Default Configuration

Test with a minimal config to isolate issues:
```bash
# Backup current config
cp ~/.config/yamy/config.mayu ~/.config/yamy/config.mayu.bak

# Use minimal config
cat > ~/.config/yamy/config.mayu << 'EOF'
keymap Global
key C-A = Home
EOF
```

Then reload (Ctrl+Shift+S or tray > Reload).

---

## Known Limitations

### Applications That Bypass YAMY

Some applications capture keyboard input before YAMY can process it:

| Application Type | Issue | Workaround |
|-----------------|-------|------------|
| Full-screen games | DirectInput/raw input | None (by design) |
| Some remote desktop | Virtual keyboard | Use host-side remapping |
| VMware/VirtualBox | VM captures keys | Configure VM keyboard settings |

### Wayland Limitations

YAMY on Linux uses X11 for window matching. Under Wayland:

- Window matching may not work reliably
- Some window functions unavailable
- Consider XWayland or using YAMY under X11

### Key Combinations Limitations

Some system-level shortcuts cannot be remapped:
- Ctrl+Alt+F1-F7 (VT switching)
- Ctrl+Alt+Delete (on some systems)
- SysRq combinations

---

## How to Report Bugs

### Before Reporting

1. Update to the latest version
2. Check this troubleshooting guide
3. Search existing issues

### Information to Include

1. **YAMY version:**
   ```bash
   yamy --version
   ```

2. **Operating system:**
   ```bash
   cat /etc/os-release
   uname -a
   ```

3. **Desktop environment:**
   ```bash
   echo $XDG_CURRENT_DESKTOP
   echo $XDG_SESSION_TYPE  # X11 or Wayland
   ```

4. **Configuration file** (sanitized if needed)

5. **Log output:**
   - Export logs from Log dialog
   - Include relevant error messages

6. **Steps to reproduce:**
   - What you did
   - What you expected
   - What happened instead

### Where to Report

Report bugs at: https://github.com/your-repo/yamy/issues

---

## FAQ

### General Questions

**Q: How do I reload my configuration?**

A: Three ways:
1. Press Ctrl+Shift+S (if configured)
2. Right-click tray icon > Reload
3. Add to config: `key C-S-S = &LoadSetting`

**Q: How do I temporarily disable YAMY?**

A: Click the tray icon or right-click > Enable/Disable.

**Q: Can I have different configs for different applications?**

A: Yes! Use the `window` directive:
```mayu
window Firefox /:firefox:/ : Global
key C-L = F6

window Terminal /:gnome-terminal:/ : Global
key C-A = &Default  # Don't remap in terminal
```

**Q: How do I make CapsLock act as Control?**

A:
```mayu
keymap Global
mod control += CapsLock
key *CapsLock = *LControl
```

### Configuration Questions

**Q: Why doesn't my key binding work in a specific app?**

A: Use the Investigate dialog to check which keymap is active. Your binding might be in a different keymap or overridden.

**Q: How do I pass a key through without remapping?**

A: Use `&Default`:
```mayu
key C-A = &Default  # Let C-A work normally
```

**Q: How do I ignore a key completely?**

A:
```mayu
key CapsLock = &Ignore  # CapsLock does nothing
```

**Q: How do I create multi-key sequences?**

A:
```mayu
# Single key triggers multiple
key C-K = S-End Delete

# Prefix key (like Emacs C-x)
keymap2 MyPrefix : Global
key C-S = C-S
key C-F = C-O

keymap Global
key C-X = &Prefix(MyPrefix)
```

### Performance Questions

**Q: Is there a performance impact?**

A: Minimal. YAMY adds <1ms latency to key processing under normal conditions.

**Q: Does YAMY affect gaming?**

A: Most games use DirectInput which bypasses YAMY. For games that don't, you may want to add them to a keymap that passes through all keys:
```mayu
window MyGame /:MyGame:/ : KeymapDefault
```

### Security Questions

**Q: Is it safe to add myself to the input group?**

A: Adding yourself to `input` grants the ability to read all keyboard input system-wide. Only add trusted users. YAMY requires this to function.

**Q: Can YAMY be used as a keylogger?**

A: The input group grants keylogger capability to any process run by members. YAMY itself does not log keys, but the capability exists. Be aware of this when adding users to the input group.

---

## Getting Help

- **Documentation**: See [User Guide](user-guide.md) and [Configuration Guide](configuration-guide.md)
- **Help Menu**: Right-click tray icon > Help
- **Bug Reports**: https://github.com/your-repo/yamy/issues
- **Source Code**: https://github.com/your-repo/yamy
