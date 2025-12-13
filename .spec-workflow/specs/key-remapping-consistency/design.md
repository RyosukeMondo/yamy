# Design Document: Key Remapping Consistency

## Overview

This design establishes a **unified, algorithmic event processing architecture** for YAMY's Linux implementation. The system transforms from heuristic, inconsistent key remapping to a provably correct, traceable, and testable implementation.

**Core Principles**:
1. **Single Path Processing**: ALL keys flow through the same code path
2. **Layer Purity**: Each layer is a pure function with no side effects
3. **Event Type Symmetry**: PRESS and RELEASE handled identically
4. **Zero Special Cases**: No key-specific or modifier-specific branches
5. **Complete Traceability**: Every event fully logged at every layer

## Steering Document Alignment

### Technical Standards (tech.md)

- **C++ Best Practices**: Uses const references, RAII, and modern C++17 features
- **Logging Standards**: Structured logging with consistent format `[LAYER:DIRECTION] message`
- **Error Handling**: Explicit error returns (0 for unmapped keys), no exceptions in hot path
- **Testing Standards**: GoogleTest for unit tests, custom framework for integration/E2E

### Project Structure (structure.md)

```
src/
├── core/engine/
│   ├── engine.cpp                    # Modified: unified event processing
│   ├── engine_event_processor.cpp    # NEW: pure event processing logic
│   └── engine_event_processor.h      # NEW: event processor interface
├── platform/linux/
│   ├── keycode_mapping.cpp           # Modified: scan map priority fix
│   ├── input_hook_linux.cpp          # Modified: event type preservation
│   └── input_injector_linux.cpp      # Modified: test injection support
tests/
├── automated_keymap_test.py          # NEW: autonomous test framework
├── test_event_processor_ut.cpp       # NEW: unit tests for layers
├── test_event_processor_it.cpp       # NEW: integration tests
└── test_event_processor_e2e.py       # NEW: end-to-end tests
docs/
└── EVENT_FLOW_ARCHITECTURE.md        # NEW: formal specification
```

## Code Reuse Analysis

### Existing Components to Leverage

- **keycode_mapping.cpp**:
  - Reuse: `g_evdevToYamyMap`, `g_scanToEvdevMap_US/JP`, `g_yamyToEvdevMap`
  - Modify: `yamyToEvdevKeyCode()` - already fixed to prioritize scan maps
  - Extend: Add logging points for Layer 1 and Layer 3

- **engine.cpp**:
  - Reuse: Substitution table loading from .mayu files
  - Modify: Event processing loop to use new unified processor
  - Extend: Add Layer 2 substitution logging

- **input_hook_linux.cpp**:
  - Reuse: Event reading from `/dev/input/eventX`
  - Verify: Event type (PRESS/RELEASE) is preserved correctly
  - Extend: Add event type logging

- **input_injector_linux.cpp**:
  - Reuse: Virtual device for output injection
  - Extend: Add test mode for synthetic event injection

### Integration Points

- **Debug Logging System**: Leverage existing `PLATFORM_LOG_INFO` macros
- **Configuration System**: Reuse .mayu file parser and substitution table structure
- **IPC Control**: Reuse `yamy-ctl` interface for test coordination
- **Build System**: Integrate new test files into existing CMakeLists.txt

## Architecture

### High-Level Event Flow

