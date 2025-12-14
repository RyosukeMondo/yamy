# Requirements Document: Modal Modifier Remapping

## Introduction

Modal modifier remapping is a core feature that enables advanced keyboard customization through hold-vs-tap key detection and modal layer activation. This feature allows users to transform any key into a dual-purpose key (tap for normal function, hold for modifier activation) and use modal modifiers (mod0-mod19) in key combinations.

**Purpose**: Enable power users to maximize keyboard efficiency by eliminating the need for dedicated modifier keys, allowing any key to act as both a regular key and a modifier layer trigger.

**Value**: This feature is critical for:
- **Cross-platform developers**: Maintain identical keyboard layouts on Windows and Linux
- **Emacs/Vim users**: System-wide modal editing with custom prefix keys
- **Keyboard enthusiasts**: Full QMK-like functionality without custom hardware

## Alignment with Product Vision

This feature directly supports the YAMY Product Vision (product.md):

1. **Zero Compromise Cross-Platform** (Principle 1)
   - Implements 100% feature parity with Windows modal modifier system
   - Identical .mayu file syntax works on both platforms
   - Same keyboard behavior on Windows and Linux

2. **Performance First** (Principle 2)
   - Sub-millisecond latency for modifier detection
   - Efficient hold/tap threshold detection (default 200ms)
   - No frame drops during modifier state transitions

3. **Power User Focused** (Principle 3)
   - Full Turing-complete modal layer programming
   - 20 modal layers (mod0-mod19) for complex workflows
   - Supports chording with standard modifiers (Ctrl+Mod9+X)

4. **Respectful of Legacy** (Principle 4)
   - Backward compatible with all existing .mayu files
   - Preserves Windows modal modifier semantics
   - No breaking changes to configuration syntax

**Success Metric Alignment**:
- Performance: <1ms input latency (99th percentile) ✅
- Quality: <5 P0 bugs ✅
- Adoption: Critical blocker for Linux feature parity ✅

## Requirements

### Requirement 1: Modal Modifier Definition and Parsing

**User Story**: As a power user, I want to define modal modifiers in my .mayu file using `mod modX = !!key` syntax, so that holding a key activates a modal layer without requiring dedicated modifier keys.

#### Acceptance Criteria (EARS Format)

**Event**: WHEN user defines `mod mod9 = !!A` in .mayu configuration
**Action**: System SHALL parse the modal modifier definition and register key A as a trigger for the Modifier::Type_Mod9 state
**Result**: Configuration loads successfully with modal modifier registered within 100ms, no parsing errors

**Baseline → Target**:
- **Current**: Modal modifier definitions are parsed but NOT connected to hold detection system (0% functional)
- **Target**: 100% of modal modifier definitions activate hold detection with correct modifier type mapping

**Edge Cases**:
1. WHEN user defines `mod mod9 = !!A` AND `def subst *A = *Tab` THEN system SHALL prioritize hold detection (hold → mod9, tap → Tab substitution)
2. WHEN user defines invalid modifier number `mod mod25 = !!A` (>mod19) THEN system SHALL reject with clear error message
3. WHEN user defines `mod mod9 = !!A` AND `mod mod9 = !!B` THEN system SHALL accept multiple triggers for same modifier
4. IF key is already a hardware modifier (`mod mod9 = !!LShift`) THEN system SHALL reject with warning (hardware modifiers cannot be modal triggers)

### Requirement 2: Number Modifier Definition and Registration

**User Story**: As an advanced user, I want to define number modifiers using `def numbermod *_1 = *LShift` syntax, so that number keys can function as hardware modifiers when held.

#### Acceptance Criteria (EARS Format)

**Event**: WHEN user defines `def numbermod *_1 = *LShift` in .mayu configuration
**Action**: System SHALL register the mapping and enable hold/tap detection for the _1 key
**Result**: Number key behaves as LShift when held ≥200ms, normal key when tapped <200ms, within 1ms processing latency

