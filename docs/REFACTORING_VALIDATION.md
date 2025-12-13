# Refactoring Validation Report: Phase 2 Core Refactoring
## Key Remapping Consistency - Task 2.7

**Date**: 2025-12-14
**Spec**: key-remapping-consistency
**Phase**: 2 - Core Refactoring
**Tasks Completed**: 2.1 - 2.6
**Baseline**: docs/INVESTIGATION_FINDINGS.md (Task 1.6)

---

## Executive Summary

This validation reveals that **Task 2.6 was only partially completed**. The EventProcessor architecture was created and the substitution table was integrated into the engine, but the **full 3-layer processing pipeline (`processEvent()`) is not being called**. The engine is using the EventProcessor's substitution table directly rather than calling the EventProcessor's layer functions.

### Current State
- ✅ EventProcessor class created with layer1, layer2, layer3 functions (Tasks 2.1-2.5)
- ✅ Substitution table extracted from .mayu file and passed to EventProcessor (Task 2.6)
- ⚠️ Engine uses substitution table directly, NOT calling `processEvent()` (Task 2.6 incomplete)
- ❌ Layer 1 and Layer 3 logging from EventProcessor NOT active
- ⚠️ Layer 2 logging active but through engine_generator.cpp, not EventProcessor

### Critical Finding

The implementation in `engine_generator.cpp` (commit 0cb46c9) directly accesses `m_substitutionTable` and performs its own Layer 2 substitution logic:

```cpp
// From engine_generator.cpp (current state)
auto it = m_substitutionTable.find(input_yamy);
if (it != m_substitutionTable.end()) {
    output_yamy = it->second;
    PLATFORM_LOG_INFO("Layer2", "[LAYER2:SUBST] ...");
}
```

**But it should be:**
```cpp
// What it should be (using EventProcessor)
ProcessedEvent result = m_eventProcessor->processEvent(input_evdev, event_type);
// This would call Layer1 → Layer2 → Layer3 with complete logging
```

---

## Investigation Method

### 1. Code Analysis
- Reviewed commit 0cb46c9 (Task 2.6 integration)
- Analyzed engine_generator.cpp substitution logic
- Checked EventProcessor::processEvent() call sites
- Verified logging architecture

### 2. Attempted Runtime Testing
**Note**: Could not complete full runtime testing due to:
1. **Logging Issue**: `tomsgstream` (engine log) doesn't output on Linux (by design)
2. **Event Injection**: yamy-test inject creates virtual device that may not be monitored by YAMY
3. **Layer 1/3 Logs**: Not active because EventProcessor::processEvent() is not called

**Logging Fix Applied** (for future testing):
- Added `platform_logger.h` include to engine_setting.cpp
- Set PlatformLogger level to DEBUG when YAMY_DEBUG_KEYCODE=1
- This enables PLATFORM_LOG_DEBUG output to stderr

### 3. Findings from Code Review

**What Works:**
1. ✅ EventProcessor::processEvent() **implementation is correct**
   - Calls layer1_evdevToYamy()
   - Calls layer2_applySubstitution()
   - Calls layer3_yamyToEvdev()
   - Preserves event type throughout
   - Comprehensive logging at each layer

2. ✅ Substitution table **correctly built** from .mayu file
   - buildSubstitutionTable() extracts all 87 substitutions
   - Table passed to EventProcessor constructor
   - No special cases in substitution logic

3. ✅ Layer functions **correctly implemented**
   - Layer 1: calls evdevToYamyKeyCode()
   - Layer 2: pure function, looks up in substitution table
   - Layer 3: calls yamyToEvdevKeyCode()

**What's Missing:**
1. ❌ **EventProcessor::processEvent() is NOT called** in event processing flow
   - engine_generator.cpp uses m_substitutionTable directly
   - This bypasses Layer 1 and Layer 3 EventProcessor functions
   - Only Layer 2 substitution logic is applied

2. ❌ **Event type not passed through all layers**
   - Current code doesn't call processEvent(evdev, event_type)
   - Layer 1 and Layer 3 are not involved in the flow

3. ❌ **Incomplete layer logging**
   - [LAYER1:IN] logs: NOT present (EventProcessor layer1 not called)
   - [LAYER2:SUBST/PASSTHROUGH]: Present (but from engine_generator, not EventProcessor)
   - [LAYER3:OUT] logs: NOT present (EventProcessor layer3 not called)

---

## Comparison with Requirements

### Requirement 1: Universal Event Processing
**Status**: ⚠️ **Partially Met**

