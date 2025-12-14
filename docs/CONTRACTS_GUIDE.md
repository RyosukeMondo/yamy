# Contract Programming Guide

This guide explains how to use Microsoft GSL contracts in YAMY to catch bugs early and document invariants.

## Overview

YAMY uses Microsoft GSL (Guidelines Support Library) for contract programming. Contracts are preconditions and postconditions that define valid states and catch bugs at API boundaries.

**Key Benefits**:
- **Early bug detection**: Violations crash immediately in debug builds at the source
- **Self-documenting code**: Contracts explicitly state assumptions
- **Zero-cost abstractions**: Contracts are optimized out in release builds

## When to Use Each Contract Type

### Expects() - Preconditions

Use `Expects()` at the **entry point** of public APIs to validate caller assumptions.

**When to use**:
- Null pointer checks on input parameters
- Range checks on indices or enum values
- State validation (e.g., "object must be initialized")

**Example**:
```cpp
void Engine::initialize(IWindowSystem* i_windowSystem,
                        IConfigStore* i_configStore) {
    Expects(i_windowSystem != nullptr);
    Expects(i_configStore != nullptr);

    // Safe to use - preconditions guarantee validity
    m_windowSystem = i_windowSystem;
    m_configStore = i_configStore;
}
```

**Example from YAMY** (src/core/engine/engine_lifecycle.cpp:335):
```cpp
void Engine::onKey(const KeyEvent& event) {
    Expects(event.scanCode <= 0xFFFF);  // Scan codes are 16-bit

    // Process event - guaranteed valid scan code
    processKeyEvent(event);
}
```

### Ensures() - Postconditions

Use `Ensures()` at **exit points** to validate implementation guarantees.

**When to use**:
- Non-null return value guarantees
- Valid range guarantees for return values
- State consistency checks after operations

**Example**:
```cpp
Keymap* Keymap::getParent() const {
    Keymap* result = m_parent;
    Ensures(result != nullptr);  // We guarantee parent is always valid
    return result;
}
```

**Example from YAMY** (src/core/engine/engine_modifier.cpp:85):
```cpp
CurrentModifiers Engine::getCurrentModifiers() const {
    CurrentModifiers cmods = /* calculate modifiers */;

    // Invariant: Up and Down arrow modifiers are mutually exclusive
    Ensures(cmods.isPressed(Modifier::Type_Up) !=
            cmods.isPressed(Modifier::Type_Down));

    return cmods;
}
```

**Important**: Add `Ensures()` at **all return paths** in functions with postconditions:
```cpp
Keymap* findKeymap(const std::string& name) {
    if (name.empty()) {
        Keymap* result = getDefaultKeymap();
        Ensures(result != nullptr);
        return result;
    }

    Keymap* result = lookupKeymap(name);
    if (!result) {
        result = createKeymap(name);
    }
    Ensures(result != nullptr);  // Must add to all returns
    return result;
}
```

### gsl::span - Bounds-Safe Arrays

Use `gsl::span<T>` instead of pointer+size pairs to prevent buffer overruns.

**When to use**:
- Any function taking an array parameter
- Replacing `T* ptr, size_t size` pairs
- Range-based iteration over arrays

**Example**:
```cpp
// Old style - error-prone
void processKeys(const Keys* keys, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        handleKey(keys[i]);  // Easy to mess up bounds
    }
}

// New style - bounds-safe
void processKeys(gsl::span<const Keys> keys) {
    for (const auto& key : keys) {  // Automatic bounds checking
        handleKey(key);
    }
}
```

**Example from YAMY** (src/core/input/keyboard.h:421):
```cpp
class KeyIterator {
    gsl::span<Keys> m_hashedKeys;

public:
    KeyIterator(gsl::span<Keys> i_hashedKeys)
        : m_hashedKeys(i_hashedKeys) {}

    void iterate() {
        for (const auto& key : m_hashedKeys) {
            // Range-based for automatically respects bounds
        }
    }
};
```

### assert() - Internal Invariants

Use standard `assert()` for internal-only checks that should never fail.

**When to use**:
- Verifying internal logic (not caller errors)
- Checking loop invariants
- Debugging complex algorithms

**Example**:
```cpp
void Engine::processKeySequence() {
    int depth = 0;
    while (hasMoreKeys()) {
        processNextKey();
        ++depth;
        assert(depth < MAX_DEPTH);  // Internal invariant - not user error
    }
}
```

## Contracts vs Exceptions

**Use contracts for**: Programming errors (bugs)
- Null pointers passed by caller
- Invalid indices
- Precondition violations

**Use exceptions for**: Runtime errors (not bugs)
- File not found
- Network timeouts
- Out of memory

**Example**:
```cpp
void Engine::loadConfig(const std::string& path) {
    Expects(!path.empty());  // Contract - caller bug if empty

    // Exception - runtime error if file doesn't exist
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Config file not found: " + path);
    }
}
```

