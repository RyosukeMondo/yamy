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

## Memory Leak Detection with AddressSanitizer

YAMY supports building with AddressSanitizer (ASAN) for detecting memory leaks and other memory errors.

### Building with ASAN

```bash
# Clean build with ASAN enabled
rm -rf build_asan
cmake -B build_asan -DENABLE_ASAN=ON
cmake --build build_asan
```

### Running Leak Tests

```bash
# Run the leak test suite
./build_asan/bin/yamy_leak_test

# Or use CTest
cd build_asan && ctest -R yamy_leak_test --output-on-failure
```

ASAN will automatically report any memory leaks at program exit.

### Using Valgrind (Alternative)

For systems where ASAN is not available, you can use Valgrind:

```bash
# Build without ASAN (normal build)
cmake -B build ..
cmake --build build

# Run with Valgrind (use suppression file to filter library issues)
valgrind --leak-check=full --suppressions=valgrind.supp ./build/bin/yamy_leak_test
```

### ASAN Environment Variables

Useful ASAN runtime options:

```bash
# Detect memory leaks (enabled by default)
ASAN_OPTIONS=detect_leaks=1 ./build_asan/bin/yamy_leak_test

# Get more detailed stack traces
ASAN_OPTIONS=malloc_context_size=30 ./build_asan/bin/yamy_leak_test

# Abort on first error (useful for debugging)
ASAN_OPTIONS=halt_on_error=1 ./build_asan/bin/yamy_leak_test
```

## Note

The current Linux implementation is a stub. It validates that the platform abstraction layer (interfaces) can be compiled and linked against the new Linux backend stubs.

**Known Issue:** The full `yamy_engine` executable does not yet compile on Linux. This is because the core engine (`src/core/`) heavily relies on Windows-specific APIs that are currently being refactored in parallel branches (Branch 2: Engine Migration, Branch 10: Win32 Type Audit). Once those branches are merged, the core engine will use the new `src/core/platform` interfaces, enabling a full Linux build.
