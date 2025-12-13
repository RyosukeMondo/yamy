# Number-to-Modifier Mapping System Design

## Overview

This document specifies the design for an advanced feature that allows number keys (0-9) to function as **custom hardware modifier keys** for users with small keyboards without ten-keys (numpad). The system enables hold-vs-tap detection: holding a number key activates a hardware modifier, while tapping it applies the normal substitution.

**Primary Use Case**: Users with compact keyboards (60%, 65%, 75% layouts) need more physical modifiers without sacrificing number keys entirely.

**Design Principles**:
1. **Backward Compatibility**: Existing number key substitutions continue to work (tap behavior)
2. **Zero Special Cases**: Integrates seamlessly with EventProcessor architecture
3. **Deterministic Timing**: Hold-vs-tap detection with clear, configurable threshold
4. **Hardware Modifier Wiring**: Direct mapping to physical modifier keys (LShift, RCtrl, etc.)
5. **Modal Layer Support**: Extends existing `mod modX = !!_Y` syntax

## Background: Existing Modal Layer System

YAMY already has a modal layer system that allows keys to activate custom modifier layers:

```mayu
mod mod4 = !!_1         # Hold number 1 to activate mod4 layer
key m4-*A = *Enter      # When mod4 active, A → Enter
```

**Current Behavior**:
- The `!!` prefix means "hold to activate modifier"
- Creates a **modal layer** (mod0-mod9) that changes key behavior when active
- Existing implementation in engine handles modal layer switching

**Limitation**:
- Modal layers are **engine-level** modifiers, not **hardware modifiers**
- They don't work outside YAMY (e.g., can't use mod4+A in other applications)
- Can't combine with system-level shortcuts

**Enhancement Goal**:
- Allow number keys to activate **hardware modifiers** (LShift, RCtrl, etc.)
- These work system-wide, not just within YAMY
- Preserve existing modal layer functionality for non-hardware use cases

## Requirements

From **Requirement 8: Advanced Feature - Number Keys as Custom Modifiers**:

1. User can define number key as modifier in .mayu: `mod mod4 = !!_1`
2. Holding number key as modifier + pressing another key activates modal layer
3. If number key has both substitution and modifier definition, modifier takes precedence when held, substitution when tapped
4. Releasing number modifier key deactivates the modal layer
5. Support direct wiring to hardware modifiers: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin

**Additional Design Goals**:
- Hold-vs-tap threshold: **200ms** (configurable)
- No performance regression (< 1ms processing overhead)
- Thread-safe timer implementation
- Clean integration with Layer 2 (before substitution lookup)

## Architecture

### High-Level Integration Point

The ModifierKeyHandler integrates into **Layer 2** of the EventProcessor pipeline, **before** substitution lookup:

```
┌─────────────────────────────────────────────────────────────┐
│  EventProcessor::processEvent(evdev, event_type)            │
│                                                              │
│  Layer 1: evdev → yamy_scan_code                            │
│           ↓                                                  │
│  ┌────────────────────────────────────────────────┐         │
│  │ Layer 2: Check Number Modifier (NEW)          │         │
│  │                                                 │         │
│  │  IF yamy_code is registered number modifier:   │         │
│  │    - Delegate to ModifierKeyHandler            │         │
│  │    - HOLD detected → output hardware modifier  │         │
│  │    - TAP detected → fall through to subst      │         │
│  │  ELSE:                                          │         │
│  │    - Proceed to normal substitution lookup     │         │
│  └────────────────────────────────────────────────┘         │
│           ↓                                                  │
│  Layer 2: Apply substitution (if not modifier)              │
│           ↓                                                  │
│  Layer 3: yamy_scan_code → output_evdev                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Key Design Decision**: Number modifier check happens **before** substitution lookup but **within** Layer 2 logic. This maintains the single-path processing principle while allowing modifier behavior to take precedence.

### State Machine: Hold vs Tap Detection

The system uses a **time-based state machine** to distinguish hold from tap:

```
                    PRESS event
                         │
                         ↓
     ┌───────────────[ IDLE ]
     │                   │
     │           Start 200ms timer
     │                   ↓
     │         ┌────[ WAITING ]────┐
     │         │                    │
     │    Timer expires       RELEASE before timer
     │    (200ms elapsed)     (< 200ms elapsed)
     │         │                    │
     │         ↓                    ↓
     │    [ MODIFIER_ACTIVE ]   [ TAP_DETECTED ]
     │         │                    │
     │    Output hardware       Apply normal
     │    modifier PRESS        substitution
     │         │                (fall through)
     │         │                    │
     │    Other key presses         │
     │    use this modifier         │
     │         │                    │
     │    RELEASE event         RELEASE event
     │         │                    │
     │         ↓                    ↓
     │    Output modifier     Output substituted
     │    RELEASE            key RELEASE
     │         │                    │
     └─────────┴────────────────────┘
              Back to IDLE
