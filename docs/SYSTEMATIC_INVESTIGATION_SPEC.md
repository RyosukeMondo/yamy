# Systematic Investigation & Refactoring Specification
## YAMY Linux Key Remapping Consistency

**Goal**: Transform the key remapping system from heuristic debugging to algorithmic correctness through systematic investigation, consistent architecture, and automated verification.

---

## 1. ARCHITECTURE ANALYSIS

### 1.1 Current 3-Layer Key Transformation Pipeline

```
Physical Keyboard Input
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER 1: Input Mapping (evdev → YAMY scan code)                │
│ File: src/platform/linux/keycode_mapping.cpp                   │
│ Function: evdevToYamyKeyCode(uint16_t evdev_code)              │
│ Map: g_evdevToYamyMap                                           │
└─────────────────────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER 2: Engine Processing (YAMY scan code transformations)    │
│ File: src/core/engine/engine.cpp                               │
│ Process: def subst substitutions from .mayu files              │
│ Input: YAMY scan code from Layer 1                             │
│ Output: Transformed YAMY scan code                             │
└─────────────────────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER 3: Output Mapping (YAMY scan code → evdev)               │
│ File: src/platform/linux/keycode_mapping.cpp                   │
│ Function: yamyToEvdevKeyCode(uint16_t yamy_code)               │
│ Maps: g_scanToEvdevMap_US/JP, g_yamyToEvdevMap (fallback)      │
└─────────────────────────────────────────────────────────────────┘
    ↓
Virtual Input Device Output
```

### 1.2 Working vs Non-Working Cases

#### ✅ WORKING (Confirmed)
| Physical Key | Layer 1 In | Layer 2 Transform | Layer 3 Out | Result | Event Type |
|--------------|------------|-------------------|-------------|---------|------------|
| W (evdev 17) | scan 0x11  | W→A (scan 0x1E)   | evdev 30 (A)| **A**   | Press/Release |

#### ⚠️ PARTIAL (Key Up Only)
| Physical Key | Layer 1 In | Layer 2 Transform | Layer 3 Out | Result | Event Type |
|--------------|------------|-------------------|-------------|---------|------------|
| R (evdev 19) | scan 0x13  | R→E (scan 0x12)   | evdev 18 (E)| **E**   | **RELEASE ONLY** |
| T (evdev 20) | scan 0x14  | T→U (scan 0x16)   | evdev 22 (U)| **U**   | **RELEASE ONLY** |

#### ❌ NOT WORKING
| Physical Key | Layer 1 In | Layer 2 Transform | Layer 3 Out | Result | Issue |
|--------------|------------|-------------------|-------------|---------|-------|
| N (evdev 49) | scan 0x31  | N→LShift          | ??? | **N** | No Layer 2/3 output in logs |
| 4 (evdev 5)  | scan 0x05  | (not mapped)      | evdev 5 | **4** | No substitution defined |

### 1.3 Asymmetries Identified

#### A. Event Handling Asymmetry
- **W key**: Works on both PRESS and RELEASE events
- **R/T keys**: Only works on RELEASE events
- **Question**: Why do some keys process PRESS events and others only RELEASE?

#### B. Modifier Key Asymmetry
- **Regular keys** (W, R, T, etc.): Transform through all 3 layers
- **Modifier keys** (N→LShift): Captured in Layer 1, but no Layer 2/3 processing
- **Question**: Do modifier key substitutions require special handling?

#### C. Logging Asymmetry
- **Working keys**: Show LAYER1→LAYER2→LAYER3 progression in logs
- **N key**: Only shows LAYER1, no LAYER2/LAYER3 logs
- **Question**: Why does the engine skip Layer 2 processing for some keys?

---

## 2. ROOT CAUSE INVESTIGATION PLAN

### 2.1 Issue #1: Key Up/Down Event Processing

**Hypothesis**: The engine may be processing key events inconsistently, handling some keys on PRESS and others on RELEASE.

**Investigation Steps**:

1. **Instrument Engine Event Handling**
   - File: `src/core/engine/engine.cpp`
   - Add logging for ALL key events (PRESS, RELEASE, REPEAT)
   - Log format: `[EVENT] evdev=%d, type=%s (PRESS/RELEASE), scan=0x%04X`

