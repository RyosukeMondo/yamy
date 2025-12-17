# Design Document: EventProcessor Lookup Refactoring

## Overview

We will implement a **Bucket-Based Lookup Table**. Instead of one giant list of substitutions, we will organize substitutions into buckets based on the input `ScanCode`. Inside each bucket, we keep a small list of `CompiledRule` objects.

When a key is pressed, `EventProcessor` jumps directly to the bucket for that key and checks only the relevant rules against the unified `ModifierState`.

## Architecture

### 1. Compiled Rule Representation (`src/core/engine/compiled_rule.h`)

This struct flattens the complex `KeyAssignment` logic into pure bitmasks.

```cpp
#pragma once
#include <bitset>
#include "../input/modifier_state.h" // For ModifierState::TOTAL_BITS

namespace yamy::engine {

struct CompiledRule {
    // Bitmask of modifiers that MUST be active (1)
    std::bitset<yamy::input::ModifierState::TOTAL_BITS> requiredOn;

    // Bitmask of modifiers that MUST be inactive (0)
    std::bitset<yamy::input::ModifierState::TOTAL_BITS> requiredOff;

    // The output scan code (or action ID)
    uint16_t outputScanCode; 
    
    // Future: Action abstraction (e.g., KeySeq*, FunctionData*)
    // void* actionData; 

    // Helper to check if this rule matches the current state
    bool matches(const std::bitset<yamy::input::ModifierState::TOTAL_BITS>& currentState) const {
        // 1. Check ON requirements: (State & OnMask) == OnMask
        if ((currentState & requiredOn) != requiredOn) return false;

        // 2. Check OFF requirements: (~State & OffMask) == OffMask
        //    Equivalently: (State & OffMask) == 0
        if ((currentState & requiredOff).any()) return false;

        return true;
    }
};

} // namespace yamy::engine
```

### 2. Lookup Table (`src/core/engine/lookup_table.h`)

```cpp
#pragma once
#include <vector>
#include <unordered_map>
#include "compiled_rule.h"

namespace yamy::engine {

class RuleLookupTable {
public:
    // Add a rule to the table
    void addRule(uint16_t inputScanCode, const CompiledRule& rule) {
        m_buckets[inputScanCode].push_back(rule);
    }

    // clear table
    void clear() { m_buckets.clear(); }

    // Find the first matching rule
    const CompiledRule* findMatch(uint16_t scanCode, const std::bitset<yamy::input::ModifierState::TOTAL_BITS>& state) const {
        auto it = m_buckets.find(scanCode);
        if (it == m_buckets.end()) return nullptr;

        const auto& rules = it->second;
        for (const auto& rule : rules) {
            if (rule.matches(state)) {
                return &rule;
            }
        }
        return nullptr;
    }

private:
    // Map ScanCode -> Vector of Rules (Ordered by priority)
    std::unordered_map<uint16_t, std::vector<CompiledRule>> m_buckets;
};

} // namespace yamy::engine
```

### 3. Compilation Logic (`src/core/engine/engine.cpp` or `config_compiler.cpp`)

The `buildSubstitutionTable` method in `Engine` needs to be updated (or a new `compileRules` method added) to populate this table.

**Algorithm:**
1.  Iterate through `Keyboard::getSubstitutes()` (legacy list).
2.  For each substitute:
    a.  Extract `fromKey` scan code.
    b.  Construct `requiredOn` and `requiredOff` bitsets.
        *   Iterate all Modifier types (Shift, Ctrl, M00, L00...).
        *   If modifier is `PRESS` -> Set bit in `requiredOn`.
        *   If modifier is `RELEASE` (or not present/implied off) -> Set bit in `requiredOff`.
        *   If modifier is `DONTCARE` -> Set bit in neither.
    c.  `m_lookupTable.addRule(fromKey, rule)`.

## Integration Points

-   **`src/core/engine/engine_event_processor.h`**: Add `RuleLookupTable` member.
-   **`src/core/engine/engine_event_processor.cpp`**: Update `layer2_applySubstitution` to use `findMatch`.

## Technical Specifications (Guard Rails)

*   **Namespace:** All new engine structs must be in `yamy::engine`.
*   **Bitset Size:** MUST use `yamy::input::ModifierState::TOTAL_BITS`.
*   **Matching Logic:** MUST use the bitwise logic defined above (`(S & On) == On && (S & Off) == 0`).