**Baseline → Target**:
- **Current**: `def numbermod` syntax is parsed but number modifiers are NOT registered with EventProcessor (0% functional)
- **Target**: 100% of number modifier definitions are activated with correct hardware modifier mapping

**Edge Cases**:
1. WHEN user holds _1 for 250ms THEN system SHALL generate VK_LSHIFT down event AND suppress the _1 key event
2. WHEN user taps _1 for 50ms THEN system SHALL apply normal substitution (`*_1 = *LShift` becomes actual LShift key output)
3. WHEN user defines `def numbermod *A = *LCtrl` (non-number key) THEN system SHALL accept (number modifier naming is historical, any key allowed)
4. IF user defines multiple hardware modifiers for same key THEN system SHALL reject with error

### Requirement 3: Hold vs Tap Detection State Machine

**User Story**: As a developer, I want a robust state machine that detects hold vs tap gestures with configurable thresholds, so that modal modifiers activate reliably without false positives.

#### Acceptance Criteria (EARS Format)

**Event**: WHEN user presses a key registered as modal modifier or number modifier
**Action**: System SHALL start timer and track state transition: IDLE → PRESS → WAITING → (TAP_DETECTED or MODIFIER_ACTIVE)
**Result**: Modifier state is correctly determined within threshold time (default 200ms), with <1ms jitter

**Baseline → Target**:
- **Current**: ModifierKeyHandler class exists with full state machine BUT is never instantiated or used (50% complete)
- **Target**: 100% of modal/number modifier keys use state machine with accurate hold/tap detection

**State Machine Specification**:
```
IDLE: No key pressed
  ↓ (Key Down)
PRESS: Key just pressed, timer started
  ↓ (Wait)
WAITING: Timer running (0-200ms elapsed)
  ↓ (Key Up before threshold)
TAP_DETECTED: Release before 200ms → Apply substitution
  ↓ (Threshold reached while held)
MODIFIER_ACTIVE: Hold ≥200ms → Activate modifier
  ↓ (Key Up)
IDLE: Modifier deactivated
```

**Performance Requirements**:
1. WHEN threshold is set to 200ms THEN detection accuracy SHALL be ±5ms (±2.5% error)
2. IF system is under heavy load (1000 events/sec) THEN hold detection SHALL NOT delay by >10ms
3. WHEN modifier is activated THEN latency from threshold to modifier down event SHALL be <1ms

**Edge Cases**:
1. WHEN user performs rapid tap-tap-hold sequence THEN each event SHALL be independently evaluated
2. IF timer is running AND system suspends (laptop sleep) THEN timer SHALL reset on wake
3. WHEN two modal modifier keys are held simultaneously THEN both modifiers SHALL be active concurrently

### Requirement 4: Modifier State Tracking

**User Story**: As the engine, I need to track the active state of all 20 modal modifiers (mod0-mod19) during runtime, so that modifier key combinations can be correctly matched against keymap definitions.

#### Acceptance Criteria (EARS Format)

**Event**: WHEN modal modifier key is held ≥200ms OR released
**Action**: System SHALL update modifier state bitmask and propagate state change to keymap lookup system
**Result**: Active modifiers are reflected in real-time for key combination matching with <0.5ms latency

**Baseline → Target**:
- **Current**: Modifier state is tracked for standard modifiers (Shift, Ctrl, Alt, Win) but NOT for modal modifiers mod0-mod19 (0% coverage)
- **Target**: 100% of modal modifiers are tracked with same reliability as hardware modifiers

**State Storage**:
```cpp
// Required data structure (example)
struct ModifierState {
    uint32_t standard_modifiers;  // Shift(1), Ctrl(2), Alt(4), Win(8)
    uint32_t modal_modifiers;     // mod0(1), mod1(2), ..., mod19(1<<19)

    bool isActive(Modifier::Type type) const;
    void activate(Modifier::Type type);
    void deactivate(Modifier::Type type);
};
```

