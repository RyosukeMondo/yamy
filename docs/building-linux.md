# Building YAMY on Linux (Stub)

This is a stub build for architectural validation only.
It will not provide actual functionality as the Linux backend is implemented as stubs.

## Requirements
- GCC 7+ or Clang 6+
- CMake 3.10+
- pthread library (standard on most Linux systems)

## Build Steps

1.  Create a build directory:
    ```bash
    mkdir -p build/linux
    cd build/linux
    ```

2.  Run CMake:
    ```bash
    cmake ../..
    ```

3.  Build the project:
    ```bash
    make
    ```

## Run

To run the application (which will print stub messages):

```bash
./bin/yamy_stub
```

## Note

The current Linux implementation is a stub. It validates that the platform abstraction layer (interfaces) can be compiled and linked against the new Linux backend stubs.

**Known Issue:** The full `yamy_engine` executable does not yet compile on Linux. This is because the core engine (`src/core/`) heavily relies on Windows-specific APIs that are currently being refactored in parallel branches (Branch 2: Engine Migration, Branch 10: Win32 Type Audit). Once those branches are merged, the core engine will use the new `src/core/platform` interfaces, enabling a full Linux build.
