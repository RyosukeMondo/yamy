# Code Metrics Policy

## Overview

YAMY enforces automated code quality metrics to maintain maintainability and reduce technical debt. These metrics are checked in CI and via pre-commit hooks.

## Metrics Limits

| Metric | Limit | Rationale |
|--------|-------|-----------|
| **Lines per file** | 500 | Files exceeding 500 lines typically violate Single Responsibility Principle and become difficult to navigate and test |
| **Lines per function** | 50 | Functions over 50 lines are hard to understand, test, and maintain. They usually have multiple responsibilities |
| **Cyclomatic Complexity (CCN)** | 15 | CCN >15 indicates too many decision points, making code hard to test exhaustively and reason about |

**Note**: Line counts exclude blank lines and comments to focus on actual code density.

## Rationale

### Why 500 Lines Per File?

**Cognitive Load**: Beyond 500 lines, developers struggle to keep the entire file's context in mind.

**Single Responsibility**: Large files often mix multiple concerns (e.g., parsing, validation, and execution).

**Testability**: Smaller files encourage focused, testable components.

**Good Example**:
```cpp
// src/core/engine/engine_keyboard_handler.cpp - 150 lines
// Single responsibility: handle keyboard events
class KeyboardEventHandler {
    void handleKeyDown(const KeyEvent& event);
    void handleKeyUp(const KeyEvent& event);
};
```

**Bad Example**:
```cpp
// engine.cpp - 680 lines (before refactoring)
// Mixed: event handling, IPC, state management, configuration
class Engine {
    void handleKeyDown(...);
    void handleIPC(...);
    void loadConfig(...);
    void validateState(...);
    // ... 50+ more methods
};
```

### Why 50 Lines Per Function?

**Single Level of Abstraction**: Functions over 50 lines typically mix abstraction levels (high-level logic + low-level details).

**Testability**: Long functions require complex test setups and often have hidden dependencies.

**Debuggability**: Smaller functions make stack traces more informative and bugs easier to isolate.

**Good Example**:
```cpp
// 15 lines - focused, single abstraction level
void processKeyEvent(const KeyEvent& event) {
    Expects(event.isValid());

    auto mapping = findKeyMapping(event.scanCode);
    if (!mapping) {
        return;
    }

    if (shouldSuppressKey(mapping)) {
        suppressEvent();
    } else {
        executeAction(mapping.action);
    }
}
```

**Bad Example**:
```cpp
// 156 lines - multiple abstractions, hard to test
void beginGeneratingKeyboardEvents(...) {
    // Validate inputs (10 lines)
    // Lock state (5 lines)
    // Check modifiers (20 lines)
    // Look up mappings (30 lines)
    // Apply layer logic (25 lines)
    // Generate events (40 lines)
    // Update state (15 lines)
    // Log results (11 lines)
}
```

### Why CCN ≤ 15?

**Test Coverage**: CCN represents minimum test cases needed for branch coverage. CCN >15 requires 15+ tests.

**Bug Density**: Research shows exponential bug increase beyond CCN 15 (McConnell, Code Complete).

**Understandability**: High complexity makes code review ineffective - reviewers can't reason about all paths.

**Good Example**:
```cpp
// CCN = 4 (simple control flow)
void adjustModifier(ModifierKey key) {
    if (!isModifier(key)) return;        // +1

    if (isPressed(key)) {                // +2
        addActiveModifier(key);
    } else {                             // +3
        removeActiveModifier(key);
    }
}
```

**Bad Example**:
```cpp
// CCN = 54 (from engine_generator.cpp:beginGeneratingKeyboardEvents)
// Contains deeply nested if/else, multiple switch statements,
// exception handling, and early returns - extremely hard to test
void complexFunction() {
    if (condition1) {                    // +1
        switch (type) {                  // +1
            case A: if (x) ...           // +2
            case B: if (y) ...           // +2
            // ... 20+ more branches
        }
    } else if (condition2) {             // +1
        for (auto& item : items) {       // +1
            if (filter(item)) {          // +1
                try { ... }              // +1
                catch (...) { ... }      // +1
                // ... more nesting
            }
        }
    }
    // ... 40+ more decision points
}
```

## Enforcement

### Pre-Commit Hook

Automatically checks staged C++ files:

```bash
# Install
./scripts/install-hooks.sh

# Bypass (use sparingly!)
git commit --no-verify
```

**Output Example**:
```
Running code metrics check on staged files...

src/core/parser.cpp
  Line 245: parseExpression(...)
    ⚠ Function length: 65 (max: 50)
    ⚠ Cyclomatic complexity: 18 (max: 15)

Commit rejected.
```

### CI Pipeline

All PRs run `cmake --build . --target check-metrics`:

```bash
# Local check (same as CI)
cmake --build build --target check-metrics
```

Violations **block PR merges**.

### Manual Check

```bash
# Check entire src/ directory
lizard --length 500 --arguments 50 --CCN 15 -w src/

# Check specific file
lizard --length 50 --CCN 15 myfile.cpp
```

## Requesting Exceptions

Some files may legitimately exceed limits due to architectural constraints. Follow this process:

### 1. Document Justification

Create entry in `docs/CODE_METRICS_VIOLATIONS.md`:

