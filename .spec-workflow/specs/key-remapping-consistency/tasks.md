# Tasks Document: Key Remapping Consistency

## Phase 1: Investigation & Instrumentation (Week 1)

- [x] 1.1 Add comprehensive Layer 1 logging to keycode_mapping.cpp
  - Files: `src/platform/linux/keycode_mapping.cpp`
  - Add logging to `evdevToYamyKeyCode()`: `[LAYER1:IN] evdev X (EVENT_TYPE) → yamy 0xYYYY`
  - Log unmapped keys: `[LAYER1:IN] NOT FOUND`
  - Include event type parameter (PRESS/RELEASE) in function signature
  - _Leverage: Existing PLATFORM_LOG_INFO macro_
  - _Requirements: 1, 3, 4_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Systems programmer with expertise in C++ logging and input event handling | Task: Add comprehensive logging to evdevToYamyKeyCode() function in src/platform/linux/keycode_mapping.cpp following requirements 1, 3, and 4. Log format: [LAYER1:IN] evdev X (PRESS/RELEASE) → yamy 0xYYYY. Include event type in function signature. Leverage existing PLATFORM_LOG_INFO macro. | Restrictions: Do not modify map tables, maintain function performance (< 1ms), ensure debug logging can be toggled via YAMY_DEBUG_KEYCODE env var, do not break existing callers | _Leverage: Existing PLATFORM_LOG_INFO macro in src/platform/linux/platform_log.h | _Requirements: Requirements 1 (Universal Event Processing), 3 (Layer Completeness), 4 (Comprehensive Logging) | Success: All Layer 1 events logged with evdev code, event type, and YAMY scan code. Unmapped keys show NOT FOUND. No performance degradation. Existing code compiles without changes._

- [x] 1.2 Add Layer 3 logging to yamyToEvdevKeyCode()
  - Files: `src/platform/linux/keycode_mapping.cpp`
  - Add logging to `yamyToEvdevKeyCode()`: `[LAYER3:OUT] yamy 0xYYYY → evdev Z (KEY_NAME)`
  - Log which map was used: "Found in US scan map" or "Found in VK map"
  - Log unmapped keys: `[LAYER3:OUT] NOT FOUND in any map`
  - _Leverage: Existing PLATFORM_LOG_INFO macro, getKeyName() helper_
  - _Requirements: 1, 3, 4_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Systems programmer with expertise in C++ and keycode mapping | Task: Enhance yamyToEvdevKeyCode() in src/platform/linux/keycode_mapping.cpp with detailed logging following requirements 1, 3, and 4. Log format: [LAYER3:OUT] yamy 0xYYYY → evdev Z (KEY_NAME). Indicate which map (US scan, JP scan, or VK) was used. | Restrictions: Do not modify the scan-map-first priority fix already applied, maintain < 1ms performance, use existing getKeyName() helper for key names, no breaking changes | _Leverage: Existing PLATFORM_LOG_INFO macro, getKeyName() function in keycode_mapping.cpp | _Requirements: Requirements 1 (Universal Event Processing), 3 (Layer Completeness), 4 (Comprehensive Logging) | Success: All Layer 3 outputs logged with YAMY code, evdev code, key name, and source map. Clear indication when key is unmapped. No performance impact._

- [x] 1.3 Add Layer 2 substitution logging to engine.cpp
  - Files: `src/core/engine/engine.cpp`
  - Add logging before substitution lookup: `[LAYER2:IN] Processing yamy 0xYYYY`
  - Add logging after substitution: `[LAYER2:SUBST] 0xAAAA → 0xBBBB` or `[LAYER2:PASSTHROUGH] 0xAAAA (no substitution)`
  - Identify exact location in engine where substitutions are applied
  - _Leverage: Existing engine substitution table, PLATFORM_LOG_INFO macro_
  - _Requirements: 1, 3, 4_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Engine developer with expertise in keyboard remapping logic | Task: Add comprehensive Layer 2 logging to substitution processing in src/core/engine/engine.cpp following requirements 1, 3, and 4. Log format: [LAYER2:IN] Processing yamy 0xYYYY, [LAYER2:SUBST] 0xAAAA → 0xBBBB (if substitution found), [LAYER2:PASSTHROUGH] 0xAAAA (if no substitution). | Restrictions: Do not modify substitution table structure, do not change substitution lookup logic, maintain engine performance, ensure logging is optional via debug flag | _Leverage: Existing substitution table in engine.cpp, PLATFORM_LOG_INFO macro | _Requirements: Requirements 1 (Universal Event Processing), 3 (Layer Completeness), 4 (Comprehensive Logging) | Success: All substitution lookups logged showing input, output, and whether substitution was applied or passthrough. Logs clearly indicate Layer 2 processing. No functional changes to substitution logic._

- [x] 1.4 Create log analysis script
  - Files: `tests/analyze_event_flow.py`
  - Parse debug log file to extract event sequences
  - Identify missing layers (events that show LAYER1 but not LAYER2/3)
  - Generate report showing: key → LAYER1 → LAYER2 → LAYER3 progression
  - Highlight asymmetries: keys that only work on RELEASE, keys that skip layers
  - _Leverage: Python regex for log parsing, existing log format_
  - _Requirements: 3, 4_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: DevOps engineer with expertise in log analysis and Python scripting | Task: Create automated log analysis tool in tests/analyze_event_flow.py following requirements 3 and 4. Parse debug logs to extract event sequences, identify missing layers, and generate comprehensive flow report. Highlight keys with asymmetric behavior (RELEASE-only, layer skipping). | Restrictions: Must handle large log files efficiently, robust regex parsing for all log formats, output human-readable and machine-parseable formats (JSON option), no external dependencies beyond Python stdlib | _Leverage: Existing log format from PLATFORM_LOG_INFO with [LAYER:DIRECTION] markers | _Requirements: Requirements 3 (Layer Completeness), 4 (Comprehensive Logging) | Success: Tool successfully parses logs, identifies all event flows, reports missing layers, highlights asymmetries, generates clear report with examples. Handles edge cases gracefully._

