# Linux Platform Implementation

This directory contains the Linux implementation of YAMY's platform abstraction layer.

## Implementation Status

The implementation is split into **12 independent tracks** for parallel development:

### Foundation
- [ ] Track 6: POSIX Synchronization (`sync_linux.cpp`)
- [ ] Track 8: Hook Data (`hook_data_linux.cpp`)
- [ ] Track 12: Thread Management (`thread_linux.cpp`)

### Window System
- [ ] Track 1: Window Queries (`window_system_linux_queries.cpp`)
- [ ] Track 2: Window Manipulation (`window_system_linux_manipulation.cpp`)
- [ ] Track 3: Window Hierarchy (`window_system_linux_hierarchy.cpp`)
- [ ] Track 4: Mouse & Cursor (`window_system_linux_mouse.cpp`)
- [ ] Track 5: Monitor Support (`window_system_linux_monitor.cpp`)

### Input System
- [ ] Track 9: evdev Input Capture (`input_hook_linux.cpp`, `device_manager_linux.cpp`)
- [ ] Track 10: uinput Injection (`input_injector_linux.cpp`)
- [ ] Track 11: Key Code Mapping (`keycode_mapping.cpp`)

### IPC
- [ ] Track 7: Unix IPC (`ipc_linux.cpp`)

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

### Permission Setup (for input system)
```bash
# Add user to input group
sudo usermod -a -G input $USER

# Reload groups (or log out/in)
newgrp input
```

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
