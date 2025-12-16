# Conan Workflow Validation (2025-12-15)

## Environment
- Host: Linux (gcc 13.3, Ninja 1.11.1)
- Conan: 2.23.0 (installed via `pipx install conan`)
- Repository root: `/home/rmondo/repos/yamy`

## Linux Debug Workflow
Commands run from repo root:
```bash
# Fresh profile/cache for a clean-machine simulation
CONAN_HOME=$PWD/.conan-cache-clean conan profile detect --force

# Install deps and generate toolchain/deps files for the preset
CONAN_HOME=$PWD/.conan-cache-clean conan install . \
  --output-folder=build/linux-debug \
  --build=missing \
  -s build_type=Debug

# Configure with the generated toolchain
cmake --preset linux-debug

# Build (stops on unit-test compilation failures below)
cmake --build --preset linux-debug
```

Results:
- Conan successfully built and cached:
  - quill/3.9.0
  - fmt/10.2.1
  - ms-gsl/4.0.0
  - rapidcheck/cci.20230815
  - catch2/3.5.0
- Toolchain and deps emitted to `build/linux-debug/conan_toolchain.cmake` and accompanying `*-config.cmake` files in the same folder.
- CMake configure succeeded. Mold/LLD not installed on host; build fell back to the system linker with a warning.
- Build failed in unit/integration tests because `EventProcessor::processEvent` now requires a third `ModifierState*` argument. Tests in `tests/test_event_processor_{ut,it}.cpp` call the two-argument signature and need to be updated or provided with modifier state. Dependency toolchain itself is working.

## Windows Workflow (not executed here)
- Expected commands (from PowerShell):
```powershell
conan install . --output-folder=build\windows-clang-debug --build=missing -s build_type=Debug
cmake --preset windows-clang-debug
cmake --build --preset windows-clang-debug
```
- Not run due to lack of a Windows host in this environment; pending validation on a Windows runner.

## Observations & Follow-ups
- Linker detection now falls back to the system linker if neither mold nor `ld.lld` is present; no hard failure in clean environments.
- Conan manifest uses quill/3.9.0 and fmt/10.2.1 (matches upstream requirements).
- Next step to get a green build: fix test callers to pass `ModifierState*` into `processEvent` or add helpers that supply the state for tests.
