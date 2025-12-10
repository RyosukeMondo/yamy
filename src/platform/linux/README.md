# Linux Platform Support

This directory contains Linux-specific implementations of the Platform Abstraction Layer (PAL).
Currently, it implements a **stub backend** to facilitate architecture verification and CI testing without requiring a full X11/Wayland implementation.

## Build Instructions

To build the Linux stub on a Linux system (e.g., Ubuntu):

```bash
mkdir build
cd build
cmake .. -DBUILD_LINUX_STUB=ON
make
```

This will generate a `yamy_stub` executable in `bin/`.

## Running

The stub executable demonstrates the platform abstraction initialization but does not interact with the window system yet.

```bash
./bin/yamy_stub
```

## CI Verification

The Linux build is part of the CI pipeline to ensure:
1.  **Architecture Separation:** Core logic does not depend on Windows-specific headers.
2.  **Compilation:** The codebase compiles with GCC on Linux.
3.  **Static Analysis:** Checks verify no `windows.h` or Win32 types leak into `src/core`.

## Files
- `window_system_linux.cpp`: Stub implementation of `IWindowSystem`.
- `input_injector_linux.cpp`: Stub implementation of `IInputInjector`.
- `input_hook_linux.cpp`: Stub implementation of `IInputHook`.
- `input_driver_linux.cpp`: Stub implementation of `IInputDriver`.