```markdown
### ⚠️ Deferred (Architectural Complexity)

| File | Lines | Reason | Plan |
|------|-------|--------|------|
| parser.cpp | 536 | UTF-8 parsing, complex state machine | Refactor in Q2 2025 |
```

### 2. Required Justification Criteria

At least **one** must apply:

- **Critical Infrastructure**: Core parsing/config logic used throughout codebase
- **High Risk**: Refactoring requires extensive test coverage not yet in place
- **Architectural Debt**: Requires design changes, not just code splitting
- **Third-Party Code**: Generated or vendored code we don't control

### 3. Approval Required

- Team lead approval for exceptions
- Must include refactoring plan with timeline
- Exception reviewed quarterly

### 4. Temporary Bypass

For approved exceptions, update `CMakeLists.txt`:

```cmake
add_custom_target(check-metrics
    COMMAND ${LIZARD_PROGRAM}
        --length 500
        --arguments 50
        --CCN 15
        --exclude "*/parser.cpp"  # Documented exception
        --ignore_warnings 1000
        ${CMAKE_SOURCE_DIR}/src
    ...
)
```

## Refactoring Strategies

### Splitting Large Files

**Extract by Responsibility**:
```cpp
// Before: engine.cpp (680 lines)
class Engine { ... };

// After:
// engine.cpp (20 lines) - facade
// engine_keyboard_handler.cpp (150 lines)
// engine_ipc_handler.cpp (180 lines)
// engine_state.cpp (120 lines)
```

### Splitting Long Functions

**Extract Helper Functions**:
```cpp
// Before: 80 lines
void load_MODIFIER_ASSIGNMENT(...) {
    // Parse modifier name (15 lines)
    // Validate syntax (20 lines)
    // Look up modifiers (25 lines)
    // Apply assignment (20 lines)
}

// After: 4 × 20 lines each
void load_MODIFIER_ASSIGNMENT(...) {
    auto modName = parseModifierName(...);
    validateModifierSyntax(modName);
    auto modifiers = lookupModifiers(modName);
    applyModifierAssignment(modifiers, ...);
}
```

### Reducing Complexity

**Replace Nested Conditions with Early Returns**:
```cpp
// Before: CCN = 8
void process(Item item) {
    if (item.isValid()) {
        if (item.isActive()) {
            if (hasPermission()) {
                doWork(item);
            }
        }
    }
}

// After: CCN = 4
void process(Item item) {
    if (!item.isValid()) return;
    if (!item.isActive()) return;
    if (!hasPermission()) return;
    doWork(item);
}
```

**Extract Complex Conditions**:
```cpp
// Before: CCN = 6
if (user.isAdmin() || (user.isPowerUser() && project.isOpen()) || user.id == project.owner) {
    allowAccess();
}

// After: CCN = 2
if (hasProjectAccess(user, project)) {
    allowAccess();
}

bool hasProjectAccess(User user, Project project) {
    return user.isAdmin()
        || (user.isPowerUser() && project.isOpen())
        || user.id == project.owner;
}
```

**Replace Switch with Polymorphism** (when CCN from switch/case):
```cpp
// Before: CCN = 10 (8 cases)
void handleEvent(EventType type) {
    switch (type) {
        case KEY_DOWN: ...
        case KEY_UP: ...
        // ... 6 more cases
    }
}

// After: CCN = 1 per handler
class EventHandler {
    virtual void handle() = 0;
};

class KeyDownHandler : public EventHandler { ... };
class KeyUpHandler : public EventHandler { ... };
```

## Tools

### Lizard

**Installation**:
```bash
pip install lizard
# or
pipx install lizard
```

**Usage**:
```bash
# Basic check
lizard src/

# With limits
lizard --length 50 --CCN 15 src/

# Warnings only
lizard --length 50 --CCN 15 -w src/

# Specific file types
lizard -l cpp src/

# JSON output (for tooling)
lizard --json src/ > metrics.json
```

**CI Integration**: See `.github/workflows/ci.yml`

## References

- McConnell, Steve. *Code Complete 2nd Edition*. Microsoft Press, 2004.
  - Chapter 7: High-Quality Routines (50-line guideline)
  - Chapter 19: Complexity metrics and maintainability

- Martin, Robert C. *Clean Code*. Prentice Hall, 2008.
  - Single Responsibility Principle
  - Functions should do one thing

- Thomas J. McCabe. ["A Complexity Measure"](https://www.literateprogramming.com/mccabe.pdf). IEEE Transactions on Software Engineering, 1976.
  - Original cyclomatic complexity paper
  - Recommends CCN ≤ 10 for critical code

## Summary

| Metric | Limit | Check Frequency | Enforcement |
|--------|-------|-----------------|-------------|
| Lines/file | 500 | Pre-commit + CI | Blocking |
| Lines/function | 50 | Pre-commit + CI | Blocking |
| CCN | 15 | Pre-commit + CI | Blocking |

**Bypass**: Use `--no-verify` for commits, but violations still block PRs in CI.

**Exceptions**: Documented in `CODE_METRICS_VIOLATIONS.md`, require justification + refactoring plan.

**Goal**: Maintain code quality, not create bureaucracy. Metrics catch problems early, making refactoring cheaper.