**Edge Cases**:
1. WHEN all 20 modal modifiers are active simultaneously THEN system SHALL track all without overflow
2. IF focus changes while modifier is held THEN modifier state SHALL be preserved or cleared based on configuration
3. WHEN system receives modifier up event without preceding down THEN state SHALL be force-cleared (recovery)

### Requirement 5: Modifier Combination Keymap Lookup

**User Story**: As a user, I want to define key combinations using modal modifiers like `key *W-*A-*S-*C-m9-*X = *W-*A-*S-*C-Y`, so that modal layers can be combined with standard modifiers.

#### Acceptance Criteria (EARS Format)

**Event**: WHEN user presses X key WHILE mod9 is active AND standard modifiers Ctrl+Alt+Shift are pressed
**Action**: System SHALL lookup keymap for the complete modifier combination `*W-*A-*S-*C-m9-*X`
**Result**: Correct output Y is generated within <1ms, matching the defined keymap entry

**Baseline → Target**:
- **Current**: Keymap definitions with modal modifiers are parsed and stored BUT never matched during lookup (0% functional)
- **Target**: 100% of modal modifier combinations are correctly matched with same performance as standard combinations

**Keymap Matching Priority** (highest to lowest):
1. Exact match with all modifiers: `*W-*A-*S-*C-m9-*X`
2. Match without modal: `*W-*A-*S-*C-*X` (fallback)
3. Match with fewer modifiers: `*C-m9-*X`
4. Default (no modifiers): `*X`

**Edge Cases**:
1. WHEN keymap has `m9-*X = Y` AND `m8-m9-*X = Z` THEN with both mod8+mod9 active, system SHALL match the more specific entry (Z)
2. IF user holds mod9 but no keymap entries use mod9 THEN normal keymaps SHALL still match (mod9 ignored if not referenced)
3. WHEN user defines duplicate entries `m9-*X = A` and `m9-*X = B` THEN last definition SHALL win (config file order)

### Requirement 6: Integration with Event Processing Pipeline

**User Story**: As the engine, I need modal modifier detection to integrate seamlessly into the existing 3-layer event processing pipeline, so that modifier remapping works alongside key substitution.

#### Acceptance Criteria (EARS Format)

**Event**: WHEN evdev input event is received for a key registered as modal/number modifier
**Action**: System SHALL process event through: Layer 1 (evdev→YAMY) → Layer 2 (hold detection + substitution + modifier state) → Layer 3 (YAMY→evdev)
**Result**: Correct output is generated with total pipeline latency <1ms (P99)

**Baseline → Target**:
- **Current**: Event pipeline exists but skips modifier detection (direct substitution lookup)
- **Target**: 100% of modal/number modifier events go through full detection pipeline

**Pipeline Integration Points**:
```
INPUT: evdev code (e.g., KEY_A = 30)
   ↓
[Layer 1] evdev → YAMY scan code
   Result: 0x001E (KEY_A in YAMY)
   ↓
[Layer 2] Apply modifier detection + substitution
   CRITICAL ORDER:
   1. Check if key is modal/number modifier → ModifierKeyHandler::processNumberKey()
   2. If WAITING_FOR_THRESHOLD → suppress event (return 0)
   3. If TAP_DETECTED → proceed to substitution lookup
   4. If MODIFIER_ACTIVE → update modifier state + return hardware VK code
   5. Else → normal substitution lookup
   ↓
[Layer 3] YAMY scan code → evdev code
   Result: Output evdev code
   ↓
OUTPUT: uinput injection
```

**Performance Requirements**:
1. WHEN processing modal modifier event THEN Layer 2 overhead SHALL be <10μs
2. IF hold detection suppresses event THEN no output SHALL be generated (zero injection cost)
3. WHEN tap is detected THEN substitution lookup SHALL execute in <10μs

**Edge Cases**:
1. WHEN modifier is activated mid-sequence THEN in-flight key events SHALL complete with previous modifier state
2. IF event processing encounters error THEN system SHALL fall back to passthrough (no remapping)
3. WHEN multiple keyboards are active (Linux multi-device) THEN each device SHALL have independent modifier state

