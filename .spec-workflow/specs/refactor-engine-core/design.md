# Design Document: Core Engine Refactoring

## Overview

The refactoring transforms the monolithic `SettingLoader` and `EventProcessor` into a pipeline: **Parse -> Compile -> Execute**. This separates the concerns of reading text (Parser), verifying and structuring data (Compiler), and running the logic (Engine).

## Architecture

### 1. Configuration AST (Abstract Syntax Tree)
We introduce a lightweight data structure to represent the parsed configuration before it becomes runtime objects.

```cpp
struct ConfigAST {
    std::vector<KeyDefinition> keys;
    std::vector<ModifierDefinition> modifiers;
    std::vector<KeymapDefinition> keymaps;
    // ...
};
```

### 2. The Parser
Refactored `SettingLoader` (or a new `ConfigParser` class) reads `.mayu` text and populates `ConfigAST`.
- **Lexer/Tokenizer:** Breaks stream into tokens (existing `Parser` class can be adapted or replaced).
- **Parser Logic:** `parseModifier`, `parseKey`, `parseKeymap` methods that populate the AST.

### 3. The Compiler
A new phase that takes `ConfigAST` and builds the `Engine` runtime structures.
- Validates references (e.g., "does key X exist?").
- Resolves inheritance (keymap parents).
- Optimizes lookup tables.

### 4. The Runtime (EventProcessor)
Refactored to use a **Unified Lookup Table**.

**Old Flow:**
`processEvent` -> Linear scan of `m_substitutesList` -> Check legacy mods -> Check virtual mods -> Check locks.

**New Flow:**
`processEvent` -> Calculate `CurrentModifierMask` (Hardware | Virtual | Locks) -> `LookupTable.find(InputKey, CurrentModifierMask)` -> Execute Action.

```cpp
struct LookupKey {
    uint16_t scanCode;
    uint32_t modifierMask;
};

// Main lookup table
std::unordered_map<LookupKey, Action> m_actionTable;
```

## Technical Specifications (Guard Rails)

To ensure consistency, implementations MUST adhere to the following type definitions.

### 1. AST Data Structures (`src/core/settings/config_ast.h`)

The AST should be a set of POD (Plain Old Data) structs or simple classes with `std::` containers.

```cpp
#pragma once
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace yamy::ast {

// Represents a raw key token (e.g., "*A", "S-", "M00-")
struct ModifierToken {
    std::string text;
    // For M00-MFF, L00-LFF, or standard names
};

struct KeyToken {
    std::string name;
    // e.g., "A", "Space", "V_MyKey"
};

struct KeyAssignment {
    // Left side: The modified key triggering the action
    std::vector<ModifierToken> modifiers;
    KeyToken key;

    // Right side: The sequence of actions
    // Note: In AST, we keep this as raw tokens or a lightweight ActionAST
    // rather than full Runtime Actions.
    // For Phase 1, we might wrap the existing Action* or KeySeq* 
    // if we haven't fully decoupled Action parsing yet.
    // Ideally:
    std::string actionSource; // Raw string for now, or parsed Action nodes
};

struct KeymapDefinition {
    std::string name;
    std::string parentName; // Optional inheritance
    std::string windowClassRegex;
    std::string windowTitleRegex;
    std::vector<KeyAssignment> assignments;
};

struct ConfigAST {
    std::vector<std::string> includedFiles;
    std::vector<KeymapDefinition> keymaps;
    // ... other definitions like 'def key', 'def mod'
};

} // namespace yamy::ast
```

### 2. SettingLoader Integration

The `SettingLoader` currently mixes parsing and object creation.
**Interim Step (Task 2-4):**
Pass state explicitly instead of `thread_local`.

```cpp
// Instead of s_pendingVirtualMod (thread_local)
struct ParserContext {
    uint8_t pendingVirtualMod = 0xFF;
    bool hasVirtualMod = false;
    // ... other transient parse state
};

// Functions accept context
bool parseMxxModifier(..., ParserContext& ctx);
KeySeq* load_KEY_SEQUENCE(..., ParserContext& ctx);
```

### 3. Unified Modifier State (`src/core/input/modifier_state.h`)

We need a unified bitset to hold:
- 8 Standard Modifiers (Shift, Ctrl, Alt, Win x L/R)
- 32 Legacy Modifiers (if any remain)
- 256 Virtual Modifiers (M00-MFF)
- 256 Lock States (L00-LFF)

```cpp
#include <bitset>

class UnifiedModifierState {
public:
    static constexpr size_t STD_OFFSET = 0;
    static constexpr size_t VIRT_OFFSET = 16; // Leave space for standard
    static constexpr size_t LOCK_OFFSET = VIRT_OFFSET + 256;
    static constexpr size_t TOTAL_BITS = LOCK_OFFSET + 256; // ~528 bits

    std::bitset<TOTAL_BITS> state;

    void setVirtual(uint8_t index, bool val) {
        state[VIRT_OFFSET + index] = val;
    }
    // ...
};
```

## Component Changes


### `src/core/settings/setting_loader.h/cpp`
- **Change:** Remove direct calls to `m_setting->m_keyboard.addKey()`.
- **Add:** `ConfigAST` member.
- **Refactor:** `load_MODIFIER` to use helper functions (Done).

### `src/core/input/modifier_state.h`
- **Change:** Expand to support 256 bits (or more) for M00-MFF + L00-LFF + Standard.
- **Implementation:** `std::bitset<300>` or custom `BitVector`.

### `src/core/engine/engine_event_processor.cpp`
- **Change:** Replace `layer2_applySubstitution` linear scan with `m_actionTable` lookup.

## Migration Strategy

1.  **Refactor Parser Logic (In-Place):** Clean up `SettingLoader` logic while still populating legacy structures (Current State).
2.  **Introduce AST:** Modify `SettingLoader` to populate AST side-by-side or instead of legacy.
3.  **Implement Compiler:** Build legacy structures from AST.
4.  **Refactor Engine:** Switch Engine to use optimized lookup table built by Compiler.

## Error Handling
- Parser errors (syntax) are caught during Phase 1.
- Compiler errors (invalid references, cycles) are caught during Phase 2.
- Runtime errors are logged via `JourneyLogger`.
