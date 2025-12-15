# Tasks: Virtual Key System

**Spec Name**: virtual-key-system
**Created**: 2025-12-15
**Status**: Draft

## Task Overview

Total tasks: 12
Estimated effort: 2-3 days
Priority: High (fixes critical bugs)

## Task List

### Phase 1: Foundation (Keycode & State)

#### Task 1.1: Define Virtual Key Ranges
**Status**: [x] Completed
**Priority**: P0
**Files**:
- `src/platform/linux/keycode_mapping.h`

**Description**:
Add keycode range definitions for virtual keys (V_*), modifiers (M00-MFF), and locks (L00-LFF).

**Requirements**: US9
**Dependencies**: None

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a C++ systems developer working on keyboard input processing.

Task: Add keycode range definitions for the new virtual key system.

Context:
- Current keycode mapping: src/platform/linux/keycode_mapping.h
- YAMY uses uint16_t for internal key codes
- Need to allocate three non-overlapping ranges for virtual keys

Requirements:
Add the following definitions to keycode_mapping.h:

1. Virtual regular keys (V_*):
   - Range: 0xE000-0xEFFF (4096 keys)
   - Base: YAMY_VIRTUAL_KEY_BASE = 0xE000
   - Max: YAMY_VIRTUAL_KEY_MAX = 0xEFFF

2. Modal modifiers (M00-MFF):
   - Range: 0xF000-0xF0FF (256 modifiers)
   - Start: YAMY_MOD_00 = 0xF000
   - End: YAMY_MOD_FF = 0xF0FF

3. Lock keys (L00-LFF):
   - Range: 0xF100-0xF1FF (256 locks)
   - Start: YAMY_LOCK_00 = 0xF100
   - End: YAMY_LOCK_FF = 0xF1FF

4. Helper functions (inline):
   - bool isVirtualKey(uint16_t code)
   - bool isModifier(uint16_t code)
   - bool isLock(uint16_t code)
   - uint8_t getModifierNumber(uint16_t code)
   - uint8_t getLockNumber(uint16_t code)

Restrictions:
- Do NOT modify existing keycode ranges
- Do NOT change existing function signatures
- Use inline functions for helpers (no runtime overhead)
- Add clear comments explaining each range

_Leverage:
- Existing keycode definitions in keycode_mapping.h
- Existing helper function patterns

_Requirements: US9 (256 modifiers and locks)

Success Criteria:
- All ranges defined with #define
- Helper functions compile without errors
- No overlap with existing keycode ranges
- Clear comments explain purpose of each range

After completing:
1. Edit tasks.md: Change task 1.1 from [ ] to [-]
2. Implement the code
3. Call log-implementation tool with:
   - taskId: "1.1"
   - summary: "Added virtual key ranges and helper functions"
   - filesModified: ["src/platform/linux/keycode_mapping.h"]
   - artifacts: {functions: [{name, signature, location}]}
4. Edit tasks.md: Change task 1.1 from [-] to [x]
```

---

#### Task 1.2: Implement ModifierState Class Updates
**Status**: [x] Completed
**Priority**: P0
**Files**:
- `src/core/input/modifier_state.h`
- `src/core/input/modifier_state.cpp`

**Description**:
Update ModifierState to support 256 modifiers (M00-MFF) using bitmask array.

**Requirements**: US5, US9
**Dependencies**: Task 1.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a C++ developer specializing in state management.

Task: Update ModifierState class to support 256 modal modifiers using efficient bitmask storage.

Context:
- Current ModifierState uses uint32_t m_modal for 20 modifiers
- Need to expand to 256 modifiers (M00-MFF)
- Must maintain high performance (O(1) operations)

Requirements:
1. Change m_modal from single uint32_t to array:
   - uint32_t m_modal[8];  // 8 * 32 = 256 bits

2. Update methods to use array indexing:
   - activate(uint8_t mod_num): Set bit at position mod_num
   - deactivate(uint8_t mod_num): Clear bit at position mod_num
   - isActive(uint8_t mod_num): Check bit at position mod_num

3. Add getter for bitmask:
   - const uint32_t* getModifierBits() const { return m_modal; }

4. Array indexing logic:
   - word_idx = mod_num / 32
   - bit_idx = mod_num % 32
   - mask = 1u << bit_idx

Restrictions:
- Do NOT change existing method signatures for hardware modifiers
- Do NOT break backward compatibility with Shift/Ctrl/Alt/Win modifiers
- Keep O(1) complexity for all operations
- No dynamic allocation

_Leverage:
- Existing ModifierState class in src/core/input/modifier_state.h
- Existing bit manipulation patterns

_Requirements: US5, US9

Success Criteria:
- All 256 modifiers can be activated/deactivated
- isActive() returns correct state for all M00-MFF
- No performance degradation
- Existing hardware modifier code still works

After completing:
1. Mark task 1.2 as in-progress [-] in tasks.md
2. Implement the code
3. Log implementation with artifacts (classes, methods)
4. Mark task 1.2 as completed [x] in tasks.md
```

