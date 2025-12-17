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
