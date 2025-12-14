# Tasks Document: Modal Modifier Remapping

## Overview

This document breaks down the modal modifier remapping feature into atomic, testable tasks. Each task is designed to be completed in 1-3 files with clear success criteria.

**Implementation Strategy**: Bottom-up approach - build foundational components first (state tracking, handler), then integrate into engine, then add configuration layer connections.

**Total Estimated Tasks**: 12 core tasks + 3 testing tasks = 15 tasks

---

## Phase 1: Foundation - Data Structures and State Tracking (Tasks 1-3)

- [x] 1. Enhance ModifierState struct to support modal modifiers
  - **Files**:
    - `src/core/engine/engine_modifier.cpp` (modify existing struct)
    - `src/core/engine/engine_modifier.h` (add modal field)
  - **Description**: Add uint32_t modal field to ModifierState struct and implement activate/deactivate methods for mod0-mod19
  - **Acceptance Criteria**:
    - ModifierState has modal field (32-bit bitmask for mod0-mod19)
    - activate(Modifier::Type_ModX) sets correct bit
    - deactivate(Modifier::Type_ModX) clears correct bit
    - isActive(Modifier::Type_ModX) returns correct state
    - All tests pass with >95% coverage
  - **_Leverage**: Existing ModifierState structure in engine_modifier.cpp_
  - **_Requirements**: Requirement 4 (Modifier State Tracking)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: C++ Systems Engineer specializing in low-level state management and bit manipulation

    Task: Enhance the ModifierState struct in src/core/engine/engine_modifier.cpp to support 20 modal modifiers (mod0-mod19) using a uint32_t bitmask field. Implement activate/deactivate methods following requirement 4.

    Context:
    - Currently ModifierState only tracks standard modifiers (Shift, Ctrl, Alt, Win)
    - Need to add modal modifier tracking without breaking existing functionality
    - Modal modifiers are Modifier::Type_Mod0 through Modifier::Type_Mod19 (enum values 16-35)

    Implementation:
    1. Add uint32_t modal field to ModifierState struct
    2. Implement activate(Modifier::Type type) method:
       - If type is Type_Mod0..Mod19: set bit (type - Type_Mod0) in modal field
       - If type is standard modifier: update existing standard field
    3. Implement deactivate(Modifier::Type type) method (reverse of activate)
    4. Implement bool isActive(Modifier::Type type) const method
    5. Add getActiveBitmask() method that returns modal field
    6. Add clear() method to reset all modifiers

    Restrictions:
    - DO NOT modify existing standard modifier behavior
    - DO NOT change method signatures of existing functions
    - Maintain backward compatibility with existing code
    - Use bitwise operations (no loops for bit manipulation)

    Leverage:
    - Existing Modifier::Type enum in src/core/input/keyboard.h (Type_Mod0 = 16, Type_Mod19 = 35)
    - Existing ModifierState struct in src/core/engine/engine_modifier.cpp

    Success Criteria:
    - ModifierState struct compiles without errors
    - All new methods are implemented and tested
    - Backward compatibility maintained (existing code still works)
    - Unit tests demonstrate correct bitmask manipulation

    Instructions:
    1. Mark task 1 as in-progress in .spec-workflow/specs/modal-modifier-remapping/tasks.md (change [ ] to [-])
    2. Implement the changes to ModifierState
    3. Write unit tests in tests/test_modifier_state.cpp (create file if needed)
    4. Run tests and verify all pass
    5. Log implementation using log-implementation tool with detailed artifacts
    6. Mark task 1 as complete in tasks.md (change [-] to [x])
    ```

- [x] 2. Create ModifierState unit test suite
  - **Files**:
    - `tests/test_modifier_state.cpp` (create new file)
  - **Description**: Comprehensive unit tests for ModifierState modal modifier functionality
  - **Acceptance Criteria**:
    - 15 unit tests covering activate/deactivate/isActive/clear methods
    - All edge cases tested (activate twice, deactivate inactive, all 20 modifiers concurrently)
    - 100% coverage of new ModifierState methods
    - All tests pass in <100ms
  - **_Leverage**: tests/test_main.cpp for Google Test setup_
  - **_Requirements**: Requirement 4 (Modifier State Tracking)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: QA Engineer specializing in C++ unit testing and Google Test framework

    Task: Create comprehensive unit tests for ModifierState modal modifier functionality in tests/test_modifier_state.cpp, covering all methods and edge cases following requirement 4.

    Context:
    - ModifierState was just enhanced with modal modifier support (Task 1)
    - Need to verify bitmask manipulation is correct
    - Must test edge cases like concurrent modifiers and invalid states

    Implementation:
    1. Create tests/test_modifier_state.cpp with Google Test framework
    2. Write 15 unit tests:
       - ActivateSingleModifier (mod0, mod9, mod19)
       - DeactivateSingleModifier
       - ActivateMultipleConcurrent (mod0+mod9+mod19)
       - IsActiveReturnsCorrect
       - ActivateTwice (should be idempotent)
       - DeactivateInactive (should be safe)
       - All20ModifiersConcurrent (stress test)
       - GetActiveBitmaskCorrect
       - ClearResetsAll
       - StandardAndModalCombined (Ctrl + mod9)
       - ActivateInvalidType (Type_Mod20, should gracefully handle)
       - BitmaskOverflow (all 32 bits set, should handle)
       - ThreadSafety (if applicable)
       - StateQuery methods
       - Serialization (if needed for debugging)

    Restrictions:
    - Use Google Test macros (TEST, EXPECT_EQ, ASSERT_TRUE, etc.)
    - DO NOT test standard modifier behavior (already tested)
    - Focus ONLY on modal modifier functionality
    - Each test must be independent (no shared state)

    Leverage:
    - tests/test_main.cpp for test runner setup
    - Existing test patterns in tests/ directory

    Success Criteria:
    - All 15 tests compile and run
    - 100% coverage of ModifierState modal methods
    - Tests execute in <100ms total
    - No test failures, no memory leaks (valgrind clean)

    Instructions:
    1. Mark task 2 as in-progress in tasks.md
    2. Create tests/test_modifier_state.cpp
    3. Write all 15 unit tests
    4. Run: ./build/yamy_test --gtest_filter="ModifierStateTest.*"
    5. Verify all pass and coverage is 100%
    6. Log implementation with artifacts (test file created, test count)
    7. Mark task 2 as complete
    ```