---

#### Task 1.3: Implement LockState Class
**Status**: [x] Completed
**Priority**: P0
**Files**:
- `src/core/input/lock_state.h` (new)
- `src/core/input/lock_state.cpp` (new)

**Description**:
Create new LockState class to manage 256 lock keys with toggle functionality and GUI notifications.

**Requirements**: US6, US7, US8, US9
**Dependencies**: Task 1.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a C++ developer creating a new state management class.

Task: Create LockState class for managing 256 toggle-able lock keys with GUI notifications.

Context:
- Similar to ModifierState but for persistent toggle state
- Must notify GUI when locks change (for visual indicators)
- Used in event processing to match lock-based keymaps

Requirements:
Create lock_state.h with:
```cpp
class LockState {
public:
    void toggleLock(uint8_t lock_num);
    bool isLockActive(uint8_t lock_num) const;
    const uint32_t* getLockBits() const { return m_locks; }
    void notifyGUI();

private:
    uint32_t m_locks[8];  // 256 bits for L00-LFF
    void setBit(uint8_t lock_num, bool value);
};
```

Create lock_state.cpp with:
1. toggleLock(): XOR bit to toggle on/off, then call notifyGUI()
2. isLockActive(): Check bit state
3. notifyGUI(): Send IPC message with lock state (see design doc)

Restrictions:
- Do NOT add dependencies on GUI libraries in this class
- Keep IPC notification generic (reuse existing IPC mechanism)
- No dynamic allocation
- Thread-safe not required (single-threaded event processing)

_Leverage:
- ModifierState implementation as reference
- Existing IPC notification patterns in Engine class

_Requirements: US6, US7, US8, US9

Success Criteria:
- LockState can toggle all 256 locks
- isLockActive() returns correct state
- notifyGUI() sends IPC message (verified with logging)
- No memory leaks

After completing:
1. Mark task 1.3 as in-progress [-]
2. Implement the code
3. Log implementation with artifacts (classes created)
4. Mark task 1.3 as completed [x]
```

---

### Phase 2: Parser Updates

#### Task 2.1: Add Token Recognition for V_, M, L Prefixes
**Status**: [x] Completed
**Priority**: P0
**Files**:
- `src/core/settings/setting_loader.h`
- `src/core/settings/setting_loader.cpp`

**Description**:
Update parser to recognize V_KeyName, M00-MFF, and L00-LFF tokens.

**Requirements**: US1, US2, US3
**Dependencies**: Task 1.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a parser developer working on .mayu configuration file parsing.

Task: Add token recognition for new virtual key syntax (V_, M00-MFF, L00-LFF).

Context:
- Parser currently recognizes physical key names (A, B, Enter, etc.)
- Need to add three new token types for virtual keys
- Must convert token strings to keycode values

Requirements:
1. Update token recognition in setting_loader.cpp:
   - If token starts with "V_": Virtual key
     - Extract key name after "V_"
     - Lookup base keycode for key name
     - Return YAMY_VIRTUAL_KEY_BASE + base_code

   - If token matches "M[0-9A-F]{2}": Modifier (M00-MFF)
     - Extract hex digits
     - Convert to number (0x00-0xFF)
     - Return YAMY_MOD_00 + number

   - If token matches "L[0-9A-F]{2}": Lock (L00-LFF)
     - Extract hex digits
     - Convert to number (0x00-0xFF)
     - Return YAMY_LOCK_00 + number

2. Examples:
   - "V_A" → 0xE000 + offset_of_A
   - "M00" → 0xF000
   - "M0F" → 0xF00F
   - "MFF" → 0xF0FF
   - "L00" → 0xF100
   - "LFF" → 0xF1FF

Restrictions:
- Do NOT break existing key name parsing
- Validate hex digits are valid (0-9, A-F, case insensitive)
- Return error for invalid formats (e.g., "M100", "LGG")
- Do NOT modify grammar rules yet (next task)

_Leverage:
- Existing key name lookup functions
- Existing token parsing patterns in setting_loader.cpp

_Requirements: US1, US2, US3

Success Criteria:
- Parser recognizes V_A, V_Enter, etc.
- Parser recognizes M00-MFF (all 256)
- Parser recognizes L00-LFF (all 256)
- Invalid tokens return error with clear message
- Existing key parsing still works

After completing:
1. Mark task 2.1 as in-progress [-]
2. Implement the code
3. Log implementation with artifacts (functions modified)
4. Mark task 2.1 as completed [x]
```