- [x] 1.5 Create test event injection utility
  - Files: `src/test/yamy_test_main.cpp` (enhanced existing utility)
  - Create standalone utility `yamy-test inject <evdev> <PRESS|RELEASE>`
  - Inject synthetic key event into YAMY engine for testing
  - Supports both PRESS and RELEASE event types
  - _Leverage: Existing virtual keyboard implementation via uinput_
  - _Requirements: 5, 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test infrastructure engineer with expertise in C++ and input event injection | Task: Create test utility src/tests/test_inject_key.cpp that can inject synthetic key events into running YAMY engine for autonomous testing following requirements 5 and 6. CLI: yamy-test inject <evdev_code> <PRESS|RELEASE>. Events must flow through all 3 layers identically to real keypresses. | Restrictions: Must not require user to actually press keys, use existing IPC/injection infrastructure, events must be indistinguishable from real hardware events to engine, proper error handling for YAMY not running | _Leverage: Existing IPC channel in src/platform/linux/ipc_channel_linux.cpp, input_injector_linux.cpp for virtual device | _Requirements: Requirements 5 (Automated Testing), 6 (Test Coverage) | Success: Utility successfully injects PRESS and RELEASE events, events appear in debug logs with all 3 layers, engine processes synthetic events identically to real keys, can be called from test scripts._

- [x] 1.6 Run initial diagnostic tests and document asymmetries
  - Files: `docs/INVESTIGATION_FINDINGS.md`
  - Use log analysis tool to analyze current behavior with config_clean.mayu
  - Test all 87 substitutions manually or with initial test script
  - Document specific findings:
    - Which keys work on PRESS + RELEASE (W→A)
    - Which keys only work on RELEASE (R→E, T→U)
    - Which keys show Layer 1 but not Layer 2/3 (N→LShift)
    - Any other asymmetries discovered
  - Create baseline metrics for comparison after refactoring
  - _Leverage: Log analysis script from task 1.4, test injection from task 1.5_
  - _Requirements: All from Phase 1_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA engineer with expertise in systematic testing and documentation | Task: Conduct comprehensive diagnostic testing using tools from tasks 1.4 and 1.5, documenting all asymmetries in docs/INVESTIGATION_FINDINGS.md following all Phase 1 requirements. Test all 87 substitutions from config_clean.mayu, categorize issues (PRESS/RELEASE asymmetry, layer skipping, etc.), establish baseline metrics. | Restrictions: Must test systematically not randomly, document specific code locations where issues occur, provide reproducible test cases, quantify issues with numbers (X keys work, Y keys partial, Z keys broken) | _Leverage: Log analysis script from task 1.4 (tests/analyze_event_flow.py), test injection utility from task 1.5 (yamy-test inject) | _Requirements: All Phase 1 requirements (1, 3, 4, 5, 6) | Success: Complete documentation of current state, specific examples of each issue type, baseline pass rate (currently ~50%), clear categories of failures, reproducible test procedure._

## Phase 2: Core Refactoring (Week 2)

- [ ] 2.1 Create EventProcessor interface and class structure
  - Files: `src/core/engine/engine_event_processor.h`, `src/core/engine/engine_event_processor.cpp`
  - Define `EventProcessor` class with pure layer functions: `layer1_evdevToYamy()`, `layer2_applySubstitution()`, `layer3_yamyToEvdev()`
  - Define `ProcessedEvent` struct to hold results
  - Constructor takes reference to substitution table
  - Main entry point: `processEvent(evdev, event_type) → ProcessedEvent`
  - _Leverage: Design pattern from design.md Component 1_
  - _Requirements: 1, 2, 7_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Software architect with expertise in C++ class design and clean architecture | Task: Create EventProcessor class in new files src/core/engine/engine_event_processor.h/cpp following requirements 1, 2, and 7. Implement clean interface with pure layer functions, ProcessedEvent struct, and main processEvent() entry point. Follow exact design from design.md Component 1. | Restrictions: Make layer functions private pure functions, no global state, no side effects except logging, constructor takes const reference to substitution table, ensure header guards and proper includes, follow project C++ style | _Leverage: Design pattern from design.md Component 1 (EventProcessor interface specification) | _Requirements: Requirements 1 (Universal Event Processing), 2 (Event Type Consistency), 7 (Code Consistency) | Success: Clean class interface defined, ProcessedEvent struct captures output evdev + event type + validity, layer functions are pure and private, processEvent() is public entry point, compiles without errors, follows design exactly._

- [ ] 2.2 Implement Layer 1 in EventProcessor
  - Files: `src/core/engine/engine_event_processor.cpp`
  - Implement `layer1_evdevToYamy()` calling existing `evdevToYamyKeyCode()`
  - Add `[LAYER1:IN]` logging with event type
  - Handle unmapped keys (return 0)
  - _Leverage: Existing evdevToYamyKeyCode() from keycode_mapping.cpp_
  - _Requirements: 1, 3, 4_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Systems programmer with expertise in input event handling | Task: Implement layer1_evdevToYamy() in src/core/engine/engine_event_processor.cpp following requirements 1, 3, and 4. Call existing evdevToYamyKeyCode(), add comprehensive logging, handle unmapped keys gracefully returning 0. | Restrictions: Must reuse existing evdevToYamyKeyCode() function not reimplement, maintain < 1ms performance, proper error handling for unmapped keys, logging must match format from Phase 1 | _Leverage: Existing evdevToYamyKeyCode() from src/platform/linux/keycode_mapping.cpp, PLATFORM_LOG_INFO macro | _Requirements: Requirements 1 (Universal Event Processing), 3 (Layer Completeness), 4 (Comprehensive Logging) | Success: Layer 1 function works correctly for all keys, calls existing keycode mapping, logs all events with proper format, unmapped keys return 0 and log NOT FOUND, no performance regression._