## Non-Functional Requirements

### Code Architecture and Modularity

**Single Responsibility Principle**:
- `modifier_key_handler.cpp`: Hold/tap detection logic ONLY (no substitution, no injection)
- `engine_event_processor.cpp`: Event pipeline orchestration ONLY (delegates to handlers)
- `engine_modifier.cpp`: Modifier state tracking ONLY (no keymap lookup)
- `engine_setting.cpp`: Configuration → runtime data transformation ONLY (no event processing)

**Modular Design**:
- ModifierKeyHandler is reusable (unit testable in isolation)
- Event processor is configurable (can be instantiated without Engine)
- Modifier state is observable (can be queried/logged independently)

**Dependency Management**:
```
engine_event_processor.cpp
  ↓ depends on
modifier_key_handler.h (interface only)
  ↓ implements
modifier_key_handler.cpp (zero external deps)
```

**Clear Interfaces**:
```cpp
// ModifierKeyHandler interface (example)
class ModifierKeyHandler {
public:
    struct NumberKeyResult {
        ProcessingAction action;  // ACTIVATE, DEACTIVATE, APPLY_SUBSTITUTION, WAITING
        uint16_t output_yama_code;
    };

    // Register a key as number modifier
    void registerNumberModifier(uint16_t numberKey, uint16_t modifierKey);

    // Process key event (returns action to take)
    NumberKeyResult processNumberKey(uint16_t yama_code, EventType type);

    // Query methods
    bool isNumberModifier(uint16_t yama_code) const;
    bool isWaitingForThreshold(uint16_t yama_code) const;
};
```

### Performance

**Latency Requirements** (P99):
- **Hold detection overhead**: <10μs per event
- **Modifier state update**: <5μs
- **Keymap lookup with modifiers**: <15μs
- **Total pipeline (evdev in → uinput out)**: <1ms

**Throughput Requirements**:
- Support 1000 events/sec sustained input without queue buildup
- Handle burst of 100 events in 10ms window without dropped events

**Resource Usage**:
- **Memory**: Modifier state tracking <1KB, handler state <10KB
- **CPU**: Idle <0.1%, under load <2% (single core)

**Measurement Method**:
```cpp
// Built-in timing instrumentation (example)
auto start = std::chrono::high_resolution_clock::now();
result = modifierHandler->processNumberKey(yama_code, type);
auto end = std::chrono::high_resolution_clock::now();
auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
// Log if >100μs
```

### Security

**Input Validation**:
1. WHEN config defines modal modifier THEN validate modifier index is 0-19
2. WHEN registering number modifier THEN validate target is valid hardware modifier VK code
3. IF invalid configuration is detected THEN reject with error, do not partially apply

**Resource Limits**:
1. Maximum 20 modal modifiers (mod0-mod19) - enforced by parser
2. Maximum 100 number modifier definitions - enforced by parser
3. Hold threshold range: 50ms-5000ms - validated at config load

**Safety Guarantees**:
1. WHEN modifier detection fails THEN fall back to passthrough (no key loss)
2. IF timer system is broken THEN disable hold detection, log error, continue with tap-only mode
3. WHEN state becomes inconsistent THEN reset all modifier state on next keymap change

### Reliability

**Correctness** (CRITICAL):
- **Baseline**: 0% of modal modifier configurations work (feature non-functional)
- **Target**: 100% of valid modal modifier definitions work correctly (zero false positives/negatives)

**Error Handling**:
1. WHEN hold detection encounters OS timer error THEN log error, disable hold detection for that key, continue processing
2. IF modifier state overflow occurs (>32 modifiers) THEN log critical error, reset state, continue
3. WHEN config reload happens while modifiers are active THEN preserve active state if config still valid, else reset

**Recovery**:
- WHEN system detects stuck modifier (held >60s) THEN auto-release with warning
- IF event stream is interrupted (suspend/resume) THEN clear all modifier state on resume