- [x] 3. Verify ModifierKeyHandler exists and add getter for testing
  - **Files**:
    - `src/core/engine/modifier_key_handler.h` (add getters for testing)
    - `src/core/engine/modifier_key_handler.cpp` (implement getters)
  - **Description**: Verify ModifierKeyHandler implementation is complete and add query methods for testing
  - **Acceptance Criteria**:
    - ModifierKeyHandler compiles and links successfully
    - Added getKeyStates() method for testing (returns const reference)
    - Added isWaitingForThreshold() method
    - Existing tests pass (tests/test_number_modifiers.cpp)
  - **_Leverage**: Existing ModifierKeyHandler implementation_
  - **_Requirements**: Requirement 3 (Hold vs Tap Detection State Machine)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: C++ Developer with expertise in API design and testability

    Task: Verify ModifierKeyHandler implementation is complete and add query methods for testing in src/core/engine/modifier_key_handler.h/cpp.

    Context:
    - ModifierKeyHandler already exists and is fully implemented
    - Need to add getter methods to enable testing and integration
    - Must NOT change existing functionality

    Implementation:
    1. Read src/core/engine/modifier_key_handler.h and verify it has:
       - registerNumberModifier() method
       - processNumberKey() method
       - isNumberModifier() method
    2. Add new query methods:
       - const std::unordered_map<uint16_t, KeyState>& getKeyStates() const; // For testing
       - bool isWaitingForThreshold(uint16_t yama_code) const;
    3. Implement getters in modifier_key_handler.cpp
    4. Verify existing tests still pass: tests/test_number_modifiers.cpp

    Restrictions:
    - DO NOT modify existing methods (registerNumberModifier, processNumberKey)
    - Only ADD new query methods
    - Getters must be const (read-only)
    - DO NOT change state machine logic

    Leverage:
    - Existing ModifierKeyHandler at src/core/engine/modifier_key_handler.cpp
    - Existing tests at tests/test_number_modifiers.cpp

    Success Criteria:
    - ModifierKeyHandler compiles without errors
    - New getter methods implemented and return correct data
    - Existing tests still pass (./yamy_test --gtest_filter="NumberModifierTest.*")
    - No regressions in functionality

    Instructions:
    1. Mark task 3 as in-progress
    2. Add getter methods to modifier_key_handler.h
    3. Implement getters in modifier_key_handler.cpp
    4. Run existing tests to verify no regressions
    5. Log implementation with artifacts (methods added)
    6. Mark task 3 as complete
    ```

## Phase 2: Event Processing Integration (Tasks 4-7)

- [x] 4. Extend EventProcessor to accept ModifierKeyHandler
  - **Files**:
    - `src/core/engine/engine_event_processor.h` (add handler member + setter)
    - `src/core/engine/engine_event_processor.cpp` (implement setter)
  - **Description**: Add ModifierKeyHandler member to EventProcessor and implement setter method
  - **Acceptance Criteria**:
    - EventProcessor has unique_ptr<ModifierKeyHandler> m_modifierHandler member
    - setModifierHandler(unique_ptr<ModifierKeyHandler> handler) method implemented
    - Handler is stored and can be accessed
    - Compiles without errors
  - **_Leverage**: Existing EventProcessor class structure_
  - **_Requirements**: Requirement 6 (Integration with Event Processing Pipeline)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: C++ Software Engineer specializing in dependency injection and RAII patterns

    Task: Extend EventProcessor to accept and store ModifierKeyHandler in src/core/engine/engine_event_processor.h/cpp following requirement 6.

    Context:
    - EventProcessor currently has no modifier detection capability
    - Need to add ModifierKeyHandler as an injectable dependency
    - Use unique_ptr for ownership transfer

    Implementation:
    1. Add to engine_event_processor.h:
       - #include <memory> (if not already)
       - Forward declaration: class ModifierKeyHandler;
       - Private member: std::unique_ptr<ModifierKeyHandler> m_modifierHandler;
       - Public method: void setModifierHandler(std::unique_ptr<ModifierKeyHandler> handler);
    2. Implement in engine_event_processor.cpp:
       - setModifierHandler() should use std::move() to transfer ownership
    3. Add null check helper method for safety:
       - bool hasModifierHandler() const { return m_modifierHandler != nullptr; }

    Restrictions:
    - DO NOT instantiate ModifierKeyHandler in EventProcessor (dependency injection only)
    - Use unique_ptr, NOT raw pointers
    - DO NOT modify existing EventProcessor methods yet (Layer 2 integration is next task)

    Leverage:
    - Existing EventProcessor at src/core/engine/engine_event_processor.cpp
    - Modern C++ smart pointers (unique_ptr, move semantics)

    Success Criteria:
    - EventProcessor compiles with new member
    - setModifierHandler() accepts and stores handler
    - hasModifierHandler() returns correct state
    - No memory leaks (handler is owned by EventProcessor)

    Instructions:
    1. Mark task 4 as in-progress
    2. Modify engine_event_processor.h to add handler member and setter
    3. Implement setter in engine_event_processor.cpp
    4. Compile and verify no errors
    5. Log implementation with artifacts (member added, setter implemented)
    6. Mark task 4 as complete
    ```