- [ ] 2.3 Implement Layer 2 in EventProcessor
  - Files: `src/core/engine/engine_event_processor.cpp`
  - Implement `layer2_applySubstitution()` as pure function
  - Look up scan code in substitution table
  - Log `[LAYER2:SUBST]` or `[LAYER2:PASSTHROUGH]`
  - **Critical**: Ensure NO special cases for modifier keys - N→LShift uses same logic as W→A
  - _Leverage: Substitution table passed in constructor_
  - _Requirements: 1, 3, 4, 7_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Engine developer with expertise in keyboard mapping logic | Task: Implement layer2_applySubstitution() in src/core/engine/engine_event_processor.cpp following requirements 1, 3, 4, and 7. Pure function that looks up substitution, logs result, NO special cases for any key type. Modifier substitutions (N→LShift) must use identical logic to regular substitutions (W→A). | Restrictions: Must be pure function (no side effects except logging), no branching based on key type, no modifier special cases, passthrough unchanged if not in table, maintain performance, const reference to substitution table | _Leverage: Substitution table (std::unordered_map<uint16_t, uint16_t>) passed in constructor | _Requirements: Requirements 1 (Universal Event Processing), 3 (Layer Completeness), 4 (Comprehensive Logging), 7 (Code Consistency) | Success: Substitution lookup works for all 87 keys, modifier keys processed identically to regular keys, logs show SUBST or PASSTHROUGH for every event, no special cases in code, pure function with no state._

- [ ] 2.4 Implement Layer 3 in EventProcessor
  - Files: `src/core/engine/engine_event_processor.cpp`
  - Implement `layer3_yamyToEvdev()` calling existing `yamyToEvdevKeyCode()`
  - Verify scan map is checked FIRST (already fixed in earlier session)
  - Log `[LAYER3:OUT]` with map source
  - Handle unmapped keys (return 0)
  - _Leverage: Existing yamyToEvdevKeyCode() from keycode_mapping.cpp_
  - _Requirements: 1, 3, 4_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Systems programmer with expertise in keycode mapping | Task: Implement layer3_yamyToEvdev() in src/core/engine/engine_event_processor.cpp following requirements 1, 3, and 4. Call existing yamyToEvdevKeyCode() which already has scan-map-first priority fix. Add comprehensive logging showing which map was used. Handle unmapped keys gracefully. | Restrictions: Must reuse existing yamyToEvdevKeyCode() not reimplement, do not modify scan map priority logic, maintain performance, log which map (US scan, JP scan, VK) was used for traceability | _Leverage: Existing yamyToEvdevKeyCode() from src/platform/linux/keycode_mapping.cpp (with scan map priority fix) | _Requirements: Requirements 1 (Universal Event Processing), 3 (Layer Completeness), 4 (Comprehensive Logging) | Success: Layer 3 works for all keys, scan maps checked before VK map, logs show source map, unmapped keys return 0 and log NOT FOUND, no regression from earlier VK/scan fix._

- [ ] 2.5 Implement processEvent() main entry point with event type preservation
  - Files: `src/core/engine/engine_event_processor.cpp`
  - Implement `processEvent(evdev, event_type)` that calls Layer 1 → Layer 2 → Layer 3
  - **Critical**: Preserve event type throughout - PRESS in = PRESS out, RELEASE in = RELEASE out
  - Return `ProcessedEvent` with output evdev, event type, and validity flag
  - Add top-level logging: `[EVENT:START]` and `[EVENT:END]`
  - _Leverage: Layer functions from tasks 2.2, 2.3, 2.4_
  - _Requirements: 1, 2, 3, 7_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Systems architect with expertise in event-driven systems | Task: Implement processEvent() main entry point in src/core/engine/engine_event_processor.cpp following requirements 1, 2, 3, and 7. Compose layer functions: output = f₃(f₂(f₁(input))). CRITICAL: Preserve event type - PRESS in must produce PRESS out, RELEASE in must produce RELEASE out. No branching based on event type except to preserve the flag. | Restrictions: Must call all 3 layers for every event (no layer skipping), event type preservation is mandatory, no special cases for any key, return ProcessedEvent struct with all fields populated, add event start/end logging | _Leverage: Layer functions from tasks 2.2 (layer1_evdevToYamy), 2.3 (layer2_applySubstitution), 2.4 (layer3_yamyToEvdev) | _Requirements: Requirements 1 (Universal Event Processing), 2 (Event Type Consistency), 3 (Layer Completeness), 7 (Code Consistency) | Success: All events processed through all 3 layers, event type preserved correctly, ProcessedEvent returns output evdev + original event type, logs show complete EVENT:START → LAYER1 → LAYER2 → LAYER3 → EVENT:END sequence, no event type branching._

- [ ] 2.6 Integrate EventProcessor into engine.cpp main event loop
  - Files: `src/core/engine/engine.cpp`
  - Replace existing event processing logic with EventProcessor::processEvent()
  - Pass substitution table to EventProcessor constructor
  - Remove any old special-case code for modifiers or specific keys
  - Ensure output events are injected with correct event type
  - _Leverage: Existing event loop structure, input_injector for output_
  - _Requirements: 1, 2, 7_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Engine developer with expertise in refactoring and integration | Task: Integrate EventProcessor into main event loop in src/core/engine/engine.cpp following requirements 1, 2, and 7. Replace old event processing with new unified processor, remove all special cases for modifiers or specific keys, ensure output injection preserves event type. | Restrictions: Must remove all old special-case code (no partial migration), EventProcessor is single source of truth for event processing, maintain engine startup/shutdown logic, ensure substitution table is loaded once and passed to processor, no breaking changes to .mayu file loading | _Leverage: Existing event loop in engine.cpp, EventProcessor from previous tasks, input_injector_linux.cpp for output injection | _Requirements: Requirements 1 (Universal Event Processing), 2 (Event Type Consistency), 7 (Code Consistency) | Success: Engine uses EventProcessor for all events, old special-case code completely removed, event type preserved in output injection, engine starts and loads config correctly, no functional regressions in working keys._

- [ ] 2.7 Verify refactoring with manual testing
  - Files: `docs/REFACTORING_VALIDATION.md`
  - Test previously working keys still work: W→A (PRESS and RELEASE)
  - Test previously partial keys now work fully: R→E, T→U (both PRESS and RELEASE)
  - Test previously broken modifier: N→LShift (should now work)
  - Verify logs show complete LAYER1→LAYER2→LAYER3 for all keys
  - Document any remaining issues
  - _Leverage: Test injection utility from task 1.5, log analysis from task 1.4_
  - _Requirements: All from Phase 2_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA engineer with expertise in manual testing and validation | Task: Validate refactoring in docs/REFACTORING_VALIDATION.md following all Phase 2 requirements. Test working keys still work, partial keys now work fully, broken keys now work, verify complete layer logs for all events. Use test injection and log analysis tools. | Restrictions: Must test systematically covering: previously working, previously partial, previously broken keys, must verify both PRESS and RELEASE events, must check logs for layer completeness, document any remaining issues with specific examples | _Leverage: yamy-test inject utility from task 1.5, analyze_event_flow.py from task 1.4, comparison with baseline from task 1.6 | _Requirements: All Phase 2 requirements (1, 2, 3, 4, 7) | Success: Documentation shows: working keys still work, partial keys (R, T) now work on PRESS, broken keys (N→LShift) now work, all events show complete layer flow, quantified improvement (e.g., 50% → 90% pass rate)._

