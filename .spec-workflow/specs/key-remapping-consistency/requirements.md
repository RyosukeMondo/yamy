# Requirements Document: Key Remapping Consistency

## Introduction

This specification addresses systematic inconsistencies in YAMY's Linux key remapping implementation. Currently, the system exhibits asymmetric behavior where some keys work correctly (W→A), some work partially (R/T only on key release), and others don't work at all (N→LShift modifier substitution). This project will transform the system from heuristic debugging to algorithmic correctness through:

1. **Systematic investigation** of the 3-layer key transformation pipeline
2. **Unified event processing** with consistent behavior for all keys
3. **Autonomous testing framework** requiring zero user interaction
4. **Advanced feature**: Number keys as custom modifier keys (for small keyboards without ten-keys)

The value to users is a **reliable, predictable, and testable** key remapping system where all substitutions work consistently, regardless of key type, event type, or target mapping.

## Alignment with Product Vision

This aligns with YAMY's core mission to provide **cross-platform keyboard remapping** by:
- Ensuring Linux implementation matches Windows YAMY's reliability
- Providing systematic debugging and testing capabilities
- Supporting advanced use cases (custom modifiers for small keyboards)
- Establishing architectural consistency for future development

## Requirements

### Requirement 1: Universal Event Processing

**User Story:** As a YAMY user, I want ALL key substitutions to work identically, regardless of whether the key is a letter, number, modifier, or special key, so that I can rely on consistent behavior across my entire keyboard configuration.

#### Acceptance Criteria

1. **WHEN** a user presses any key with a defined substitution **THEN** the system **SHALL** process the PRESS event and output the substituted key immediately
2. **WHEN** a user releases any key with a defined substitution **THEN** the system **SHALL** process the RELEASE event and output the substituted key release
3. **WHEN** a key event enters Layer 1 **THEN** the system **SHALL** process it through Layer 2 (substitution) and Layer 3 (output) without skipping any layer
4. **IF** a substitution maps to a modifier key (e.g., N→LShift) **THEN** the system **SHALL** process it identically to regular key substitutions (e.g., W→A)
5. **WHEN** comparing two different key substitutions **THEN** the event processing path **SHALL** be identical with no key-specific branches

### Requirement 2: Event Type Consistency

**User Story:** As a YAMY user, I want key presses and releases to be processed symmetrically, so that all my key mappings work correctly during both press and release events.

#### Acceptance Criteria

1. **WHEN** a PRESS event for key K with substitution K→T is received **THEN** the system **SHALL** output a PRESS event for key T
2. **WHEN** a RELEASE event for key K with substitution K→T is received **THEN** the system **SHALL** output a RELEASE event for key T
3. **IF** a key works on PRESS events **THEN** it **SHALL** also work on RELEASE events
4. **IF** a key works on RELEASE events **THEN** it **SHALL** also work on PRESS events
5. **WHEN** processing any key event **THEN** the system **SHALL NOT** have different code paths for PRESS vs RELEASE (except for preserving the event type flag)

### Requirement 3: Layer Completeness Invariant

**User Story:** As a YAMY developer, I want all key events to flow through all three transformation layers (Input→Substitution→Output), so that I can trace and debug any remapping issue systematically.

#### Acceptance Criteria

1. **WHEN** any key event is received **THEN** Layer 1 **SHALL** map evdev code to YAMY scan code and log the transformation
2. **WHEN** Layer 1 completes **THEN** Layer 2 **SHALL** check for substitutions and apply them (or pass through unchanged) and log the result
3. **WHEN** Layer 2 completes **THEN** Layer 3 **SHALL** map YAMY scan code to output evdev code and log the transformation
4. **IF** Layer 1 processes an event **AND** Layer 2/3 do not appear in logs **THEN** the system **SHALL** be considered in violation (failure condition)
5. **WHEN** examining debug logs **THEN** every key event **SHALL** show LAYER1→LAYER2→LAYER3 progression

### Requirement 4: Comprehensive Logging and Traceability

**User Story:** As a YAMY developer, I want complete event tracing for every key transformation, so that I can identify exactly where and why a remapping fails without user interaction.

#### Acceptance Criteria