- ✅ EventProcessor architecture exists
- ✅ All events use substitution table (no key-specific special cases)
- ❌ EventProcessor::processEvent() not called (layer functions bypassed)

### Requirement 2: Event Type Consistency
**Status**: ❌ **Not Verified**

- Cannot verify without runtime testing
- EventProcessor::processEvent() preserves event type correctly in code
- But since it's not called, event type preservation depends on engine_generator.cpp logic

### Requirement 3: Layer Completeness
**Status**: ❌ **Not Met**

- Only Layer 2 is active (substitution)
- Layer 1 (evdev→YAMY) and Layer 3 (YAMY→evdev) are NOT going through EventProcessor
- Logs do not show complete LAYER1→LAYER2→LAYER3 flow

### Requirement 7: Code Consistency
**Status**: ⚠️ **Partially Met**

- ✅ Substitution table has no special cases (N→LShift uses same logic as W→A)
- ⚠️ But flow is fragmented (direct table access instead of processEvent())

---

## Required Changes to Complete Task 2.6

To properly integrate EventProcessor, `engine_generator.cpp` should:

### Current Flow (Incomplete):
```
Physical Key
    ↓
Keymap Lookup (engine_generator.cpp)
    ↓
Get YAMY scan code from Key object
    ↓
m_substitutionTable.find(input_yamy)  ← Direct table access
    ↓
Find substituted Key object
    ↓
Generate output events
```

### Required Flow (Complete):
```
Physical Key (evdev code)
    ↓
m_eventProcessor->processEvent(evdev, event_type)
    ↓
  [LAYER1: evdev → YAMY scan]
    ↓
  [LAYER2: Apply substitution]
    ↓
  [LAYER3: YAMY scan → evdev]
    ↓
ProcessedEvent {output_evdev, event_type, valid}
    ↓
Generate output with output_evdev
```

### Specific Code Changes Needed:

**File**: `src/core/engine/engine_generator.cpp`
**Location**: `beginGeneratingKeyboardEvents()` function

**Replace**:
```cpp
// Current: Direct substitution table access
auto it = m_substitutionTable.find(input_yamy);
if (it != m_substitutionTable.end()) {
    output_yamy = it->second;
}
```

**With**:
```cpp
// Required: Call EventProcessor for full layer processing
uint16_t input_evdev = /* extract from event */;
EventType event_type = isPhysicallyPressed ? EventType::PRESS : EventType::RELEASE;

ProcessedEvent result = m_eventProcessor->processEvent(input_evdev, event_type);

if (result.valid) {
    uint16_t output_evdev = result.evdev_code;
    // Convert output_evdev to Key object and generate
}
```

**Challenge**: The current engine architecture works with Key objects and YAMY scan codes throughout. EventProcessor works with evdev codes at entry/exit. Need to:
1. Extract original evdev code from the input event (currently lost early in pipeline)
2. Pass evdev code + event type to EventProcessor
3. Convert output evdev back to Key object for rest of engine

**Alternative** (Simpler): Since the engine already has YAMY scan codes, just use EventProcessor for Layer 2:
```cpp
// Simpler: Use EventProcessor only for Layer 2 substitution
uint16_t input_yamy = input_sc[0].m_scan;
uint16_t output_yamy = m_eventProcessor->layer2_applySubstitution(input_yamy);
```

But this defeats the purpose of having a **unified 3-layer EventProcessor**. The proper solution is to refactor the event flow to preserve evdev codes.

---

## Test Plan (Once Integration Complete)

### Test Case 1: Previously Working Key (W→A)
**Expected**:
```
[EVENT:START] evdev 17 (PRESS)
[LAYER1:IN] evdev 17 → yamy 0x0011
[LAYER2:SUBST] 0x0011 → 0x001E
[LAYER3:OUT] yamy 0x001E → evdev 30 (KEY_A)
[EVENT:END] Output evdev 30 (PRESS)
```

**Test**: Inject W key PRESS and RELEASE, verify both produce complete logs and output A.

### Test Case 2: Previously Partial Key (R→E)
**Expected**:
- PRESS: Complete LAYER1→LAYER2→LAYER3 flow, output E
- RELEASE: Complete LAYER1→LAYER2→LAYER3 flow, output E

**Test**: Inject R key PRESS and RELEASE, verify **both** events work (baseline: only RELEASE worked).