### Usability

**Configuration Syntax Clarity**:
```mayu
# Clear, self-documenting syntax
mod mod9 = !!A              # Hold A → activates mod9 layer
def numbermod *_1 = *LShift # Hold 1 → acts as LShift

# Modifier combinations
key m9-*X = Y               # While mod9 active: X outputs Y
key *C-m9-*X = Z            # While Ctrl+mod9 active: X outputs Z
```

**Error Messages**:
```
Error: Invalid modal modifier index 'mod25' (valid range: mod0-mod19)
  Line 42: mod mod25 = !!A
           ^^^^^^^^^

Error: Cannot use hardware modifier as modal trigger
  Line 15: mod mod9 = !!LShift
                      ^^^^^^^^
  Suggestion: Use a regular key like !!A or !!Space
```

**Logging** (Strategic, not verbose):
```
[ENGINE:INIT] Registered 5 modal modifiers, 3 number modifiers
[MODIFIER:ACTIVATE] mod9 activated (key: A, threshold: 200ms, hold time: 215ms)
[MODIFIER:TAP] _1 tapped (hold time: 45ms) → applying substitution
[KEYMAP:MATCH] Matched: *C-m9-*X → Z (4 modifiers active)
```

## Testing Requirements

### Unit Testing

**Coverage Target**: >95% line coverage for modifier detection code

**Test Distribution**:
- **ModifierKeyHandler** (25 tests):
  - Hold detection state machine (10 tests)
  - Multiple modifiers independently (5 tests)
  - Edge cases (tap→hold→tap rapid sequence) (5 tests)
  - Timer accuracy (threshold ±5ms) (5 tests)

- **Modifier State Tracking** (15 tests):
  - Activate/deactivate single modifier (5 tests)
  - Multiple modifiers concurrent (5 tests)
  - Bitmask overflow protection (3 tests)
  - State query methods (2 tests)

- **Keymap Lookup with Modifiers** (20 tests):
  - Standard + modal modifier combinations (10 tests)
  - Priority/fallback logic (5 tests)
  - Edge cases (duplicate entries, undefined modifiers) (5 tests)

**Example Test**:
```cpp
TEST(ModifierKeyHandlerTest, TapDetectedWhenReleasedBefore200ms) {
    ModifierKeyHandler handler;
    handler.registerNumberModifier(0x001E, 0x002A);  // A → LShift

    // Simulate key down
    auto result1 = handler.processNumberKey(0x001E, EventType::DOWN);
    EXPECT_EQ(ProcessingAction::WAITING_FOR_THRESHOLD, result1.action);

    // Wait 50ms (below threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Simulate key up
    auto result2 = handler.processNumberKey(0x001E, EventType::UP);
    EXPECT_EQ(ProcessingAction::APPLY_SUBSTITUTION_ON_DOWN, result2.action);
}
```

### Integration Testing

**Test Harness**:
- Mock evdev device using `/dev/uinput` (Linux)
- Inject test events, capture output
- Validate timing using `clock_gettime(CLOCK_MONOTONIC)`

**Critical Scenarios** (30 tests):
1. **Hold Detection** (10 tests):
   - Single key hold →activate →release →deactivate
   - Tap vs hold threshold accuracy
   - Multiple keys in rapid succession

2. **Modifier Combinations** (10 tests):
   - Mod9+X outputs Y (simple modal)
   - Ctrl+Mod9+X outputs Z (standard+modal)
   - Mod8+Mod9+X outputs W (multiple modal)

3. **Real-World Workflows** (10 tests):
   - Emacs-style C-x prefix (Mod9 = X, Mod9+F = find-file)
   - Vim-style modal editing (Mod9 = Esc, Mod9+hjkl = arrow keys)
   - Number row as modifiers (1-0 held = F1-F10)