```
┌─────────────────────────────────────────────────────────────┐
│                   Physical Keyboard Input                    │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  Input Hook (input_hook_linux.cpp)                          │
│  - Reads from /dev/input/eventX                             │
│  - Captures evdev code + event type (PRESS/RELEASE)         │
│  - Passes to Engine                                         │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  Unified Event Processor (NEW: engine_event_processor.cpp)  │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ LAYER 1: Input Mapping                              │   │
│  │ f₁: evdev_code → yamy_scan_code                     │   │
│  │ Function: evdevToYamyKeyCode(evdev)                 │   │
│  │ Map: g_evdevToYamyMap                               │   │
│  │ Log: [LAYER1:IN] evdev X → yamy 0xYYYY              │   │
│  └──────────────────────┬──────────────────────────────┘   │
│                         ↓                                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ LAYER 2: Substitution Application                   │   │
│  │ f₂: yamy_scan_code → yamy_scan_code                 │   │
│  │ Function: applySubstitution(yamy)                   │   │
│  │ Table: loaded from .mayu "def subst" lines          │   │
│  │ Log: [LAYER2:SUBST] 0xAAAA → 0xBBBB (or PASSTHROUGH)│   │
│  └──────────────────────┬──────────────────────────────┘   │
│                         ↓                                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ LAYER 3: Output Mapping                             │   │
│  │ f₃: yamy_scan_code → evdev_code                     │   │
│  │ Function: yamyToEvdevKeyCode(yamy)                  │   │
│  │ Maps: g_scanToEvdevMap_US/JP (priority), then VK    │   │
│  │ Log: [LAYER3:OUT] yamy 0xCCCC → evdev Z             │   │
│  └──────────────────────┬──────────────────────────────┘   │
│                                                              │
│  Event Type Preservation: PRESS in = PRESS out               │
│                          RELEASE in = RELEASE out            │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  Output Injector (input_injector_linux.cpp)                 │
│  - Injects output_evdev + event_type to virtual device      │
│  - Preserves timing (no delay)                              │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ↓
┌─────────────────────────────────────────────────────────────┐
│                    Virtual Input Device Output               │
└─────────────────────────────────────────────────────────────┘
```

### Modular Design Principles

- **Layer Isolation**: Each layer is a pure function in its own module
- **No Side Effects**: Layers don't modify global state, only return values
- **Composability**: `output = f₃(f₂(f₁(input)))`
- **Testability**: Each layer can be unit tested independently

## Components and Interfaces

### Component 1: EventProcessor (NEW)

**File**: `src/core/engine/engine_event_processor.h/cpp`

**Purpose**: Unified event processing with guaranteed layer completeness

**Interface**:
```cpp
class EventProcessor {
public:
    struct ProcessedEvent {
        uint16_t output_evdev;
        EventType type;  // PRESS or RELEASE
        bool valid;      // false if unmapped
    };

    EventProcessor(const SubstitutionTable& subst_table);

    // Main processing function - ALWAYS processes all 3 layers
    ProcessedEvent processEvent(uint16_t input_evdev, EventType type);

private:
    // Layer 1: Input mapping
    uint16_t layer1_evdevToYamy(uint16_t evdev);

    // Layer 2: Substitution (pure function)
    uint16_t layer2_applySubstitution(uint16_t yamy_in);

    // Layer 3: Output mapping
    uint16_t layer3_yamyToEvdev(uint16_t yamy);

    const SubstitutionTable& substitutions_;
    bool debug_logging_;
};
```

**Dependencies**:
- `keycode_mapping.cpp` for Layer 1 and Layer 3 map lookups
- `.mayu` substitution table for Layer 2
- Logging system for traceability

**Reuses**: Existing `evdevToYamyKeyCode()` and `yamyToEvdevKeyCode()` functions

### Component 2: Enhanced KeycodeMapping

**File**: `src/platform/linux/keycode_mapping.cpp` (modified)

**Purpose**: Provide deterministic evdev↔scan code mappings with logging

**Modifications**:
```cpp
// ALREADY FIXED: Scan map priority over VK map
uint16_t yamyToEvdevKeyCode(uint16_t yamy_code) {
    // 1. Check scan maps FIRST (US/JP layout-aware)
    // 2. Fallback to VK map (for special cases)
    // 3. Log which map was used
}

// ENHANCED: Add event type parameter for logging
uint16_t evdevToYamyKeyCode(uint16_t evdev_code, EventType type) {
    // Log: [LAYER1:IN] evdev X (PRESS/RELEASE) → yamy 0xYYYY
}
```

