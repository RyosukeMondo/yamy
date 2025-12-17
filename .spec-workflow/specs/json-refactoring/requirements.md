# JSON Refactoring Requirements

## Overview

Replace the complex .mayu text parser with a simple JSON-based configuration system, focusing on core key remapping functionality and M00-MFF virtual modifiers while removing ~5,000 LOC of unnecessary complexity.

## User Requirements

### FR-1: JSON Configuration Format
**Priority**: P0 (Critical)
**Description**: Users must be able to configure YAMY using JSON files instead of .mayu text files.

**Acceptance Criteria**:
- [ ] JSON schema supports keyboard key definitions (scan code mappings)
- [ ] JSON schema supports basic modifiers (Shift, Ctrl, Alt, Win)
- [ ] JSON schema supports M00-MFF virtual modifiers with tap actions
- [ ] JSON schema supports key mappings (from → to rules)
- [ ] JSON schema supports key sequences (output multiple keys)
- [ ] Schema is documented with examples
- [ ] JSON files load in <10ms (vs ~100ms for .mayu)

**Example**:
```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "CapsLock": "0x3a"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tap": "Escape",
      "holdThresholdMs": 200
    }
  },
  "mappings": [
    {
      "from": "A",
      "to": "Tab"
    },
    {
      "from": "M00-A",
      "to": "Left"
    },
    {
      "from": "M00-B",
      "to": ["Escape", "B"]
    }
  ]
}
```

---

### FR-2: M00-MFF Virtual Modifier Support
**Priority**: P0 (Critical)
**Description**: Users must be able to define and use M00-MFF virtual modifiers for modal key remapping.

**Acceptance Criteria**:
- [ ] All 256 virtual modifiers (M00-MFF) are supported
- [ ] Hold-vs-tap detection works with 200ms threshold (configurable)
- [ ] Tap actions output specified keys when quickly pressed
- [ ] Hold actions activate modifiers for combinations
- [ ] Virtual modifiers work identically to .mayu version
- [ ] M00-MFF combinations work (e.g., Shift-M00-A)

**Rationale**: M00-MFF system is well-designed and critical for vim/emacs-style modal editing.

---

### FR-3: Global Keymap Only (No Per-Window)
**Priority**: P0 (Critical)
**Description**: Phase 1 focuses on a single global keymap, removing per-window configuration complexity.

**Acceptance Criteria**:
- [ ] All key mappings apply globally (no window matching)
- [ ] No regex window class/title matching
- [ ] No thread-specific keymaps
- [ ] Engine simplified to use single keymap reference

**Rationale**: Per-window keymaps add ~800 LOC of complexity. Can be added later if users request it.

---

### FR-4: Key Sequence Support
**Priority**: P1 (High)
**Description**: Users must be able to output sequences of multiple keys.

**Acceptance Criteria**:
- [ ] JSON supports array syntax for key sequences: `["Escape", "B"]`
- [ ] Sequences execute in order with proper timing
- [ ] Works with virtual modifiers (e.g., M00-B → Esc+B)

**Rationale**: Essential for vim-like workflows and chording.

---

### FR-5: Standard Modifier Support
**Priority**: P0 (Critical)
**Description**: Users must be able to use standard modifiers in mappings.

**Acceptance Criteria**:
- [ ] Shift modifier supported (`Shift-A`)
- [ ] Ctrl modifier supported (`Ctrl-A`)
- [ ] Alt modifier supported (`Alt-A`)
- [ ] Win modifier supported (`Win-A`)
- [ ] Modifiers can combine (`Shift-Ctrl-A`)
- [ ] Modifiers work with M00-MFF (`M00-Shift-A`)

---

### FR-6: Error Handling & Validation
**Priority**: P1 (High)
**Description**: JSON config errors must be clearly reported to users.

**Acceptance Criteria**:
- [ ] Invalid JSON syntax reports line numbers
- [ ] Missing required fields report field names
- [ ] Invalid scan codes report the problematic key
- [ ] Unknown key names suggest corrections
- [ ] Schema validation happens on load
- [ ] Errors are logged to user-visible log

---

### NFR-1: Performance
**Priority**: P0 (Critical)
**Description**: The refactoring must improve performance, not degrade it.

**Acceptance Criteria**:
- [ ] Config load time <10ms (vs ~100ms for .mayu)
- [ ] Event processing latency reduced by ~50% (no regex matching)
- [ ] Memory usage <10MB resident set
- [ ] Binary size reduced by ~30% (~5,000 LOC removed)

---

### NFR-2: No Backward Compatibility
**Priority**: P0 (Critical)
**Description**: Clean break from .mayu format - no backward compatibility required.

**Acceptance Criteria**:
- [ ] .mayu parser deleted (Phase 4)
- [ ] No dual loader (JSON-only after Phase 4)
- [ ] Migration guide provided for .mayu → JSON conversion
- [ ] Example JSON configs provided (basic, vim-mode, emacs-mode)