```

**State Definitions**:

1. **IDLE**: No number modifier key currently active
2. **WAITING**: Number key pressed, timer running, waiting for 200ms threshold
3. **MODIFIER_ACTIVE**: Hold detected (≥200ms), hardware modifier activated
4. **TAP_DETECTED**: Release detected before threshold, treat as normal key substitution

**Critical Timing Invariants**:
- Timer starts **immediately** on PRESS event
- Timer threshold: **200ms** (configurable via constant)
- RELEASE before threshold → TAP
- Timer expiry before RELEASE → HOLD
- No ambiguous states (deterministic decision)

### Component Design

#### Class: ModifierKeyHandler

**File**: `src/core/engine/modifier_key_handler.h`, `src/core/engine/modifier_key_handler.cpp`

```cpp
#ifndef _MODIFIER_KEY_HANDLER_H
#define _MODIFIER_KEY_HANDLER_H

#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include "../platform/types.h"

namespace yamy::engine {

/// Hardware modifier types (aligned with ModifierState flags)
enum class HardwareModifier : uint8_t {
    NONE = 0,
    LSHIFT,
    RSHIFT,
    LCTRL,
    RCTRL,
    LALT,
    RALT,
    LWIN,
    RWIN
};

/// Number key state for hold-vs-tap detection
enum class NumberKeyState : uint8_t {
    IDLE,              // Not pressed
    WAITING,           // Pressed, timer running
    MODIFIER_ACTIVE,   // Hold detected, modifier activated
    TAP_DETECTED       // Release before threshold, treat as tap
};

/// Processing action returned by processNumberKey()
enum class ProcessingAction : uint8_t {
    NOT_A_NUMBER_MODIFIER,      // Key is not registered as number modifier
    ACTIVATE_MODIFIER,          // HOLD detected, activate hardware modifier
    DEACTIVATE_MODIFIER,        // RELEASE after HOLD, deactivate modifier
    APPLY_SUBSTITUTION_PRESS,   // TAP detected on PRESS, apply substitution
    APPLY_SUBSTITUTION_RELEASE, // TAP detected on RELEASE, apply substitution
    WAITING_FOR_THRESHOLD       // Still waiting for hold threshold
};

/// Result from processNumberKey()
struct NumberKeyResult {
    ProcessingAction action;
    uint16_t output_yamy_code;  // Hardware modifier scancode if ACTIVATE/DEACTIVATE
    bool valid;
};

/// Handler for number keys as custom modifiers
/// Implements hold-vs-tap detection with 200ms threshold
class ModifierKeyHandler {
public:
    /// Constructor
    /// @param hold_threshold_ms Hold detection threshold in milliseconds (default: 200)
    explicit ModifierKeyHandler(uint32_t hold_threshold_ms = 200);

    /// Register a number key as a hardware modifier
    /// @param yamy_scancode YAMY scan code for number key (e.g., 0x0002 for _1)
    /// @param modifier Hardware modifier to activate (e.g., LSHIFT)
    void registerNumberModifier(uint16_t yamy_scancode, HardwareModifier modifier);

    /// Process a number key event (PRESS or RELEASE)
    /// @param yamy_scancode YAMY scan code of the key
    /// @param event_type PRESS or RELEASE
    /// @return Processing result indicating action to take
    NumberKeyResult processNumberKey(uint16_t yamy_scancode,
                                      yamy::platform::EventType event_type);

    /// Check if a YAMY scan code is registered as a number modifier
    /// @param yamy_scancode YAMY scan code to check
    /// @return true if registered, false otherwise
    bool isNumberModifier(uint16_t yamy_scancode) const;

    /// Check if a number modifier is currently held (MODIFIER_ACTIVE state)
    /// @param yamy_scancode YAMY scan code to check
    /// @return true if held, false otherwise
    bool isModifierHeld(uint16_t yamy_scancode) const;