- [x] 5. Integrate modifier detection into EventProcessor::layer2
  - **Files**:
    - `src/core/engine/engine_event_processor.cpp` (modify layer2_applySubstitution method)
  - **Description**: Add modifier detection logic to Layer 2 processing before substitution lookup
  - **Acceptance Criteria**:
    - layer2_applySubstitution() checks if key is number/modal modifier
    - If WAITING → suppresses event (returns 0)
    - If TAP_DETECTED → proceeds to substitution
    - If ACTIVATE/DEACTIVATE → updates modifier state and returns VK code
    - Correct processing order maintained
    - Integration test passes
  - **_Leverage**: Existing layer2_applySubstitution method, ModifierKeyHandler API_
  - **_Requirements**: Requirement 6 (Integration with Event Processing Pipeline)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: C++ Systems Programmer with expertise in event processing pipelines

    Task: Integrate modifier detection into EventProcessor::layer2_applySubstitution method in src/core/engine/engine_event_processor.cpp following requirement 6.

    Context:
    - layer2_applySubstitution currently only does substitution lookup
    - Need to add modifier detection BEFORE substitution
    - Processing order critical: modifier detection → substitution → output

    Implementation:
    1. Modify layer2_applySubstitution(uint16_t yama_in, EventType type, ModifierState* io_modState):
       - At start of method, check: if (!m_modifierHandler) { /* existing logic */ }
       - Add check: if (m_modifierHandler->isNumberModifier(yama_in))
       - Call: auto result = m_modifierHandler->processNumberKey(yama_in, type);
       - Handle result:
         - WAITING_FOR_THRESHOLD → return 0 (suppress event)
         - APPLY_SUBSTITUTION_ON_DOWN or APPLY_SUBSTITUTION_ON_UP → fall through to substitution
         - ACTIVATE_MODIFIER → io_modState->activate(result.modifierType); return result.output_yama_code;
         - DEACTIVATE_MODIFIER → io_modState->deactivate(result.modifierType); return result.output_yama_code;
         - APPLY_SUBSTITUTION → fall through to substitution lookup
    2. Ensure existing substitution logic still works for non-modifier keys

    Critical Order:
    ```cpp
    uint16_t layer2_applySubstitution(uint16_t yama_in, EventType type, ModifierState* io_modState) {
        // 1. Check if modifier handler exists
        if (!m_modifierHandler) {
            // Fallback: normal substitution
        }

        // 2. Check if key is registered modifier
        if (m_modifierHandler->isNumberModifier(yama_in)) {
            auto result = m_modifierHandler->processNumberKey(yama_in, type);
            // Handle result (see above)
        }

        // 3. Normal substitution lookup (for non-modifiers or TAP)
        auto it = m_substitutions.find(yama_in);
        return (it != m_substitutions.end()) ? it->second : yama_in;
    }
    ```

    Restrictions:
    - MUST check m_modifierHandler != nullptr before using
    - DO NOT change existing substitution logic
    - Preserve event type (DOWN/UP) through pipeline
    - Handle all ProcessingAction enum values

    Leverage:
    - ModifierKeyHandler::processNumberKey() returns NumberKeyResult
    - ProcessingAction enum (WAITING, TAP, ACTIVATE, DEACTIVATE, APPLY_SUBSTITUTION)
    - Existing substitution table m_substitutions

    Success Criteria:
    - Modifier detection runs before substitution
    - All ProcessingAction cases handled correctly
    - WAITING suppresses event (no output)
    - TAP falls through to substitution
    - ACTIVATE/DEACTIVATE updates state and returns VK code
    - Existing substitution tests still pass

    Instructions:
    1. Mark task 5 as in-progress
    2. Modify layer2_applySubstitution in engine_event_processor.cpp
    3. Add modifier detection logic at start of method
    4. Handle all result cases correctly
    5. Test with: ./yamy --test-config test_modal.mayu
    6. Log implementation with artifacts (method modified, processing action handling)
    7. Mark task 5 as complete
    ```

- [x] 6. Add ModifierState parameter to EventProcessor methods
  - **Files**:
    - `src/core/engine/engine_event_processor.h` (update method signatures)
    - `src/core/engine/engine_event_processor.cpp` (update implementations)
  - **Description**: Thread ModifierState through EventProcessor methods for state propagation
  - **Acceptance Criteria**:
    - processEvent() accepts ModifierState* io_modState parameter
    - layer2_applySubstitution() accepts and updates ModifierState
    - Modifier state changes are propagated correctly
    - All callers updated to pass ModifierState
  - **_Leverage**: Existing EventProcessor method signatures_
  - **_Requirements**: Requirement 4 (Modifier State Tracking)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: C++ Refactoring Specialist with expertise in API evolution

    Task: Add ModifierState parameter to EventProcessor methods in src/core/engine/engine_event_processor.h/cpp to enable state propagation following requirement 4.

    Context:
    - EventProcessor currently doesn't track modifier state
    - Need to thread ModifierState through pipeline for activate/deactivate
    - Maintain backward compatibility where possible

    Implementation:
    1. Update method signatures in engine_event_processor.h:
       - processEvent(uint16_t evdev_code, EventType type, ModifierState* io_modState);
       - layer2_applySubstitution(uint16_t yama_in, EventType type, ModifierState* io_modState);
    2. Update implementations in engine_event_processor.cpp:
       - processEvent() passes io_modState to layer2_applySubstitution
       - layer2_applySubstitution() updates io_modState when ACTIVATE/DEACTIVATE
    3. Find all call sites and update:
       - Search for "processEvent(" calls
       - Pass current ModifierState from caller

    Restrictions:
    - Use pointer (not reference) to allow nullptr check if needed
    - DO NOT modify Layer 1 or Layer 3 signatures (only Layer 2)
    - Maintain const-correctness (io_ prefix means input/output)

    Leverage:
    - Existing ModifierState struct from Task 1
    - Existing EventProcessor method structure

    Success Criteria:
    - All method signatures updated
    - ModifierState propagated through pipeline
    - activate/deactivate calls update state correctly
    - All call sites updated
    - Compiles without errors

    Instructions:
    1. Mark task 6 as in-progress
    2. Update method signatures in .h file
    3. Update implementations in .cpp file
    4. Find and update all call sites (use grep/search)
    5. Compile and verify
    6. Log implementation with artifacts (methods updated, call sites changed)
    7. Mark task 6 as complete
    ```

