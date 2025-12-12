# #ifdef Reduction Strategy

## Current Status
- **Total #ifdef _WIN32**: 21 (was 75, -72%)
- **Inline conditionals**: 0 ✓ (excellent!)
- **Top offenders**: setting_loader (5, 24%), engine_lifecycle.cpp (4, 19%)
- **Recent progress** (this session):
  - Consolidated #ifdef blocks in engine.cpp and engine_lifecycle.cpp (-4)
  - Removed unused windowstool.h includes (-2)
  - Removed switchConfiguration #ifdef guard from engine.h (-1)
  - Created ConfigWatcher Windows stub, eliminated #ifdefs from config_manager (-2)
  - Fixed build errors and added platform compatibility (+1 for engine_setting.cpp stub)
  - Batch eliminated include guards in 4 engine files (-8)
  - Eliminated include guards in engine_log.cpp (-2)
  - Eliminated include guards in setting_loader.cpp (-3)
  - Eliminated #ifdefs in vkeytable.cpp (-2)
  - Used std::filesystem in config_validator (-1)
  - Created platform abstraction for executable directory path (-2)
  - Eliminated registerWindowMessage conditionals in command files (-3)
  - Refactored stream handling in engine_log.cpp (-3)
  - Used std::filesystem for path operations (-3)
  - Replaced Sleep/usleep with std::this_thread::sleep_for (-2)
  - Used strcasecmp_platform macro (-2)
  - Always use to_tstring() for stream conversions (-1)
  - **Added comprehensive platform abstractions for sync primitives (-5)**:
    - Created Event API (createEvent, setEvent, resetEvent, destroyEvent)
    - Created Mutex API (createMutex, acquireMutex, releaseMutex, destroyMutex)
    - Added destroyThread to thread API
    - Replaced all Event/Mutex operations in engine_lifecycle.cpp
    - Changed thread entry signatures from WINAPI to platform-agnostic
    - Replaced _beginthreadex with yamy::platform::createThread()
- **Files with ZERO #ifdefs**: setting.cpp ✓, vkeytable.cpp ✓, engine_log.cpp ✓, engine_modifier.cpp ✓, engine_input.cpp ✓, engine_generator.cpp ✓, engine_focus.cpp ✓, config_validator.cpp ✓, config_manager.h ✓, config_manager.cpp ✓, engine.h ✓

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

### Baseline (2025-12-13)
```
Initial: 75
Current: 21 (-54 total, -72%)
Inline: 0
Files with ZERO #ifdefs: 11
Should refactor: 13 (setting_loader: 5, engine_lifecycle.cpp: 4, engine.cpp: 1, engine_window.cpp: 1, engine_setting.cpp: 1, target.cpp: 1)
Acceptable pattern: 8 (platform abstraction headers + Windows-only files)
```

### Target (v2.0)
```
Total: 0 (eliminate ALL #ifdefs)
Inline: 0
File-level separation for all platform-specific code
Platform abstraction layer for all platform differences
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