**Integration Test Example**:
```cpp
TEST(IntegrationTest, ModalModifierActivatesOnHold) {
    // Load config: mod mod9 = !!A, key m9-*X = Y
    Engine engine;
    engine.loadConfig("test_modal.mayu");

    // Inject: A down, wait 250ms, X down, X up, A up
    MockInputDevice device;
    device.sendKeyDown(KEY_A);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    device.sendKeyDown(KEY_X);
    device.sendKeyUp(KEY_X);
    device.sendKeyUp(KEY_A);

    // Verify output
    auto events = device.readOutputEvents();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ(KEY_Y, events[0].code);  // X remapped to Y via mod9
}
```

### End-to-End Testing

**User Acceptance Tests** (15 scenarios):

1. **UAT-1: Basic Modal Modifier**
   - Config: `mod mod9 = !!A`, `key m9-*X = Y`
   - Steps: Hold A for 300ms, press X, release X, release A
   - Expected: Y key output

2. **UAT-2: Tap vs Hold**
   - Config: `mod mod9 = !!A`, `def subst *A = *Tab`
   - Steps: Tap A (50ms), then hold A (300ms)
   - Expected: First outputs Tab, second activates mod9 (no output)

3. **UAT-3: Number Modifier**
   - Config: `def numbermod *_1 = *LShift`, `key *S-A = *S-A` (no remap)
   - Steps: Hold 1 for 300ms, press A, release A, release 1
   - Expected: Capital A (Shift+A output)

4. **UAT-4: Multi-Modal Combination**
   - Config: `mod mod9 = !!A`, `mod mod8 = !!S`, `key m8-m9-*X = Z`
   - Steps: Hold A, hold S, press X, release all
   - Expected: Z output

5. **UAT-5: Cross-Platform Config**
   - Config: Load same .mayu file on Windows and Linux
   - Steps: Perform UAT-1 through UAT-4 on both platforms
   - Expected: Identical behavior on both systems

**Performance Benchmarks** (Automated):
```bash
# Measure latency P50, P99, P99.9
./yamy --benchmark --config test_modal.mayu --iterations 10000
```

**Expected Results**:
- P50 latency: <500μs
- P99 latency: <1ms
- P99.9 latency: <5ms

## Acceptance Criteria Summary (EARS Format)

### AC-1: Configuration Parsing
**Event**: WHEN user loads .mayu file with modal modifier definitions
**Action**: System parses and validates all `mod modX = !!key` and `def numbermod` entries
**Result**: Configuration loads successfully with all modifiers registered within 100ms, zero parsing errors for valid syntax

### AC-2: Hold Detection Accuracy
**Event**: WHEN user holds a modal/number modifier key
**Action**: System measures hold time and activates modifier if ≥200ms threshold
**Result**: Detection accuracy within ±5ms (±2.5%), no false activations/deactivations

### AC-3: Tap Fallback
**Event**: WHEN user taps a modal/number modifier key (<200ms)
**Action**: System applies normal substitution from `def subst` mapping
**Result**: Substitution executes within <1ms, correct output generated

### AC-4: Modifier State Synchronization
**Event**: WHEN modal modifier activates/deactivates
**Action**: System updates modifier state bitmask and propagates to keymap lookup
**Result**: Active modifiers reflected in real-time with <0.5ms latency

### AC-5: Keymap Matching
**Event**: WHEN user presses key while modal modifiers are active
**Action**: System performs keymap lookup including modal modifier prefix (e.g., `m9-`)
**Result**: Correct keymap entry matched with priority ordering, output generated within <1ms

### AC-6: Cross-Platform Parity
**Event**: WHEN same .mayu configuration is loaded on Windows and Linux
**Action**: Both systems apply identical modal modifier behavior
**Result**: 100% functional parity, zero platform-specific quirks or bugs

### AC-7: Performance Under Load
**Event**: WHEN system processes 1000 events/sec with modal modifiers active
**Action**: System maintains hold detection, modifier tracking, and keymap lookup
**Result**: P99 latency remains <1ms, zero dropped events, CPU usage <2%

## Code Quality Gates