- [x] 7. Create EventProcessor integration test
  - **Files**:
    - `tests/test_event_processor_modal.cpp` (create new file)
  - **Description**: Integration tests for EventProcessor with modifier detection
  - **Acceptance Criteria**:
    - 10 integration tests covering modifier detection in Layer 2
    - Tests verify WAITING suppresses events
    - Tests verify TAP proceeds to substitution
    - Tests verify ACTIVATE/DEACTIVATE updates state
    - All tests pass
  - **_Leverage**: Existing test patterns in tests/ directory_
  - **_Requirements**: Requirement 6 (Integration with Event Processing Pipeline)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: Integration Test Engineer with C++ and Google Test expertise

    Task: Create integration tests for EventProcessor modifier detection in tests/test_event_processor_modal.cpp covering all processing actions following requirement 6.

    Context:
    - EventProcessor now has modifier detection integrated (Tasks 4-6)
    - Need to verify all ProcessingAction cases work correctly
    - Must test interaction between modifier detection and substitution

    Implementation:
    1. Create tests/test_event_processor_modal.cpp
    2. Write 10 integration tests:
       - ModifierKeyWaiting_SuppressesEvent (returns 0)
       - ModifierKeyTap_AppliesSubstitution (falls through)
       - ModifierKeyActivate_UpdatesState (io_modState changed)
       - ModifierKeyDeactivate_ClearsState
       - NonModifierKey_UsesSubstitution (no interference)
       - RapidTapTapHold_HandlesCorrectly
       - MultipleModifiersIndependent
       - StatePreservedAcrossEvents
       - NullModifierHandler_FallsBackSafely
       - ModifierStateNullptr_HandlesGracefully

    3. Use mock/stub objects:
       - Mock ModifierKeyHandler (return controlled results)
       - Mock substitution table
       - Real ModifierState object

    Restrictions:
    - Test EventProcessor in isolation (no Engine dependency)
    - Use mocks for dependencies
    - Each test is independent (no shared state)

    Leverage:
    - Google Test mocking framework (if available)
    - Existing test utilities in tests/
    - EventProcessor class from engine_event_processor.cpp

    Success Criteria:
    - All 10 tests compile and pass
    - Cover all ProcessingAction enum values
    - Verify state updates correctly
    - Tests run in <200ms total

    Instructions:
    1. Mark task 7 as in-progress
    2. Create tests/test_event_processor_modal.cpp
    3. Write all 10 integration tests
    4. Run: ./yamy_test --gtest_filter="EventProcessorModalTest.*"
    5. Verify all pass
    6. Log implementation with artifacts (test file, test count)
    7. Mark task 7 as complete
    ```

## Phase 3: Configuration Layer Integration (Tasks 8-10)

- [x] 8. Connect modal modifier definitions to ModifierKeyHandler registration
  - **Files**:
    - `src/core/engine/engine_setting.cpp` (modify buildSubstitutionTable or similar)
  - **Description**: Register modal modifiers from config into ModifierKeyHandler during engine initialization
  - **Acceptance Criteria**:
    - Modal modifier definitions (mod modX = !!key) are read from config
    - Each definition results in registerNumberModifier() call
    - ModifierKeyHandler has all modal modifiers registered after config load
    - Config reload updates registrations
  - **_Leverage**: Existing config loading in engine_setting.cpp, Keyboard::getModalModifiers()_
  - **_Requirements**: Requirement 1 (Modal Modifier Definition and Parsing)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: Configuration Integration Engineer with C++ and YAMY architecture expertise

    Task: Connect modal modifier definitions from .mayu config to ModifierKeyHandler registration in src/core/engine/engine_setting.cpp following requirement 1.

    Context:
    - Modal modifiers are parsed by SettingLoader and stored in Keyboard object
    - Need to bridge config → runtime registration gap
    - Registration happens during Engine::loadSetting() or similar

    Implementation:
    1. Find method that builds EventProcessor (likely buildSubstitutionTable or loadSetting)
    2. After EventProcessor creation, get modal modifiers from config:
       ```cpp
       const auto& keyboard = setting.getKeyboard();
       // Iterate modal modifiers (if method exists)
       ```
    3. For each modal modifier:
       - Extract trigger key scan code
       - Extract modifier type (Type_Mod0..Mod19)
       - Call: eventProcessor->registerNumberModifier(triggerKey, modifierType);
    4. Do same for number modifiers:
       ```cpp
       for (const auto& numMod : keyboard.getNumberModifiers()) {
           uint16_t numberKey = numMod.m_numberKey->getScanCodes()[0].m_scan;
           uint16_t modifierKey = numMod.m_modifierKey->getScanCodes()[0].m_scan;
           eventProcessor->registerNumberModifier(numberKey, modifierKey);
       }
       ```

    Critical: Modal modifiers and number modifiers use SAME registration method (registerNumberModifier) but different parameters:
    - Modal: registerNumberModifier(triggerKey, 0) + store modifierType separately
    - OR: Extend registerNumberModifier to accept Modifier::Type

    Restrictions:
    - DO NOT modify SettingLoader (parsing already works)
    - Only add registration calls, do not change config structure
    - Handle empty config (zero modifiers) gracefully

    Leverage:
    - Keyboard class methods: getNumberModifiers(), possibly getModalModifiers()
    - Existing config loading in engine_setting.cpp
    - ModifierKeyHandler::registerNumberModifier()

    Success Criteria:
    - All modal modifiers from config are registered
    - All number modifiers from config are registered
    - Config with 20 modal + 100 number modifiers loads correctly
    - Config reload updates registrations (old cleared, new added)

    Instructions:
    1. Mark task 8 as in-progress
    2. Find config loading method in engine_setting.cpp
    3. Add modal modifier registration loop
    4. Add number modifier registration loop
    5. Test with config: mod mod9 = !!A, def numbermod *_1 = *LShift
    6. Verify registrations via debug logging or breakpoint
    7. Log implementation with artifacts (registration loops added)
    8. Mark task 8 as complete
    ```