    /// Reset all number key states (for testing or recovery)
    void reset();

private:
    /// Mapping: YAMY number key scan code → hardware modifier type
    std::unordered_map<uint16_t, HardwareModifier> m_number_to_modifier;

    /// Per-key state tracking
    struct KeyState {
        NumberKeyState state;
        std::chrono::steady_clock::time_point press_time;
        HardwareModifier target_modifier;
    };

    /// State for each registered number key
    std::unordered_map<uint16_t, KeyState> m_key_states;

    /// Hold threshold in milliseconds
    uint32_t m_hold_threshold_ms;

    /// Get hardware modifier YAMY scan code for a given modifier type
    /// @param modifier Hardware modifier type
    /// @return YAMY scan code (e.g., VK_LSHIFT)
    static uint16_t getModifierScancode(HardwareModifier modifier);

    /// Check if hold threshold has been exceeded
    /// @param press_time Timestamp when key was pressed
    /// @return true if >= threshold, false otherwise
    bool hasExceededThreshold(const std::chrono::steady_clock::time_point& press_time) const;
};

} // namespace yamy::engine

#endif // _MODIFIER_KEY_HANDLER_H
```

**Key Design Decisions**:

1. **No Separate Timer Thread**: Uses `std::chrono::steady_clock` for timestamp-based timeout detection. Checking happens on subsequent events (RELEASE or other key events), avoiding timer thread complexity.

2. **State Per Key**: Each registered number key has its own `KeyState` struct, allowing multiple number modifiers to be held simultaneously.

3. **Pure Function Style**: `processNumberKey()` is mostly pure - takes input, checks time, returns action. Only side effect is updating internal state.

4. **Thread Safety**: Not required - ModifierKeyHandler is owned by EventProcessor, which is single-threaded in event processing path.

5. **Hardware Modifier Mapping**: Uses existing YAMY VK codes (VK_LSHIFT, VK_LCTRL, etc.) for modifier output.

### Data Flow Examples

#### Example 1: TAP Behavior (Quick Press/Release)

```
Time: 0ms    | User presses number 1
             | PRESS event: yamy_code = 0x0002 (_1)
             |
             | processNumberKey(0x0002, PRESS):
             |   - state = IDLE → WAITING
             |   - press_time = now()
             |   - return WAITING_FOR_THRESHOLD
             |
             | Layer 2 does NOT output anything yet
             |
Time: 50ms   | User releases number 1 (before 200ms threshold)
             | RELEASE event: yamy_code = 0x0002
             |
             | processNumberKey(0x0002, RELEASE):
             |   - elapsed = 50ms < 200ms
             |   - state = WAITING → TAP_DETECTED
             |   - return APPLY_SUBSTITUTION_PRESS then APPLY_SUBSTITUTION_RELEASE
             |
             | Layer 2 applies normal substitution:
             |   - 0x0002 (_1) → VK_LSHIFT (from config_clean.mayu)
             |   - Output: PRESS VK_LSHIFT, then RELEASE VK_LSHIFT
             |
Result: Number 1 substitution works normally (outputs LShift press+release)
```

**Note**: TAP behavior maintains backward compatibility with existing substitutions.

#### Example 2: HOLD Behavior (Held for Modifier)

```
Time: 0ms    | User presses number 1
             | PRESS event: yamy_code = 0x0002 (_1)
             |
             | processNumberKey(0x0002, PRESS):
             |   - state = IDLE → WAITING
             |   - press_time = now()
             |   - return WAITING_FOR_THRESHOLD
             |
             | Layer 2 does NOT output anything yet
             |
Time: 100ms  | User presses letter A (while number 1 still held)
             | PRESS event: yamy_code = 0x001E (A)
             |
             | processNumberKey(0x001E, PRESS):
             |   - Not a registered number modifier
             |   - return NOT_A_NUMBER_MODIFIER
             |
             | But FIRST, check if any number modifiers exceeded threshold:
             |   - Check key 0x0002: elapsed = 100ms
             |   - Still < 200ms threshold
             |   - Still WAITING
             |
             | Layer 2 applies substitution for A normally
             |