**Reuses**: Existing map tables (`g_evdevToYamyMap`, `g_scanToEvdevMap_US/JP`)

### Component 3: Substitution Engine

**File**: `src/core/engine/engine.cpp` (modified)

**Purpose**: Apply .mayu substitutions with complete logging

**Modifications**:
```cpp
// Extract substitution logic into pure function
uint16_t applySubstitution(uint16_t yamy_code, const SubstitutionTable& table) {
    auto it = table.find(yamy_code);
    if (it != table.end()) {
        LOG_INFO("[LAYER2:SUBST] 0x%04X → 0x%04X", yamy_code, it->second);
        return it->second;
    } else {
        LOG_INFO("[LAYER2:PASSTHROUGH] 0x%04X (no substitution)", yamy_code);
        return yamy_code;  // Passthrough unchanged
    }
}
```

**Reuses**: Existing .mayu parser and substitution table structure

### Component 4: Automated Test Framework

**File**: `tests/automated_keymap_test.py` (NEW)

**Purpose**: Autonomous testing of all key mappings without user interaction

**Interface**:
```python
class AutomatedKeymapTest:
    def __init__(self, config_file="keymaps/config_clean.mayu"):
        self.config = parse_mayu_config(config_file)
        self.yamy_ctl = "./build/bin/yamy-ctl"
        self.log_file = "/tmp/yamy_test.log"

    # Inject synthetic key event via test interface
    def inject_key(self, evdev_code, event_type):
        """Inject PRESS or RELEASE event for testing"""

    # Read and parse debug log to verify output
    def verify_output(self, expected_evdev, event_type):
        """Parse [LAYER3:OUT] to verify correct output"""

    # Test a single key substitution
    def test_substitution(self, input_key, expected_key):
        """Test: input PRESS → output PRESS, input RELEASE → output RELEASE"""

    # Test all substitutions from config
    def test_all_substitutions(self):
        """Run 174 tests (87 keys × 2 event types)"""

    # Generate HTML report
    def generate_report(self):
        """Pass/fail report with specific failures highlighted"""
```

**Dependencies**:
- YAMY binary with debug logging enabled
- Test injection capability (via `yamy-test` utility or IPC)
- Log file access for output verification

**Reuses**: Existing .mayu parser for loading substitutions

### Component 5: Number-to-Modifier Mapper (ADVANCED)

**File**: `src/core/engine/modifier_key_handler.cpp` (NEW)

**Purpose**: Support number keys as custom modifier keys for small keyboards

**Interface**:
```cpp
class ModifierKeyHandler {
public:
    // Define number key as modifier
    void registerNumberModifier(uint16_t number_key, ModifierType mod_type);
    // e.g., registerNumberModifier(KEY_1, MOD_LSHIFT)

    // Check if key is held as modifier
    bool isModifierHeld(uint16_t number_key);

    // Process number key event with modifier/tap distinction
    ProcessingAction processNumberKey(uint16_t key, EventType type);
    // Returns: ACTIVATE_MODIFIER, DEACTIVATE_MODIFIER, or APPLY_SUBSTITUTION
};
```

**Integration**: Hooks into Layer 2 before substitution lookup

**Reuses**: Existing modal layer system (`mod mod4 = !!_1`)

## Data Models

### SubstitutionTable

```cpp
struct SubstitutionTable {
    // Map: YAMY scan code → YAMY scan code
    std::unordered_map<uint16_t, uint16_t> mappings;

    // Example entries (from config_clean.mayu):
    // 0x0011 (W) → 0x001E (A)
    // 0x0013 (R) → 0x0012 (E)
    // 0x0031 (N) → VK_LSHIFT (modifier)
};
```

### EventType (existing)

```cpp
enum EventType {
    PRESS = 1,
    RELEASE = 0,
    REPEAT = 2  // Not used in substitution
};
```