1. **WHEN** debug logging is enabled **THEN** the system **SHALL** log every event with: evdev code, event type (PRESS/RELEASE), YAMY scan code, substitution result, and output evdev code
2. **WHEN** a key event is processed **THEN** the log **SHALL** include: `[LAYER1:IN]`, `[LAYER2:OUT]`, `[LAYER3:OUT]` markers for each layer
3. **IF** a substitution is found **THEN** the log **SHALL** show: `[LAYER2:SUBST] Original→Target`
4. **IF** no substitution exists **THEN** the log **SHALL** show: `[LAYER2:PASSTHROUGH]`
5. **WHEN** analyzing failures **THEN** logs **SHALL** be sufficient to identify the exact layer and code location of the issue

### Requirement 5: Automated Testing Framework (Zero User Interaction)

**User Story:** As a YAMY developer, I want a fully automated test suite that verifies all key mappings without any user interaction, so that I can confidently refactor code and catch regressions immediately.

#### Acceptance Criteria

1. **WHEN** the test suite runs **THEN** it **SHALL** test all 87 key substitutions from config_clean.mayu without requiring any manual key presses
2. **WHEN** testing a key mapping **THEN** the system **SHALL** inject synthetic key events, capture debug logs, and verify output automatically
3. **WHEN** the test completes **THEN** it **SHALL** generate a report showing: total tests, passed, failed, and specific failures with input→expected→actual
4. **IF** any test fails **THEN** the report **SHALL** identify: which key, which event type (PRESS/RELEASE), and which layer failed
5. **WHEN** running in CI/CD **THEN** the test suite **SHALL** exit with status 0 for pass, non-zero for failures

### Requirement 6: Unit, Integration, and E2E Test Coverage

**User Story:** As a YAMY developer, I want comprehensive test coverage at all levels (unit, integration, E2E), so that I can verify individual components, layer interactions, and complete end-to-end flows systematically.

#### Acceptance Criteria

1. **WHEN** running unit tests **THEN** the system **SHALL** test individual layer functions: evdevToYamyKeyCode(), substitution lookup, yamyToEvdevKeyCode()
2. **WHEN** running integration tests **THEN** the system **SHALL** test layer combinations: Layer1→Layer2, Layer2→Layer3, Layer1→Layer2→Layer3
3. **WHEN** running E2E tests **THEN** the system **SHALL** test complete event flows: synthetic input → engine processing → virtual output verification
4. **IF** any layer function changes **THEN** unit tests **SHALL** catch behavioral changes
5. **WHEN** refactoring event processing **THEN** integration tests **SHALL** verify layers still communicate correctly

### Requirement 7: Code Consistency and No Special Cases

**User Story:** As a YAMY developer, I want a single canonical event processing path with no key-specific or type-specific branches, so that the code is obvious, maintainable, and impossible to break asymmetrically.

#### Acceptance Criteria

1. **WHEN** processing any key event **THEN** the code **SHALL** use the same function call sequence regardless of key type
2. **IF** modifier key substitution code exists **THEN** it **SHALL NOT** have different logic than regular key substitution code
3. **WHEN** processing PRESS and RELEASE events **THEN** the code **SHALL NOT** branch based on event type (except to preserve the flag)
4. **IF** special handling is required **THEN** it **SHALL** be generalized and applied uniformly to all keys
5. **WHEN** reviewing the event processing code **THEN** there **SHALL** be zero instances of key-specific conditionals (e.g., `if (key == N) { special_case() }`)

### Requirement 8: Advanced Feature - Number Keys as Custom Modifiers

**User Story:** As a YAMY user with a small keyboard without ten-keys, I want to use number keys (0-9) as custom modifier keys, so that I can create complex key combinations without running out of physical keys.

#### Acceptance Criteria

1. **WHEN** a user defines a number key as a modifier in .mayu (e.g., `mod mod4 = !!_1`) **THEN** the system **SHALL** treat it as a modifier key that can be held
2. **WHEN** a number key is held as a modifier **AND** another key is pressed **THEN** the system **SHALL** activate the corresponding modal layer
3. **IF** the same number key has both a substitution and a modifier definition **THEN** the modifier behavior **SHALL** take precedence when held, and substitution when tapped
4. **WHEN** the user releases the number modifier key **THEN** the system **SHALL** deactivate the modal layer
5. **WHEN** wiring number keys directly to hardware modifiers **THEN** the system **SHALL** support mapping to: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin

