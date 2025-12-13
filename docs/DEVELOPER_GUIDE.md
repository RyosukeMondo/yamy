# YAMY Developer Onboarding Guide

**Target Audience**: New contributors to the YAMY keyboard remapper project
**Time to Complete**: ~30 minutes reading + 1 hour hands-on practice
**Last Updated**: 2025-12-14

---

## Table of Contents

1. [Quick Start (5 Minutes)](#quick-start-5-minutes)
2. [Architecture Overview](#architecture-overview)
3. [The 3-Layer Event Flow](#the-3-layer-event-flow)
4. [Adding New Key Mappings](#adding-new-key-mappings)
5. [Testing Your Changes](#testing-your-changes)
6. [Debugging with Logs](#debugging-with-logs)
7. [Common Development Tasks](#common-development-tasks)
8. [Best Practices](#best-practices)
9. [Troubleshooting](#troubleshooting)
10. [Getting Help](#getting-help)

---

## Quick Start (5 Minutes)

### What is YAMY?

YAMY is a keyboard remapper for Linux that allows you to:
- Remap any key to any other key (e.g., W → A)
- Use number keys as modifiers when held (e.g., hold _1 for Shift)
- Create complex keyboard layouts using .mayu configuration files

### Core Architecture in 3 Sentences

1. **Every keystroke flows through 3 layers**: evdev input → YAMY internal → evdev output
2. **EventProcessor is the brain**: It orchestrates all transformations in a pure, testable way
3. **No special cases**: Modifiers, regular keys, numbers—all follow the same code path

### Build and Run

```bash
# Build YAMY
mkdir -p build && cd build
cmake .. && make

# Run YAMY with debug logging
YAMY_DEBUG_KEYCODE=1 ./bin/yamy

# Run tests
./tests/run_all_tests.sh
```

### Your First Contribution

The simplest contribution is adding a new key mapping. See [Adding New Key Mappings](#adding-new-key-mappings).

---

## Architecture Overview

### System Components

```
┌─────────────────────────────────────────────────────────────┐
│                         YAMY System                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────┐      ┌──────────────┐      ┌─────────────┐ │
│  │   Linux    │      │ EventPro-    │      │   Linux     │ │
│  │   evdev    │─────▶│   cessor     │─────▶│   evdev     │ │
│  │   Input    │      │  (3 layers)  │      │   Output    │ │
│  └────────────┘      └──────────────┘      └─────────────┘ │
│                             │                                │
│                             │                                │
│                             ▼                                │
│                    ┌─────────────────┐                      │
│                    │  Substitution   │                      │
│                    │     Table       │                      │
│                    │  (.mayu config) │                      │
│                    └─────────────────┘                      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Key Files and Directories

```
yamy/
├── src/
│   ├── core/
│   │   ├── engine/
│   │   │   ├── engine.cpp                 # Main event loop
│   │   │   ├── engine_event_processor.cpp # 3-layer processing
│   │   │   └── modifier_key_handler.cpp   # Number modifier feature
│   │   └── settings/
│   │       └── setting_loader.cpp         # .mayu parser
│   └── platform/
│       └── linux/
│           └── keycode_mapping.cpp        # evdev ↔ YAMY mapping tables
├── tests/
│   ├── test_event_processor_ut.cpp        # Unit tests
│   ├── test_event_processor_it.cpp        # Integration tests
│   ├── automated_keymap_test.py           # E2E tests
│   └── run_all_tests.sh                   # Complete test suite
├── docs/
│   ├── EVENT_FLOW_ARCHITECTURE.md         # Complete architecture doc
│   ├── NUMBER_MODIFIER_USER_GUIDE.md      # Number modifier feature
│   └── DEVELOPER_GUIDE.md                 # This file
└── configs/
    └── config_clean.mayu                  # Example configuration
```

---

## The 3-Layer Event Flow

Every key event flows through **exactly 3 layers** with **no special cases**.

### Layer 1: evdev → YAMY (Platform Translation)

**Purpose**: Convert Linux evdev codes to YAMY's internal scan codes
**Function**: `layer1_evdevToYamy(evdev_code) → yamy_scan_code`
**Location**: `src/core/engine/engine_event_processor.cpp:XXX`

**Example**:
```
Input:  evdev 17 (KEY_W)
Output: YAMY 0x0011 (scan code for W)
```

**Log Format**:
```
[LAYER1:IN] evdev 17 (PRESS) → yamy 0x0011
```

### Layer 2: Apply Substitution (Configuration Logic)

**Purpose**: Look up substitutions from .mayu config
**Function**: `layer2_applySubstitution(yamy_scan_code) → yamy_scan_code`
**Location**: `src/core/engine/engine_event_processor.cpp:XXX`

**Example**:
```
Input:  YAMY 0x0011 (W)
Config: key *W = *A
Output: YAMY 0x001E (A)
```

**Log Format**:
```
[LAYER2:SUBST] 0x0011 → 0x001e
```

If no substitution exists:
```
[LAYER2:PASSTHROUGH] 0x0011 (no substitution)
```

### Layer 3: YAMY → evdev (Platform Translation)

**Purpose**: Convert YAMY scan codes back to evdev for output
**Function**: `layer3_yamyToEvdev(yamy_scan_code) → evdev_code`
**Location**: `src/core/engine/engine_event_processor.cpp:XXX`

**Example**:
```
Input:  YAMY 0x001E (A)
Output: evdev 30 (KEY_A)
```

**Log Format**:
```
[LAYER3:OUT] yamy 0x001e → evdev 30 (KEY_A) [US scan map]
```

### Complete Event Flow Example

**Configuration**: `key *W = *A`
**User Action**: Press W key

```
[EVENT:START] evdev 17 (PRESS)
[LAYER1:IN] evdev 17 (PRESS) → yamy 0x0011
[LAYER2:SUBST] 0x0011 → 0x001e
[LAYER3:OUT] yamy 0x001e → evdev 30 (KEY_A) [US scan map]
[EVENT:END] evdev 30 (PRESS)

Result: System receives KEY_A press event instead of KEY_W
```

### Key Architectural Principles

1. **Universal Processing**: Every event goes through all 3 layers
2. **Event Type Consistency**: PRESS in = PRESS out, RELEASE in = RELEASE out
3. **Layer Completeness**: No layer skipping, no event type branching
4. **Pure Functions**: Layers are side-effect-free (except logging)
5. **No Special Cases**: Modifiers use the same code path as regular keys

---

## Adding New Key Mappings

### Scenario 1: Add a New Keyboard Key

If you have a keyboard with a key not yet supported by YAMY:

#### Step 1: Find the evdev code

```bash
# Install evtest
sudo apt-get install evtest

# List input devices
sudo evtest

# Select your keyboard device (usually /dev/input/eventX)
# Press the new key and note the code
# Example output: Event: time 1234567890.123456, type 1 (EV_KEY), code 183 (KEY_COFFEE), value 1
```

#### Step 2: Add to evdev → YAMY map

File: `src/platform/linux/keycode_mapping.cpp`

Find the `g_evdevToYamyMap` table (around line 50):

```cpp
const std::unordered_map<uint32_t, uint16_t> g_evdevToYamyMap = {
    // ... existing mappings ...
    {17, 0x0011},  // KEY_W → W

    // Add your new key here
    {183, 0x0099},  // KEY_COFFEE → Custom scan code 0x0099
};
```

**Scan code assignment rules**:
- Use unused scan code in range 0x0001 - 0x00FF
- Check existing codes to avoid conflicts
- Document the key name in a comment

#### Step 3: Add to YAMY → evdev map

Find the `g_yamyToEvdevMap` table (around line 200):

```cpp
const std::unordered_map<uint16_t, uint32_t> g_yamyToEvdevMap = {
    // ... existing mappings ...

    // Add reverse mapping
    {0x0099, 183},  // Custom Coffee key
};
```

#### Step 4: Test your mapping

```bash
# Rebuild
cd build && make

# Run with debug logging
YAMY_DEBUG_KEYCODE=1 ./bin/yamy

# Press your new key and verify logs show:
# [LAYER1:IN] evdev 183 (PRESS) → yamy 0x0099
# [LAYER2:PASSTHROUGH] 0x0099 (no substitution)
# [LAYER3:OUT] yamy 0x0099 → evdev 183 (KEY_COFFEE)
```

#### Step 5: Add unit tests

File: `tests/test_event_processor_ut.cpp`

```cpp
TEST_F(EventProcessorLayerTest, Layer1_NewCoffeeKey) {
    uint16_t yamy_code = m_processor.layer1_evdevToYamy(183);
    EXPECT_EQ(0x0099, yamy_code);
}

TEST_F(EventProcessorLayerTest, Layer3_NewCoffeeKey) {
    uint32_t evdev_code = m_processor.layer3_yamyToEvdev(0x0099);
    EXPECT_EQ(183, evdev_code);
}
```

Run tests:
```bash
./build/bin/yamy_event_processor_ut
```

### Scenario 2: Add Support for a New Keyboard Layout

Example: Adding German keyboard layout

#### Step 1: Create scan code map

File: `src/platform/linux/keycode_mapping.cpp`

```cpp
// German keyboard scan map (QWERTZ layout)
const std::unordered_map<uint16_t, uint32_t> g_scanToEvdevMap_DE = {
    {0x0015, 44},   // Z key (different from US)
    {0x002C, 21},   // Y key (swapped with Z)
    // ... other layout-specific mappings ...
};
```

#### Step 2: Update yamyToEvdevKeyCode() function

Add layout detection logic:

```cpp
uint32_t yamyToEvdevKeyCode(uint16_t yamy_code) {
    // Determine active layout (from config or environment)
    const char* layout = getenv("YAMY_KEYBOARD_LAYOUT");

    // Check scan maps first (priority order)
    if (layout && strcmp(layout, "DE") == 0) {
        auto it = g_scanToEvdevMap_DE.find(yamy_code);
        if (it != g_scanToEvdevMap_DE.end()) {
            return it->second;
        }
    }

    // Fall back to US scan map, then VK map
    // ... existing logic ...
}
```

#### Step 3: Test with different layouts

```bash
# Test German layout
YAMY_KEYBOARD_LAYOUT=DE YAMY_DEBUG_KEYCODE=1 ./bin/yamy

# Verify Y and Z keys map correctly for QWERTZ
```

### Scenario 3: Add a Key Substitution in .mayu

The easiest way to remap keys is via .mayu configuration:

#### Example: Remap W to A

File: `configs/my_config.mayu`

```
# Simple key substitution
key *W = *A

# Explanation:
# - Press W, get A
# - Works for both PRESS and RELEASE events
# - No code changes needed!
```

#### Example: Remap Number Key as Modifier

```
# Hold _1 for Left Shift, tap for normal _1
def numbermod *_1 = *LShift

# Now:
# - Hold _1 for 200ms → activates Shift modifier
# - Tap _1 quickly → types "1"
```

See `docs/NUMBER_MODIFIER_USER_GUIDE.md` for complete syntax.

---

## Testing Your Changes

### Test Pyramid

YAMY uses a 3-tier testing strategy:

```
        ┌──────────────┐
        │  E2E Tests   │  ← Real YAMY + real config + synthetic events
        │   (Python)   │
        ├──────────────┤
        │ Integration  │  ← Real EventProcessor + real substitution table
        │    Tests     │
        ├──────────────┤
        │  Unit Tests  │  ← Individual layer functions + mocks
        │   (C++)      │
        └──────────────┘
```

### Running Tests

#### All Tests (Recommended)

```bash
# Run complete test suite: unit → integration → E2E → report
./tests/run_all_tests.sh

# Expected output:
# ✓ Phase 1: Unit Tests (44/44 passed)
# ✓ Phase 2: Integration Tests (23/23 passed)
# ✓ Phase 3: E2E Tests (164/164 passed)
# ✓ Phase 4: HTML Report Generated
```

#### Unit Tests Only (Fast)

```bash
./build/bin/yamy_event_processor_ut

# Expected time: < 1 second
```

#### Integration Tests Only

```bash
./build/bin/yamy_event_processor_it

# Expected time: < 2 seconds
```

#### E2E Tests Only (Requires Running YAMY)

```bash
# Terminal 1: Start YAMY with debug logging
YAMY_DEBUG_KEYCODE=1 ./build/bin/yamy > /tmp/yamy_debug.log 2>&1

# Terminal 2: Run E2E tests
python3 tests/automated_keymap_test.py --json test_results.json

# View results
python3 tests/generate_test_report.py \
    --input test_results.json \
    --output test_report.html

# Open in browser
xdg-open test_report.html
```

### Writing Tests for Your Changes

#### Unit Test Template

File: `tests/test_event_processor_ut.cpp`

```cpp
TEST_F(EventProcessorLayerTest, Layer1_YourNewKey) {
    // Arrange
    uint32_t evdev_input = 183;  // Your new key

    // Act
    uint16_t yamy_code = m_processor.layer1_evdevToYamy(evdev_input);

    // Assert
    EXPECT_EQ(0x0099, yamy_code);  // Expected YAMY scan code
}
```

#### Integration Test Template

File: `tests/test_event_processor_it.cpp`

```cpp
TEST_F(EventProcessorIntegrationTest, CompleteFlow_YourSubstitution) {
    // Arrange: Set up substitution table
    m_substitution_table[0x0011] = 0x001E;  // W → A
    m_processor = std::make_unique<EventProcessor>(m_substitution_table);

    // Act: Process complete event
    auto result = m_processor->processEvent(17, EVENT_TYPE_PRESS);

    // Assert: Verify complete transformation
    EXPECT_TRUE(result.is_valid);
    EXPECT_EQ(30, result.output_evdev);  // Should output KEY_A
    EXPECT_EQ(EVENT_TYPE_PRESS, result.event_type);  // Type preserved
}
```

#### E2E Test Template

File: `tests/test_my_substitution.py`

```python
#!/usr/bin/env python3
import subprocess
import time

def test_w_to_a_substitution():
    # Inject W key press
    subprocess.run(['./build/bin/yamy-test', 'inject', '17', 'PRESS'])
    time.sleep(0.1)

    # Inject W key release
    subprocess.run(['./build/bin/yamy-test', 'inject', '17', 'RELEASE'])
    time.sleep(0.1)

    # Parse logs to verify output
    with open('/tmp/yamy_debug.log', 'r') as f:
        logs = f.read()

    # Verify Layer 3 output is KEY_A (evdev 30)
    assert '[LAYER3:OUT] yamy 0x001e → evdev 30' in logs
    print("✓ W → A substitution works!")

if __name__ == '__main__':
    test_w_to_a_substitution()
```

### Test Coverage Requirements

- **Unit Tests**: > 95% code coverage for layer functions
- **Integration Tests**: All substitution types covered (regular keys, modifiers, passthrough)
- **E2E Tests**: All .mayu substitutions tested for PRESS + RELEASE

Check coverage:
```bash
# Build with coverage flags
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make

# Run tests
./tests/run_all_tests.sh

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html

# View report
xdg-open coverage_html/index.html
```

---

## Debugging with Logs

### Enable Debug Logging

```bash
# Enable all debug logs
YAMY_DEBUG_KEYCODE=1 ./build/bin/yamy > /tmp/yamy_debug.log 2>&1

# Enable specific log levels (if supported)
YAMY_LOG_LEVEL=DEBUG ./build/bin/yamy
```

### Log Format

Every event produces logs for all 3 layers:

```
[EVENT:START] evdev 17 (PRESS)
[LAYER1:IN] evdev 17 (PRESS) → yamy 0x0011
[LAYER2:SUBST] 0x0011 → 0x001e
[LAYER3:OUT] yamy 0x001e → evdev 30 (KEY_A) [US scan map]
[EVENT:END] evdev 30 (PRESS)
```

### Log Analysis Tools

#### Automated Flow Analysis

```bash
# Analyze complete event flows
python3 tests/analyze_event_flow.py /tmp/yamy_debug.log

# Example output:
# Event Flow Analysis
# ===================
# Total events: 348
# Complete flows (all 3 layers): 342 (98.3%)
# Incomplete flows: 6 (1.7%)
#
# Asymmetries detected:
# - None! All events flow through all layers.
```

#### Search for Specific Key

```bash
# Find all events for W key (evdev 17)
grep 'evdev 17' /tmp/yamy_debug.log

# Find all Layer 2 substitutions
grep '\[LAYER2:SUBST\]' /tmp/yamy_debug.log

# Find unmapped keys
grep 'NOT FOUND' /tmp/yamy_debug.log
```

### Common Debugging Scenarios

#### Problem: Key doesn't remap as expected

**Steps**:
1. Check logs for the complete event flow
2. Verify Layer 1 converts evdev correctly
3. Verify Layer 2 shows substitution (not passthrough)
4. Verify Layer 3 outputs correct evdev code

**Example Debug Session**:
```bash
# Expected: W → A
# Actual: W → W (no change)

# Check logs
grep 'evdev 17' /tmp/yamy_debug.log

# Output shows:
# [LAYER1:IN] evdev 17 (PRESS) → yamy 0x0011  ✓ Correct
# [LAYER2:PASSTHROUGH] 0x0011 (no substitution)  ✗ Problem!
# [LAYER3:OUT] yamy 0x0011 → evdev 17 (KEY_W)

# Diagnosis: Layer 2 shows PASSTHROUGH instead of SUBST
# Solution: Check .mayu config has "key *W = *A"
```

#### Problem: Modifier substitution doesn't work

**Steps**:
1. Verify modifier key is in evdev → YAMY map
2. Verify substitution table has entry
3. Check Layer 3 prioritizes scan map over VK map
4. Verify both PRESS and RELEASE events processed

**Example Debug Session**:
```bash
# Expected: N → LShift
# Actual: N → CapsLock (wrong!)

# Check Layer 3 logs
grep 'yamy 0x002a' /tmp/yamy_debug.log

# Output shows:
# [LAYER3:OUT] yamy 0x002a → evdev 58 (KEY_CAPSLOCK) [VK map]

# Diagnosis: Layer 3 using VK map instead of scan map
# Solution: Check yamyToEvdevKeyCode() checks scan maps FIRST
# Verify g_scanToEvdevMap_US contains {0x002A, 42} for LShift
```

#### Problem: Event type changes (PRESS becomes RELEASE)

**Steps**:
1. Check processEvent() preserves event type
2. Verify no branching based on event type in layer functions
3. Check logs show same event type throughout

**Example Debug Session**:
```bash
# Expected: PRESS → PRESS
# Actual: PRESS → RELEASE

# Check complete flow
grep -A 5 '\[EVENT:START\].*PRESS' /tmp/yamy_debug.log | head -20

# Look for event type changes between layers
# Event type should be identical in:
# - [EVENT:START] evdev X (PRESS)
# - [LAYER1:IN] evdev X (PRESS)
# - [EVENT:END] evdev Y (PRESS)

# If type changes, check processEvent() code
```

### Performance Debugging

```bash
# Profile event processing latency
perf record -g ./build/bin/yamy
# Press some keys
perf report

# Expected: < 1ms per event
# If higher, check for:
# - Expensive map lookups (should be O(1) hash maps)
# - Excessive logging (disable in release builds)
# - Lock contention (should be lock-free for layer functions)
```

---

## Common Development Tasks

### Task 1: Add a New .mayu Syntax Feature

**Example**: Add support for "toggle" keys (press once to enable, press again to disable)

#### Step 1: Design the feature

File: `docs/TOGGLE_KEY_DESIGN.md`

```markdown
# Toggle Key Feature Design

## Syntax
```
def toggle *CapsLock = mod ToggleMode
```

## Behavior
- First press: Activate ToggleMode
- Second press: Deactivate ToggleMode
- Visual indicator: LED or notification
```

#### Step 2: Extend the parser

File: `src/core/settings/setting_loader.cpp`

Add new parsing function:
```cpp
void SettingLoader::load_DEFINE_TOGGLE() {
    // Parse syntax: def toggle *KEY = mod MODE
    // Similar to load_DEFINE_NUMBER_MODIFIER()
    // Store in m_setting->m_keyboard.addToggleKey()
}
```

Register in parser dispatch:
```cpp
else if (*t == "toggle") {
    load_DEFINE_TOGGLE();
}
```

#### Step 3: Implement the feature

File: `src/core/engine/toggle_key_handler.h` and `.cpp`

```cpp
class ToggleKeyHandler {
public:
    void registerToggleKey(uint16_t key, const std::string& mode_name);
    bool processToggleKey(uint16_t key, EventType event_type);
    bool isToggleActive(uint16_t key) const;

private:
    std::unordered_map<uint16_t, bool> m_toggle_states;
};
```

#### Step 4: Integrate with EventProcessor

File: `src/core/engine/engine_event_processor.cpp`

In `layer2_applySubstitution()`:
```cpp
// Check for toggle keys before substitution lookup
if (m_toggle_handler->isToggleKey(yamy_code)) {
    bool state_changed = m_toggle_handler->processToggleKey(yamy_code, event_type);
    if (state_changed) {
        // Update mode state
        // Optionally emit substitution based on toggle state
    }
}
```

#### Step 5: Write tests

Unit tests, integration tests, E2E tests covering:
- Toggle on/off transitions
- State persistence
- Multiple toggle keys
- Edge cases (rapid toggling, etc.)

#### Step 6: Document

Update:
- User guide with syntax examples
- Architecture docs with toggle state machine
- Developer guide with integration points

### Task 2: Optimize Performance Bottleneck

**Example**: Reduce event processing latency from 0.8ms to 0.5ms

#### Step 1: Profile current performance

```bash
# Build with profiling enabled
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make

# Profile with perf
perf record -g ./build/bin/yamy
# Press keys for ~30 seconds
perf report
```

#### Step 2: Identify hotspot

Look for functions consuming most CPU time:
- Map lookups in keycode_mapping.cpp
- Substitution table lookups in EventProcessor
- Logging overhead in debug mode

#### Step 3: Optimize

Example: Optimize map lookup with perfect hashing

```cpp
// Before: std::unordered_map (hash table)
const std::unordered_map<uint32_t, uint16_t> g_evdevToYamyMap = { ... };

// After: std::array with direct indexing (for dense key ranges)
constexpr uint16_t MAX_EVDEV_CODE = 256;
const std::array<uint16_t, MAX_EVDEV_CODE> g_evdevToYamyArray = {
    // evdev 0 → yamy 0x0000
    0x0000,
    // evdev 1 → yamy 0x0001
    0x0001,
    // ...
};

// Lookup: O(1) array access instead of O(1) hash lookup
uint16_t evdevToYamyKeyCode(uint32_t evdev_code) {
    if (evdev_code < MAX_EVDEV_CODE) {
        return g_evdevToYamyArray[evdev_code];
    }
    return 0;  // Unmapped
}
```

#### Step 4: Measure improvement

```bash
# Re-profile
perf record -g ./build/bin/yamy
perf report

# Verify latency reduction
# Expected: 0.8ms → 0.5ms (37.5% improvement)
```

#### Step 5: Verify correctness

```bash
# Run full test suite to ensure no regressions
./tests/run_all_tests.sh

# All tests should still pass
```

### Task 3: Debug a Test Failure

**Example**: Integration test fails after refactoring

```
[  FAILED  ] EventProcessorIntegrationTest.CompleteFlow_WtoA
Expected: result.output_evdev == 30
  Actual: result.output_evdev == 17
```

#### Step 1: Reproduce locally

```bash
# Run specific test with verbose output
./build/bin/yamy_event_processor_it --gtest_filter=*CompleteFlow_WtoA
```

#### Step 2: Add debug logging to test

```cpp
TEST_F(EventProcessorIntegrationTest, CompleteFlow_WtoA) {
    // Arrange
    m_substitution_table[0x0011] = 0x001E;  // W → A
    m_processor = std::make_unique<EventProcessor>(m_substitution_table);

    // Act
    auto result = m_processor->processEvent(17, EVENT_TYPE_PRESS);

    // Debug: Print intermediate values
    std::cout << "Layer 1 output: 0x" << std::hex << result.layer1_yamy << std::endl;
    std::cout << "Layer 2 output: 0x" << std::hex << result.layer2_yamy << std::endl;
    std::cout << "Layer 3 output: " << std::dec << result.output_evdev << std::endl;

    // Assert
    EXPECT_EQ(30, result.output_evdev);
}
```

#### Step 3: Identify root cause

```
Output shows:
Layer 1 output: 0x0011  (correct: W)
Layer 2 output: 0x0011  (wrong: should be 0x001E for A)
Layer 3 output: 17      (wrong: passthrough W instead of substituted A)

Root cause: Layer 2 not finding substitution in table
```

#### Step 4: Fix the bug

```cpp
// In layer2_applySubstitution()
// Bug: Looking up wrong variable
auto it = m_substitution_table.find(wrong_variable);  // ✗

// Fix: Look up correct yamy_code
auto it = m_substitution_table.find(yamy_code);  // ✓
```

#### Step 5: Verify fix

```bash
./build/bin/yamy_event_processor_it --gtest_filter=*CompleteFlow_WtoA

# Expected: [  PASSED  ]
```

---

## Best Practices

### Code Style

1. **Follow existing patterns**: Look at similar code before writing new code
2. **Use const correctness**: `const` references for read-only parameters
3. **Prefer hash maps**: `std::unordered_map` for O(1) lookups
4. **RAII for resources**: Use smart pointers, no manual `delete`
5. **Descriptive names**: `yamy_scan_code` not `code1`

Example:
```cpp
// Good
uint16_t layer1_evdevToYamy(uint32_t evdev_code) const {
    const auto it = g_evdevToYamyMap.find(evdev_code);
    if (it != g_evdevToYamyMap.end()) {
        return it->second;
    }
    return 0;  // Unmapped key
}

// Bad
uint16_t convert(uint32_t c) {
    if (g_map.find(c) != g_map.end()) return g_map[c];  // Non-const access
    else return 0;
}
```

### Architecture Principles

1. **No special cases**: All keys follow the same code path
2. **Pure functions**: Layer functions have no side effects (except logging)
3. **Event type preservation**: PRESS in = PRESS out, RELEASE in = RELEASE out
4. **Layer completeness**: Every event goes through all 3 layers
5. **Testability**: Design for unit testing from the start

### Testing Guidelines

1. **Test pyramid**: More unit tests, fewer E2E tests
2. **Test behavior, not implementation**: Test what code does, not how
3. **Mock dependencies**: Use mock substitution tables in unit tests
4. **Test edge cases**: Unmapped keys, rapid events, system suspend/resume
5. **Measure coverage**: Aim for > 95% code coverage

### Documentation Standards

1. **Update docs with code**: Documentation is part of the feature
2. **Use examples**: Show concrete examples, not just abstract descriptions
3. **Document "why"**: Explain design decisions, not just "what"
4. **Keep docs in sync**: Outdated docs are worse than no docs
5. **User-facing vs developer**: Separate user guides from architecture docs

---

## Troubleshooting

### Build Issues

#### Problem: CMake can't find dependencies

```
CMake Error: Could not find package X
```

**Solution**:
```bash
# Install missing dependencies (Ubuntu/Debian)
sudo apt-get install \
    libx11-dev \
    libxtst-dev \
    libevdev-dev \
    libgtest-dev

# Reconfigure
rm -rf build && mkdir build && cd build
cmake ..
```

#### Problem: Undefined reference errors during linking

```
undefined reference to `EventProcessor::processEvent(...)`
```

**Solution**:
```bash
# Clean build
rm -rf build && mkdir build && cd build
cmake .. && make clean && make

# If still failing, check:
# 1. Function signature matches declaration in .h file
# 2. Function is not in anonymous namespace (should be class member)
# 3. CMakeLists.txt includes all source files
```

### Runtime Issues

#### Problem: YAMY doesn't start

```bash
./build/bin/yamy
# No output, process exits immediately
```

**Solution**:
```bash
# Run with verbose logging
YAMY_DEBUG_KEYCODE=1 ./build/bin/yamy

# Check for error messages
# Common issues:
# - Can't access /dev/input/eventX (need sudo or input group)
# - Config file parse error (check .mayu syntax)
# - Another YAMY instance running (kill old process)
```

#### Problem: Keys not remapping

**Solution**:
```bash
# 1. Verify YAMY is running
ps aux | grep yamy

# 2. Check config is loaded
YAMY_DEBUG_KEYCODE=1 ./build/bin/yamy 2>&1 | grep 'substitution'

# 3. Verify logs show all 3 layers
# Should see: LAYER1 → LAYER2 → LAYER3 for each key

# 4. Check .mayu syntax
cat configs/my_config.mayu
# Ensure "key *W = *A" syntax is correct
```

### Test Issues

#### Problem: E2E tests fail with "YAMY not running"

```
Error: No events logged. Is YAMY running with debug logs enabled?
```

**Solution**:
```bash
# Terminal 1: Start YAMY with debug logging
YAMY_DEBUG_KEYCODE=1 ./build/bin/yamy > /tmp/yamy_debug.log 2>&1

# Terminal 2: Verify logs are being written
tail -f /tmp/yamy_debug.log

# Terminal 3: Run E2E tests
python3 tests/automated_keymap_test.py

# If still failing:
# - Check yamy-test inject binary exists
# - Verify /tmp/yamy_debug.log is writable
# - Ensure YAMY has permissions to read input devices
```

#### Problem: Unit tests compile but crash at runtime

```
Segmentation fault (core dumped)
```

**Solution**:
```bash
# Run under debugger
gdb ./build/bin/yamy_event_processor_ut
(gdb) run
(gdb) backtrace  # When crash occurs

# Common causes:
# - Null pointer dereference (check mock objects initialized)
# - Accessing destroyed object (check object lifetime)
# - Stack overflow (check for infinite recursion)
```

### Performance Issues

#### Problem: Event processing too slow (> 1ms)

**Solution**:
```bash
# Profile to find bottleneck
perf record -g ./build/bin/yamy
perf report

# Check for:
# - Excessive logging (disable in release build)
# - Lock contention (minimize locks in hot path)
# - Inefficient lookups (use hash maps, not linear search)

# Build release version
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Disable debug logging
unset YAMY_DEBUG_KEYCODE
./build/bin/yamy

# Re-test performance
```

---

## Getting Help

### Documentation

- **Architecture**: `docs/EVENT_FLOW_ARCHITECTURE.md`
- **Number Modifiers**: `docs/NUMBER_MODIFIER_USER_GUIDE.md`
- **Test Framework**: `tests/README_CI_TESTING.md`
- **Investigation Spec**: `docs/SYSTEMATIC_INVESTIGATION_SPEC.md`

### Code Examples

- **Layer Implementation**: `src/core/engine/engine_event_processor.cpp`
- **Unit Tests**: `tests/test_event_processor_ut.cpp`
- **Integration Tests**: `tests/test_event_processor_it.cpp`
- **E2E Tests**: `tests/automated_keymap_test.py`

### Common Questions

**Q: How do I add a new key to YAMY?**
A: Add to `g_evdevToYamyMap` and `g_yamyToEvdevMap` in `keycode_mapping.cpp`. See [Adding New Key Mappings](#adding-new-key-mappings).

**Q: Why does my modifier substitution output the wrong key?**
A: Check that `yamyToEvdevKeyCode()` checks scan maps BEFORE VK map. Scan map priority ensures correct modifier mapping.

**Q: How do I test my changes without running full YAMY?**
A: Write unit tests in `test_event_processor_ut.cpp`. Unit tests run in isolation without needing running YAMY.

**Q: What's the difference between unit and integration tests?**
A: Unit tests test individual layer functions with mocks. Integration tests test complete EventProcessor with real substitution tables.

**Q: How do I debug event flow?**
A: Enable debug logging with `YAMY_DEBUG_KEYCODE=1` and analyze logs with `tests/analyze_event_flow.py`.

**Q: Can I add new .mayu syntax?**
A: Yes! Extend the parser in `setting_loader.cpp`. See [Task 1: Add a New .mayu Syntax Feature](#task-1-add-a-new-mayu-syntax-feature).

**Q: How do I profile performance?**
A: Use `perf record -g ./build/bin/yamy` and `perf report`. Target < 1ms per event.

**Q: What should I do before submitting a pull request?**
A: Run `./tests/run_all_tests.sh` to ensure all tests pass. Update documentation for any new features.

---

## Next Steps

### After Reading This Guide

1. **Build and run YAMY**: Follow [Quick Start](#quick-start-5-minutes)
2. **Run the test suite**: `./tests/run_all_tests.sh`
3. **Read architecture docs**: `docs/EVENT_FLOW_ARCHITECTURE.md`
4. **Make a simple change**: Add a new key mapping (Scenario 1)
5. **Write a test**: Add unit test for your new key
6. **Debug with logs**: Enable `YAMY_DEBUG_KEYCODE=1` and observe event flow

### Recommended Learning Path

1. **Week 1**: Understand 3-layer architecture, build and run YAMY, explore codebase
2. **Week 2**: Add a new key mapping, write unit tests, run test suite
3. **Week 3**: Fix a bug, debug with logs, improve code coverage
4. **Week 4**: Add a new feature (e.g., toggle keys), write comprehensive tests, update docs

### Contributing

Ready to contribute? Check out:
- Open issues on GitHub
- Feature requests from users
- Code review comments on pull requests
- Documentation improvements needed

**Welcome to the YAMY development team!**

---

**Document Version**: 1.0
**Last Updated**: 2025-12-14
**Author**: YAMY Development Team
**License**: Same as YAMY project
