# Modern C++ Toolchain Migration Guide

## Overview

This guide walks you through migrating to YAMY's modern C++ toolchain, which provides:
- **10x faster linking** with Mold (Linux) / LLD (Windows)
- **<5s incremental builds** with ccache and Ninja
- **Reproducible builds** with Conan 2.0 dependency management
- **Zero-latency logging** with Quill
- **Contract programming** with Microsoft GSL
- **Property-based testing** with RapidCheck
- **AI-compatible codebase** structure

## Prerequisites

### All Platforms
- CMake 3.28+ (`cmake --version`)
- Python 3.8+ for Conan (`python --version`)
- Git for version control

### Linux
- GCC 9+ or Clang 10+
- Ninja build system
- Mold linker (recommended, LLD fallback)

### Windows
- LLVM 14+ (includes clang-cl and lld-link)
- Ninja build system
- Visual Studio 2019+ (for Windows SDK)

## Installation

### Step 1: Install Conan 2.0

**Linux/macOS**:
```bash
# Recommended: isolated installation via pipx
pipx install conan

# Alternative: user installation
pip install --user conan

# Verify installation
conan --version  # Should show 2.x
```

**Windows (PowerShell)**:
```powershell
# Install via pip
pip install conan

# Verify installation
conan --version
```

**Configure Conan remotes**:
```bash
conan remote add conancenter https://center.conan.io --force
conan remote list  # Verify conancenter is configured
```

### Step 2: Install Build Tools

#### Linux

**Ubuntu/Debian**:
```bash
# Install Ninja
sudo apt install ninja-build

# Install Mold linker (fastest option)
sudo apt install mold

# If Mold unavailable, install LLD
sudo apt install lld

# Verify installations
ninja --version
mold --version  # or ld.lld --version
```

**Fedora/RHEL**:
```bash
sudo dnf install ninja-build mold
```

**Arch Linux**:
```bash
sudo pacman -S ninja mold
```

#### Windows

**Install LLVM (includes clang-cl and lld-link)**:
1. Download from https://releases.llvm.org/
2. Run installer, ensure "Add LLVM to PATH" is checked
3. Verify installation:
```powershell
clang-cl --version
lld-link --version
```

**Install Ninja**:
1. Download from https://github.com/ninja-build/ninja/releases
2. Extract to `C:\Program Files\Ninja\`
3. Add to PATH: `C:\Program Files\Ninja\`
4. Verify: `ninja --version`

### Step 3: Install ccache (Optional but Recommended)

**Linux**:
```bash
# Ubuntu/Debian
sudo apt install ccache

# Fedora
sudo dnf install ccache

# Verify
ccache --version
ccache -M 5G  # Set cache size to 5GB
```

**Windows**:
```powershell
# Install via Chocolatey
choco install ccache

# Or download from https://ccache.dev/
# Add to PATH after installation

# Verify
ccache --version
ccache -M 5G
```

## Migration Workflow

### Phase 1: Dependency Setup

#### 1.1 Install Conan Dependencies

**Linux**:
```bash
cd /path/to/yamy

# Debug build
conan install . --output-folder=build/linux-debug --build=missing -s build_type=Debug

# Release build
conan install . --output-folder=build/linux-release --build=missing -s build_type=Release
```

**Windows**:
```powershell
cd C:\path\to\yamy

# Debug build
conan install . --output-folder=build\windows-clang-debug --build=missing -s build_type=Debug

# Release build
conan install . --output-folder=build\windows-clang-release --build=missing -s build_type=Release
```

**What this does**:
- Downloads dependencies: Quill 4.1.0, Microsoft GSL 4.0.0, RapidCheck, Catch2, fmt
- Generates `conan_toolchain.cmake` in the build directory
- Caches binaries in `~/.conan2` for reuse

**Expected time**:
- First run (cold cache): 5-10 minutes
- Subsequent runs (warm cache): <10 seconds

#### 1.2 Verify Conan Cache

```bash
# Check cached packages
conan list "*" --cache