### TestResult

```python
class TestResult:
    input_evdev: int
    expected_evdev: int
    actual_evdev: int
    event_type: str  # "PRESS" or "RELEASE"
    passed: bool
    layer_failure: str  # "LAYER1", "LAYER2", "LAYER3", or None
```

## Error Handling

### Error Scenarios

1. **Unmapped Key in Layer 1**
   - **Handling**: Return 0, log `[LAYER1:IN] NOT FOUND in evdev→YAMY map`
   - **User Impact**: Key passes through unmodified (fallback behavior)

2. **No Substitution in Layer 2**
   - **Handling**: Return input unchanged, log `[LAYER2:PASSTHROUGH]`
   - **User Impact**: Key works but isn't remapped (expected for unmapped keys)

3. **Unmapped Key in Layer 3**
   - **Handling**: Return 0, log `[LAYER3:OUT] NOT FOUND in scan/VK map`
   - **User Impact**: Key is dropped (indicates configuration error)

4. **Test Injection Failure**
   - **Handling**: Retry once, then fail test with detailed error
   - **User Impact**: Test suite reports failure, requires investigation

5. **Log Parsing Failure**
   - **Handling**: Mark test as inconclusive, flag for manual review
   - **User Impact**: Test suite warns about verification issues

## Testing Strategy

### Unit Testing

**Framework**: GoogleTest (C++)

**Tests**:
```cpp
// Test Layer 1: evdev → yamy mapping
TEST(Layer1, WorkingKeyMapping) {
    EXPECT_EQ(evdevToYamyKeyCode(17), 0x0011);  // W key
}

TEST(Layer1, UnmappedKeyReturnsZero) {
    EXPECT_EQ(evdevToYamyKeyCode(9999), 0);
}

// Test Layer 2: Substitution application
TEST(Layer2, BasicSubstitution) {
    SubstitutionTable table = {{0x0011, 0x001E}};  // W → A
    EXPECT_EQ(applySubstitution(0x0011, table), 0x001E);
}

TEST(Layer2, NoSubstitutionPassthrough) {
    SubstitutionTable table = {};
    EXPECT_EQ(applySubstitution(0x0011, table), 0x0011);  // Unchanged
}

// Test Layer 3: yamy → evdev mapping
TEST(Layer3, ScanMapPriority) {
    EXPECT_EQ(yamyToEvdevKeyCode(0x0014), 20);  // Should be T, not CAPSLOCK
}

TEST(Layer3, VKMapFallback) {
    EXPECT_EQ(yamyToEvdevKeyCode(VK_ESCAPE), KEY_ESC);
}
```

**Coverage**: > 95% for layer functions

### Integration Testing

**Framework**: GoogleTest (C++) with mock substitution tables

**Tests**:
```cpp
// Test Layer 1 → Layer 2 → Layer 3 composition
TEST(Integration, CompleteTransformation) {
    SubstitutionTable table = {{0x0011, 0x001E}};  // W → A
    EventProcessor processor(table);

    auto result = processor.processEvent(17, PRESS);  // W key press

    EXPECT_EQ(result.output_evdev, 30);  // A key
    EXPECT_EQ(result.type, PRESS);
    EXPECT_TRUE(result.valid);
}

// Test event type preservation
TEST(Integration, EventTypeSymmetry) {
    EventProcessor processor(table);

    auto press = processor.processEvent(17, PRESS);
    auto release = processor.processEvent(17, RELEASE);

    EXPECT_EQ(press.type, PRESS);
    EXPECT_EQ(release.type, RELEASE);
    EXPECT_EQ(press.output_evdev, release.output_evdev);  // Same key
}
```

**Coverage**: All layer combinations, all event types

### End-to-End Testing

**Framework**: Python script with YAMY binary in test mode