Time: 210ms  | Timer threshold exceeded (200ms since press)
             | Next event (could be RELEASE of number 1, or another key)
             |
             | processNumberKey(0x0002, <any event>):
             |   - elapsed = 210ms >= 200ms
             |   - state = WAITING → MODIFIER_ACTIVE
             |   - Output hardware modifier PRESS
             |   - return ACTIVATE_MODIFIER, output_yamy_code = VK_LSHIFT
             |
             | Layer 3 outputs: PRESS VK_LSHIFT → evdev KEY_LEFTSHIFT
             |
Time: 500ms  | User presses letter A (number 1 still held)
             | System sees: LShift + A = Shift+A (handled by OS)
             |
Time: 600ms  | User releases number 1
             | RELEASE event: yamy_code = 0x0002
             |
             | processNumberKey(0x0002, RELEASE):
             |   - state = MODIFIER_ACTIVE → IDLE
             |   - return DEACTIVATE_MODIFIER, output_yamy_code = VK_LSHIFT
             |
             | Layer 3 outputs: RELEASE VK_LSHIFT → evdev KEY_LEFTSHIFT
             |
Result: Number 1 acts as LShift modifier, works system-wide
```

**Key Insight**: Threshold check happens on **next event**, not via timer callback. This avoids threading complexity.

### Configuration Syntax Extension

#### Current .mayu Syntax (Modal Layers)

```mayu
mod mod4 = !!_1         # Hold number 1 to activate mod4 layer (engine modifier)
key m4-*A = *Enter      # When mod4 active, A → Enter
```

This creates an **engine-level modal layer**, not a hardware modifier.

#### NEW Syntax: Direct Hardware Modifier Mapping

```mayu
# Method 1: Extend mod syntax with hardware modifier target
mod *LShift = !!_1      # Hold number 1 → activate hardware LShift

# Method 2: New def modifier syntax (clearer)
def modifier *_1 = *LShift   # Number 1 → LShift modifier
def modifier *_2 = *RShift   # Number 2 → RShift modifier
def modifier *_3 = *LCtrl    # Number 3 → LCtrl modifier
def modifier *_4 = *RCtrl    # Number 4 → RCtrl modifier
def modifier *_5 = *LAlt     # Number 5 → LAlt modifier
def modifier *_6 = *RAlt     # Number 6 → RAlt modifier
def modifier *_7 = *LWin     # Number 7 → LWin modifier
def modifier *_8 = *RWin     # Number 8 → RWin modifier
```

**Recommended Syntax**: `def modifier *_X = *<HardwareModifier>`

**Reasoning**:
- Clear distinction from `def subst` (substitutions) and `mod modX` (modal layers)
- Explicit about modifier nature
- Consistent with existing YAMY syntax patterns
- Parser can validate that target is a valid hardware modifier

#### Backward Compatibility

If a number key has **both** a substitution and a modifier definition:

```mayu
def subst *_1 = *LShift         # Number 1 substitution (for tap)
def modifier *_1 = *LShift      # Number 1 modifier (for hold)
```

**Behavior**:
- **TAP** (< 200ms): Apply substitution (`def subst`)
- **HOLD** (≥ 200ms): Activate hardware modifier (`def modifier`)

**Validation**: Parser should **allow** both, as they serve different purposes (tap vs hold).

#### Modal Layer Compatibility

Number keys can still be used for modal layers:

```mayu
mod mod4 = !!_1                 # Number 1 activates mod4 layer
def modifier *_1 = *LShift      # Number 1 also acts as LShift when held

