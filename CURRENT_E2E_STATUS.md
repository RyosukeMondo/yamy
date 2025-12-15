# E2E Testing Status - 2025-12-15

## ✅ What's Working

1. **Basic Test Infrastructure**
   - Persistent test keyboard daemon (`tests/test_keyboard_daemon.py`) ✓
   - Event injection tool (`yamy-inject`) ✓
   - Event capture tool (`yamy-capture`) ✓
   - Test orchestrator (`yamy-test-runner`) ✓

2. **Complete Event Flow Verified**
   - Test Keyboard → InputHook → EventReaderThread → Engine Callback → EventProcessor → InputInjector → Virtual Output
   - Confirmed with debug logging:
     ```
     [EVENT] Read from /dev/input/event8: scancode=0x11 DOWN
     [EVENT] Callback returned BLOCK
     [OUTPUT] Injecting evdev code 0x1e (A) DOWN
     ```

3. **Simple Substitution Working**
   - `def subst *W = *A` mapping works correctly
   - Test: Inject KEY_W → Output KEY_A ✓
   - Verified with yamy-capture tool

## ❌ Critical Bugs Found

### Bug #1: Config Reload Loop
**Severity**: CRITICAL
**Symptom**: `switchConfiguration()` is called repeatedly in a tight loop
**Impact**:
- Config reloaded dozens of times per second
- Modal modifier state likely being reset continuously
- Performance degradation

**Evidence**:
```
switchConfiguration: successfully switched to: /home/rmondo/repos/yamy/keymaps/test_minimal_m0.mayu
  loading: /home/rmondo/repos/yamy/keymaps/test_minimal_m0.mayu
  loading: /home/rmondo/repos/yamy/keymaps/109_clean.mayu
Built substitution table with 2 mappings
Registered 0 number modifiers
Registered 1 modal modifiers
switchConfiguration: successfully switched to: /home/rmondo/repos/yamy/keymaps/test_minimal_m0.mayu
  [repeats indefinitely]
```

**Suspected Cause**: Something in the event processing path is triggering IPC commands or config reload

**Files to Investigate**:
- src/core/engine/engine_ipc_handler.cpp (lines 110-160)
- src/core/engine/engine_setting.cpp (line 92+)
- src/app/engine_adapter.cpp (line 135-150)

### Bug #2: Modal Modifier Not Working
**Severity**: HIGH
**Symptom**: `mod mod0 = !!B` does not activate M0 mode
**Impact**:
- M0-A does not output _1 as configured
- B is output as B instead of being suppressed during modal activation
- Modal modifier feature completely broken

**Evidence**:
- Config shows: `[REGISTER_MODAL] 0x0030 → mod0 (is_modal=1)`
- Test: Double-tap B, then press W (→A)
- Expected: B suppressed, A→1 in M0 mode
- Actual: B output as B (0x30), A output as A (not 1)

**Event Flow**:
```
[EVENT] Read scancode=0x30 DOWN  (B press)
[EVENT] Callback returned BLOCK
[EVENT] Read scancode=0x30 UP    (B release)
[EVENT] Callback returned BLOCK
[OUTPUT] Injecting evdev code 0x30 (B) DOWN  ← WRONG! Should be suppressed
```

**Root Cause Hypothesis**:
1. Modal modifier logic in `ModifierKeyHandler::processNumberKey()` not detecting double-tap correctly
2. OR the config reload loop is resetting modal modifier state before activation completes
3. OR B→Enter substitution is interfering with modal modifier detection

**Files to Investigate**:
- src/core/engine/modifier_key_handler.cpp
- src/core/engine/engine_event_processor.cpp (lines 130-198)

## 🔍 Debug Logging Issues

- EventProcessor has debug logging enabled (`m_debugLogging = true`) but LOG_DEBUG output not visible
- Reason: Log level set to INFO, DEBUG logs filtered out
- Workaround: Added `std::cerr` debug prints for [EVENT] and [OUTPUT] tracking
- These stderr prints ARE working and visible in /tmp/claude/tasks/*.output

## 📋 Next Steps

### Priority 1: Fix Config Reload Loop
1. Add debug logging to identify what's triggering switchConfiguration
2. Check if IPC commands are being sent in a loop
3. Check if window focus changes are triggering reloads
4. Fix the trigger and verify config only loads once at startup

### Priority 2: Fix Modal Modifier
1. After fixing reload loop, re-test modal modifier
2. If still broken, add detailed logging to `ModifierKeyHandler::processNumberKey()`
3. Verify double-tap detection logic
4. Check if ProcessingAction::ACTIVATE_MODIFIER is being returned
5. Verify modal modifier state is being updated in ModifierState

### Priority 3: Complete E2E Test Suite
1. Once modal modifiers work, create test scenarios:
   - Simple substitution (W→A) ✓ DONE
   - Modal modifier activation (!!B)
   - Modal key mapping (M0-A → 1)
   - Number modifier (hold for modifier, tap for key)
   - Complex combinations
2. Integrate into automated test runner
3. Run full P0 test suite from tests/suites/

## 📊 Test Results

| Test Scenario | Status | Notes |
|---------------|--------|-------|
| W→A substitution | ✅ PASS | Simple substitution works |
| M0 activation (!!B) | ❌ FAIL | Config loop + modal modifier bug |
| M0-A → 1 mapping | ❌ FAIL | Depends on M0 activation |
| Event capture | ✅ PASS | yamy-capture tool works |
| Event injection | ✅ PASS | Python evdev injection works |

## 🔧 Configuration Used

**File**: `keymaps/test_minimal_m0.mayu`
```
include "109_clean.mayu"

def subst *B = *Enter
def subst *W = *A

mod mod0 = !!B

keymap Global : GLOBAL
    key M0-A = *_1
```

**Test Device**: YAMY-Test-KB (persistent virtual keyboard)
**YAMY Output Device**: Yamy Virtual Input Device
**Debug Environment**: YAMY_DEBUG_KEYCODE=1

## 📝 Architecture Notes

**Event Processing Layers**:
1. **LAYER1**: evdev code → YAMY internal code conversion
2. **LAYER2**: Modal/Number modifiers + Substitution
   - Modifiers checked FIRST (line 133)
   - Substitution applied SECOND (line 201)
3. **LAYER3**: YAMY internal code → evdev code conversion

**Modal Modifier Logic** (from engine_event_processor.cpp:130-198):
- Modal/Number modifiers checked BEFORE substitution
- Double-tap (!!
) activates one-shot mode
- ACTIVATE_MODIFIER → return 0 (suppress output)
- DEACTIVATE_MODIFIER → return 0 (suppress output)
- TAP detected → fall through to normal substitution

**Why This Matters**: The fact that B is being OUTPUT means the modal modifier logic is NOT returning 0 to suppress it. Either:
1. `isModalModifier(0x30)` returns false (not registered)
2. `processNumberKey()` returns wrong action
3. The return value is being ignored somewhere
