# YAMY Development Scripts

This directory contains utility scripts for YAMY development.

## Git Hooks

### Pre-commit Hook for Code Metrics

The pre-commit hook enforces code quality metrics using [lizard](https://github.com/terryyin/lizard).

**Metrics enforced:**
- Max 500 lines per file (excluding blank/comment lines)
- Max 50 lines per function
- Max cyclomatic complexity (CCN) ≤ 15

### Installation

```bash
# Install lizard (required)
pip install lizard

# Install the Git hooks
./scripts/install-hooks.sh
```

### Usage

Once installed, the pre-commit hook runs automatically on every commit:

```bash
# Normal commit - hook runs automatically
git commit -m "Add feature"

# Bypass hook temporarily (use sparingly!)
git commit --no-verify -m "WIP: Large refactoring"
```

**Hook behavior:**
- Checks only staged C++ files (`.cpp`, `.h`, `.hpp`, etc.)
- Fast execution (typically < 2 seconds)
- Clear violation reports with file and line numbers
- Automatic skip if lizard is not installed (with warning)

**Example output on violation:**
```
Running code metrics check on staged files...

Code metrics violations found:

src/core/engine/engine_large.cpp
  Line 123:  processComplexInput()
    ⚠ Function length: 75 (max: 50)
    ⚠ Cyclomatic complexity: 18 (max: 15)

Commit rejected.

Please fix the violations or use 'git commit --no-verify' to bypass this check.
```

### Manual Check

To manually check metrics without committing:

```bash
# Check all files
cmake --build build --target check-metrics

# Check specific file
lizard --length 50 --CCN 15 -w path/to/file.cpp
```

## Build Benchmarking

See `benchmark_build.sh` for build performance testing.

## Platform-specific Scripts

- `linux/`: Linux-specific build and development scripts
- `windows/`: Windows-specific build and development scripts