**Tests**:
```python
def test_w_to_a_press_release():
    """E2E: W key press → A press, W release → A release"""
    test = AutomatedKeymapTest()

    # Test PRESS
    test.inject_key(evdev=17, event_type=PRESS)
    assert test.verify_output(expected=30, event_type=PRESS)

    # Test RELEASE
    test.inject_key(evdev=17, event_type=RELEASE)
    assert test.verify_output(expected=30, event_type=RELEASE)

def test_all_87_substitutions():
    """E2E: All config_clean.mayu substitutions"""
    test = AutomatedKeymapTest()
    results = test.test_all_substitutions()

    assert results.pass_rate == 100.0
    assert len(results.failures) == 0
```

**Coverage**: All 87 substitutions × 2 event types = 174 tests

### Continuous Integration

```bash
# tests/run_all_tests.sh
#!/bin/bash

# Unit tests
./build/bin/test_event_processor_ut || exit 1

# Integration tests
./build/bin/test_event_processor_it || exit 1

# E2E tests
YAMY_DEBUG_KEYCODE=1 ./build/bin/yamy keymaps/master.mayu &
YAMY_PID=$!
sleep 2

python3 tests/automated_keymap_test.py || { kill $YAMY_PID; exit 1; }

kill $YAMY_PID
echo "All tests passed!"
```

## Advanced Feature: Number Keys as Modifiers

### Design Overview

Allow number keys (0-9) to function as custom modifiers for users with small keyboards (no ten-key/numpad).

### Implementation Strategy

1. **Modifier Definition in .mayu**:
   ```
   mod mod4 = !!_1       # Hold number 1 = activate mod4 layer
   def subst *_1 = *LShift  # Wire number 1 directly to LShift hardware modifier
   ```

2. **Hold vs Tap Detection**:
   - **Hold**: Number key PRESS → delay 200ms → no other key pressed → activate modifier
   - **Tap**: Number key PRESS → RELEASE within 200ms → apply substitution

3. **Direct Hardware Modifier Mapping**:
   ```cpp
   // New map: Number keys → hardware modifiers
   std::unordered_map<uint16_t, uint16_t> g_numberToModifierMap = {
       {KEY_1, KEY_LEFTSHIFT},
       {KEY_2, KEY_RIGHTSHIFT},
       {KEY_3, KEY_LEFTCTRL},
       {KEY_4, KEY_RIGHTCTRL},
       {KEY_5, KEY_LEFTALT},
       {KEY_6, KEY_RIGHTALT},
       {KEY_7, KEY_LEFTMETA},
       {KEY_8, KEY_RIGHTMETA},
   };
   ```

4. **Integration Point**: Layer 2 checks for number-modifier mapping before substitution lookup

### Why This Is Advanced (Implemented Last)

- **Complexity**: Requires hold/tap timing logic
- **State Management**: Must track which number keys are currently held
- **Backward Compatibility**: Must not break existing number key substitutions
- **Testing**: Requires timing-based E2E tests

**Dependency**: Implement after core refactoring is complete and all 87 basic substitutions work reliably.

## Implementation Phases

1. **Phase 1: Investigation** (Week 1)
   - Add comprehensive logging
   - Create log analysis tools
   - Verify current asymmetries

2. **Phase 2: Core Refactoring** (Week 2)
   - Implement unified EventProcessor
   - Remove special cases
   - Ensure layer completeness

3. **Phase 3: Testing Framework** (Week 3)
   - Unit tests for layers
   - Integration tests for composition
   - E2E autonomous test suite

4. **Phase 4: Advanced Features** (Week 4)
   - Number-to-modifier mapping
   - Hold vs tap detection
   - Advanced test scenarios

## Success Criteria

- ✅ 100% of 87 substitutions pass automated tests
- ✅ PRESS and RELEASE events work identically
- ✅ All events show LAYER1→LAYER2→LAYER3 in logs
- ✅ Zero special cases in event processing code
- ✅ Test suite runs in < 10 seconds without user interaction
- ✅ New developers understand architecture in < 5 minutes