# You should see:
# quill/4.1.0
# ms-gsl/4.0.0
# rapidcheck/cci.20230815
# catch2/3.5.0
# fmt/10.2.1
```

### Phase 2: Build Configuration

#### 2.1 Configure with CMake Presets

**Linux**:
```bash
# Debug configuration
cmake --preset linux-debug

# Release configuration
cmake --preset linux-release
```

**Windows**:
```powershell
# Debug configuration
cmake --preset windows-clang-debug

# Release configuration
cmake --preset windows-clang-release
```

**What this does**:
- Configures Ninja as the build generator
- Points CMake to Conan-generated toolchain
- Detects and configures Mold/LLD linker
- Enables ccache if available

**Expected output**:
```
-- Using mold linker: /usr/bin/mold
-- Enabling ccache: /usr/bin/ccache
-- Found quill 4.1.0
-- Found Microsoft.GSL 4.0.0
-- Configuring done
-- Generating done
```

#### 2.2 Build the Project

```bash
# Using preset (recommended)
cmake --build --preset linux-debug

# Or specify build directory
cmake --build build/linux-debug
```

**Expected times**:
- Clean build: 30-60 seconds (with ccache: 10-20 seconds after first build)
- Incremental build (1 file changed): <5 seconds
- Null build (no changes): <1 second

### Phase 3: Verify Toolchain

#### 3.1 Build Performance Benchmark

Run the build benchmark to verify performance targets:

```bash
# Clean build
rm -rf build/linux-debug
conan install . --output-folder=build/linux-debug --build=missing -s build_type=Debug
cmake --preset linux-debug
time cmake --build build/linux-debug

# Incremental build (touch a file)
touch src/core/engine/engine.cpp
time cmake --build build/linux-debug

# Null build (no changes)
time cmake --build build/linux-debug
```

**Expected results**:
- Incremental: <5 seconds
- Null: <1 second

#### 3.2 Verify Logging (Quill)

```bash
# Run the application
./build/linux-debug/yamy

# Check log output
cat logs/yamy.json | jq '.'

# Verify JSON format
jq 'select(.level == "INFO")' logs/yamy.json
```

Expected log structure:
```json
{
  "timestamp": "2025-12-15 10:30:45.123456789",
  "level": "INFO",
  "message": "YAMY engine initialized successfully",
  "logger": "root"
}
```

#### 3.3 Run Tests

```bash
# Run all tests
cd build/linux-debug
ctest

# Run property-based tests (1000 iterations)
RC_PARAMS="max_success=1000" ctest -R property

# Run specific test
./yamy_property_tests
```

**Expected**:
- All unit tests pass
- Property tests complete in <30 seconds
- No contract violations in debug build

### Phase 4: Adopt New Practices

#### 4.1 Logging with Quill

**Replace old logging**:
```cpp
// OLD: printf/std::cout
printf("Key pressed: %d\n", keyCode);
std::cout << "Error: " << error << std::endl;

// NEW: Quill macros
#include "utils/logger.h"
LOG_INFO("Key pressed: {}", keyCode);
LOG_ERROR("Error: {}", error);
```

See `docs/LOGGING_GUIDE.md` for complete usage.

#### 4.2 Contract Programming with GSL

**Add preconditions**:
```cpp
#include <gsl/gsl>

void processKey(KeyEvent* event) {
    Expects(event != nullptr);  // Precondition
    Expects(event->scanCode <= 0xFFFF);

    // Implementation
    auto result = process(event);

    Ensures(result != nullptr);  // Postcondition
    return result;
}
```

**Use gsl::span for arrays**:
```cpp
// OLD: pointer + size
void processKeys(const Key* keys, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        handle(keys[i]);
    }
}

// NEW: gsl::span
void processKeys(gsl::span<const Key> keys) {
    for (const auto& key : keys) {  // Bounds-safe
        handle(key);
    }
}
```

See `docs/CONTRACTS_GUIDE.md` for complete usage.

#### 4.3 Property-Based Testing with RapidCheck

**Write property tests**:
```cpp
#include <rapidcheck.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Keymap lookup is idempotent") {
    rc::check("searching twice returns same result", []() {
        Keymap km;
        auto key = *rc::gen::arbitrary<Key>();

        auto result1 = km.lookup(key);
        auto result2 = km.lookup(key);

        RC_ASSERT(result1 == result2);
    });
}
```

See `docs/PROPERTY_TESTING_GUIDE.md` for complete usage.

#### 4.4 Code Metrics Enforcement

**Install pre-commit hook**:
```bash
# Install lizard
pip install lizard

