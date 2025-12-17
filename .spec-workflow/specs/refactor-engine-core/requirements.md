# Requirements Document: Core Engine Refactoring

## Introduction

This specification outlines the refactoring of the Yamy core engine, specifically focusing on the `SettingLoader` (parsing) and `EventProcessor` (execution) subsystems. The goal is to decouple parsing from execution, modernize the codebase, and enable support for advanced features like 256 virtual modifiers (M00-MFF) without legacy hacks.

## Alignment with Product Vision

This refactoring aligns with the goal of making Yamy a robust, cross-platform keyboard remapping tool. By decoupling the parser, we enable better testing and future support for alternative config formats (e.g., JSON/YAML). Optimizing the event processor ensures low latency and reliable behavior.

## Requirements

### Requirement 1: Decoupled Parsing

**User Story:** As a developer, I want the parser to be a standalone component that produces a configuration object, so that I can test it in isolation and potentially support multiple configuration formats.

#### Acceptance Criteria
1.  **Intermediate Representation:** `SettingLoader` SHALL produce a `ConfigAST` (Abstract Syntax Tree) or intermediate struct representation of the configuration.
2.  **No Direct Mutation:** The parsing phase SHALL NOT directly call methods on `Engine`, `Keyboard`, or `Window` classes during the tokenization phase.
3.  **Validation:** Validation SHALL occur after parsing is complete, operating on the intermediate representation.

### Requirement 2: Robust Modifier Parsing

**User Story:** As a user, I want reliable support for all 256 virtual modifiers (M00-MFF) and locks (L00-LFF) without syntax errors or edge cases.

#### Acceptance Criteria
1.  **Standardized Parsing:** The parser SHALL use a unified logic for standard (S-, C-) and virtual (Mxx-, Lxx-) modifiers.
2.  **No Goto Logic:** The parsing loop SHALL use structured control flow (loops/conditionals) instead of `goto` labels.
3.  **Data-Driven:** Modifier string-to-enum mappings SHALL be defined in a lookup table/map, not hardcoded if-else chains.

### Requirement 3: Unified Event Processing

**User Story:** As a user, I expect consistent behavior whether I use hardware modifiers, virtual modifiers, or locks.

#### Acceptance Criteria
1.  **Unified Lookup:** `EventProcessor` SHALL use a single O(1) lookup mechanism (e.g., a hash map or direct table) for determining key actions, replacing the linear scan of substitutions.
2.  **Unified State:** Hardware modifiers, virtual modifiers, and locks SHALL be treated uniformly as bits in a state vector during lookup.
3.  **Latency:** Event processing overhead SHALL be minimized by removing redundant list iterations.

### Requirement 4: Removal of Legacy Hacks

**User Story:** As a developer, I want a clean codebase free of thread-local storage hacks for passing state between components.

#### Acceptance Criteria
1.  **No Thread-Local Storage:** The usage of `thread_local` variables for passing M00-MFF state in `SettingLoader` SHALL be removed.
2.  **Explicit State Passing:** State SHALL be passed explicitly via function arguments or context objects.

## Non-Functional Requirements

### Code Architecture
- **Separation of Concerns:** distinct `Parser`, `Compiler`, and `Runtime` layers.
- **Maintainability:** Reduce Cyclomatic Complexity of `SettingLoader::load_MODIFIER` and `EventProcessor::processEvent`.

### Performance
- **Startup Time:** Configuration loading time should remain under 1 second for standard files.
- **Input Latency:** Processing latency per key event should not increase (goal: < 1ms).