- [x] 9. Extend Keymap hash key generation to include modal modifiers
  - **Files**:
    - `src/core/input/keymap.cpp` (modify lookup method)
    - `src/core/input/keymap.h` (update buildHashKey signature if needed)
  - **Description**: Include modal modifier bitmask in keymap hash key for matching
  - **Acceptance Criteria**:
    - buildHashKey() accepts modal modifier bitmask parameter
    - Hash key is 64-bit: (modal_mods << 32) | (standard_mods << 16) | scancode
    - lookup() passes ModifierState.modal to buildHashKey
    - Keymap matching works with modal modifiers
  - **_Leverage**: Existing Keymap::lookup and hash key generation_
  - **_Requirements**: Requirement 5 (Modifier Combination Keymap Lookup)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: Data Structures Engineer with hash table and performance optimization expertise

    Task: Extend Keymap hash key generation to include modal modifiers in src/core/input/keymap.cpp following requirement 5.

    Context:
    - Keymap currently uses 32-bit hash: (modifiers << 16) | scancode
    - Need 64-bit hash to fit 20 modal modifiers: (modal << 32) | (standard << 16) | scancode
    - Must maintain backward compatibility with non-modal keymaps

    Implementation:
    1. Change hash key type from uint32_t to uint64_t:
       - Update m_bindings map: unordered_map<uint64_t, Action*>
       - Update buildHashKey() return type to uint64_t
    2. Update buildHashKey() signature:
       - Add parameter: uint32_t modal_modifiers
       - Calculate: return ((uint64_t)modal_modifiers << 32) | (standard_modifiers << 16) | scancode;
    3. Update lookup() method:
       - Accept ModifierState parameter (not just standard modifiers)
       - Pass modState.modal to buildHashKey()
    4. Update all call sites of lookup() to pass ModifierState

    Hash Key Format (64-bit):
    ```
    Bits 63-32: Modal modifiers (mod0-mod19, bits 0-19 used)
    Bits 31-16: Standard modifiers (Shift, Ctrl, Alt, Win, bits 0-7 used)
    Bits 15-0:  Scan code
    ```

    Restrictions:
    - Maintain backward compatibility (non-modal entries have modal bits = 0)
    - DO NOT change Action* value storage
    - Hash function must be deterministic
    - Consider hash collision (unlikely with 64-bit, but handle gracefully)

    Leverage:
    - Existing Keymap class at src/core/input/keymap.cpp
    - std::unordered_map hash function (automatically handles uint64_t)
    - ModifierState struct from Task 1

    Success Criteria:
    - Keymap uses 64-bit hash keys
    - Modal modifier combinations are uniquely hashed
    - lookup() matches entries with modal modifiers
    - Backward compatibility maintained (standard-only entries still work)
    - No hash collisions in test cases

    Instructions:
    1. Mark task 9 as in-progress
    2. Update hash key type to uint64_t in keymap.h
    3. Update buildHashKey() to accept modal parameter
    4. Update lookup() to pass ModifierState
    5. Update all call sites
    6. Test: keymap with m9-*X entry matches when mod9 active
    7. Log implementation with artifacts (hash key extended, lookup updated)
    8. Mark task 9 as complete
    ```

- [x] 10. Add fallback logic for keymap lookup with modal modifiers
  - **Files**:
    - `src/core/input/keymap.cpp` (extend lookup method)
  - **Description**: Implement priority-based fallback when exact modal+standard match not found
  - **Acceptance Criteria**:
    - lookup() tries exact match first (all modifiers)
    - Falls back to match without modal modifiers
    - Falls back to match with fewer modifiers (priority ordering)
    - Correct entry is matched in all test cases
  - **_Leverage**: Existing Keymap::lookup method_
  - **_Requirements**: Requirement 5 (Modifier Combination Keymap Lookup)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: Algorithm Designer with expertise in priority-based matching

    Task: Implement fallback logic for keymap lookup when exact modal+standard match is not found in src/core/input/keymap.cpp following requirement 5.

    Context:
    - User may press mod9+X but only *X is defined in keymap (no m9-*X entry)
    - Need graceful fallback to less specific matches
    - Priority: exact match > without modal > with fewer modifiers > no match

    Implementation:
    1. Modify lookup() to try matches in order:
       - Attempt 1: Exact match (all standard + all modal modifiers)
         ```cpp
         uint64_t hash = buildHashKey(scancode, modState.standard, modState.modal);
         auto it = m_bindings.find(hash);
         if (it != m_bindings.end()) return it->second;
         ```
       - Attempt 2: Without modal modifiers (standard only)
         ```cpp
         hash = buildHashKey(scancode, modState.standard, 0);
         it = m_bindings.find(hash);
         if (it != m_bindings.end()) return it->second;
         ```
       - Attempt 3: Without any modifiers (base key only)
         ```cpp
         hash = buildHashKey(scancode, 0, 0);
         return m_bindings[hash];  // May be nullptr
         ```

    Advanced (Optional): If multiple modal modifiers active, try subsets:
    - mod9+mod8 active: try (mod9+mod8), then (mod9), then (mod8), then (none)
    - This is complex, implement only if time allows

    Restrictions:
    - DO NOT infinite loop (max 3 attempts)
    - Return nullptr if no match found (do not crash)
    - Maintain performance (<20μs per lookup)

    Leverage:
    - Existing m_bindings hash table
    - buildHashKey() from Task 9
    - ModifierState from Task 1

    Success Criteria:
    - Exact match is always preferred
    - Fallback to standard-only works
    - Fallback to no-modifiers works
    - nullptr returned if truly no match
    - Performance <20μs P99

    Instructions:
    1. Mark task 10 as in-progress
    2. Modify lookup() method in keymap.cpp
    3. Implement 3-attempt fallback logic
    4. Test with configs:
       - Only *X defined: mod9+X should match *X
       - m9-*X and *X defined: mod9+X should match m9-*X (exact)
    5. Benchmark lookup performance
    6. Log implementation with artifacts (fallback logic added)
    7. Mark task 10 as complete
    ```

## Phase 4: End-to-End Integration and Testing (Tasks 11-12)