## Debug vs Release Behavior

YAMY configures GSL contract behavior differently per build type:

### Debug Builds
- `GSL_THROW_ON_CONTRACT_VIOLATION` defined
- Contract violations throw exceptions that crash immediately
- Violations can be caught in debugger

### Release Builds
- `GSL_UNENFORCED_ON_CONTRACT_VIOLATION` defined
- Contracts are **completely removed** (zero runtime cost)
- Assumption: debug testing caught all violations

**Configuration** (CMakeLists.txt:181-184):
```cmake
target_compile_definitions(yamy PRIVATE
    $<$<CONFIG:Debug>:GSL_THROW_ON_CONTRACT_VIOLATION>
    $<$<CONFIG:Release>:GSL_UNENFORCED_ON_CONTRACT_VIOLATION>
)
```

## Best Practices

### DO:
- ✅ Check all pointer parameters with `Expects(ptr != nullptr)`
- ✅ Validate array indices: `Expects(index < size)`
- ✅ Document contracts in Doxygen: `@pre`, `@post`
- ✅ Use `gsl::span` for all array parameters
- ✅ Add `Ensures()` to all return paths

### DON'T:
- ❌ Don't use contracts for runtime errors (use exceptions)
- ❌ Don't put side effects in contracts (they're removed in release!)
- ❌ Don't check the same condition twice (contract + if statement)
- ❌ Don't use contracts in hot loops (too expensive even in debug)
- ❌ Don't rely on contracts in release builds (they don't run!)

### Side Effects Example (WRONG):
```cpp
void processData(int* data) {
    // WRONG - incrementing in contract!
    Expects(data != nullptr && ++callCount > 0);

    // In release builds, callCount is never incremented!
}
```

**Correct version**:
```cpp
void processData(int* data) {
    Expects(data != nullptr);
    ++callCount;  // Side effect outside contract
}
```

## Common Patterns

### Pattern 1: Non-null Pointer
```cpp
void setWindowSystem(IWindowSystem* i_ws) {
    Expects(i_ws != nullptr);
    m_windowSystem = i_ws;
}
```

### Pattern 2: Valid Range
```cpp
void handleScanCode(uint32_t scanCode) {
    Expects(scanCode <= 0xFFFF);  // 16-bit scan codes only
    m_scanCode = static_cast<uint16_t>(scanCode);
}
```

### Pattern 3: Valid Enum
```cpp
void setModifier(Modifier::Type type) {
    Expects(type >= Modifier::Type_Shift && type <= Modifier::Type_Alt);
    m_currentModifier = type;
}
```

### Pattern 4: State Precondition
```cpp
void start() {
    Expects(!m_isRunning);  // Cannot start twice
    m_isRunning = true;
}
```

### Pattern 5: Mutually Exclusive States
```cpp
State getState() const {
    State result = computeState();

    // Exactly one state must be active
    Ensures((result.isIdle() ? 1 : 0) +
            (result.isActive() ? 1 : 0) +
            (result.isPaused() ? 1 : 0) == 1);

    return result;
}
```

### Pattern 6: Bounds-Safe Iteration
```cpp
void processKeyBuffer(gsl::span<const KeyEvent> events) {
    for (const auto& event : events) {  // Automatic bounds checking
        handleKeyEvent(event);
    }
}
```

## Critical Path Restrictions

**DO NOT** use contracts in the critical input processing path:
- Input event polling loop
- Key event dispatch
- Real-time modifier tracking

Even in debug builds, contracts add overhead. For critical paths, prefer:
1. Design-by-contract at API boundaries only
2. Rely on property-based tests to explore edge cases
3. Use assert() sparingly for truly impossible conditions

**Example** (input loop - no contracts):
```cpp
void inputProcessingLoop() {
    while (running) {
        KeyEvent event = pollNextEvent();

        // No contracts here - too hot
        // Preconditions validated at higher level
        dispatchEvent(event);
    }
}
```

## Testing Contracts

Verify contracts work in debug builds:

```cpp
TEST_CASE("Engine initialization validates parameters") {
    Engine engine;

    // This should throw in debug builds
    REQUIRE_THROWS([&]() {
        engine.initialize(nullptr, nullptr);
    }());
}
```

## Summary

| Contract Type | Purpose | Location | Example |
|--------------|---------|----------|---------|
| `Expects()` | Precondition | Entry point | `Expects(ptr != nullptr)` |
| `Ensures()` | Postcondition | Exit point | `Ensures(result != nullptr)` |
| `gsl::span` | Bounds safety | Parameters | `void f(gsl::span<T> data)` |
| `assert()` | Internal check | Algorithm | `assert(depth < MAX)` |

**Golden Rule**: Contracts document assumptions and catch bugs early. They are optimization hints in release, not runtime safety nets.