key m4-*A = *Enter              # When mod4 active, A → Enter
```

**Behavior**:
- **HOLD** number 1 (≥ 200ms):
  - Activates **both** mod4 modal layer (engine-level)
  - Activates **LShift** hardware modifier (system-level)
  - Pressing A:
    - Engine sees mod4 + A → Enter (via modal layer)
    - System sees LShift + A → Shift+A (if engine doesn't consume it)

**Recommendation**: Don't combine modal layer and hardware modifier for same key to avoid confusion. Use one or the other.

### Validation Rules

When parsing `.mayu` file with `def modifier` syntax:

1. **Target Validation**: Modifier target MUST be one of:
   - `*LShift`, `*RShift`
   - `*LCtrl`, `*RCtrl`
   - `*LAlt`, `*RAlt`
   - `*LWin`, `*RWin`

2. **Source Validation**: Modifier source MUST be a number key:
   - `*_0` through `*_9`
   - Optional: Allow letter keys if use case exists

3. **Conflict Detection**: Warn (not error) if number key has:
   - Both `def modifier` and `mod modX` definition
   - (This is allowed but may confuse users)

4. **Error Handling**: Invalid syntax → clear error message:
   ```
   ERROR: Line 42: Invalid modifier target '*Delete'
   Valid targets: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin
   ```

### Integration with EventProcessor Layer 2

#### Current Layer 2 Implementation

```cpp
uint16_t EventProcessor::layer2_applySubstitution(uint16_t yamy_in) {
    auto it = substitutions_.find(yamy_in);
    if (it != substitutions_.end()) {
        LOG_INFO("[LAYER2:SUBST] 0x%04X → 0x%04X", yamy_in, it->second);
        return it->second;
    } else {
        LOG_INFO("[LAYER2:PASSTHROUGH] 0x%04X (no substitution)", yamy_in);
        return yamy_in;
    }
}
```

#### Enhanced Layer 2 with Number Modifier Check

```cpp
struct ProcessedEvent {
    uint16_t output_evdev;
    uint16_t output_yamy;  // NEW: needed for modifier handler
    EventType type;
    bool valid;
};

ProcessedEvent EventProcessor::processEvent(uint16_t input_evdev, EventType type) {
    // Layer 1: evdev → yamy
    uint16_t yamy_in = layer1_evdevToYamy(input_evdev, type);
    if (yamy_in == 0) {
        return {0, 0, type, false};  // Unmapped
    }

    // Layer 2: Check number modifier FIRST
    uint16_t yamy_out = yamy_in;

    if (m_modifier_handler && m_modifier_handler->isNumberModifier(yamy_in)) {
        auto result = m_modifier_handler->processNumberKey(yamy_in, type);

        switch (result.action) {
            case ProcessingAction::ACTIVATE_MODIFIER:
                // Hold detected - output hardware modifier PRESS
                yamy_out = result.output_yamy_code;
                LOG_INFO("[LAYER2:MODIFIER] Hold detected: 0x%04X → modifier 0x%04X",
                         yamy_in, yamy_out);
                break;

            case ProcessingAction::DEACTIVATE_MODIFIER:
                // Hold detected - output hardware modifier RELEASE
                yamy_out = result.output_yamy_code;
                LOG_INFO("[LAYER2:MODIFIER] Release modifier: 0x%04X → 0x%04X",
                         yamy_in, yamy_out);
                break;

            case ProcessingAction::APPLY_SUBSTITUTION_PRESS:
            case ProcessingAction::APPLY_SUBSTITUTION_RELEASE:
                // Tap detected - fall through to normal substitution
                LOG_INFO("[LAYER2:MODIFIER] Tap detected, applying substitution");
                yamy_out = layer2_applySubstitution(yamy_in);
                break;

            case ProcessingAction::WAITING_FOR_THRESHOLD:
                // Still waiting - don't output anything yet
                LOG_INFO("[LAYER2:MODIFIER] Waiting for threshold");
                return {0, 0, type, false};  // Invalid, don't output

            case ProcessingAction::NOT_A_NUMBER_MODIFIER:
                // Shouldn't reach here (isNumberModifier check above)
                yamy_out = layer2_applySubstitution(yamy_in);
                break;
        }
    } else {
        // Not a number modifier - normal substitution
        yamy_out = layer2_applySubstitution(yamy_in);
    }

    // Layer 3: yamy → evdev
    uint16_t output_evdev = layer3_yamyToEvdev(yamy_out);

    return {output_evdev, yamy_out, type, output_evdev != 0};
}
```

**Key Points**:
1. Number modifier check happens **before** substitution lookup
2. `WAITING_FOR_THRESHOLD` returns invalid event (no output yet)
3. `ACTIVATE/DEACTIVATE` output hardware modifier scancodes
4. `APPLY_SUBSTITUTION` falls through to normal substitution logic
5. Maintains single-path processing (no key-specific branches outside handler)

### Error Handling and Edge Cases

#### Edge Case 1: Rapid Tap Before Threshold

```
Time: 0ms    | PRESS _1
Time: 50ms   | RELEASE _1 (before 200ms)
Time: 100ms  | PRESS _1 again
Time: 150ms  | RELEASE _1 again
```

**Handling**: Each press-release pair is independent. State resets to IDLE after each TAP_DETECTED.

#### Edge Case 2: Hold Then Another Number Key

```
Time: 0ms    | PRESS _1 (number 1)
Time: 250ms  | HOLD detected → activate LShift
Time: 300ms  | PRESS _2 (number 2)
             | _2 has def modifier *_2 = *RShift