**Max Lines Per File**: 500
- `modifier_key_handler.cpp`: Target <300 lines
- `engine_event_processor.cpp`: Target <400 lines (including existing code)
- `engine_modifier.cpp`: Target <200 lines

**Max Lines Per Function**: 50
- `processNumberKey()`: <40 lines
- `updateModifierState()`: <30 lines
- `lookupKeymapWithModifiers()`: <50 lines

**Test Coverage**:
- Unit tests: >95% line coverage for new code
- Integration tests: 100% of AC scenarios covered
- E2E tests: 100% of UAT scenarios automated

**Performance**:
- Hold detection: <10μs per event (measured via benchmark)
- Modifier state update: <5μs
- Total pipeline: <1ms P99

**No Special Cases**:
- All modal modifiers use same code path (mod0 through mod19)
- Hardware modifiers and modal modifiers share state tracking interface
- Tap and hold use unified event processing (no separate branches)

## Debugging/Instrumentation

**Strategic Logging Points**:
1. **Configuration Loading**: Log each modal modifier registration
   ```
   [MODIFIER:REGISTER] mod9 registered with trigger key A (YAMY: 0x001E)
   ```

2. **Hold Detection State Transitions**: Log activation/deactivation
   ```
   [MODIFIER:HOLD] Key A held for 215ms → mod9 ACTIVATED
   [MODIFIER:TAP] Key A released at 45ms → TAP detected, applying substitution
   ```

3. **Keymap Matching**: Log modifier combination matching
   ```
   [KEYMAP:LOOKUP] Active modifiers: C(1), mod9(1), looking up key X
   [KEYMAP:MATCH] Matched: *C-m9-*X → Y (2 modifiers)
   ```

4. **Performance Anomalies**: Log only if threshold exceeded
   ```
   [PERF:WARN] Hold detection took 150μs (threshold: 100μs)
   ```

**Test Utilities**:
```cpp
// MockTimer for unit testing hold detection
class MockTimer {
    void advance(std::chrono::milliseconds ms);
    std::chrono::milliseconds elapsed();
};

// ModifierStateObserver for testing
class ModifierStateObserver {
    std::vector<ModifierStateChange> getHistory();
    bool wasActivated(Modifier::Type type);
};
```

**Benchmarking**:
```bash
# Built-in benchmark mode
./yamy --benchmark --iterations 10000
# Outputs: P50, P95, P99, P99.9 latencies + CSV for graphing
```

## Success Metrics

### Functional Correctness
**Baseline**: 0% of modal modifier configurations functional
**Target**: 100% of valid configurations work correctly

**Measurement**:
- Run UAT-1 through UAT-5 (all must pass)
- Integration test suite (100% pass rate)
- Manual testing checklist (25/25 items pass)

### Performance
**Target**: <1ms P99 latency (entire pipeline)

**Measurement**:
```bash
./yamy --benchmark --config production.mayu --iterations 100000
```
**Expected output**:
```
Modal Modifier Performance Benchmark
=====================================
Iterations: 100,000
P50 latency: 0.42ms
P95 latency: 0.78ms
P99 latency: 0.95ms
P99.9 latency: 2.1ms
PASS: P99 < 1ms target ✓
```

### Cross-Platform Parity
**Target**: 100% identical behavior on Windows and Linux

**Measurement**:
- Same .mayu file loaded on both platforms
- Run identical test suite on both
- Compare outputs byte-for-byte
- Zero platform-specific failures

### Code Quality
**Target**: All quality gates met

**Measurement**:
```bash
# Lines per file
find src/ -name "*.cpp" | xargs wc -l | awk '$1 > 500 {print "FAIL: " $2 " has " $1 " lines"}'

# Test coverage
./run_tests --coverage
# Expected: >95% for new code

# Performance benchmark
./yamy --benchmark
# Expected: P99 < 1ms
```

---

**Document Version**: 1.0
**Created**: 2025-12-14
**Reviewed By**: (Pending approval)
