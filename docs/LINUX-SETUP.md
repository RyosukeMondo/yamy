# YAMY Linux Setup Guide

## Overview

YAMY on Linux uses **evdev** (event device) for keyboard capture and **uinput** for key injection. This requires special permissions to access `/dev/input/event*` devices.

## Quick Setup

Run the automated setup script:

```bash
sudo ./linux_setup.sh
```

Then **log out and log back in** for group changes to take effect.

## What the Script Does

1. **Adds user to `input` group** - Grants permission to read `/dev/input/event*`
2. **Creates udev rules** - Ensures input devices have correct permissions
3. **Loads uinput module** - Enables virtual keyboard creation
4. **Verifies permissions** - Checks everything is configured correctly

## Manual Setup (Alternative)

If you prefer to set up manually:

### Step 1: Add User to Input Group

```bash
sudo usermod -a -G input $USER
```

### Step 2: Create udev Rule

Create `/etc/udev/rules.d/99-input.rules`:

```bash
sudo tee /etc/udev/rules.d/99-input.rules > /dev/null <<'EOF'
KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"
EOF
```

Reload udev:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Step 3: Load uinput Module

```bash
sudo modprobe uinput
echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf
```

### Step 4: Log Out and Log Back In

Group membership changes require re-login.

## Verification

After re-login, verify setup:

### Check Group Membership

```bash
groups | grep input
```

Expected output: Should include `input`

### Check Device Permissions

```bash
ls -la /dev/input/event*
```

Expected output:
```
crw-rw---- 1 root input 13, 64 Dec 10 10:00 /dev/input/event0
crw-rw---- 1 root input 13, 65 Dec 10 10:00 /dev/input/event1
...
```

Group should be `input` with `rw-` permissions.

### Check uinput

```bash
ls -la /dev/uinput
lsmod | grep uinput
```

Expected output:
```
crw-rw---- 1 root input 10, 223 Dec 10 10:00 /dev/uinput
uinput                 20480  0
```

### Test Device Access

Try reading from an input device (Ctrl+C to stop):

```bash
cat /dev/input/event0
# Press some keys, should see binary output
# Ctrl+C to stop
```

If you get "Permission denied", check:
1. User is in `input` group: `groups`
2. Device has correct group: `ls -la /dev/input/event0`
3. You've logged out and back in after adding group

## Troubleshooting

### "Permission denied" on /dev/input/event*

**Cause:** User not in input group or didn't re-login

**Fix:**
```bash
# Check group membership
groups | grep input

# If not present, add and re-login
sudo usermod -a -G input $USER
# Log out and log back in
```

### "No such device" /dev/uinput

**Cause:** uinput module not loaded

**Fix:**
```bash
sudo modprobe uinput
echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf
```

### Devices have wrong group/permissions

**Cause:** udev rules not applied

**Fix:**
```bash
# Recreate udev rule
sudo ./linux_setup.sh

# Or manually trigger:
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### "Device or resource busy" when grabbing

**Cause:** Another process already grabbed the device

**Fix:**
```bash
# Check what's using the device
sudo lsof /dev/input/event0

# Kill the process or restart
```

## Security Implications

### What Access Means

- **Input group:** Can read ALL keyboard/mouse input system-wide
- **Device grab:** Exclusive access prevents other apps from reading
- **uinput access:** Can inject keyboard/mouse events

### Considerations

1. **Input group = keylogger capability** - Only add trusted users
2. **YAMY grabs keyboards exclusively** - Other apps won't receive those keys
3. **Injected keys are system-wide** - Can control any application

### Best Practices

- Only run YAMY when needed
- Don't add untrusted users to `input` group
- Be aware YAMY can capture passwords/sensitive input
- Review your .yamy config file regularly

## Architecture Notes

### evdev (Event Device)

- **Location:** `/dev/input/event0`, `event1`, etc.
- **Interface:** Binary stream of `struct input_event`
- **Capabilities:** Each device reports what events it supports (keys, mouse, etc.)
- **Grab:** `EVIOCGRAB` ioctl for exclusive access

### uinput (User Input)

- **Location:** `/dev/uinput`
- **Purpose:** Create virtual input devices
- **Usage:** YAMY creates virtual keyboard to inject remapped keys
- **Requires:** `uinput` kernel module loaded

### Device Detection

YAMY uses **libudev** to:
1. Enumerate all input devices
2. Filter for keyboards (check EV_KEY capability)
3. Monitor hotplug events (keyboards plugged/unplugged)

## System Requirements

### Kernel

- Linux kernel 2.6+ (evdev support)
- CONFIG_INPUT_EVDEV=y or =m
- CONFIG_INPUT_UINPUT=y or =m

### Libraries

```bash
sudo apt install libudev-dev linux-headers-$(uname -r)
```

### Desktop Environment

Works with:
- X11 (tested)
- Wayland (untested, may have issues)
- Console/TTY (should work)

## Uninstall

To remove YAMY's setup:

```bash
# Remove user from input group
sudo deluser $USER input

# Remove udev rule
sudo rm /etc/udev/rules.d/99-input.rules
sudo udevadm control --reload-rules

# Remove uinput auto-load
sudo rm /etc/modules-load.d/uinput.conf

# Unload uinput (if no other apps use it)
sudo rmmod uinput
```

Then log out and back in.

## References

- [Linux Input Subsystem](https://www.kernel.org/doc/html/latest/input/input.html)
- [evdev Documentation](https://www.kernel.org/doc/html/latest/input/event-codes.html)
- [uinput Documentation](https://www.kernel.org/doc/html/latest/input/uinput.html)
- [libudev API](https://www.freedesktop.org/software/systemd/man/libudev.html)
