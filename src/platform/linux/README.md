# Linux Platform Support (Placeholder)

This directory will contain Linux-specific implementations of the Platform Abstraction Layer (PAL).
Currently, it serves as a placeholder to ensure directory structure symmetry with `src/platform/windows`.

## Planned Implementations
- `window_system_linux.h/cpp`: X11/Wayland implementation of `WindowSystem`.
- `hook_linux.h/cpp`: Input hooking for Linux (e.g., using uinput or X11 XInput).