- [-] 11. Create full integration test with mock evdev device
  - **Files**:
    - `tests/test_modal_e2e.cpp` (create new file)
  - **Description**: End-to-end test using mock evdev device to simulate real keyboard input
  - **Acceptance Criteria**:
    - 15 E2E tests covering UAT scenarios
    - Mock evdev device injects realistic event sequences
    - Output is captured and verified
    - All UAT scenarios pass (hold, tap, combinations)
  - **_Leverage**: Mock evdev device creation (may need to implement), existing test utilities_
  - **_Requirements**: All requirements (E2E validation)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: End-to-End Test Automation Engineer with Linux evdev expertise

    Task: Create comprehensive end-to-end tests using mock evdev device in tests/test_modal_e2e.cpp covering all UAT scenarios.

    Context:
    - Need to simulate real keyboard input (evdev events)
    - Verify entire pipeline: evdev → engine → uinput
    - Test real-world workflows (Emacs prefix, Vim modal, etc.)

    Implementation:
    1. Create mock evdev device helper:
       ```cpp
       class MockEvdevDevice {
       public:
           void sendKeyDown(int evdev_code);
           void sendKeyUp(int evdev_code);
           void sleep(int ms);  // Simulate time passing
           std::vector<Event> readOutputEvents();  // Capture uinput output
       };
       ```
    2. Create tests/test_modal_e2e.cpp with 15 tests:
       - UAT1_BasicModalModifier: Hold A, press X, verify Y output
       - UAT2_TapVsHold: Tap A (Tab), hold A (mod9)
       - UAT3_NumberModifierAsShift: Hold 1, press A, verify Shift+A
       - UAT4_MultiModalCombination: Hold A+S, press X, verify Z
       - UAT5_EmacsPrefixKey: Hold X, press F, verify OpenFile action
       - UAT6_VimModalEditing: Hold Esc, press H/J/K/L, verify arrows
       - UAT7_RapidTapping: Tap-tap-tap A quickly, verify all Tabs
       - UAT8_HoldReleaseTap: Hold A, release, tap A, verify mod9 then Tab
       - UAT9_CrossModifierInterference: mod9 + Ctrl, verify both active
       - UAT10_ConfigReload: Load config, activate mod9, reload, verify state
       - UAT11_MaxConcurrentModifiers: Activate 20 modal modifiers simultaneously
       - UAT12_LatencyMeasurement: Measure hold→output latency (<1ms)
       - UAT13_ThroughputTest: 1000 events/sec, verify no drops
       - UAT14_LongHold: Hold for 60s, verify no stuck state
       - UAT15_SuspendResume: Simulate system suspend/resume, verify recovery

    3. Load config for each test:
       ```cpp
       Engine engine;
       engine.loadConfig("test_configs/modal_basic.mayu");
       ```

    Restrictions:
    - Use real Engine instance (not mocked)
    - Mock ONLY evdev/uinput (platform layer)
    - Measure latency using std::chrono::high_resolution_clock
    - Tests must be reproducible (no flaky timing)

    Leverage:
    - Existing Engine class
    - Platform abstraction layer for evdev/uinput mocking
    - Google Test framework

    Success Criteria:
    - All 15 UAT tests pass
    - Latency measurements <1ms P99
    - Throughput test achieves 1000 events/sec
    - No memory leaks (valgrind clean)

    Instructions:
    1. Mark task 11 as in-progress
    2. Create MockEvdevDevice helper class
    3. Write all 15 E2E tests
    4. Create test configs in test_configs/ directory
    5. Run: ./yamy_test --gtest_filter="ModalE2ETest.*"
    6. Verify all pass and performance targets met
    7. Log implementation with artifacts (test file, helper class, configs)
    8. Mark task 11 as complete
    ```

- [ ] 12. Add performance benchmark for modal modifier pipeline
  - **Files**:
    - `benchmarks/bench_modal_modifier.cpp` (create new file, or add to existing benchmark)
  - **Description**: Automated benchmark measuring latency of modal modifier detection
  - **Acceptance Criteria**:
    - Benchmark runs 100,000 iterations
    - Measures P50, P95, P99, P99.9 latencies
    - Reports hold detection, state update, and keymap lookup separately
    - All targets met (<1ms P99 total pipeline)
    - Outputs CSV for graphing
  - **_Leverage**: Existing benchmark infrastructure (if any)_
  - **_Requirements**: Non-Functional Requirements (Performance)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: Performance Engineer with profiling and benchmarking expertise

    Task: Create automated performance benchmark for modal modifier pipeline in benchmarks/bench_modal_modifier.cpp measuring all latency targets.

    Context:
    - Need quantitative performance data (not just "feels fast")
    - Measure each component separately (hold detection, state update, keymap lookup)
    - Compare against targets: <10μs hold detection, <5μs state update, <15μs lookup, <1ms total

    Implementation:
    1. Create benchmarks/bench_modal_modifier.cpp
    2. Implement benchmark harness:
       ```cpp
       void benchmarkHoldDetection(int iterations) {
           ModifierKeyHandler handler;
           handler.registerNumberModifier(0x001E, 0x002A);  // A → LShift

           std::vector<std::chrono::nanoseconds> latencies;
           for (int i = 0; i < iterations; i++) {
               auto start = std::chrono::high_resolution_clock::now();
               auto result = handler.processNumberKey(0x001E, EventType::DOWN);
               auto end = std::chrono::high_resolution_clock::now();
               latencies.push_back(end - start);
           }

           // Calculate percentiles
           std::sort(latencies.begin(), latencies.end());
           auto p50 = latencies[iterations * 0.50];
           auto p95 = latencies[iterations * 0.95];
           auto p99 = latencies[iterations * 0.99];
           auto p999 = latencies[iterations * 0.999];

           // Output results
           std::cout << "Hold Detection: P50=" << p50.count() << "ns, P99=" << p99.count() << "ns\n";
       }
       ```
    3. Benchmark all components:
       - Hold detection (processNumberKey)
       - Modifier state update (activate/deactivate)
       - Keymap lookup (with modal modifiers)
       - Full pipeline (evdev → uinput)
    4. Output CSV for graphing:
       ```
       Component,P50,P95,P99,P99.9
       HoldDetection,8200,11500,12800,45300
       StateUpdate,3100,4800,5900,8200
       KeymapLookup,11700,15200,18500,32100
       FullPipeline,387000,720000,842000,2100000
       ```

    Restrictions:
    - Use high_resolution_clock (not steady_clock for measurement)
    - Warm up: run 1000 iterations before measuring
    - Disable debug logging during benchmark
    - Run with CPU governor set to performance (document this requirement)

    Leverage:
    - std::chrono for timing
    - Statistical percentile calculation
    - Existing benchmark patterns in project (if any)

    Success Criteria:
    - Benchmark completes 100,000 iterations
    - P50, P95, P99, P99.9 latencies reported
    - All targets met:
      - Hold detection P99 < 10μs
      - State update P99 < 5μs
      - Keymap lookup P99 < 15μs
      - Full pipeline P99 < 1ms
    - CSV output for graphing

    Instructions:
    1. Mark task 12 as in-progress
    2. Create benchmarks/bench_modal_modifier.cpp
    3. Implement benchmark for each component
    4. Run benchmark: ./bench_modal_modifier --iterations 100000
    5. Verify all targets met
    6. Save CSV output to benchmarks/results/modal_modifier_latency.csv
    7. Log implementation with artifacts (benchmark file, results CSV)
    8. Mark task 12 as complete
    ```