## Phase 3: Automated Testing Framework (Week 3)

- [ ] 3.1 Create unit tests for Layer 1 (evdevToYamyKeyCode)
  - Files: `tests/test_event_processor_ut.cpp`
  - Set up GoogleTest framework for C++ unit tests
  - Test known evdev codes map to correct YAMY scan codes (e.g., 17→0x0011 for W)
  - Test unmapped evdev codes return 0
  - Test both US and JP keyboard layouts
  - _Leverage: GoogleTest framework, existing keycode maps_
  - _Requirements: 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test engineer with expertise in C++ unit testing and GoogleTest | Task: Create comprehensive unit tests for Layer 1 in tests/test_event_processor_ut.cpp following requirement 6. Test evdevToYamyKeyCode() function with known mappings, unmapped keys, US and JP layouts. Set up GoogleTest framework if not already present. | Restrictions: Tests must be isolated (no YAMY engine running), mock dependencies if needed, test both success and failure cases, cover edge cases (unmapped keys, layout differences), maintain fast test execution | _Leverage: GoogleTest framework, g_evdevToYamyMap from src/platform/linux/keycode_mapping.cpp | _Requirements: Requirement 6 (Unit, Integration, E2E Test Coverage) | Success: Unit tests verify Layer 1 correctness, known keys map correctly, unmapped keys return 0, US and JP layouts handled, tests run in < 1 second, > 95% code coverage for Layer 1 function._

- [ ] 3.2 Create unit tests for Layer 2 (applySubstitution)
  - Files: `tests/test_event_processor_ut.cpp`
  - Test substitution lookup with mock substitution table
  - Test key WITH substitution returns transformed code (W→A: 0x0011→0x001E)
  - Test key WITHOUT substitution returns original code (passthrough)
  - Test modifier key substitution works identically (N→LShift: 0x0031→VK_LSHIFT)
  - _Leverage: GoogleTest framework, mock substitution tables_
  - _Requirements: 6, 7_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test engineer with expertise in unit testing and mocking | Task: Create unit tests for Layer 2 substitution in tests/test_event_processor_ut.cpp following requirements 6 and 7. Test with mock substitution tables, verify both substitution and passthrough cases, ensure modifier substitutions tested identically to regular substitutions. | Restrictions: Use mock substitution tables not real .mayu files, test pure function behavior (no side effects), verify modifier keys have no special treatment, cover all code paths, fast execution | _Leverage: GoogleTest mocking capabilities, substitution table structure from EventProcessor | _Requirements: Requirement 6 (Test Coverage), Requirement 7 (Code Consistency - no modifier special cases) | Success: Tests verify substitution lookup correctness, passthrough works when no substitution, modifier substitutions tested same as regular, no special cases detected, > 95% code coverage for Layer 2 function._

- [ ] 3.3 Create unit tests for Layer 3 (yamyToEvdevKeyCode)
  - Files: `tests/test_event_processor_ut.cpp`
  - Test YAMY scan codes map to correct evdev output codes
  - Test scan map is checked BEFORE VK map (scan code 0x0014 → evdev 20 for T, NOT evdev 58 for CAPSLOCK)
  - Test VK map fallback for keys not in scan maps
  - Test unmapped codes return 0
  - _Leverage: GoogleTest framework, existing scan and VK maps_
  - _Requirements: 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test engineer with expertise in keycode mapping testing | Task: Create unit tests for Layer 3 in tests/test_event_processor_ut.cpp following requirement 6. Verify yamyToEvdevKeyCode() correctness, test scan map priority over VK map (critical test: 0x0014 → KEY_T not KEY_CAPSLOCK), test VK fallback, test unmapped keys. | Restrictions: Must specifically test scan map priority fix, verify no VK/scan conflicts, test both US and JP scan maps, test VK map fallback for special keys, fast execution, isolated tests | _Leverage: GoogleTest framework, g_scanToEvdevMap_US/JP and g_yamyToEvdevMap from keycode_mapping.cpp | _Requirements: Requirement 6 (Test Coverage) | Success: Tests verify Layer 3 correctness, scan map priority confirmed (0x0014 test passes), VK fallback works, unmapped keys return 0, > 95% code coverage for Layer 3 function._

- [ ] 3.4 Create integration tests for Layer 1→2→3 composition
  - Files: `tests/test_event_processor_it.cpp`
  - Test complete event flow: evdev input → YAMY scan → substitution → output evdev
  - Test W→A: evdev 17 → 0x0011 → 0x001E → evdev 30
  - Test N→LShift: evdev 49 → 0x0031 → VK_LSHIFT → evdev (LShift)
  - Test event type preservation: PRESS in → PRESS out, RELEASE in → RELEASE out
  - Test passthrough for unmapped keys
  - _Leverage: EventProcessor class, real substitution table from config_clean.mayu_
  - _Requirements: 1, 2, 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Integration test engineer with expertise in end-to-end test scenarios | Task: Create integration tests for complete layer composition in tests/test_event_processor_it.cpp following requirements 1, 2, and 6. Test EventProcessor with real substitution table, verify complete transformations, test event type preservation, test known working and previously broken keys. | Restrictions: Use real EventProcessor class, load actual substitution table from config_clean.mayu, test both PRESS and RELEASE events, verify complete transformation pipeline, no mocking of layer functions (integration test) | _Leverage: EventProcessor class from Phase 2, config_clean.mayu for real substitution table, GoogleTest for integration testing | _Requirements: Requirement 1 (Universal Event Processing), Requirement 2 (Event Type Consistency), Requirement 6 (Test Coverage) | Success: Integration tests verify end-to-end transformations, W→A works, N→LShift works, event types preserved, passthrough works, tests use real components not mocks, all critical substitutions tested._