---

#### Task 2.2: Add "mod assign" Grammar Rule
**Status**: [x] Completed
**Priority**: P0
**Files**:
- `src/core/settings/setting_loader.cpp`

**Description**:
Add parser support for "mod assign M00 = *Enter" syntax to define tap actions.

**Requirements**: US4
**Dependencies**: Task 2.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a parser developer adding new grammar rules.

Task: Add "mod assign" statement parsing for modifier tap actions.

Context:
- Need to parse: "mod assign M00 = *Enter"
- This defines what modifier outputs on quick tap
- Result should be stored for ModifierKeyHandler to use

Requirements:
1. Add grammar rule recognition:
   - Pattern: "mod" "assign" MODIFIER "=" "*" KEY
   - Example: "mod assign M00 = *Enter"

2. Parse components:
   - Extract modifier number (00-FF from M00-MFF)
   - Extract tap output key code
   - Store in map: m_modTapActions[mod_num] = key_code

3. Error handling:
   - Error if modifier is not M00-MFF
   - Error if key after '*' is invalid
   - Error if duplicate assignment (warn and overwrite)

4. Storage:
   - Add member: std::unordered_map<uint8_t, uint16_t> m_modTapActions;
   - Pass to Engine during configuration loading

Restrictions:
- Do NOT change existing "mod" syntax for hardware modifiers
- Do NOT require mod assign for modifiers to work (it's optional)
- Do NOT parse "mod mod0 = !!Key" old syntax (removed)

_Leverage:
- Existing grammar parsing patterns in setting_loader.cpp
- Existing key parsing after '*' operator

_Requirements: US4

Success Criteria:
- "mod assign M00 = *Enter" parses successfully
- m_modTapActions map populated correctly
- Parser error for invalid syntax
- Duplicate assignments handled gracefully

After completing:
1. Mark task 2.2 as in-progress [-]
2. Implement the code
3. Log implementation with artifacts
4. Mark task 2.2 as completed [x]
```

---

### Phase 3: Event Processing

#### Task 3.1: Update Substitution Layer to Handle Virtual Keys
**Status**: [x] Completed
**Priority**: P0
**Files**:
- `src/core/engine/engine_event_processor.cpp`

**Description**:
Update Layer 2a substitution to properly handle V_, M, L virtual keys in substitution table.

**Requirements**: US1, US3
**Dependencies**: Task 1.1, Task 2.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a systems developer working on event processing pipeline.

Task: Update substitution layer to handle virtual keys (V_, M00-MFF, L00-LFF).

Context:
- Current substitution: physical key → physical key
- New: physical key → virtual key (V_*, M00-MFF, L00-LFF)
- Substitution table already exists (std::unordered_map<uint16_t, uint16_t>)

Requirements:
1. No code changes needed for substitution lookup!
   - Table already supports any uint16_t → uint16_t mapping
   - Parser populates table with virtual key codes

2. Update documentation/comments:
   - Clarify that substitution can map to virtual keys
   - Add examples in comments

3. Verify existing logic:
   - Check substitution table lookup happens BEFORE modifier/lock processing
   - Ensure virtual keys are not output to evdev layer

Restrictions:
- Do NOT change substitution table data structure
- Do NOT add special cases for virtual keys
- Keep existing performance (O(1) hash lookup)

_Leverage:
- Existing substitution table in engine_event_processor.cpp
- Existing lookup logic

_Requirements: US1, US3

Success Criteria:
- Substitution table can store virtual key mappings
- Lookup returns virtual key codes correctly
- Comments explain virtual key support
- No performance regression

After completing:
1. Mark task 3.1 as in-progress [-]
2. Implement the code
3. Log implementation
4. Mark task 3.1 as completed [x]
```

---

#### Task 3.2: Implement Modifier Processing with Tap/Hold Detection
**Status**: [ ] Pending
**Priority**: P0
**Files**:
- `src/core/engine/modifier_key_handler.h`
- `src/core/engine/modifier_key_handler.cpp`

**Description**:
Replace broken modal modifier logic with new M00-MFF processing including tap/hold detection.

**Requirements**: US4, US5
**Dependencies**: Task 1.2, Task 2.2

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a developer implementing tap/hold key detection logic.

Task: Implement modifier processing with tap/hold detection for M00-MFF.

Context:
- Current ModifierKeyHandler has broken modal modifier logic
- Need to replace with clean M00-MFF implementation
- Must support tap (quick press) vs hold (activate modifier)

Requirements:
1. Add tap/hold state tracking:
```cpp
struct ModState {
    std::chrono::steady_clock::time_point press_time;
    bool is_pressed;
    uint16_t tap_output;  // From mod assign
};
std::unordered_map<uint8_t, ModState> m_modStates;
const int TAP_THRESHOLD_MS = 200;
```

2. Process modifier events:
   - PRESS: Record timestamp, return WAITING_FOR_THRESHOLD
   - RELEASE: Check duration
     - If < threshold: Output tap action (if defined)
     - If ≥ threshold: Deactivate modifier

3. Threshold timer:
   - After TAP_THRESHOLD_MS, activate modifier in ModifierState
   - Suppress output

4. Load tap actions from parser:
   - Store m_modTapActions from SettingLoader

Restrictions:
- Do NOT block event processing thread
- Do NOT use sleep() or blocking waits
- Keep state machine simple (IDLE → PRESSED → TAP/HOLD)
- Do NOT remove existing hardware modifier code

_Leverage:
- Existing event processing patterns
- std::chrono for timing

_Requirements: US4, US5

Success Criteria:
- Quick tap (<200ms) outputs tap action
- Hold (≥200ms) activates modifier without output
- Multiple modifiers can be active simultaneously
- No blocking or thread safety issues

After completing:
1. Mark task 3.2 as in-progress [-]
2. Implement the code
3. Log implementation with detailed artifacts
4. Mark task 3.2 as completed [x]
```

---

#### Task 3.3: Implement Lock Processing
**Status**: [ ] Pending
**Priority**: P0
**Files**:
- `src/core/engine/engine_event_processor.cpp`

**Description**:
Add lock key processing that toggles lock state on press.

**Requirements**: US6
**Dependencies**: Task 1.3

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a developer implementing lock key toggle logic.

Task: Add lock key processing to EventProcessor.

Context:
- After substitution, event might be lock key (L00-LFF)
- Need to toggle lock state on key press
- Lock state persists until next toggle

Requirements:
1. Add lock detection in processEvent():
```cpp
if (isLock(yamy_code)) {
    return processLock(yamy_code, type, lockState);
}
```

2. Implement processLock():
   - If PRESS: Toggle lock state, return 0 (suppress output)
   - If RELEASE: Return 0 (suppress output)

3. Lock state changes:
   - Call lockState->toggleLock(lock_num)
   - notifyGUI() is called automatically inside toggleLock()

Restrictions:
- Do NOT output lock keys to evdev layer
- Do NOT toggle on both PRESS and RELEASE (only PRESS)
- Keep logic simple (no timing required)

_Leverage:
- LockState class from task 1.3
- isLock() helper from task 1.1

_Requirements: US6

Success Criteria:
- Press L00: Toggles lock on/off
- Lock state persists across events
- Lock keys never output to system
- GUI notification sent on toggle

After completing:
1. Mark task 3.3 as in-progress [-]
2. Implement the code
3. Log implementation
4. Mark task 3.3 as completed [x]
```

---

#### Task 3.4: Implement Keymap Matching with Specificity
**Status**: [ ] Pending
**Priority**: P0
**Files**:
- `src/core/engine/engine_event_processor.cpp`
- `src/core/engine/engine.h`

**Description**:
Implement keymap lookup with modifier/lock matching and specificity-based priority.

**Requirements**: US5, US7
**Dependencies**: Task 1.2, Task 1.3

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a developer implementing keymap matching logic.

Task: Implement keymap lookup with modifier/lock combination matching and specificity rules.

Context:
- Keymap entries can require specific modifiers and locks
- More specific matches (more mods/locks) should win
- Need efficient matching algorithm

Requirements:
1. Extend KeymapEntry structure:
```cpp
struct KeymapEntry {
    uint32_t required_mods[8];   // M00-MFF bitmask
    uint32_t required_locks[8];  // L00-LFF bitmask
    uint16_t input_key;
    uint16_t output_key;
    uint8_t specificity;  // popcount(mods) + popcount(locks)
};
```

2. Sort keymap by specificity (DESC) after loading

3. Implement lookupKeymap():
   - Iterate keymap (most specific first)
   - Check input_key matches
   - Check all required modifiers are active
   - Check all required locks are active
   - Return first match

4. Bitmask matching:
   - (active_mods[i] & required_mods[i]) == required_mods[i]
   - Same for locks

Restrictions:
- Do NOT use complex data structures (keep vector)
- Do NOT break existing keymap parsing
- Keep O(n) complexity (acceptable for <1000 entries)

_Leverage:
- Existing keymap data structure
- ModifierState::getModifierBits()
- LockState::getLockBits()

_Requirements: US5, US7

Success Criteria:
- M00-M01-A matches when both M00 and M01 are active
- More specific matches take priority
- L00-L01-A matches when both locks are active
- Mixed M00-L00-A works correctly

After completing:
1. Mark task 3.4 as in-progress [-]
2. Implement the code
3. Log implementation with detailed artifacts
4. Mark task 3.4 as completed [x]
```

---

#### Task 3.5: Update Layer 3 to Suppress Virtual Keys
**Status**: [ ] Pending
**Priority**: P0
**Files**:
- `src/core/engine/engine_event_processor.cpp`

**Description**:
Update YAMY→evdev conversion to suppress virtual keys (don't output them).

**Requirements**: US1
**Dependencies**: Task 1.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a developer updating output layer logic.

Task: Suppress virtual keys in YAMY→evdev conversion layer.

Context:
- Virtual keys (V_*, M00-MFF, L00-LFF) have no evdev codes
- Should never be output to system
- Layer 3 converts YAMY codes to evdev codes

Requirements:
1. Update layer3_yamyToEvdev():
```cpp
if (isVirtualKey(yamy) || isModifier(yamy) || isLock(yamy)) {
    return 0;  // Suppress output
}
return yamyToEvdevKeyCode(yamy);
```

2. Ensure 0 return value is handled:
   - Check calling code handles 0 correctly
   - Should skip output when 0 is returned

Restrictions:
- Do NOT change existing yamyToEvdevKeyCode() function
- Do NOT suppress physical keys
- Keep existing error handling

_Leverage:
- Helper functions: isVirtualKey(), isModifier(), isLock()
- Existing layer3_yamyToEvdev() function

_Requirements: US1

Success Criteria:
- V_A, V_B, etc. return 0 (not output)
- M00-MFF return 0 (not output)
- L00-LFF return 0 (not output)
- Physical keys still output correctly

After completing:
1. Mark task 3.5 as in-progress [-]
2. Implement the code
3. Log implementation
4. Mark task 3.5 as completed [x]
```

---

### Phase 4: GUI Integration

#### Task 4.1: Add Lock Status IPC Message
**Status**: [ ] Pending
**Priority**: P1
**Files**:
- `src/core/ipc/ipc_messages.h`
- `src/core/engine/engine.h`

**Description**:
Define IPC message structure for lock status updates.

**Requirements**: US8
**Dependencies**: Task 1.3

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a developer adding IPC message types.

Task: Add LockStatusUpdate message for GUI communication.

Context:
- GUI needs to know which locks are active
- LockState calls notifyGUI() when state changes
- Use existing IPC infrastructure

Requirements:
1. Add to MessageType enum:
```cpp
enum class MessageType : uint32_t {
    // ... existing types
    LockStatusUpdate = 0x0200,
};
```

2. Define message structure:
```cpp
struct LockStatusMessage {
    uint32_t lockBits[8];  // 256 bits for L00-LFF
};
```

3. Implement notifyGUI() in LockState:
   - Create LockStatusMessage
   - Copy m_locks to lockBits
   - Send via Engine::notifyGUI()

Restrictions:
- Do NOT break existing IPC messages
- Keep message size small (<64 bytes)
- Use existing IPC send mechanism

_Leverage:
- Existing MessageType enum
- Existing notifyGUI() patterns in Engine

_Requirements: US8

Success Criteria:
- LockStatusMessage defined
- notifyGUI() sends message via IPC
- Message size is minimal
- No IPC compatibility issues

After completing:
1. Mark task 4.1 as in-progress [-]
2. Implement the code
3. Log implementation
4. Mark task 4.1 as completed [x]
```

---

#### Task 4.2: Implement Lock Indicator Widget (Qt)
**Status**: [ ] Pending
**Priority**: P1
**Files**:
- `src/ui/qt/lock_indicator_widget.h` (new)
- `src/ui/qt/lock_indicator_widget.cpp` (new)

**Description**:
Create Qt widget to display lock status with visual indicators.

**Requirements**: US8
**Dependencies**: Task 4.1

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a Qt GUI developer creating status indicators.

Task: Create LockIndicatorWidget to show which locks are active.

Context:
- Need visual feedback for lock state
- Show L00-LFF with green (active) or gray (inactive) indicators
- Update in real-time via IPC

Requirements:
1. Create lock_indicator_widget.h:
```cpp
class LockIndicatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit LockIndicatorWidget(QWidget* parent = nullptr);
    void updateLockStatus(const uint32_t lockBits[8]);

private:
    struct Indicator {
        QLabel* label;    // "L00"
        QLabel* status;   // Colored dot
    };
    std::vector<Indicator> m_indicators;
    void setLockActive(uint8_t lock_num, bool active);
};
```

2. Implementation:
   - Create indicators for locks on demand (not all 256 upfront)
   - Show only active locks or most recently used
   - Update colors: green = active, gray = inactive

3. UI layout:
   - Vertical list of indicators
   - Each row: "Lxx [●]" where ● is colored dot
   - Compact and minimal

Restrictions:
- Do NOT create all 256 indicators at startup
- Do NOT block UI thread with updates
- Keep visual design simple and clear

_Leverage:
- Existing Qt widget patterns in src/ui/qt/
- Existing IPC receive handling

_Requirements: US8

Success Criteria:
- Widget displays lock indicators
- Colors update when locks toggle
- UI is responsive and clean
- Memory usage is reasonable

After completing:
1. Mark task 4.2 as in-progress [-]
2. Implement the code
3. Log implementation with artifacts (components created)
4. Mark task 4.2 as completed [x]
```