# Install hook
./scripts/install-hooks.sh
```

**Check code metrics**:
```bash
# Check entire codebase
cmake --build build/linux-debug --target check-metrics

# Check specific file
lizard --length 500 --arguments 50 --CCN 15 src/core/engine/engine.cpp
```

**Limits**:
- Max 500 lines per file
- Max 50 lines per function
- Cyclomatic complexity ≤ 15

See `docs/CODE_METRICS.md` for refactoring strategies.

## Troubleshooting

### Issue: "Conan toolchain file missing"

**Symptom**:
```
CMake Error: Conan toolchain file missing at 'build/linux-debug/conan_toolchain.cmake'.
Run `conan install . --output-folder=build/linux-debug` before configuring.
```

**Solution**:
```bash
# Install dependencies first
conan install . --output-folder=build/linux-debug --build=missing -s build_type=Debug

# Then configure
cmake --preset linux-debug
```

### Issue: "Mold linker not found"

**Symptom**:
```
Warning: Neither mold nor LLD found. Using default system linker.
```

**Solution**:
```bash
# Install Mold (preferred)
sudo apt install mold

# Or install LLD
sudo apt install lld

# Reconfigure
rm -rf build/linux-debug
cmake --preset linux-debug
```

**Impact**: Build still works but linking is slower (30s vs 3s).

### Issue: "lld-link not found" (Windows)

**Symptom**:
```
Warning: clang-cl detected but lld-link not found.
```

**Solution**:
1. Install LLVM from https://releases.llvm.org/
2. Ensure "Add LLVM to PATH" is checked during installation
3. Verify: `lld-link --version`
4. Reconfigure CMake

### Issue: Conan dependencies fail to build

**Symptom**:
```
ERROR: quill/4.1.0: Error in build() method, line 123
```

**Solutions**:

**Option 1: Use prebuilt binaries**:
```bash
# Force download binaries (don't build)
conan install . --output-folder=build/linux-debug -s build_type=Debug
```

**Option 2: Update Conan**:
```bash
pip install --upgrade conan
conan --version  # Verify 2.x
```

**Option 3: Clean cache**:
```bash
conan remove "*" -c  # Clean all packages
conan install . --output-folder=build/linux-debug --build=missing -s build_type=Debug
```

### Issue: ccache not working

**Symptom**:
```
-- ccache not found, skipping
```

**Solution**:
```bash
# Install ccache
sudo apt install ccache  # Linux
choco install ccache     # Windows

# Configure cache size
ccache -M 5G

# Verify
ccache -s  # Should show cache statistics

# Reconfigure
cmake --preset linux-debug
```

### Issue: Ninja not found

**Symptom**:
```
CMake Error: CMake was unable to find a build program corresponding to "Ninja".
```

**Solution**:
```bash
# Linux
sudo apt install ninja-build

# Windows: Download from https://github.com/ninja-build/ninja/releases
# Extract to C:\Program Files\Ninja\ and add to PATH

# Verify
ninja --version

# Reconfigure
cmake --preset linux-debug
```

### Issue: Tests fail in debug build

**Symptom**:
```
Contract violation: Expects(ptr != nullptr)
```

**This is expected behavior**! Debug builds enforce contracts with exceptions.

**Solution**:
1. Fix the bug causing the contract violation
2. Or, if this is a test case verifying the contract works:
```cpp
TEST_CASE("Contract validates null pointer") {
    REQUIRE_THROWS(engine.initialize(nullptr));  // Expected
}
```

### Issue: Property tests timeout

**Symptom**:
```
Test timeout after 600 seconds
```

**Solution**:
```bash
# Reduce iterations for local testing
RC_PARAMS="max_success=100" ctest -R property

# Or increase timeout
ctest -R property --timeout 1200
```

### Issue: Build slower than expected

**Check linker**:
```bash
# Should show "Using mold linker" or "Using lld-link"
cmake --preset linux-debug 2>&1 | grep -i linker
```

**Check ccache**:
```bash
# Should show cache statistics
ccache -s

