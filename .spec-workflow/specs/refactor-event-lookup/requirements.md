# Requirements Document: EventProcessor Lookup Refactoring

## Introduction

This specification focuses on **Task 6** of the core engine refactoring: optimizing the `EventProcessor` event detection logic. Currently, the system performs a linear scan over all substitution rules for every key press. We will replace this with an optimized **Lookup Table** approach that indexes rules by their input scan code, significantly reducing the search space and improving determinism.

## Alignment with Product Vision

This change directly supports the "Robustness" and "Performance" pillars. As users add more complex rules (especially with the new 256 virtual modifiers), a linear scan becomes a performance bottleneck and a maintenance liability. A structured lookup table ensures that rule evaluation is consistent, fast, and easier to debug.

## Requirements

### Requirement 1: Optimized Rule Lookup

**User Story:** As a user with a large configuration file (1000+ bindings), I want my key presses to be processed instantly without lag.

#### Acceptance Criteria
1.  **Index by Scan Code:** The system SHALL index rules based on the triggering `ScanCode`.
2.  **Reduced Search Space:** Instead of iterating through *all* defined rules, the system SHALL only evaluate rules associated with the currently pressed key.
3.  **Bitwise Matching:** Modifier matching (Legacy, Virtual, Locks) SHALL be performed using efficient bitwise operations on the unified `ModifierState` bitset.

### Requirement 2: Unified "Compiled Rule" Structure

**User Story:** As a developer, I want a standardized internal representation of a "Rule" so that the event processor logic is simple and uniform.

#### Acceptance Criteria
1.  **Unified Structure:** A `CompiledRule` struct SHALL be defined that holds all necessary criteria for a match (Required ON bits, Required OFF bits).
2.  **No Dynamic Casting:** The matching logic SHALL NOT rely on checking `if (isVirtual)` or `if (isLock)` at runtime; these distinctions should be baked into the bitmasks.
3.  **Preserve Priority:** The order of rules defined in the `.mayu` file (and potential overrides) MUST be preserved in the lookup list to maintain existing precedence behavior.

### Requirement 3: Backward Compatibility

**User Story:** As a user, I expect my existing "wildcard" or "dontcare" modifiers (e.g., `key *S-A`) to continue working exactly as before.

#### Acceptance Criteria
1.  **DontCare Support:** The lookup logic MUST correctly handle "dontcare" modifiers (modifiers that are neither required ON nor required OFF).
2.  **Exact Behavior:** The result of the new lookup MUST be identical to the old linear scan for any given input state.

## Non-Functional Requirements

### Performance
- **Lookup Complexity:** O(k) where k is the number of rules *for a specific key* (typically < 10), reduced from O(N) where N is total rules.

### Guard Rails (Implementation)
- Use `std::vector` for the bucket bucket to preserve rule order (priority).
- Use `std::bitset` for mask operations to utilize CPU vector instructions where possible.