---

### Phase 5: Testing & Documentation

#### Task 5.1: Create E2E Test Scenarios
**Status**: [ ] Pending
**Priority**: P1
**Files**:
- `tests/scenarios/virtual-keys-basic.json` (new)
- `tests/scenarios/modifiers-tap-hold.json` (new)
- `tests/scenarios/locks-combinatorial.json` (new)

**Description**:
Create E2E test scenarios for virtual key system using existing test infrastructure.

**Requirements**: All user stories
**Dependencies**: All implementation tasks

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a QA engineer writing E2E tests.

Task: Create test scenarios for virtual key system.

Context:
- Existing E2E test infrastructure in tests/
- Use JSON scenario format
- Test all major features

Requirements:
Create three test scenario files:

1. virtual-keys-basic.json:
   - Test V_ virtual keys
   - Verify physical vs virtual distinction
   - Config: def subst *A = *V_B, key M00-B = *C, key M00-V_B = *D

2. modifiers-tap-hold.json:
   - Test M00 tap/hold
   - Verify tap outputs Enter
   - Verify hold activates modifier
   - Config: def subst *B = *M00, mod assign M00 = *Enter

3. locks-combinatorial.json:
   - Test L00, L01 toggle
   - Verify single lock: key L00-A = *B
   - Verify combo: key L00-L01-A = *C
   - Config: def subst *CapsLock = *L00, def subst *ScrollLock = *L01

