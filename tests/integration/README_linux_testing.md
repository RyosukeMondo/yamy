# Linux Key Remapping Test Guide

This document describes how to test basic key remapping functionality on Linux with X11.

## Prerequisites

1. **X11 Environment**: Tests must run in an X11 session (not Wayland)
2. **Input Group Access**: User must be in the `input` group for evdev access
   ```bash
   sudo usermod -aG input $USER
   # Log out and back in for changes to take effect
   ```
3. **Uinput Access**: For key injection, root privileges or uinput access is needed
   ```bash
   sudo modprobe uinput
   sudo chmod 0660 /dev/uinput
   sudo chown root:input /dev/uinput
   ```

## Test Configuration

The test configuration file `test_linux_basic.mayu` contains the following test scenarios:

### Test 1: Simple Key Remapping
- **Input**: Press `A`
- **Expected**: Outputs `B`
- **Verification**: Type in a text editor; pressing A should produce B

### Test 2: Key Swap (F1 <-> Escape)
- **Input**: Press `F1` or `Escape`
- **Expected**: F1 outputs Escape, Escape outputs F1
- **Verification**: Press F1 in a terminal; should not show help but close dialog

### Test 3: CapsLock as Control
- **Input**: Press `CapsLock`
- **Expected**: Acts as Left Control
- **Verification**: CapsLock + C should copy text in applications

### Test 4: Ctrl+J as Enter
- **Input**: Press `Ctrl+J`
- **Expected**: Outputs Enter
- **Verification**: In a text editor, Ctrl+J should create a new line

### Test 5: Alt+H as Backspace
- **Input**: Press `Alt+H`
- **Expected**: Outputs Backspace
- **Verification**: Should delete character before cursor

### Test 6: Key Sequence (X Y -> Z)
- **Input**: Press `X` then `Y` in sequence
- **Expected**: Outputs `Z`
- **Verification**: Quick succession X-Y should produce Z

### Test 7: Ctrl+Shift+A as Ctrl+Shift+Z
- **Input**: Press `Ctrl+Shift+A`
- **Expected**: Outputs `Ctrl+Shift+Z` (typically Redo)
- **Verification**: In applications that support Ctrl+Shift+Z for redo

### Test 8: F5 as F10
- **Input**: Press `F5`
- **Expected**: Outputs `F10`
- **Verification**: In a GTK app, F5 should activate menu bar

### Test 9: Emacs-style Navigation
- **Input**: Press `Ctrl+P/N/B/F`
- **Expected**: Arrow key outputs (Up/Down/Left/Right)
- **Verification**: In text editor, these combos should move cursor

### Test 10: Emacs-style Home/End
- **Input**: Press `Ctrl+A` or `Ctrl+E`
- **Expected**: Home or End key output
- **Verification**: Should move cursor to line start/end

## Running Manual Tests

1. Build YAMY with Linux support:
   ```bash
   mkdir -p build_linux && cd build_linux
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)
   ```

2. Start YAMY with the test configuration:
   ```bash
   sudo ./yamy -c ../tests/integration/test_linux_basic.mayu
   ```

3. Open a text editor (gedit, kate, etc.) and verify each test case

4. Document any issues found

## Running Automated Tests

Run the unit tests that verify keymap loading and assignment:
```bash
cd build_linux
ctest --output-on-failure -R "key_remap_linux"
```

## Test Results Checklist

| Test | Description | Status | Notes |
|------|-------------|--------|-------|
| 1 | Simple A -> B remap | [ ] | |
| 2 | F1 <-> Escape swap | [ ] | |
| 3 | CapsLock as Control | [ ] | |
| 4 | Ctrl+J as Enter | [ ] | |
| 5 | Alt+H as Backspace | [ ] | |
| 6 | X Y sequence -> Z | [ ] | |
| 7 | Ctrl+Shift+A combo | [ ] | |
| 8 | F5 -> F10 | [ ] | |
| 9 | Emacs navigation | [ ] | |
| 10 | Emacs Home/End | [ ] | |

## Known Limitations

1. **Wayland**: Key hooking via evdev doesn't work on Wayland sessions
2. **Some Applications**: GTK4/Qt6 apps with direct input handling may bypass remapping
3. **Virtual Machines**: Key codes may differ in VM environments
4. **Keyboard Layout**: Tests assume US QWERTY layout

## Troubleshooting

### Keys not being captured
- Verify you're running X11: `echo $XDG_SESSION_TYPE`
- Check input group membership: `groups | grep input`
- Verify /dev/input/event* permissions

### Keys being captured but not remapped
- Check YAMY log output for configuration errors
- Verify .mayu syntax is correct
- Ensure keyboard layout matches configuration

### Double key events
- May indicate XTest extension issues
- Check for conflicting key remappers (xmodmap, etc.)

## Files

- `test_linux_basic.mayu` - Test configuration with all test cases
- `key_remap_linux_test.cpp` - Automated unit tests for keymap loading