2. **Compare Working vs Non-Working Keys**
   - Capture full event sequence for W key (working): PRESS → transform → RELEASE
   - Capture full event sequence for R key (partial): PRESS → ??? → RELEASE → transform
   - Identify WHERE the difference occurs

3. **Check Event Filtering Logic**
   - Search for code that filters PRESS vs RELEASE events
   - Look for conditional logic that might skip PRESS events
   - Check if there's key-specific event handling

**Code Locations to Inspect**:
```cpp
// src/core/engine/engine.cpp
- Engine::handleKeyboardInput()
- Engine::processKeyEvent()
- Any event type checking (if (event.type == PRESS/RELEASE))

// src/platform/linux/input_hook_linux.cpp
- How events are read from /dev/input/eventX
- Event filtering before passing to engine
```

**Expected Outcome**: Identify why R/T keys only trigger transformation on RELEASE.

---

### 2.2 Issue #2: Modifier Key Substitution

**Hypothesis**: The engine may have special handling for modifier keys that prevents normal substitution processing.

**Investigation Steps**:

1. **Trace N Key Through Engine**
   - Add detailed logging in Layer 2 processing
   - Log entry: `[LAYER2:IN] Processing key: scan=0x%04X, name=%s`
   - Log substitution lookup: `[LAYER2:LOOKUP] Checking for substitution...`
   - Log result: `[LAYER2:RESULT] Found substitution: %s → %s` OR `No substitution`

2. **Check Modifier Key Special Cases**
   - Search for "LShift", "RShift", "Shift", "modifier" in engine code
   - Look for special handling that might bypass normal substitution
   - Check if there's a "don't remap modifiers" flag or logic

3. **Verify Substitution Table**
   - Dump the loaded substitution table from .mayu files
   - Verify `N → LShift` is actually loaded
   - Check if there's a difference between:
     - Regular key substitution (W → A)
     - Modifier key substitution (N → LShift)

**Code Locations to Inspect**:
```cpp
// src/core/engine/engine.cpp
- Where substitutions are looked up
- Where modifier keys are checked
- The substitution application logic

// src/core/settings/setting.h (or keymap loading)
- How def subst is parsed and stored
- Whether modifier keys are treated differently
```

**Expected Outcome**: Identify why N→LShift substitution isn't being applied.

---

### 2.3 Issue #3: Event Flow Consistency

**Hypothesis**: The event flow from Layer 1 → Layer 2 → Layer 3 is not consistent for all keys.

**Investigation Steps**:

1. **Add Complete Event Tracing**
   ```cpp
   // For EVERY key event, log:
   [TRACE] ========== NEW EVENT ==========
   [TRACE] Physical input: evdev %d, type %s
   [TRACE] Layer 1 → scan 0x%04X
   [TRACE] Layer 2 → checking substitution
   [TRACE] Layer 2 → result: 0x%04X
   [TRACE] Layer 3 → checking output map
   [TRACE] Layer 3 → result: evdev %d
   [TRACE] Injecting: evdev %d, type %s
   [TRACE] ========== EVENT END ==========
   ```

2. **Test All Key Categories**
   - Regular letter keys (A-Z)
   - Number keys (0-9)
   - Modifier keys (Shift, Ctrl, Alt)
   - Special keys (Tab, Enter, Backspace)
   - Function keys (F1-F12)

3. **Identify Missing Steps**
   - For each non-working key, identify which layer is skipped
   - For each partial-working key, identify which event type is skipped
   - Create a matrix of key × event type × layer processing

**Expected Outcome**: Complete understanding of which keys/events skip which layers.

---

## 3. CONSISTENCY REQUIREMENTS

### 3.1 Universal Invariants (Must Be True for ALL Keys)

```
INVARIANT 1: Event Symmetry
  IF a key is pressed (PRESS event)
  THEN the transformation should happen on PRESS
  AND the same key released (RELEASE event)
  THEN the transformation should happen on RELEASE

  Violation: R/T keys only transform on RELEASE

INVARIANT 2: Layer Completeness
  FOR ALL key events
  IF Layer 1 processes the event (evdev → scan)
  THEN Layer 2 MUST process the event (apply substitution)
  AND Layer 3 MUST process the event (scan → evdev out)

  Violation: N key shows Layer 1 but not Layer 2/3

INVARIANT 3: Substitution Equality
  FOR ALL key substitutions defined in .mayu
  The substitution processing MUST be identical
  REGARDLESS of whether target is:
    - Regular key (A-Z, 0-9)
    - Modifier key (Shift, Ctrl, Alt)
    - Special key (Tab, Enter, etc.)

  Violation: N→LShift not processed same as W→A

INVARIANT 4: Event Type Preservation
  IF input is PRESS event with scan code S
  AND substitution S → T exists
  THEN output MUST be PRESS event with scan code T

  IF input is RELEASE event with scan code S
  AND substitution S → T exists
  THEN output MUST be RELEASE event with scan code T

  Violation: Event type handling may be inconsistent
```