# Expected after warm cache:
# Cache hit rate: 80-95%
```

**Benchmark**:
```bash
# Clean build
rm -rf build/linux-debug
time (conan install . --output-folder=build/linux-debug --build=missing -s build_type=Debug && \
      cmake --preset linux-debug && \
      cmake --build build/linux-debug)

# Expected: <2 minutes with warm Conan cache
```

## Platform-Specific Notes

### Linux

**Recommended setup**:
- Ubuntu 22.04+ / Fedora 38+
- GCC 11+ or Clang 14+
- Mold linker (10x faster than GNU ld)
- 8GB+ RAM for parallel builds

**Performance targets**:
- Clean build: 30-45 seconds
- Incremental: <5 seconds
- Null build: <1 second

### Windows

**Recommended setup**:
- Windows 10/11
- LLVM 16+ (clang-cl + lld-link)
- Ninja build system
- 16GB+ RAM (Visual Studio overhead)

**Performance targets**:
- Clean build: 45-60 seconds
- Incremental: <8 seconds
- Null build: <2 seconds

**Note**: MSVC compiler is not supported with this toolchain. Use clang-cl.

## Verification Checklist

After completing migration, verify:

- [ ] `conan --version` shows 2.x
- [ ] `cmake --preset linux-debug` succeeds without errors
- [ ] Build shows "Using mold linker" or "Using lld-link"
- [ ] Build shows "Enabling ccache" (if installed)
- [ ] `cmake --build build/linux-debug` completes in <5s (incremental)
- [ ] `ctest` passes all tests
- [ ] `logs/yamy.json` contains valid JSON logs
- [ ] `cmake --build . --target check-metrics` passes (or documents exceptions)
- [ ] Property tests run with `RC_PARAMS="max_success=1000"`

## Next Steps

### 1. Read Documentation
- `docs/LOGGING_GUIDE.md` - Quill logging best practices
- `docs/CONTRACTS_GUIDE.md` - GSL contract programming
- `docs/PROPERTY_TESTING_GUIDE.md` - RapidCheck property tests
- `docs/CODE_METRICS.md` - Code quality enforcement
- `docs/map.md` - Codebase structure for AI navigation

### 2. Configure Development Environment
- Install pre-commit hooks: `./scripts/install-hooks.sh`
- Configure IDE to use `compile_commands.json` (generated in build dir)
- Set up CI/CD to run metrics checks and property tests

### 3. Migrate Existing Code
- Replace printf/cout with Quill logging
- Add GSL contracts to public APIs
- Write property tests for state machines
- Refactor files exceeding metrics limits

## Performance Comparison

### Before Modern Toolchain

| Operation | Time |
|-----------|------|
| Clean build | 10 minutes |
| Incremental (1 file) | 45 seconds |
| Null build | 8 seconds |
| Linking | 30 seconds |

### After Modern Toolchain

| Operation | Time | Improvement |
|-----------|------|-------------|
| Clean build (cold cache) | 8 minutes | 20% faster |
| Clean build (warm cache) | 1.5 minutes | **83% faster** |
| Incremental (1 file) | 4 seconds | **91% faster** |
| Null build | 0.8 seconds | **90% faster** |
| Linking | 2 seconds | **93% faster** |

**Total time savings**: ~30-40 minutes per day for active development.

## References

- Conan Documentation: https://docs.conan.io/2/
- Quill Logging: https://github.com/odygrd/quill
- Microsoft GSL: https://github.com/microsoft/GSL
- RapidCheck: https://github.com/emil-e/rapidcheck
- CMake Presets: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- Mold Linker: https://github.com/rui314/mold
- Ninja Build: https://ninja-build.org/

## Support

For issues or questions:
1. Check this guide's troubleshooting section
2. Review detailed guides in `docs/`
3. Search existing issues on GitHub
4. Create new issue with:
   - Platform (Linux/Windows)
   - Tool versions (`conan --version`, `cmake --version`, etc.)
   - Full error message
   - Steps to reproduce