- [ ] 3.5 Create Python automated test framework
  - Files: `tests/automated_keymap_test.py`
  - Implement `AutomatedKeymapTest` class as designed
  - Parse config_clean.mayu to extract all 87 substitutions
  - Implement `inject_key()` using yamy-test utility from task 1.5
  - Implement `verify_output()` by parsing debug log [LAYER3:OUT] entries
  - Implement `test_substitution()` to test PRESS and RELEASE for one key
  - Implement `test_all_substitutions()` to run all 174 tests (87 keys × 2 event types)
  - _Leverage: yamy-test inject utility, log parsing patterns_
  - _Requirements: 5, 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test automation engineer with expertise in Python and test frameworks | Task: Create autonomous test framework in tests/automated_keymap_test.py following requirements 5 and 6. Implement AutomatedKeymapTest class with methods to parse .mayu config, inject synthetic events, verify output from logs, test all substitutions without user interaction. Follow exact design from design.md Component 4. | Restrictions: Zero user interaction required, must parse .mayu files to extract substitutions, use yamy-test inject for event injection, parse logs for verification, handle YAMY not running gracefully, Python 3 only (no external dependencies), robust error handling | _Leverage: yamy-test inject utility from task 1.5, log format with [LAYER3:OUT] markers, config_clean.mayu structure | _Requirements: Requirement 5 (Automated Testing - Zero User Interaction), Requirement 6 (Test Coverage) | Success: Framework successfully parses .mayu files, injects events autonomously, verifies output from logs, tests all 87 substitutions × 2 event types, runs without user interaction, generates clear pass/fail report._

- [ ] 3.6 Create test report generator
  - Files: `tests/generate_test_report.py`
  - Generate HTML report from test results
  - Show: total tests, passed, failed, pass rate percentage
  - List failures with: input key, expected key, actual key, event type, which layer failed
  - Color-coded: green for pass, red for fail, yellow for inconclusive
  - Include summary statistics and improvement vs baseline
  - _Leverage: Test results from automated_keymap_test.py_
  - _Requirements: 5, 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test reporting specialist with expertise in HTML generation and data visualization | Task: Create test report generator in tests/generate_test_report.py following requirements 5 and 6. Generate HTML report from test results with statistics, failure details, color coding, and comparison to baseline. Make report clear and actionable. | Restrictions: Must consume test results from automated_keymap_test.py, generate static HTML (no external JS libraries), color-coded for readability, show specific failure details (which key, which layer), include baseline comparison, Python stdlib only | _Leverage: Test results structure from automated_keymap_test.py, baseline metrics from task 1.6 | _Requirements: Requirement 5 (Automated Testing), Requirement 6 (Test Coverage) | Success: Report clearly shows pass/fail statistics, lists specific failures with details, color-coded for readability, includes improvement vs baseline (e.g., 50% → 100%), generates clean HTML, actionable information for debugging failures._

- [ ] 3.7 Create CI test runner script
  - Files: `tests/run_all_tests.sh`
  - Start YAMY in test mode with debug logging
  - Run unit tests (C++ GoogleTest)
  - Run integration tests (C++ GoogleTest)
  - Run E2E tests (Python autonomous framework)
  - Generate test report
  - Stop YAMY
  - Exit with status 0 for all pass, non-zero for any failures
  - _Leverage: All test components from Phase 3_
  - _Requirements: 5, 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: CI/CD engineer with expertise in test automation and shell scripting | Task: Create comprehensive test runner script in tests/run_all_tests.sh following requirements 5 and 6. Orchestrate all test phases: start YAMY, run unit/integration/E2E tests, generate report, cleanup, exit with proper status. Make script robust and CI-ready. | Restrictions: Must handle YAMY startup/shutdown gracefully, wait for engine ready before testing, collect all test results, generate unified report, proper error handling and cleanup on failure, exit code reflects test status, no user interaction | _Leverage: Unit tests from task 3.1-3.3, integration tests from 3.4, E2E framework from 3.5, report generator from 3.6, YAMY binary with debug mode | _Requirements: Requirement 5 (Automated Testing), Requirement 6 (Test Coverage) | Success: Script successfully runs all test phases, starts/stops YAMY cleanly, generates comprehensive report, exits with correct status, handles failures gracefully, ready for CI/CD integration, runs completely autonomously._

- [ ] 3.8 Validate 100% pass rate for all 87 substitutions
  - Files: `docs/TEST_VALIDATION_REPORT.md`
  - Run complete test suite using run_all_tests.sh
  - Verify 100% pass rate (174 tests: 87 keys × 2 event types)
  - If any failures, debug using logs and fix in earlier Phase 2 tasks
  - Document final test results with comparison to baseline
  - Include sample log excerpts showing complete layer flow
  - _Leverage: Complete test suite from Phase 3_
  - _Requirements: All from Phase 3, plus 9 (Algorithmic Verification)_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA validation engineer with expertise in comprehensive testing | Task: Validate 100% test pass rate in docs/TEST_VALIDATION_REPORT.md following all Phase 3 requirements and requirement 9. Run complete test suite, verify all 87 substitutions work for both PRESS and RELEASE, debug any failures, document final results vs baseline. | Restrictions: Must achieve 100% pass rate or document specific remaining issues with root cause, include statistical comparison to baseline (task 1.6), provide sample logs showing complete layer flow, verify both event types for every key, actionable debugging info for any failures | _Leverage: run_all_tests.sh from task 3.7, all test components from Phase 3, baseline from task 1.6 (docs/INVESTIGATION_FINDINGS.md), log analysis from task 1.4 | _Requirements: All Phase 3 requirements (5, 6), Requirement 9 (Algorithmic Verification - 100% pass rate) | Success: Documentation shows 100% pass rate achieved (174/174 tests), dramatic improvement from baseline (e.g., 50% → 100%), sample logs demonstrate complete layer flow, any failures documented with root cause and fix plan, quantified validation of refactoring success._

## Phase 4: Advanced Features (Week 4)