```

**Handling**: Both modifiers can be active simultaneously (LShift + RShift). Each number key has independent state.

#### Edge Case 3: RELEASE Without PRESS

```
Time: 0ms    | RELEASE _1 (spurious release, no prior press)
```

**Handling**: `processNumberKey()` checks state:
- If state = IDLE → log warning, return NOT_A_NUMBER_MODIFIER
- Graceful degradation, no crash

#### Edge Case 4: System Suspend/Resume

```
Time: 0ms    | PRESS _1
Time: 100ms  | System suspends (laptop lid close)
Time: 10000ms | System resumes
Time: 10100ms | RELEASE _1
```

**Handling**: Elapsed time is huge (> 200ms), so HOLD is detected. However:
- Add **maximum threshold** (e.g., 5 seconds)
- If elapsed > 5 seconds, treat as IDLE (reset state)
- Prevents stuck modifier state

#### Edge Case 5: Parser Encounters Invalid Modifier

```mayu
def modifier *_1 = *Delete   # ERROR: Delete is not a hardware modifier
```

**Handling**: Parser validation fails:
- Error message: "Invalid modifier target '*Delete'. Valid: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin"
- Abort parsing with clear line number
- Do NOT silently ignore (fail-fast)

### Performance Considerations

#### Latency Requirements

From design.md: **< 1ms per event processing**

**Overhead Analysis**:
- `isNumberModifier()`: O(1) hash map lookup → < 10µs
- `processNumberKey()`: State check + time diff → < 50µs
- Total overhead per event: **< 100µs** (0.1ms)
- Well within 1ms budget

#### Memory Usage

**Per-Key State Size**:
```cpp
struct KeyState {
    NumberKeyState state;                        // 1 byte
    std::chrono::steady_clock::time_point press_time;  // 8 bytes
    HardwareModifier target_modifier;            // 1 byte
    // Padding: 6 bytes
};  // Total: 16 bytes
```

**Maximum Memory**:
- 10 number keys × 16 bytes = **160 bytes**
- Negligible impact

#### Threading Considerations

**Single-Threaded Design**:
- EventProcessor runs in **main event loop thread**
- ModifierKeyHandler owned by EventProcessor (not shared)
- No mutex needed (thread-safe by design)

**No Timer Thread**:
- Threshold check is **passive** (checks time on next event)
- No active timer callbacks or separate threads
- Simpler implementation, no race conditions

### Testing Strategy

#### Unit Tests

**File**: `tests/test_number_modifiers_ut.cpp`

```cpp
TEST(ModifierKeyHandler, RegisterNumberModifier) {
    ModifierKeyHandler handler;
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);  // _1 → LShift
    EXPECT_TRUE(handler.isNumberModifier(0x0002));
    EXPECT_FALSE(handler.isNumberModifier(0x0003));
}

TEST(ModifierKeyHandler, TapDetection) {
    ModifierKeyHandler handler(200);  // 200ms threshold
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    // PRESS at T=0
    auto result1 = handler.processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result1.action, ProcessingAction::WAITING_FOR_THRESHOLD);

    // RELEASE at T=50ms (before threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto result2 = handler.processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result2.action, ProcessingAction::APPLY_SUBSTITUTION_RELEASE);
}

TEST(ModifierKeyHandler, HoldDetection) {
    ModifierKeyHandler handler(200);
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    // PRESS at T=0
    auto result1 = handler.processNumberKey(0x0002, EventType::PRESS);
    EXPECT_EQ(result1.action, ProcessingAction::WAITING_FOR_THRESHOLD);

    // Wait for threshold
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Next event triggers threshold check
    auto result2 = handler.processNumberKey(0x0002, EventType::PRESS);  // Check state
    EXPECT_EQ(result2.action, ProcessingAction::ACTIVATE_MODIFIER);
    EXPECT_EQ(result2.output_yamy_code, /* VK_LSHIFT scancode */);

    // RELEASE
    auto result3 = handler.processNumberKey(0x0002, EventType::RELEASE);
    EXPECT_EQ(result3.action, ProcessingAction::DEACTIVATE_MODIFIER);
}