## Phase 5: Documentation and Final Validation (Tasks 13-15)

- [ ] 13. Create user documentation for modal modifier syntax
  - **Files**:
    - `docs/MODAL_MODIFIER_GUIDE.md` (create new file)
  - **Description**: User-facing documentation explaining modal modifier syntax and examples
  - **Acceptance Criteria**:
    - Documentation covers: syntax, examples, use cases, troubleshooting
    - 5-minute quick start guide
    - Advanced examples (Emacs, Vim workflows)
    - Clear error message explanations
  - **_Leverage**: Existing documentation style in docs/_
  - **_Requirements**: Non-Functional Requirements (Usability)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: Technical Writer with keyboard customization and YAMY expertise

    Task: Create comprehensive user documentation for modal modifiers in docs/MODAL_MODIFIER_GUIDE.md.

    Context:
    - Users need to understand how to use modal modifiers in their .mayu files
    - Documentation should be beginner-friendly but cover advanced use cases
    - Follow SPEC_DRIVEN_DEV_GUIDE.md TIP 5 (Quick Start Guide format)

    Implementation:
    1. Create docs/MODAL_MODIFIER_GUIDE.md with sections:
       - **Quick Start (5 Minutes)**:
         - What modal modifiers do (3 sentences)
         - Simplest example: mod mod9 = !!A, key m9-*X = Y
         - How to test it works
       - **Syntax Reference**:
         - mod modX = !!key (modal modifier definition)
         - def numbermod *key = *HardwareMod (number modifier definition)
         - key mX-*key = output (modal combination)
       - **Examples**:
         - Emacs C-x prefix key
         - Vim normal mode (Esc as modal layer)
         - Number row as function keys
       - **Advanced Use Cases**:
         - Multiple modal modifiers (mod9+mod8)
         - Combining with standard modifiers (Ctrl+mod9)
         - Chaining modal layers
       - **Troubleshooting**:
         - Error: "Invalid modal modifier index mod25"
         - Error: "Cannot use hardware modifier as modal trigger"
         - Debugging: Check logs for MODIFIER:ACTIVATE messages
       - **Performance Notes**:
         - Hold threshold default: 200ms (not configurable yet)
         - Latency: <1ms typical
       - **Cross-Platform Notes**:
         - Works identically on Windows and Linux
         - Same .mayu file can be used on both

    2. Include code examples in .mayu syntax:
       ```mayu
       # Emacs-style C-x prefix
       mod mod9 = !!X
       key m9-*F = &OpenFile     # C-x C-f
       key m9-*S = &SaveFile     # C-x C-s
       key m9-*C = &Exit         # C-x C-c
       ```

    Restrictions:
    - Use markdown format
    - Keep Quick Start section under 500 words
    - Include table of contents
    - No implementation details (focus on user usage)

    Leverage:
    - Existing docs/ structure
    - SPEC_DRIVEN_DEV_GUIDE.md TIP 5 template

    Success Criteria:
    - Documentation is complete and accurate
    - Quick Start takes <5 minutes to read and try
    - All syntax examples are valid and tested
    - Troubleshooting covers common errors

    Instructions:
    1. Mark task 13 as in-progress
    2. Create docs/MODAL_MODIFIER_GUIDE.md
    3. Write all sections
    4. Test all code examples in real .mayu file
    5. Have someone else read Quick Start and give feedback
    6. Log implementation with artifacts (doc file created)
    7. Mark task 13 as complete
    ```

- [ ] 14. Add integration test config files
  - **Files**:
    - `test_configs/modal_basic.mayu` (create)
    - `test_configs/modal_advanced.mayu` (create)
    - `test_configs/modal_stress.mayu` (create)
  - **Description**: Test configuration files for integration and E2E tests
  - **Acceptance Criteria**:
    - 3 test configs covering basic, advanced, and stress scenarios
    - All configs parse without errors
    - Configs used by integration and E2E tests
  - **_Leverage**: Existing keymaps/config.mayu as reference_
  - **_Requirements**: All requirements (test coverage)_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: QA Configuration Engineer with .mayu syntax expertise

    Task: Create test configuration files for integration and E2E tests in test_configs/ directory.

    Context:
    - Integration tests (Task 11) need realistic .mayu configs
    - Need variety: basic (1 modifier), advanced (multiple), stress (edge cases)

    Implementation:
    1. Create test_configs/modal_basic.mayu:
       ```mayu
       # Basic modal modifier test
       mod mod9 = !!A
       def subst *A = *Tab

       key m9-*X = Y
       key m9-*F = &OpenFile
       ```

    2. Create test_configs/modal_advanced.mayu:
       ```mayu
       # Advanced: multiple modal modifiers + standard modifiers
       mod mod9 = !!A
       mod mod8 = !!S

       def numbermod *_1 = *LShift
       def numbermod *_2 = *LCtrl

       # Modal combinations
       key m9-*X = Y
       key *C-m9-*X = Z
       key m8-m9-*X = W

       # Number modifier combinations
       # (Hold 1 = Shift, so 1+A should output Shift+A)
       ```

    3. Create test_configs/modal_stress.mayu:
       ```mayu
       # Stress test: maximum modifiers
       mod mod0 = !!Q
       mod mod1 = !!W
       ...
       mod mod19 = !!P  # 20 total

       # 100 number modifiers (all letters)
       def numbermod *A = *F1
       def numbermod *B = *F2
       ...

       # Complex combinations
       key m0-m1-m2-*X = Y  # Multiple modal modifiers
       ```

    Restrictions:
    - All configs must parse without errors
    - Use valid key names (check keymaps/109_clean.mayu for reference)
    - DO NOT use undefined keys or actions

    Leverage:
    - Existing keymaps/config.mayu for syntax reference
    - Key names from keymaps/109_clean.mayu

    Success Criteria:
    - All 3 configs parse successfully
    - Configs cover basic, advanced, and stress scenarios
    - Configs are used by integration tests (Task 11)

    Instructions:
    1. Mark task 14 as in-progress
    2. Create test_configs/ directory if needed
    3. Write modal_basic.mayu
    4. Write modal_advanced.mayu
    5. Write modal_stress.mayu
    6. Validate: ./yamy --validate-config test_configs/modal_basic.mayu
    7. Log implementation with artifacts (3 config files)
    8. Mark task 14 as complete
    ```