Each test includes:
- Config file
- Input sequence
- Expected output
- Success criteria

Restrictions:
- Use existing test framework format
- Do NOT create new test infrastructure
- Keep tests atomic and focused

_Leverage:
- Existing test scenarios in tests/scenarios/
- Existing test runner framework
- E2E testing infrastructure from E2E_TESTING_SUMMARY.md

_Requirements: All user stories

Success Criteria:
- All test scenarios pass
- Tests cover major features
- Test output is clear and actionable
- Tests can run automatically

After completing:
1. Mark task 5.1 as in-progress [-]
2. Create test files
3. Log implementation
4. Mark task 5.1 as completed [x]
```

---

#### Task 5.2: Update Documentation and Migration Guide
**Status**: [ ] Pending
**Priority**: P2
**Files**:
- `docs/VIRTUAL_KEYS.md` (new)
- `docs/MIGRATION.md` (new)

**Description**:
Create user documentation and migration guide for virtual key system.

**Requirements**: All user stories
**Dependencies**: All implementation tasks

**_Prompt**:
```
Implement the task for spec virtual-key-system, first run spec-workflow-guide to get the workflow guide then implement the task:

Role: You are a technical writer creating user documentation.

Task: Write comprehensive documentation for virtual key system.

Context:
- Users need to migrate from old modal modifier syntax
- New users need clear examples
- Both simple and advanced use cases