### 3.2 Consistency Rules

1. **One Event Path Rule**
   - There should be ONE canonical path for ALL key events
   - No special cases, no key-specific branches
   - All keys flow through: Layer 1 → Layer 2 → Layer 3

2. **Type-Agnostic Processing Rule**
   - The engine should NOT care if a key is:
     - Letter, number, modifier, or special
   - Substitution logic should be universal

3. **Event Type Symmetry Rule**
   - PRESS and RELEASE events should be processed identically
   - The only difference: the event type flag
   - No "PRESS-only" or "RELEASE-only" logic

---

## 4. REFACTORING STRATEGY

### 4.1 Phase 1: Instrument & Understand

**Goal**: Add comprehensive logging to understand current behavior

**Tasks**:
1. Add TRACE-level logging to all 3 layers
2. Add event type logging (PRESS/RELEASE/REPEAT)
3. Add substitution lookup logging
4. Create log analysis script to visualize event flow

**Deliverables**:
- Enhanced logging in engine.cpp and keycode_mapping.cpp
- Log analysis tool: `tests/analyze_event_flow.py`
- Documentation of actual vs expected flow

### 4.2 Phase 2: Identify Root Causes

**Goal**: Pinpoint exact code locations causing asymmetries

**Tasks**:
1. Run automated test suite (see section 5)
2. Analyze logs to find WHERE each invariant is violated
3. Document specific code lines causing issues
4. Categorize issues by type (event handling, substitution lookup, modifier special cases)

**Deliverables**:
- Root cause analysis document
- Code location map for each issue
- Priority list for fixes

### 4.3 Phase 3: Unified Event Processing

**Goal**: Refactor to single, consistent event processing path

**Design**:
```cpp
// Proposed unified event handler
class KeyEventProcessor {
public:
    // Single entry point for ALL key events
    void processKeyEvent(const InputEvent& event) {
        // Step 1: Layer 1 - Input mapping (ALWAYS)
        uint16_t yamy_code = layer1_InputMapping(event.evdev_code);

        // Step 2: Layer 2 - Substitution (ALWAYS)
        uint16_t transformed_code = layer2_ApplySubstitution(yamy_code);

        // Step 3: Layer 3 - Output mapping (ALWAYS)
        uint16_t output_evdev = layer3_OutputMapping(transformed_code);

        // Step 4: Inject (ALWAYS)
        injectKeyEvent(output_evdev, event.type);  // Preserve event type!
    }

private:
    // No key-specific logic, no special cases
    // Just pure mapping: evdev → yamy → substitution → yamy → evdev
};
```

**Key Principles**:
- NO branching based on key type
- NO special cases for modifiers
- NO different handling for PRESS vs RELEASE
- Event type is preserved throughout

### 4.4 Phase 4: Algorithmic Verification

**Goal**: Make correctness provable, not heuristic

**Strategy**:
1. **Define formal specification** for each layer:
   ```
   Layer 1: f₁: evdev_code → yamy_code
     ∀ e ∈ evdev_codes: f₁(e) ∈ yamy_codes

   Layer 2: f₂: yamy_code → yamy_code (via substitution table)
     ∀ y ∈ yamy_codes:
       IF substitution y→y' exists THEN f₂(y) = y'
       ELSE f₂(y) = y

   Layer 3: f₃: yamy_code → evdev_code
     ∀ y ∈ yamy_codes: f₃(y) ∈ evdev_codes

   Complete transformation:
     output = f₃(f₂(f₁(input)))
   ```

2. **Property-based testing**:
   ```python
   # For ALL keys, for ALL event types:
   def test_round_trip_property():
       for key in ALL_KEYS:
           for event_type in [PRESS, RELEASE]:
               # Input
               input_event = (key.evdev, event_type)

               # Expected output (from .mayu substitution)
               expected_evdev = lookup_substitution(key)
               expected_event = (expected_evdev, event_type)

               # Actual output
               actual_event = process_through_yamy(input_event)

               # MUST match
               assert actual_event == expected_event
   ```