- [ ] 15. Final validation and acceptance testing
  - **Files**: (no new files, run existing tests)
  - **Description**: Run complete test suite and validate all acceptance criteria are met
  - **Acceptance Criteria**:
    - All unit tests pass (60 tests)
    - All integration tests pass (30 tests)
    - All E2E tests pass (15 tests)
    - Performance benchmarks meet targets (<1ms P99)
    - Documentation complete and accurate
    - Zero P0 bugs
  - **_Leverage**: All previous tasks_
  - **_Requirements**: All requirements_
  - **_Prompt**:
    ```
    Implement the task for spec modal-modifier-remapping, first run spec-workflow-guide to get the workflow guide then implement the task:

    Role: QA Lead with responsibility for feature acceptance and quality gates

    Task: Execute final validation checklist and verify all acceptance criteria are met for modal modifier remapping feature.

    Context:
    - All development tasks (1-14) are complete
    - Need to verify feature is ready for production
    - Checklist from requirements.md must all pass

    Implementation:
    1. Run complete test suite:
       ```bash
       ./yamy_test --gtest_filter="*"
       # Verify: 60 unit + 30 integration + 15 E2E = 105 tests pass
       ```

    2. Run performance benchmark:
       ```bash
       ./bench_modal_modifier --iterations 100000
       # Verify: P99 < 1ms total pipeline
       ```

    3. Manual UAT testing:
       - Load test_configs/modal_basic.mayu
       - Execute UAT-1 through UAT-5 from requirements.md
       - Verify correct behavior

    4. Cross-platform validation (if applicable):
       - Run tests on Windows (if ported)
       - Verify identical behavior

    5. Documentation review:
       - Read docs/MODAL_MODIFIER_GUIDE.md
       - Verify all examples work
       - Check for typos/errors

    6. Code quality verification:
       ```bash
       # Check lines per file
       wc -l src/core/engine/modifier_key_handler.cpp
       # Target: <500 lines

       # Check test coverage
       ./yamy_test --coverage
       # Target: >95%
       ```

    7. Memory leak check:
       ```bash
       valgrind --leak-check=full ./yamy_test
       # Target: zero leaks
       ```

    Restrictions:
    - DO NOT modify code (validation only)
    - If issues found, create new tasks (do not fix in this task)
    - Document all findings

    Leverage:
    - All tests from Tasks 2, 7, 11, 12
    - Documentation from Task 13
    - Acceptance criteria from requirements.md

    Success Criteria:
    - All 105 tests pass
    - Performance targets met (<1ms P99)
    - Zero memory leaks
    - All UAT scenarios work correctly
    - Documentation accurate
    - Code quality gates met (500/50 line limits, >95% coverage)

    Instructions:
    1. Mark task 15 as in-progress
    2. Run complete test suite and record results
    3. Run performance benchmark and verify targets
    4. Execute manual UAT tests
    5. Review documentation
    6. Check code quality metrics
    7. Run memory leak check
    8. Create summary report:
       ```markdown
       # Modal Modifier Remapping - Final Validation Report

       ## Test Results
       - Unit tests: 60/60 PASS
       - Integration tests: 30/30 PASS
       - E2E tests: 15/15 PASS

       ## Performance
       - P99 latency: 842μs (target: <1ms) ✓

       ## Code Quality
       - Max file: 287 lines (target: <500) ✓
       - Coverage: 97.3% (target: >95%) ✓

       ## Manual UAT
       - UAT-1: PASS
       - UAT-2: PASS
       - UAT-3: PASS
       - UAT-4: PASS
       - UAT-5: PASS

       ## Conclusion
       Feature READY for production
       ```
    9. Log implementation with validation report
    10. Mark task 15 as complete
    ```

---

## Task Dependencies

```
Phase 1 (Foundation):
  1 → 2 (ModifierState must exist before testing)
  3 (Independent, verify existing code)

Phase 2 (Event Processing):
  4 → 5 → 6 → 7 (Sequential: add handler → integrate → thread state → test)

Phase 3 (Configuration):
  8 (Depends on 4-6: EventProcessor must have handler)
  9 → 10 (Keymap extension before fallback logic)

Phase 4 (E2E):
  11 (Depends on all previous: full system integration)
  12 (Independent benchmark, can run parallel with 11)

Phase 5 (Finalization):
  13 (Documentation, can be done anytime)
  14 (Test configs, supports task 11)
  15 (Final validation, depends on ALL previous tasks)
```

## Estimated Total Effort

- **Phase 1**: 3 tasks × 2 hours = 6 hours
- **Phase 2**: 4 tasks × 3 hours = 12 hours
- **Phase 3**: 3 tasks × 2 hours = 6 hours
- **Phase 4**: 2 tasks × 4 hours = 8 hours
- **Phase 5**: 3 tasks × 2 hours = 6 hours

**Total**: ~38 hours of development time

---

**Document Version**: 1.0
**Created**: 2025-12-14
**Reviewed By**: (Pending approval)