- [ ] 4.1 Design number-to-modifier mapping system
  - Files: `docs/NUMBER_MODIFIER_DESIGN.md`
  - Design hold vs tap detection (200ms threshold)
  - Design state machine: IDLE → HELD → MODIFIER_ACTIVE vs IDLE → TAP → SUBSTITUTION
  - Design integration point with Layer 2 (before substitution lookup)
  - Design mapping table: number key → hardware modifier
  - Design .mayu syntax extension for direct modifier wiring
  - _Leverage: Existing modal layer system (mod modX = !!_Y)_
  - _Requirements: 8_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Feature architect with expertise in input systems and state machines | Task: Design comprehensive number-to-modifier system in docs/NUMBER_MODIFIER_DESIGN.md following requirement 8. Design hold/tap detection, state machine, integration with Layer 2, mapping table structure, .mayu syntax. Ensure backward compatibility with existing number key substitutions. | Restrictions: Must not break existing number key substitutions, hold/tap threshold must be configurable, integration must not bypass EventProcessor architecture, maintain event type consistency, no special-case code paths (generalized solution) | _Leverage: Existing modal layer system with mod modX = !!_Y syntax in .mayu files | _Requirements: Requirement 8 (Number Keys as Custom Modifiers) | Success: Design document covers all aspects of feature, hold/tap logic clearly specified, state machine defined, integration point identified, .mayu syntax proposed, backward compatibility ensured, ready for implementation._

- [ ] 4.2 Implement ModifierKeyHandler class
  - Files: `src/core/engine/modifier_key_handler.h`, `src/core/engine/modifier_key_handler.cpp`
  - Implement hold/tap detection timer (200ms threshold)
  - Implement state machine for number key states
  - Implement `registerNumberModifier(number_key, modifier_type)` registration
  - Implement `isModifierHeld(number_key)` state query
  - Implement `processNumberKey(key, event_type)` processing logic
  - _Leverage: Design from task 4.1_
  - _Requirements: 8_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: C++ developer with expertise in state machines and timing logic | Task: Implement ModifierKeyHandler class in new files src/core/engine/modifier_key_handler.h/cpp following requirement 8 and design from task 4.1. Implement hold/tap timer, state machine, registration/query methods, processing logic. Ensure thread safety for timer. | Restrictions: Must be thread-safe (timer runs in separate thread), no blocking in event processing path, configurable hold/tap threshold, clean state machine with no ambiguous states, proper resource cleanup (RAII for timers) | _Leverage: Design document from task 4.1, std::chrono for timing, std::atomic for state flags | _Requirements: Requirement 8 (Number Keys as Custom Modifiers) | Success: ModifierKeyHandler class implements complete state machine, hold/tap detection works correctly, timer thread-safe, registration and query methods functional, processing logic correct, compiles without errors, follows project style._

- [ ] 4.3 Create number-to-modifier mapping table
  - Files: `src/platform/linux/keycode_mapping.cpp`
  - Add `g_numberToModifierMap` table mapping number keys to hardware modifiers
  - Map KEY_1 → KEY_LEFTSHIFT, KEY_2 → KEY_RIGHTSHIFT, etc.
  - Add function to look up modifier for number key: `getModifierForNumberKey(evdev)`
  - Make table configurable via .mayu file syntax
  - _Leverage: Existing map table patterns_
  - _Requirements: 8_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Systems programmer with expertise in keycode mapping | Task: Add number-to-modifier mapping table in src/platform/linux/keycode_mapping.cpp following requirement 8. Create g_numberToModifierMap with default mappings, add lookup function, make configurable. Follow existing map table patterns. | Restrictions: Must follow existing map table style and naming, default mappings should be sensible (1→LShift, 2→RShift, etc.), lookup function must be efficient (O(1) hash map), configurable via .mayu parsing (integrate with config loader) | _Leverage: Existing map table patterns (g_evdevToYamyMap, g_scanToEvdevMap_US/JP) in keycode_mapping.cpp | _Requirements: Requirement 8 (Number Keys as Custom Modifiers) | Success: Map table created with sensible defaults, lookup function implemented and efficient, configurable via .mayu files, follows project patterns, compiles and integrates cleanly._

- [ ] 4.4 Integrate ModifierKeyHandler into EventProcessor Layer 2
  - Files: `src/core/engine/engine_event_processor.cpp`
  - Add ModifierKeyHandler instance to EventProcessor
  - In `layer2_applySubstitution()`, check if key is registered number modifier BEFORE substitution lookup
  - If number key: delegate to ModifierKeyHandler.processNumberKey()
  - If HOLD detected: activate modifier layer, output hardware modifier key
  - If TAP detected: proceed with normal substitution
  - _Leverage: ModifierKeyHandler from task 4.2, existing Layer 2 logic_
  - _Requirements: 8, 7_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Integration engineer with expertise in event processing | Task: Integrate ModifierKeyHandler into EventProcessor Layer 2 in src/core/engine/engine_event_processor.cpp following requirements 8 and 7. Check for number modifiers before substitution lookup, delegate to handler, maintain code consistency (no special-case branching pattern). | Restrictions: Must maintain Layer 2 function purity (handler is just another table lookup conceptually), no complex branching (keep code clean), TAP falls through to normal substitution, HOLD outputs modifier directly, maintain performance, event type consistency preserved | _Leverage: ModifierKeyHandler from task 4.2, existing layer2_applySubstitution() function, modifier mapping table from task 4.3 | _Requirements: Requirement 8 (Number Keys as Custom Modifiers), Requirement 7 (Code Consistency) | Success: ModifierKeyHandler integrated cleanly, number keys processed before substitution, HOLD activates modifier, TAP proceeds to substitution, code remains clean and consistent, no performance regression, event type preserved._

