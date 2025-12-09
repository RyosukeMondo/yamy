# Cross-Compiling YAMY to Windows on Linux

This guide explains how to build Windows binaries of YAMY on a Linux system using MinGW-w64 cross-compilation toolchain.

## Prerequisites

### Install MinGW-w64 Toolchain

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install mingw-w64 cmake zip
```

#### Arch Linux
```bash
sudo pacman -S mingw-w64-gcc cmake zip
```

#### Fedora
```bash
sudo dnf install mingw64-gcc mingw32-gcc cmake zip
```

### Verify Installation

Check that the toolchains are installed correctly:

```bash
# 64-bit toolchain
x86_64-w64-mingw32-gcc --version

# 32-bit toolchain
i686-w64-mingw32-gcc --version

# CMake
cmake --version
```

## Quick Start

### Build Release Package

Run the automated build script:

```bash
./scripts/cmake_package.sh
```

This will:
1. Run quality checks (anti-patterns, missing sources, encoding)
2. Cross-compile 64-bit Windows binaries
3. Cross-compile 32-bit Windows binaries
4. Package everything into `dist/yamy-dist.zip`

### Clean Build

To force a clean build (remove existing build directory):

```bash
./scripts/cmake_package.sh --clean
```

## Build Output

After a successful build, you'll find:

```
dist/
└── yamy-dist.zip          # Distribution package

logs/
├── build_log_x64.txt      # 64-bit build log
└── build_log_x86.txt      # 32-bit build log
```

The distribution package contains:

```
yamy-dist.zip
├── yamy.exe               # Launcher (64-bit)
├── yamy64.exe             # Main engine (64-bit)
├── yamy64.dll             # Hook DLL (64-bit)
├── yamy32.exe             # Main engine (32-bit)
├── yamy32.dll             # Hook DLL (32-bit)
├── yamyd32.exe            # Helper for 32-bit apps
├── keymaps/               # Default keymaps
├── docs/                  # Documentation
├── readme.txt             # Readme
├── launch_yamy.bat        # Launch script
└── launch_yamy_admin.bat  # Launch as admin script
```

## Manual Build Steps

If you want more control over the build process:

### 1. Run Quality Checks

```bash
# Check for anti-patterns
./scripts/check_antipatterns.sh

# Check for missing sources
./scripts/check_missing_sources.sh

# Check encoding
./scripts/check_encoding.sh
```

### 2. Build 64-bit

```bash
# Configure
cmake -S . -B build/x64 \
    -DCMAKE_TOOLCHAIN_FILE=build/toolchain/mingw-w64-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build/x64 --config Release
```

Output files:
- `build/x64/bin/yamy.exe`
- `build/x64/bin/yamy64.exe`
- `build/x64/bin/yamy64.dll`

### 3. Build 32-bit

```bash
# Configure
cmake -S . -B build/x86 \
    -DCMAKE_TOOLCHAIN_FILE=build/toolchain/mingw-w64-i686.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build/x86 --config Release
```

Output files:
- `build/x86/bin/yamy32.exe`
- `build/x86/bin/yamy32.dll`
- `build/x86/bin/yamyd32.exe`

## CMake Toolchain Files

The build script automatically generates toolchain files in `build/toolchain/`:

### 64-bit Toolchain (`mingw-w64-x86_64.cmake`)

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

### 32-bit Toolchain (`mingw-w64-i686.cmake`)

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

## Testing on Windows

Transfer `yamy-dist.zip` to a Windows machine and:

1. Extract the ZIP file
2. Run `launch_yamy.bat` or `launch_yamy_admin.bat`
3. Configure your keymaps in the `keymaps/` directory
4. Check logs in the YAMY directory

## Troubleshooting

### Build Fails with Linker Errors

Check the build logs:
```bash
cat logs/build_log_x64.txt
cat logs/build_log_x86.txt
```

### Missing Dependencies

Ensure all MinGW packages are installed:
```bash
# List installed MinGW packages (Debian/Ubuntu)
dpkg -l | grep mingw

# List installed MinGW packages (Arch)
pacman -Q | grep mingw

# List installed MinGW packages (Fedora)
rpm -qa | grep mingw
```

### Quality Check Failures

If quality checks fail, the build will stop. Fix the reported issues:

- **Anti-patterns**: Use `nullptr` instead of `NULL`, avoid unsafe string functions
- **Missing sources**: Add all `.cpp` files to `CMakeLists.txt`
- **Encoding**: Ensure files have UTF-8 BOM (required for MSVC compatibility)

### Resource Compilation Errors

If `windres` fails, ensure the RC compiler is available:
```bash
x86_64-w64-mingw32-windres --version
i686-w64-mingw32-windres --version
```

## Architecture Notes

### Why Both 32-bit and 64-bit?

YAMY builds both architectures because:

1. **64-bit main engine** (`yamy64.exe`) runs as the primary process
2. **32-bit helper** (`yamyd32.exe`) hooks into 32-bit applications on 64-bit Windows
3. Both architectures share the same hook DLL interface

### Static vs Dynamic Linking

The CMakeLists.txt configures:
- **32-bit**: Static linking (`-static`) to avoid DLL conflicts
- **64-bit**: Dynamic linking for better compatibility with Windows APIs

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Cross-Compile Windows

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Install MinGW
      run: |
        sudo apt-get update
        sudo apt-get install -y mingw-w64 cmake zip

    - name: Build
      run: ./scripts/cmake_package.sh

    - name: Upload Artifact
      uses: actions/upload-artifact@v3
      with:
        name: yamy-windows
        path: dist/yamy-dist.zip
```

## Comparing with Native Windows Build

| Feature | Linux Cross-Compile | Native Windows Build |
|---------|-------------------|---------------------|
| **Toolchain** | MinGW-w64 | MSVC or MinGW |
| **Speed** | Generally faster | Depends on hardware |
| **CI/CD** | Easy (Linux runners) | Requires Windows runners |
| **Dependencies** | Simpler setup | Visual Studio required |
| **Output** | Identical binaries | Identical binaries |

## Additional Resources

- [MinGW-w64 Documentation](https://www.mingw-w64.org/)
- [CMake Cross Compiling](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
- [YAMY Project Repository](https://github.com/yourusername/yamy)

## License

See the main project LICENSE file for details.