### Requirement 9: Algorithmic Verification

**User Story:** As a YAMY developer, I want the system's correctness to be algorithmically provable through formal specifications, so that behavior is deterministic and not based on heuristic testing.

#### Acceptance Criteria

1. **WHEN** defining layer transformations **THEN** each layer **SHALL** be specified as a pure function: `f(input) → output`
2. **WHEN** composing layers **THEN** the complete transformation **SHALL** be: `output = f₃(f₂(f₁(input)))`
3. **IF** property-based tests are run **THEN** they **SHALL** verify: ∀ key ∈ ALL_KEYS, ∀ event_type ∈ {PRESS, RELEASE}: process(key, event_type) = expected_output
4. **WHEN** a key mapping is defined **THEN** the system **SHALL** guarantee the transformation or fail explicitly (no silent passthrough)
5. **WHEN** analyzing test results **THEN** 100% of defined substitutions **SHALL** pass verification

## Non-Functional Requirements

### Code Architecture and Modularity

- **Single Responsibility Principle**:
  - Each layer (Input, Substitution, Output) in separate, focused modules
  - Event processing logic isolated from logging and debugging
  - Test framework separated from production code

- **Modular Design**:
  - Layer functions are pure: no side effects, deterministic output
  - Substitution tables loaded once, immutable during runtime
  - Event injection utilities reusable across unit/integration/E2E tests

- **Dependency Management**:
  - Layers communicate through well-defined interfaces (scan codes)
  - No Layer 3 → Layer 1 dependencies
  - Test framework depends on production code but not vice versa

- **Clear Interfaces**:
  - `evdevToYamyKeyCode(evdev: u16) → yamy: u16`
  - `applySubstitution(yamy: u16) → yamy: u16`
  - `yamyToEvdevKeyCode(yamy: u16) → evdev: u16`
  - `injectKeyEvent(evdev: u16, type: EventType) → void`

### Performance

- **Event Processing Latency**: Each layer transformation **SHALL** complete in < 1ms
- **Logging Overhead**: Debug logging **SHALL** add < 10% latency overhead
- **Test Suite Execution**: Complete test suite (87 keys × 2 event types = 174 tests) **SHALL** complete in < 10 seconds
- **Memory Usage**: Substitution tables **SHALL** consume < 1KB RAM (87 entries × 12 bytes/entry)

### Reliability

- **Zero Dropped Events**: System **SHALL** process 100% of input events without loss
- **Crash Recovery**: If engine crashes, **SHALL** restore state and resume within 1 second
- **Error Handling**: All layer functions **SHALL** handle invalid inputs gracefully (return 0 or passthrough)
- **Test Repeatability**: Tests **SHALL** produce identical results on repeated runs (no flakiness)

### Usability (Developer Experience)

- **Obvious Code**: A new developer **SHALL** understand the event flow in < 5 minutes of reading
- **Trivial Debugging**: Any remapping issue **SHALL** be traceable to a specific line of code using logs
- **Safe Refactoring**: Automated tests **SHALL** catch 100% of breaking changes
- **Clear Documentation**: Each function **SHALL** have a comment explaining: inputs, outputs, and purpose

### Maintainability

- **Code Clarity**: Event processing **SHALL** be < 200 lines of code (no complexity bloat)
- **Test Coverage**: **SHALL** maintain > 90% code coverage for layer functions
- **Documentation**: All architectural decisions **SHALL** be documented in design.md
- **Regression Prevention**: CI/CD **SHALL** run full test suite on every commit

### Compatibility

- **Backward Compatibility**: Existing .mayu files **SHALL** continue to work without modification
- **Platform Support**: **SHALL** work on all Linux distributions with evdev support
- **Keyboard Layout Support**: **SHALL** support US and JP keyboard layouts
- **Future Extensions**: Architecture **SHALL** support adding new layers without breaking existing code