- [ ] 4.5 Extend .mayu parser to support number modifier syntax
  - Files: `src/core/settings/mayu_parser.cpp` or equivalent
  - Add parsing for syntax: `def modifier *_1 = *LShift` or similar
  - Register number-to-modifier mappings with ModifierKeyHandler during config load
  - Validate modifier targets (must be valid hardware modifiers)
  - Maintain backward compatibility with existing .mayu files
  - _Leverage: Existing .mayu parser, modifier definitions_
  - _Requirements: 8_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Parser developer with expertise in configuration file parsing | Task: Extend .mayu parser to support number modifier syntax in src/core/settings/mayu_parser.cpp following requirement 8. Parse new syntax, register with ModifierKeyHandler, validate targets, maintain backward compatibility. | Restrictions: Must not break existing .mayu files, syntax must be intuitive and consistent with existing .mayu patterns, validation must reject invalid modifiers, parser errors must be clear and helpful, integrate with existing config loading pipeline | _Leverage: Existing .mayu parser structure, modifier definitions (LShift, RShift, etc.), config loading pipeline | _Requirements: Requirement 8 (Number Keys as Custom Modifiers) | Success: Parser correctly handles new modifier syntax, registers mappings with ModifierKeyHandler, validates modifier targets, maintains backward compatibility, clear error messages, existing configs still work._

- [ ] 4.6 Create tests for number-to-modifier feature
  - Files: `tests/test_number_modifiers.cpp`, `tests/test_number_modifiers_e2e.py`
  - Unit tests: Test hold/tap detection with mocked timer
  - Unit tests: Test state machine transitions
  - Integration tests: Test number key HOLD → modifier activation
  - Integration tests: Test number key TAP → normal substitution
  - E2E tests: Test number key combinations (e.g., hold 1 + press A = Shift+A)
  - _Leverage: Existing test frameworks from Phase 3_
  - _Requirements: 8, 6_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Test engineer with expertise in state machine and timing tests | Task: Create comprehensive tests for number modifier feature in tests/test_number_modifiers.cpp and tests/test_number_modifiers_e2e.py following requirements 8 and 6. Test hold/tap detection, state machine, HOLD→modifier, TAP→substitution, combinations. | Restrictions: Must mock timer for unit tests (fast execution), integration tests use real components, E2E tests use real timing, test edge cases (e.g., hold threshold boundary), test combinations thoroughly, maintain test autonomy (no user interaction) | _Leverage: GoogleTest framework from Phase 3, automated test framework from task 3.5, ModifierKeyHandler state machine | _Requirements: Requirement 8 (Number Keys as Custom Modifiers), Requirement 6 (Test Coverage) | Success: Comprehensive test coverage for number modifier feature, hold/tap detection tested, state machine verified, HOLD and TAP behaviors correct, combinations work, tests autonomous and fast, edge cases covered._

- [ ] 4.7 Document advanced feature and update user guide
  - Files: `docs/NUMBER_MODIFIER_USER_GUIDE.md`, update `README.md`
  - Document .mayu syntax for number modifiers
  - Provide examples for small keyboard users
  - Explain hold vs tap behavior and threshold
  - Document how to configure custom mappings
  - Update README with feature overview
  - _Leverage: Design from task 4.1_
  - _Requirements: 8_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Technical writer with expertise in user documentation | Task: Create comprehensive user documentation for number modifier feature in docs/NUMBER_MODIFIER_USER_GUIDE.md following requirement 8. Document syntax, provide examples, explain behavior, configuration. Update README overview. Make documentation clear and user-friendly. | Restrictions: Must be beginner-friendly, include concrete examples for small keyboard users, explain hold/tap threshold clearly, provide troubleshooting tips, show how to customize mappings, consistent with project documentation style | _Leverage: Design document from task 4.1, existing .mayu syntax patterns | _Requirements: Requirement 8 (Number Keys as Custom Modifiers) | Success: Documentation is clear and comprehensive, examples work correctly, hold/tap behavior well-explained, configuration instructions complete, README updated with feature summary, user-friendly and actionable._

- [ ] 4.8 Validate advanced feature end-to-end
  - Files: `docs/ADVANCED_FEATURE_VALIDATION.md`
  - Configure number modifiers in test .mayu file
  - Test HOLD detection: hold number key for >200ms, verify modifier activates
  - Test TAP detection: tap number key quickly, verify normal substitution
  - Test combinations: hold number modifier + press other key, verify modified key output
  - Test all 10 number keys (0-9) can be configured as modifiers
  - Verify backward compatibility: existing number key substitutions still work
  - _Leverage: Complete implementation from Phase 4, test frameworks_
  - _Requirements: 8_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA engineer with expertise in feature validation | Task: Validate advanced number modifier feature end-to-end in docs/ADVANCED_FEATURE_VALIDATION.md following requirement 8. Test hold/tap detection, combinations, all 10 number keys, backward compatibility. Provide quantified validation results. | Restrictions: Must test real-world use cases for small keyboards, test timing edge cases (threshold boundaries), verify modifier state cleanup on release, test interference with existing features, document any limitations or edge cases | _Leverage: Complete number modifier implementation from Phase 4 tasks, test frameworks from Phase 3, test injection utility | _Requirements: Requirement 8 (Number Keys as Custom Modifiers) | Success: Validation document shows feature works correctly, hold/tap detection accurate, combinations work, all 10 numbers configurable, backward compatibility maintained, timing threshold appropriate, no regressions, feature ready for release._

## Final Integration and Documentation

- [ ] 5.1 Run complete test suite and verify 100% pass rate
  - Files: `docs/FINAL_VALIDATION_REPORT.md`
  - Run full test suite: unit + integration + E2E + advanced feature tests
  - Verify 100% pass rate for all 87 basic substitutions
  - Verify advanced number modifier feature works
  - Generate final test report with comparison to original baseline
  - _Leverage: All test components from Phases 3-4_
  - _Requirements: 5, 6, 9_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA validation lead with expertise in comprehensive testing | Task: Run complete test suite and document final validation in docs/FINAL_VALIDATION_REPORT.md following requirements 5, 6, and 9. Verify 100% pass rate for basic substitutions, validate advanced feature, generate comprehensive report vs original baseline from task 1.6. | Restrictions: Must run complete test suite (all phases), quantify improvement vs baseline, verify both basic and advanced features, include performance metrics, document any known limitations, provide executive summary | _Leverage: run_all_tests.sh from task 3.7, advanced feature tests from task 4.6, baseline from task 1.6 (docs/INVESTIGATION_FINDINGS.md) | _Requirements: Requirement 5 (Automated Testing), Requirement 6 (Test Coverage), Requirement 9 (Algorithmic Verification) | Success: Final report shows 100% basic substitution pass rate, advanced feature validated, dramatic improvement documented (e.g., 50% → 100%), performance maintained, comprehensive statistics, clear success metrics._