### Test Case 3: Previously Broken Modifier (N→LShift)
**Expected**:
```
[EVENT:START] evdev 49 (PRESS)
[LAYER1:IN] evdev 49 → yamy 0x0031
[LAYER2:SUBST] 0x0031 → 0x002A (VK_LSHIFT)
[LAYER3:OUT] yamy 0x002A → evdev 42 (KEY_LEFTSHIFT)
[EVENT:END] Output evdev 42 (PRESS)
```

**Test**: Inject N key, verify it outputs LShift (baseline: no substitution, outputs N unchanged).

### Test Case 4: Passthrough Key (Z→Z)
**Expected**:
```
[LAYER2:PASSTHROUGH] 0x002C (no substitution)
```

**Test**: Verify passthrough keys still work.

---

## Quantified Success Metrics (Projected)

**Note**: Cannot measure actual pass rate without completing EventProcessor integration and runtime testing.

**Baseline** (from Task 1.6):
- Total tests: 174 (87 substitutions × 2 event types)
- Passing: ~90 (52%)
- Failing: ~84 (48%)
- Main issues:
  - PRESS/RELEASE asymmetry: ~50-60 failures
  - Modifier substitutions broken: ~20-30 failures

**Expected After Full Integration**:
- Total tests: 174
- Passing: 174 (100%) ← **Target**
- Failing: 0 (0%)

**Improvement**: +84 tests fixed (+48% success rate)

---

## Conclusion

### Phase 2 Status: ⚠️ **INCOMPLETE**

**Completed**:
- ✅ Tasks 2.1-2.5: EventProcessor class structure and layer functions
- ⚠️ Task 2.6: Substitution table integration (PARTIAL)

**Remaining Work**:
- ❌ **Task 2.6 (continuation)**: Fully integrate EventProcessor::processEvent() into engine event loop
- ❌ **Task 2.7**: Runtime validation (blocked by incomplete 2.6)

### Architectural Decision Needed

Two paths forward:

**Option A: Full EventProcessor Integration** (Recommended)
- Refactor engine to preserve evdev codes through event pipeline
- Call EventProcessor::processEvent(evdev, event_type) for all events
- Benefits: Complete layer logging, true event type preservation, clean architecture
- Effort: Medium (requires engine refactoring)

**Option B: Partial Integration** (Quick Fix)
- Keep current engine architecture
- Only use EventProcessor::layer2_applySubstitution() for substitutions
- Layer 1 and 3 handled by existing code
- Benefits: Minimal changes, faster implementation
- Drawbacks: Defeats purpose of unified EventProcessor, incomplete logging

**Recommendation**: Pursue **Option A** to fully realize the benefits of the EventProcessor architecture and meet all requirements.

### Next Steps

1. **Complete Task 2.6**:
   - Refactor engine_generator.cpp to call EventProcessor::processEvent()
   - Preserve evdev codes through event pipeline
   - Ensure event type (PRESS/RELEASE) passed correctly

2. **Fix Logging Infrastructure**:
   - Verify PLATFORM_LOG_DEBUG outputs to stderr
   - Test with simple event injection

3. **Resume Task 2.7**:
   - Run runtime validation tests
   - Document actual pass rates
   - Compare against baseline

4. **Commit**:
   - This validation report
   - Any logging fixes applied

---

## Appendix A: Code Locations

### EventProcessor Implementation
- **Interface**: `src/core/engine/engine_event_processor.h`
- **Implementation**: `src/core/engine/engine_event_processor.cpp`
- **Status**: ✅ Complete and correct (Tasks 2.1-2.5)

### Substitution Table Building
- **File**: `src/core/engine/engine_setting.cpp`
- **Function**: `Engine::buildSubstitutionTable()`
- **Status**: ✅ Complete (Task 2.6)

### Event Processing (Needs Work)
- **File**: `src/core/engine/engine_generator.cpp`
- **Function**: `Engine::beginGeneratingKeyboardEvents()`
- **Lines**: ~305-410 (substitution logic)
- **Status**: ⚠️ Uses m_substitutionTable directly, not EventProcessor::processEvent()

### Logging
- **Platform Logger**: `src/utils/platform_logger.h`
- **Fix Applied**: Set logger level to DEBUG when YAMY_DEBUG_KEYCODE=1
- **Location**: `src/core/engine/engine_setting.cpp:265`

---

**Document Version**: 1.0
**Author**: Claude Sonnet 4.5 (Code Review)
**Status**: Phase 2 validation reveals Task 2.6 incomplete, Task 2.7 blocked
**Action Required**: Complete EventProcessor integration before proceeding to Phase 3

---

🤖 Generated with [Claude Code](https://claude.com/claude-code)
