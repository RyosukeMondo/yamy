# Conan Binary Cache Setup (Conan 2.x)

This guide configures Conan binary caching for the modern C++ toolchain spec. It uses only the public ConanCenter remote and targets reproducible installs that drop clean build times under two minutes after the cache is warm.

## Prerequisites
- Conan 2.x installed (`pipx install conan` or `pip install --user conan`).
- Ninja and CMake 3.28+ available (presets rely on them).
- From the repo root unless noted.

Verify your Conan version and home:
```bash
conan --version
conan config home   # default: ~/.conan2
```

## Remote configuration (ConanCenter only)
```bash
conan remote list
conan remote add conancenter https://center.conan.io --force
```
No custom remotes are required; binaries are pulled from ConanCenter.

## Cache location and structure
- Default cache: `~/.conan2`
- Packages: `~/.conan2/p/<ref>` (downloaded binaries)
- Build artifacts: `~/.conan2/b/<ref>` (used during package build)
- You can relocate the cache per-workspace: `export CONAN_HOME=$PWD/.conan-cache`

Inspect current cache contents:
```bash
conan list "*" --cache
```

## Prime the cache for project presets
Run an install per preset to populate the cache and generate the toolchain/deps files where CMake expects them. Always match the preset's build type:
```bash
# Linux
conan install . --output-folder=build/linux-debug --build=missing -s build_type=Debug
conan install . --output-folder=build/linux-release --build=missing -s build_type=Release

# Windows clang-cl (from PowerShell)
conan install . --output-folder=build\\windows-clang-debug --build=missing -s build_type=Debug
conan install . --output-folder=build\\windows-clang-release --build=missing -s build_type=Release
```
What each command does:
- Downloads binaries from ConanCenter into `~/.conan2/p`.
- Builds missing binaries once (stored in cache for reuse).
- Emits `conan_toolchain.cmake` and `conan_deps*.cmake` into `build/<preset>` used by `CMakePresets.json`.

## Validate caching and performance
1) Cold fetch (no cache):
```bash
conan cache clean --build --download
time conan install . --output-folder=build/linux-debug --build=missing
```
2) Warm fetch (cache hit):
```bash
time conan install . --output-folder=build/linux-debug
```
Expected: second run finishes in seconds because binaries are reused locally. Target clean configure+build wall time after a warm cache is <2 minutes (vs ~10 minutes cold).

Check that binaries came from cache:
```bash
conan list "*" --cache | grep quill
```

## Cache cleanup and recovery
- Remove build and download artifacts (keeps package metadata):
```bash
conan cache clean --build --download
```
- Nuke everything for this project only (from repo root, when using per-project `CONAN_HOME`):
```bash
rm -rf .conan-cache
```
- Clear locks if an install was interrupted:
```bash
conan remove --locks
```

## Troubleshooting
- **Toolchain file missing**: Ensure you ran `conan install . --output-folder=build/<preset>` before `cmake --preset <preset>`.
- **Using wrong remote**: `conan remote list` should only show `conancenter` unless you intentionally add more.
- **Cache too large**: `conan cache clean --build --download` frees space without touching metadata; relocate cache with `CONAN_HOME` if multiple repos compete for space.

With the cache primed, rerunning `conan install` for any preset should be near-instant and keep clean builds under the two-minute target on typical developer hardware.
