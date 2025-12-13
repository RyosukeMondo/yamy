# Linux Platform Implementation

This directory contains the Linux implementation of YAMY's platform abstraction layer.

## Implementation Status

**ALL 12 TRACKS COMPLETE!** ✅ 🎉

The implementation is split into **12 independent tracks** for parallel development:

### Foundation
- [x] Track 6: POSIX Synchronization (`sync_linux.cpp`) - ✅ Merged from PR #32
- [x] Track 8: Hook Data (`hook_data_linux.cpp`) - ✅ Merged from PR #30
- [x] Track 12: Thread Management (`thread_linux.cpp`) - ✅ Merged from PR #38

### Window System
- [x] Track 1: Window Queries (`window_system_linux_queries.cpp`) - ✅ Implemented
- [x] Track 2: Window Manipulation (`window_system_linux_manipulation.cpp`) - ✅ Implemented
- [x] Track 3: Window Hierarchy (`window_system_linux_hierarchy.cpp`) - ✅ Merged from PR #35
- [x] Track 4: Mouse & Cursor (`window_system_linux_mouse.cpp`) - ✅ Merged from PR #33
- [x] Track 5: Monitor Support (`window_system_linux_monitor.cpp`) - ✅ Manually integrated from PR #36

### Input System
- [x] Track 9: evdev Input Capture (`input_hook_linux.cpp`, `device_manager_linux.cpp`) - ✅ **COMPLETE!**
- [x] Track 10: uinput Injection (`input_injector_linux.cpp`) - ✅ Merged from PR #37
- [x] Track 11: Key Code Mapping (`keycode_mapping.cpp`) - ✅ Implemented

### IPC
- [x] Track 7: Unix IPC (`ipc_linux.cpp`) - ✅ Manually integrated from PR #34

## Dependencies

### Required Libraries
- **X11** (`libx11-dev`) - Window system
- **XRandR** (`libxrandr-dev`) - Multi-monitor support
- **libudev** (`libudev-dev`) - Device enumeration
- **pthread** - Threading and synchronization

### Installation (Ubuntu/Debian)
```bash
sudo apt install build-essential cmake
sudo apt install libx11-dev libxrandr-dev libudev-dev
sudo apt install linux-headers-$(uname -r)
```

### Permission Setup (REQUIRED for input capture)
**Use the automated setup script:**
```bash
sudo ./scripts/linux/linux_setup.sh
# Then LOG OUT and LOG BACK IN for group changes to take effect
```

Or manually:
```bash
# Add user to input group
sudo usermod -a -G input $USER

# Create udev rules
sudo tee /etc/udev/rules.d/99-input.rules > /dev/null <<'EOF'
KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"
EOF

# Load uinput module
sudo modprobe uinput
echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf

# LOG OUT AND LOG BACK IN
```

See `docs/LINUX-SETUP.md` for detailed instructions.

## Building

```bash
cmake -B build
cmake --build build
```

## Testing Individual Tracks

Each track can be tested independently:

```bash
# Example for Track 1
g++ -o test_track1 test_track1.cpp \
    src/platform/linux/window_system_linux_queries.cpp \
    -lX11 -I src/

./test_track1
```

## Architecture

Each track implements a specific subsystem with clear boundaries:
- **No shared state** between tracks
- **Independent files** (no conflicts)
- **Clear interfaces** (defined in headers)
- **Separate PRs** (easy to review)

## Code Style

Use `.clang-format` for consistent formatting:
```bash
clang-format -i src/platform/linux/*.cpp
```

## Integration Order

Merge tracks in this order for easier testing:
1. Foundation (Tracks 6, 8, 12)
2. Window System (Tracks 1-5)
3. Input System (Tracks 9-11)
4. IPC (Track 7)

## Documentation

See `/docs/` for detailed implementation guides:
- `linux-parallel-implementation-plan.md` - Overall plan
- `linux-base-structure.md` - Base structure setup
- `jules-prompts-linux-tracks.md` - Implementation prompts