3. **Coverage verification**:
   - Test ALL keys defined in 109_clean.mayu
   - Test ALL substitutions in config_clean.mayu
   - Verify 100% pass rate

---

## 5. AUTOMATED TESTING STRATEGY

### 5.1 Test Framework Design

**No User Interaction Required** - All tests fully automated.

```python
#!/usr/bin/env python3
"""
Automated YAMY Key Remapping Test Suite
Tests ALL keys, ALL substitutions, ZERO user interaction
"""

class YamyTestFramework:
    def __init__(self):
        self.yamy_binary = "./build/bin/yamy"
        self.config = "keymaps/master.mayu"
        self.test_results = []

    def inject_key_event(self, evdev_code, event_type):
        """Inject synthetic key event directly to engine"""
        # Use yamy-test binary to inject events
        cmd = f"./build/bin/yamy-test inject {evdev_code} {event_type}"
        subprocess.run(cmd, shell=True)

    def read_output_log(self):
        """Read YAMY debug log to see what was output"""
        # Parse /tmp/yamy_clean.log for LAYER3 output
        with open("/tmp/yamy_clean.log") as f:
            return parse_layer3_output(f)

    def test_key_mapping(self, input_evdev, expected_output_evdev, event_type):
        """Test a single key mapping"""
        # Clear log
        open("/tmp/yamy_clean.log", "w").close()

        # Inject input
        self.inject_key_event(input_evdev, event_type)
        time.sleep(0.05)  # Wait for processing

        # Read output
        actual_output = self.read_output_log()

        # Verify
        result = {
            'input': input_evdev,
            'expected': expected_output_evdev,
            'actual': actual_output,
            'event_type': event_type,
            'pass': actual_output == expected_output_evdev
        }
        self.test_results.append(result)
        return result['pass']

    def test_all_substitutions(self):
        """Test ALL substitutions from config_clean.mayu"""
        substitutions = parse_mayu_substitutions("keymaps/config_clean.mayu")

        for sub in substitutions:
            # Test PRESS event
            self.test_key_mapping(sub.input_evdev, sub.output_evdev, PRESS)

            # Test RELEASE event
            self.test_key_mapping(sub.input_evdev, sub.output_evdev, RELEASE)

    def generate_report(self):
        """Generate comprehensive test report"""
        total = len(self.test_results)
        passed = sum(1 for r in self.test_results if r['pass'])

        print(f"Test Results: {passed}/{total} passed ({100*passed/total:.1f}%)")
        print()

        # Show failures
        failures = [r for r in self.test_results if not r['pass']]
        if failures:
            print("FAILURES:")
            for f in failures:
                print(f"  {get_key_name(f['input'])} → "
                      f"Expected: {get_key_name(f['expected'])}, "
                      f"Actual: {get_key_name(f['actual'])}, "
                      f"Event: {f['event_type']}")
```

### 5.2 Test Categories

1. **Basic Substitution Tests**
   - Test each `def subst` line individually
   - Verify input → output mapping
   - Test both PRESS and RELEASE events

2. **Modifier Key Tests**
   - Test N → LShift
   - Test any other modifier substitutions
   - Verify modifier state is preserved

3. **Event Type Consistency Tests**
   - For each key, verify PRESS and RELEASE both work
   - Verify output event type matches input event type

4. **Layer Completeness Tests**
   - For each key, verify Layer 1 → 2 → 3 all execute
   - Verify no layers are skipped

5. **Edge Case Tests**
   - Unmapped keys (should pass through unchanged)
   - E0-extended keys
   - Numpad keys
   - Function keys

### 5.3 Continuous Verification

```bash
#!/bin/bash
# tests/run_verification.sh
# Run complete test suite and generate report

# Start YAMY with debug logging
YAMY_DEBUG_KEYCODE=1 YAMY_DEBUG_LOG=/tmp/yamy_clean.log ./build/bin/yamy keymaps/master.mayu &
YAMY_PID=$!

sleep 2  # Wait for startup

# Run test suite
python3 tests/automated_keymap_test.py

# Generate report
python3 tests/generate_test_report.py

# Stop YAMY
kill $YAMY_PID

# Exit with test status
exit $?
```

**Run after every code change** to verify no regressions.