TEST(ModifierKeyHandler, MultipleSimultaneous) {
    ModifierKeyHandler handler;
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);  // _1
    handler.registerNumberModifier(0x0003, HardwareModifier::LCTRL);   // _2

    // Press both
    handler.processNumberKey(0x0002, EventType::PRESS);
    handler.processNumberKey(0x0003, EventType::PRESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Both should be active
    EXPECT_TRUE(handler.isModifierHeld(0x0002));
    EXPECT_TRUE(handler.isModifierHeld(0x0003));
}
```

#### Integration Tests

**File**: `tests/test_number_modifiers_it.cpp`

```cpp
TEST(NumberModifierIntegration, TapAppliesSubstitution) {
    // Setup: _1 has both substitution and modifier
    SubstitutionTable table = {{0x0002, VK_LSHIFT}};  // _1 → LShift subst
    EventProcessor processor(table);
    processor.setModifierHandler(handler_with_1_as_LSHIFT);

    // Simulate tap
    auto press = processor.processEvent(KEY_1, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto release = processor.processEvent(KEY_1, EventType::RELEASE);

    // Verify substitution applied (not modifier)
    EXPECT_EQ(press.output_evdev, KEY_LEFTSHIFT);
    EXPECT_EQ(release.output_evdev, KEY_LEFTSHIFT);
}

TEST(NumberModifierIntegration, HoldActivatesModifier) {
    EventProcessor processor(table);
    processor.setModifierHandler(handler_with_1_as_LSHIFT);

    // Simulate hold
    processor.processEvent(KEY_1, EventType::PRESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Press another key to trigger threshold check
    auto combo = processor.processEvent(KEY_A, EventType::PRESS);

    // Verify modifier was activated
    // (Modifier state should be tracked separately)
}
```

#### End-to-End Tests

**File**: `tests/test_number_modifiers_e2e.py`

```python
def test_number_modifier_hold_system_wide():
    """E2E: Verify number modifier works in other applications"""
    test = AutomatedKeymapTest()

    # Configure _1 as LShift modifier
    # (Requires .mayu with: def modifier *_1 = *LShift)

    # Hold number 1 for 300ms
    test.inject_key_hold(evdev=KEY_1, duration_ms=300)

    # While held, press A
    test.inject_key_tap(evdev=KEY_A)

    # Verify system received Shift+A
    # (Check via log or external observer)
    assert test.verify_modifier_combo(modifier=MOD_LSHIFT, key=KEY_A)

def test_tap_vs_hold_threshold():
    """E2E: Test boundary at 200ms threshold"""
    test = AutomatedKeymapTest()

    # Tap at 190ms (just before threshold)
    test.inject_key_hold(evdev=KEY_1, duration_ms=190)
    assert test.verify_substitution_output(expected=KEY_LEFTSHIFT)

    # Hold at 210ms (just after threshold)
    test.inject_key_hold(evdev=KEY_1, duration_ms=210)
    assert test.verify_modifier_active(modifier=MOD_LSHIFT)
```

### Comparison with Alternatives

#### Alternative 1: Use Existing Modal Layers Only

**Approach**: Rely on `mod modX = !!_Y` for number key modifiers

**Pros**:
- No code changes needed
- Works today

**Cons**:
- Modal layers are **engine-level**, not **hardware-level**
- Can't use in other applications
- Limited to YAMY's modal system

**Verdict**: Insufficient for use case (users need system-wide modifiers)

#### Alternative 2: External Tool (xmodmap/xkb on Linux)

**Approach**: Use Linux xmodmap to remap number keys to modifiers

**Pros**:
- System-wide solution
- No YAMY changes needed

**Cons**:
- **Can't have both substitution and modifier** (no hold-vs-tap)
- Requires user to configure outside YAMY
- Loses YAMY's power (substitutions)

**Verdict**: Incompatible with requirement (need tap = substitution, hold = modifier)

#### Alternative 3: Timer-Based Callback

**Approach**: Start actual timer thread on PRESS, callback at 200ms

**Pros**:
- "True" 200ms timing (callback fires exactly at threshold)

**Cons**:
- **Threading complexity** (mutexes, race conditions)
- **Higher overhead** (thread creation, callbacks)
- **More error-prone** (deadlocks, resource leaks)

**Verdict**: Unnecessary complexity, passive checking on next event is simpler and sufficient

#### Chosen Approach: Passive Timestamp-Based Detection

**Why**:
1. **Simplicity**: No timer threads, no mutexes
2. **Performance**: Timestamp diff is nanoseconds, negligible overhead
3. **Correctness**: Deterministic behavior, no race conditions
4. **Sufficient Accuracy**: 200ms threshold doesn't need nanosecond precision

**Trade-off**: Modifier activation happens on **next event** after threshold, not exactly at 200ms. Acceptable because:
- User is still holding key (no release yet)
- Modifier activates before they press next key
- Difference is imperceptible (< 10ms delay)

## Implementation Phases

### Phase 4.1: Design (Current Task)
- ✅ Create comprehensive design document
- ✅ Define state machine, interfaces, data flow
- ✅ Specify .mayu syntax extension

### Phase 4.2: Implement ModifierKeyHandler Class
- Implement `modifier_key_handler.h/cpp`
- Implement state machine logic
- Implement hold-vs-tap detection
- Unit tests for handler

### Phase 4.3: Create Modifier Mapping Table
- Add number-to-modifier map in keycode_mapping.cpp
- Implement lookup function
- Default mappings (1→LShift, 2→RShift, etc.)

### Phase 4.4: Integrate into EventProcessor Layer 2
- Modify `processEvent()` to check number modifiers
- Wire handler results to substitution/modifier output
- Add logging for modifier activation/deactivation

### Phase 4.5: Extend .mayu Parser
- Add `def modifier` syntax parsing
- Validate modifier targets
- Register mappings with ModifierKeyHandler

### Phase 4.6: Testing
- Unit tests (hold/tap detection, state machine)
- Integration tests (EventProcessor + handler)
- E2E tests (real timing, system-wide verification)

### Phase 4.7: Documentation
- User guide for `def modifier` syntax
- Examples for small keyboard users
- Troubleshooting (threshold tuning, conflicts)

### Phase 4.8: Validation
- Test all 10 number keys as modifiers
- Verify backward compatibility (existing substitutions)
- Performance profiling (< 1ms requirement)

## Success Criteria

1. ✅ All 10 number keys (0-9) can be configured as hardware modifiers
2. ✅ Hold (≥200ms) activates modifier, tap (<200ms) applies substitution
3. ✅ Modifiers work system-wide (not just in YAMY)
4. ✅ Backward compatibility: existing number key substitutions unchanged
5. ✅ Performance: < 100µs overhead per event
6. ✅ No threading complexity (single-threaded, passive detection)
7. ✅ Clear .mayu syntax (`def modifier *_X = *ModifierName`)
8. ✅ Comprehensive tests (unit, integration, E2E)

## Open Questions and Future Extensions

### Configurable Threshold

**Question**: Should hold threshold be configurable per-key or global?

**Proposal**: Global threshold in settings, advanced users can tune if needed:
```mayu
set NumberModifierThresholdMs = 150  # Default: 200
```

### Non-Number Keys as Modifiers

**Question**: Should feature support letter keys as modifiers (e.g., A as LShift)?

**Analysis**:
- Design supports any key (not limited to numbers)
- Risk: More likely to conflict with typing
- Recommendation: Start with numbers only, extend if user requests

### Combination with Modal Layers

**Question**: How do `def modifier` and `mod modX = !!_Y` interact?

**Current Proposal**: Both activate (hardware modifier + modal layer)

**Alternative**: Make them mutually exclusive (parser error if both defined)

**Recommendation**: Allow both initially, add exclusion if users report confusion

## Conclusion

This design provides a **clean, performant, and backward-compatible** solution for using number keys as hardware modifiers. Key innovations:

1. **Passive threshold detection** (no timer threads)
2. **Single-path integration** with EventProcessor Layer 2
3. **Clear .mayu syntax** (`def modifier`) separate from substitutions
4. **Hold-vs-tap state machine** with deterministic behavior

The design maintains all YAMY architectural principles:
- Zero special cases (generalized modifier handling)
- Layer purity (handler is just another table lookup conceptually)
- Event type consistency (PRESS/RELEASE preserved)
- Comprehensive logging and testing

Ready for implementation in Phase 4.2+.
