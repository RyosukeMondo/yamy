# #ifdef Reduction Strategy

## Current Status
- **Total #ifdef _WIN32**: 72 (was 75)
- **Inline conditionals**: 0 ✓ (good!)
- **Top offenders**: engine_lifecycle.cpp (12), setting_loader.cpp (9), engine_log.cpp (5)
- **Recent progress**: Eliminated registerWindowMessage conditionals in command files (-3)

## Categories of #ifdef Usage

### ✅ ACCEPTABLE (keep these):
1. **Include guards** for platform headers
   ```cpp
   #ifdef _WIN32
   #include "windowstool.h"
   #endif
   ```
   **Reason**: Standard practice, minimal cognitive load

2. **Type definitions** at file top
   ```cpp
   #ifdef _WIN32
   typedef HANDLE PlatformHandle;
   #else
   typedef int PlatformHandle;
   #endif
   ```
   **Reason**: Clear, localized, easy to understand

### ❌ SHOULD REMOVE (refactor these):
1. **Function implementations** switched by platform
   ```cpp
   void Engine::foo() {
   #ifdef _WIN32
       // Windows code
   #else
       // Linux code
   #endif
   }
   ```
   **Solution**: Split to separate files
   - `engine_platform_windows.cpp`
   - `engine_platform_linux.cpp`

2. **Multiple conditionals** in same function
   ```cpp
   void Engine::bar() {
       doSomething();
   #ifdef _WIN32
       doWindowsThing();
   #endif
       doMore();
   #ifdef _WIN32
       doAnotherWindowsThing();
   #endif
   }
   ```
   **Solution**: Extract to platform-specific helper functions

## Refactoring Plan

### Phase 1: Split Large Files (Priority 1)
**Target**: Files with 5+ #ifdefs

1. **engine_lifecycle.cpp** (12 #ifdefs)
   - Extract: `engine_lifecycle_windows.cpp` / `engine_lifecycle_linux.cpp`
   - Keep shared logic in base file
   - Use platform factory pattern

2. **setting_loader.cpp** (9 #ifdefs)
   - Extract platform-specific file loading to separate impl

3. **engine_log.cpp** (5 #ifdefs)
   - Already fixed most - remaining are acceptable include guards

### Phase 2: Convert to Platform Abstraction (Priority 2)
**Target**: Scattered conditionals

Create platform interface:
```cpp
// platform_interface.h
class IPlatformServices {
    virtual PlatformHandle createEvent() = 0;
    virtual void registerWindowMessage(const std::string&) = 0;
    // ...
};

// platform_windows.cpp
class WindowsPlatform : public IPlatformServices { /* impl */ };

// platform_linux.cpp
class LinuxPlatform : public IPlatformServices { /* impl */ };
```

### Phase 3: CI Enforcement
Add to `.github/workflows/ci.yml`:
```yaml
- name: Check #ifdef count
  run: |
    bash scripts/check_ifdef_usage.sh
    CURRENT=$(cat .github/metrics/ifdef_count.txt)
    BASELINE=75
    if [ "$CURRENT" -gt "$BASELINE" ]; then
      echo "ERROR: #ifdef count increased from $BASELINE to $CURRENT"
      exit 1
    fi
    echo "✓ #ifdef count: $CURRENT (baseline: $BASELINE)"
```

## Decision Matrix

| #ifdef Type | Action | Reason |
|-------------|--------|--------|
| Include guards | **KEEP** | Standard, minimal impact |
| Type aliases (file top) | **KEEP** | Clear, localized |
| Entire function body | **REFACTOR** | Split to separate files |
| Mid-function logic | **REFACTOR** | Extract to platform helpers |
| Nested #ifdef | **REFACTOR** | Too complex |
| Single statement | **EVALUATE** | Keep if trivial, refactor if repeated |

## Monitoring

### Baseline (2025-12-12)
```
Initial: 75
Current: 72 (-3 from registerWindowMessage fixes)
Inline: 0
Acceptable: ~40 (include guards + type defs)
Should refactor: ~32
```

### Target (v2.0)
```
Total: <40
Inline: 0
File-level separation for all platform-specific code
```

### Commands
```bash
# Check current count
bash scripts/check_ifdef_usage.sh

# Track progress
git log --all --pretty=format:"%h %s" --grep="ifdef"

# Find next target
bash scripts/check_ifdef_usage.sh | grep "Top 10"
```

## Next Actions
1. ✅ Add CI check to prevent increase
2. ⏳ Refactor engine_lifecycle.cpp (highest priority)
3. ⏳ Refactor setting_loader.cpp
4. ⏳ Create platform abstraction layer
5. ⏳ Document platform-specific build process