Requirements:
Create two documentation files:

1. VIRTUAL_KEYS.md:
   - Overview of V_, M00-MFF, L00-LFF
   - Syntax examples
   - Common patterns (Space Cadet, Vim mode, etc.)
   - Troubleshooting

2. MIGRATION.md:
   - Old syntax → New syntax table
   - Migration steps
   - Example conversions
   - Breaking changes

Include:
- Clear examples for each feature
- Visual diagrams (ASCII art acceptable)
- Common pitfalls and solutions
- FAQ section

Restrictions:
- Do NOT create video tutorials
- Keep language simple and clear
- Include copy-paste examples

_Leverage:
- Design document for technical details
- Requirements for user stories
- Existing documentation style

_Requirements: All user stories

Success Criteria:
- Documentation is clear and complete
- Migration guide covers all breaking changes
- Examples are copy-paste ready
- No technical jargon without explanation

After completing:
1. Mark task 5.2 as in-progress [-]
2. Write documentation
3. Log implementation
4. Mark task 5.2 as completed [x]
```

---

## Progress Tracking

Track implementation progress by editing task status:
- `[ ]` = Pending
- `[-]` = In Progress
- `[x]` = Completed

Use `spec-status` tool to view overall progress.

## Implementation Notes

1. **Order matters**: Complete tasks in phase order for clean integration
2. **Test early**: Run E2E tests after each phase
3. **Document as you go**: Update code comments during implementation
4. **Use existing patterns**: Leverage current codebase architecture

## Success Criteria

- [ ] All tasks completed
- [ ] All E2E tests pass
- [ ] No performance regression
- [ ] GUI lock indicators working
- [ ] Documentation complete
- [ ] Zero critical bugs

## Estimated Timeline

- Phase 1 (Foundation): 4-6 hours
- Phase 2 (Parser): 3-4 hours
- Phase 3 (Event Processing): 6-8 hours
- Phase 4 (GUI): 3-4 hours
- Phase 5 (Testing/Docs): 4-5 hours

**Total**: ~20-27 hours (2.5-3.5 days)