---

## 6. SUCCESS CRITERIA

### 6.1 Quantitative Metrics

- [ ] **100% of defined substitutions work correctly**
  - All `def subst` lines in config_clean.mayu pass tests

- [ ] **100% event type consistency**
  - PRESS events produce PRESS outputs
  - RELEASE events produce RELEASE outputs

- [ ] **100% layer completeness**
  - All events flow through Layer 1 → 2 → 3
  - No events skip layers

- [ ] **Zero special cases**
  - No key-specific branches in code
  - No modifier-specific branches
  - No event-type-specific branches (except preserving type)

### 6.2 Qualitative Metrics

- [ ] **Code is obvious**
  - Event processing flow is linear and clear
  - No "magic" or hidden behavior
  - New developers can understand in < 5 minutes

- [ ] **Debugging is trivial**
  - Logs show complete event flow
  - Any issue traceable to specific layer
  - Automated tests pinpoint failures

- [ ] **Changes are safe**
  - Automated tests catch regressions
  - No manual testing required
  - Refactoring is confident, not risky

---

## 7. IMPLEMENTATION ROADMAP

### Week 1: Investigation
- [ ] Add comprehensive logging to all layers
- [ ] Create log analysis tools
- [ ] Run tests on current system
- [ ] Document all asymmetries and violations

### Week 2: Root Cause Analysis
- [ ] Identify exact code causing each issue
- [ ] Categorize issues by type
- [ ] Propose specific fixes for each category
- [ ] Review architectural changes needed

### Week 3: Refactoring
- [ ] Implement unified event processing
- [ ] Remove all special cases
- [ ] Ensure consistent layer flow
- [ ] Add property-based tests

### Week 4: Verification
- [ ] Run automated test suite
- [ ] Verify 100% pass rate
- [ ] Performance testing
- [ ] Documentation update

---

## 8. FILES TO CREATE/MODIFY

### New Files
```
tests/automated_keymap_test.py       # Automated test framework
tests/analyze_event_flow.py          # Log analysis tool
tests/generate_test_report.py        # Test report generator
tests/run_verification.sh            # Complete verification script
docs/EVENT_FLOW_SPEC.md              # Formal specification
docs/INVARIANTS.md                   # System invariants documentation
```

### Files to Modify
```
src/core/engine/engine.cpp           # Unified event processing
src/platform/linux/keycode_mapping.cpp   # Ensure consistency
src/platform/linux/input_hook_linux.cpp  # Event handling
```

### Configuration
```
keymaps/config_clean.mayu            # May need adjustments based on findings
```

---

## 9. INVESTIGATION CHECKLIST

Use this checklist to systematically investigate each issue:

### For Each Non-Working Key:

- [ ] **Verify Layer 1**
  - [ ] Check evdev code is in g_evdevToYamyMap
  - [ ] Verify mapping to correct YAMY scan code
  - [ ] Confirm LAYER1 log output shows correct mapping

- [ ] **Verify Layer 2**
  - [ ] Check .mayu file has substitution defined
  - [ ] Verify substitution is loaded into engine
  - [ ] Check LAYER2 log shows substitution applied
  - [ ] If no LAYER2 log: Find WHERE in code it's skipped

- [ ] **Verify Layer 3**
  - [ ] Check YAMY scan code is in g_scanToEvdevMap_US/JP
  - [ ] Verify mapping to correct output evdev code
  - [ ] Confirm LAYER3 log output shows correct mapping

- [ ] **Verify Event Types**
  - [ ] Test with PRESS event
  - [ ] Test with RELEASE event
  - [ ] Check if both produce same transformation
  - [ ] If different: Find WHERE event type affects processing

- [ ] **Document Findings**
  - [ ] Code file and line number where issue occurs
  - [ ] Root cause explanation
  - [ ] Proposed fix

---

## 10. CONCLUSION

This specification transforms YAMY debugging from:
- **Heuristic** → **Algorithmic**
- **Manual testing** → **Automated verification**
- **Special cases** → **Universal rules**
- **Unclear flow** → **Obvious processing**

The goal is not just to fix current issues, but to create a system where:
1. Correctness is **provable**, not guessed
2. Issues are **impossible** or immediately obvious
3. Changes are **safe** and verified automatically
4. The code is **self-documenting** and clear

By following this spec systematically, we will achieve a robust, maintainable, and correct key remapping system.