---

### NFR-3: Maintainability
**Priority**: P1 (High)
**Description**: The refactored code must be simpler and easier to maintain.

**Acceptance Criteria**:
- [ ] ~5,000 LOC removed (parser, window/focus system)
- [ ] EventProcessor, ModifierState, ModifierKeyHandler unchanged (verified by tests)
- [ ] All existing tests pass after each phase
- [ ] Code metrics maintained (max 500 lines/file, 50 lines/function)

---

## Out of Scope (Phase 1)

### Per-Window Keymaps
- Window class/title regex matching
- Thread-specific keymaps
- FocusOfThread class
- `engine_focus.cpp` logic

**Rationale**: Adds complexity. Can be added in Phase 2+ if users request it.

### Advanced Features
- Lock modifiers (L00-LFF)
- Window manipulation commands
- Clipboard commands
- Emacs-specific functions
- Mouse control

**Rationale**: Rarely used by most users. Can be added later if there's demand.

### .mayu Format Support
- Text-based configuration
- Preprocessor (include, ifdef, define)
- Backward compatibility

**Rationale**: Clean break for better maintainability.

---

## Success Criteria

### Definition of Done (All Phases)
1. ✅ All unit tests pass
2. ✅ Integration tests pass
3. ✅ E2E tests pass with JSON config
4. ✅ M00-MFF functionality identical to .mayu version
5. ✅ Build size reduced ~30%
6. ✅ Event processing latency reduced ~50%
7. ✅ Config load time <10ms
8. ✅ Zero memory leaks (valgrind clean)
9. ✅ JSON schema documented
10. ✅ Example configs provided

### Phase-Specific Criteria

**Phase 1 (JSON Loader)**:
- [ ] JSON loader implemented and tested
- [ ] All existing .mayu tests still pass
- [ ] JSON and .mayu loaders coexist

**Phase 2 (Engine Simplification)**:
- [ ] FocusOfThread removed from engine.h
- [ ] engine_focus.cpp deleted
- [ ] EventProcessor tests still pass

**Phase 3 (Keymap Simplification)**:
- [ ] Single global keymap only
- [ ] Window regex removed
- [ ] Key mappings work correctly

**Phase 4 (Delete Parser)**:
- [ ] parser.cpp and setting_loader.cpp deleted
- [ ] Clean build with no parser references
- [ ] All tests pass with JSON configs only

**Phase 5 (Documentation)**:
- [ ] JSON schema documented
- [ ] Migration guide written
- [ ] Example configs created (basic, vim, emacs)
- [ ] Performance benchmarks completed

---

## Risks & Mitigations

### Risk 1: Breaking EventProcessor Integration
**Impact**: High
**Probability**: Low
**Mitigation**:
- Don't modify EventProcessor, ModifierState, or ModifierKeyHandler
- Run `test_event_processor_ut`, `test_engine_integration` after every change
- Verify M00-MFF functionality independently

### Risk 2: Loss of Per-Window Functionality
**Impact**: Medium
**Probability**: Medium
**Mitigation**:
- Clearly communicate in docs that Phase 1 is global keymap only
- Plan to add per-window support in Phase 2+ if users request it
- Provide migration examples showing workarounds

### Risk 3: User Migration Friction
**Impact**: Medium
**Probability**: High
**Mitigation**:
- Provide comprehensive migration guide
- Create example JSON configs
- Consider writing .mayu → JSON converter script
- Support both formats during transition (Phase 1-3)

---

## User Stories

### Story 1: Simple Key Remapping
**As a** user
**I want to** remap CapsLock to Escape
**So that** I can use Escape more comfortably in vim

**Acceptance**:
```json
{
  "mappings": [
    {
      "from": "CapsLock",
      "to": "Escape"
    }
  ]
}
```

### Story 2: Modal Editing (Vim-style)
**As a** vim user
**I want to** use CapsLock as a layer key for arrow keys
**So that** I can navigate without leaving home row

**Acceptance**:
```json
{
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tap": "Escape"
    }
  },
  "mappings": [
    {"from": "M00-H", "to": "Left"},
    {"from": "M00-J", "to": "Down"},
    {"from": "M00-K", "to": "Up"},
    {"from": "M00-L", "to": "Right"}
  ]
}
```

### Story 3: Emacs-style Meta Key
**As an** emacs user
**I want to** use Semicolon as a meta key
**So that** I can use emacs bindings system-wide

**Acceptance**:
```json
{
  "virtualModifiers": {
    "M01": {
      "trigger": "Semicolon",
      "tap": "Semicolon"
    }
  },
  "mappings": [
    {"from": "M01-W", "to": ["Ctrl-C"]},
    {"from": "M01-Y", "to": ["Ctrl-V"]}
  ]
}
```

---

**Document Version**: 1.0
**Created**: 2025-12-17
**Status**: Draft