- [ ] 5.2 Update architecture documentation
  - Files: `docs/EVENT_FLOW_ARCHITECTURE.md`, update `docs/SYSTEMATIC_INVESTIGATION_SPEC.md`
  - Document final EventProcessor architecture
  - Document 3-layer flow with formal specification
  - Document invariants that are now enforced
  - Document test coverage and autonomous testing approach
  - Update investigation spec with "RESOLVED" status for all issues
  - _Leverage: Design from Phase 2, test framework from Phase 3_
  - _Requirements: All requirements_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Technical architect with expertise in system documentation | Task: Create comprehensive architecture documentation in docs/EVENT_FLOW_ARCHITECTURE.md and update docs/SYSTEMATIC_INVESTIGATION_SPEC.md following all requirements. Document final architecture, formal specification, enforced invariants, testing approach. Mark all original issues as RESOLVED. | Restrictions: Must include formal layer specifications (f₁, f₂, f₃), document invariants with proofs from tests, explain autonomous testing architecture, provide architecture diagrams, maintain consistency with implementation | _Leverage: Design document from Phase 2, test framework architecture from Phase 3, original investigation spec from docs/SYSTEMATIC_INVESTIGATION_SPEC.md | _Requirements: All requirements (comprehensive documentation) | Success: Architecture clearly documented with formal specs, invariants proven by tests, testing approach explained, original spec updated with RESOLVED status, diagrams clear, new developers can understand system quickly._

- [ ] 5.3 Create developer onboarding guide
  - Files: `docs/DEVELOPER_GUIDE.md`
  - Explain the 3-layer architecture for new developers
  - Explain how to add new key mappings
  - Explain how to run tests during development
  - Explain debugging with comprehensive logs
  - Provide examples of common modifications
  - _Leverage: Complete implementation and documentation_
  - _Requirements: Developer usability (from NFR)_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Developer advocate with expertise in onboarding documentation | Task: Create developer onboarding guide in docs/DEVELOPER_GUIDE.md. Explain 3-layer architecture, how to add mappings, run tests, debug with logs, common modifications. Make approachable for new contributors. | Restrictions: Must be beginner-friendly, include step-by-step examples, provide common troubleshooting, explain testing workflow, reference automated tests for learning, consistent with project documentation style, actionable and practical | _Leverage: Complete implementation from all phases, test framework documentation, architecture docs from task 5.2 | _Requirements: Developer usability from non-functional requirements | Success: Guide enables new developers to understand architecture in < 5 minutes (requirement), provides clear examples, testing workflow explained, debugging process documented, new contributors can add mappings confidently._

- [ ] 5.4 Clean up code and remove deprecated implementations
  - Files: Various (any old special-case code not yet removed)
  - Remove any remaining old event processing code not using EventProcessor
  - Remove deprecated special-case handling for modifiers
  - Remove any unused functions or dead code
  - Ensure code follows project style guidelines
  - Run static analysis and fix any warnings
  - _Leverage: Project code style guide, static analysis tools_
  - _Requirements: 7, code quality (NFR)_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior developer with expertise in code cleanup and refactoring | Task: Clean up codebase removing deprecated code, special cases, dead code following requirement 7 and code quality standards. Run static analysis, fix warnings, ensure style compliance. | Restrictions: Must not break working code, verify with test suite after cleanup, remove only truly unused code (check for dead code analysis), maintain git history for traceability, follow project style guide strictly | _Leverage: Project code style guide (if exists), clang-tidy or similar static analysis, test suite from Phase 3 to verify no breakage | _Requirements: Requirement 7 (Code Consistency - no special cases), code quality from non-functional requirements | Success: All deprecated code removed, no special cases remain, static analysis passes with zero warnings, code follows style guide, tests still pass 100%, codebase clean and maintainable._

- [ ] 5.5 Performance profiling and optimization
  - Files: `docs/PERFORMANCE_REPORT.md`
  - Profile event processing latency for all 3 layers
  - Verify < 1ms total processing time per event
  - Verify logging overhead < 10%
  - Profile test suite execution time (< 10 seconds for 174 tests)
  - Optimize any bottlenecks found
  - _Leverage: Profiling tools (perf, gprof), test suite_
  - _Requirements: Performance (NFR)_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Performance engineer with expertise in profiling and optimization | Task: Profile event processing and test suite performance in docs/PERFORMANCE_REPORT.md. Verify latency requirements met, optimize bottlenecks. Ensure < 1ms event processing, < 10% logging overhead, < 10 seconds test suite. | Restrictions: Must not sacrifice code clarity for micro-optimizations, use proper profiling tools not guesswork, profile in release build with optimizations, verify requirements from design phase, document before/after metrics | _Leverage: Profiling tools (perf, gprof, valgrind), test suite from Phase 3, performance requirements from design.md | _Requirements: Performance requirements from non-functional requirements (< 1ms latency, < 10% logging overhead, < 10s test suite) | Success: Performance report shows all requirements met, event processing < 1ms, logging overhead < 10%, test suite < 10 seconds, any optimizations documented with metrics, no performance regressions._

- [ ] 5.6 Final code review and sign-off
  - Files: `docs/FINAL_CODE_REVIEW.md`
  - Conduct thorough code review of all changes
  - Verify all requirements are met with evidence
  - Verify all test coverage requirements met (>90%)
  - Verify documentation is complete and accurate
  - Create final sign-off checklist
  - _Leverage: All implementation and test artifacts_
  - _Requirements: All requirements_
  - _Prompt: Implement the task for spec key-remapping-consistency, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Senior code reviewer with expertise in quality assurance | Task: Conduct final code review and create sign-off document in docs/FINAL_CODE_REVIEW.md. Verify all requirements met with evidence, test coverage >90%, documentation complete. Create final checklist. | Restrictions: Must review all modified code not just samples, verify every requirement has evidence (tests, docs, or code), check test coverage metrics, ensure documentation accuracy, provide objective assessment | _Leverage: All implementation from Phases 1-4, test coverage reports, documentation, requirements.md for checklist | _Requirements: All requirements (comprehensive review) | Success: Code review document provides objective assessment, every requirement verified with evidence, test coverage >90% confirmed, documentation complete and accurate, final checklist shows 100% completion, ready for merge._
