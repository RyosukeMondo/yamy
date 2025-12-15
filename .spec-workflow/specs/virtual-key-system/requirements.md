# Requirements: Virtual Key System

**Spec Name**: virtual-key-system
**Created**: 2025-12-15
**Status**: Draft

## Overview

Replace the current broken modal modifier system with a clean virtual key architecture that provides:
- Virtual regular keys (V_ prefix) to distinguish physical from virtual keys
- 256 modal modifiers (M00-MFF) with tap/hold behavior
- 256 lock keys (L00-LFF) with persistent state and GUI indicators

This solves critical issues:
- Current modal modifier system doesn't work (mod0 activation fails)
- Ambiguity between physical and virtual keys in substitutions
- Need for visual feedback on lock state
- Artificial limits on number of modifiers/locks

## User Stories

### US1: Virtual Keys Eliminate Ambiguity

**As a** power user
**I want** to distinguish between physical and virtual keys in my config
**So that** I can have different behaviors for physical B vs substituted B

**Acceptance Criteria**:
- WHEN I define `def subst *A = *V_B`
- AND I define `key M00-B = *C` and `key M00-V_B = *D`
- THEN pressing M00+physical_B outputs C
- AND pressing M00+physical_A (which becomes V_B) outputs D

**EARS Format**:
- WHILE in any keymap
- WHEN physical key is substituted to virtual key
- THE system SHALL distinguish between physical and virtual key in modifier combinations
- WHERE virtual keys use V_ prefix

### US2: Short Modifier Notation

**As a** config author
**I want** concise modifier notation
**So that** complex key combinations are readable

**Acceptance Criteria**:
- WHEN I write `key M00-M01-L00-A = *B`
- THEN it's clear this requires M00, M01, and L00 active with key A
- AND it's more readable than `MOD_00-MOD_01-LOCK_00-A`

**EARS Format**:
- WHILE writing .mayu configuration
- WHEN defining modifier/lock combinations
- THE system SHALL accept short notation M00-MFF and L00-LFF
- WHERE M=modifier, L=lock, followed by 2-digit hex

### US3: Implicit Modifier Registration

**As a** config author
**I want** modifiers to work without explicit registration
**So that** I write less boilerplate

**Acceptance Criteria**:
- WHEN I define `def subst *Space = *M00`
- THEN M00 automatically works as a modal modifier
- AND I don't need to write `mod mod0 = !!M00`

**EARS Format**:
- WHILE parsing .mayu configuration
- WHEN M00-MFF key is referenced
- THE system SHALL automatically register it as a modal modifier
- WHERE default behavior is toggle/one-shot

### US4: Modifier Tap Assignment

**As a** user
**I want** modifiers to output keys on tap
**So that** one key can serve dual purposes (tap vs hold)

**Acceptance Criteria**:
- WHEN I define `mod assign M00 = *Enter`
- AND I quickly tap the M00 key (<200ms)
- THEN it outputs Enter
- WHEN I hold the M00 key (≥200ms)
- THEN it activates M00 modifier state without outputting

**EARS Format**:
- WHILE M00-MFF modifier key is pressed
- WHEN key is released before threshold (default 200ms)
- THE system SHALL output the assigned tap action
- WHERE tap action is defined via `mod assign M00 = *Key`
- IF no tap action is defined
- THE system SHALL suppress output

### US5: Modifier Combinations

**As a** power user
**I want** to combine multiple modifiers
**So that** I can create complex key bindings

**Acceptance Criteria**:
- WHEN I define `key M00-M01-A = *Home`
- AND I hold M00, hold M01, and press A
- THEN it outputs Home
- AND it matches most specific rule (most modifiers)

**EARS Format**:
- WHILE multiple modifiers M00-MFF are active
- WHEN key is pressed
- THE system SHALL match keymap entries requiring all active modifiers
- WHERE most specific match (most modifiers) wins

### US6: 256 Lock Keys with Persistent State

**As a** user
**I want** toggle-able lock keys that stay active
**So that** I can lock into modes without holding keys

**Acceptance Criteria**:
- WHEN I press L00 key
- THEN L00 toggles on (if off) or off (if on)
- AND L00 stays active until toggled again
- WHEN L00 is active
- AND I press a key mapped with L00 prefix
- THEN it outputs the L00-mapped action

**EARS Format**:
- WHILE L00-LFF lock key is pressed
- WHEN key is released
- THE system SHALL toggle lock state (on↔off)
- WHERE state persists until next toggle
- AND lock state affects keymap matching

### US7: Lock Combinations (Treasure Hunting)

**As a** power user
**I want** to define behavior for multiple locks simultaneously active
**So that** I can create combinatorial key bindings

**Acceptance Criteria**:
- WHEN I define `key L00-L01-A = *Secret`
- AND both L00 and L01 are active
- AND I press A
- THEN it outputs Secret

**EARS Format**:
- WHILE multiple locks L00-LFF are active simultaneously
- WHEN key is pressed
- THE system SHALL match keymap entries requiring all active locks
- WHERE most specific match (most locks) wins

### US8: Visual Lock Indicators in GUI

**As a** user
**I want** to see which locks are active
**So that** I know what mode I'm in

**Acceptance Criteria**:
- WHEN L00 is toggled on
- THEN GUI shows L00 indicator as green/active
- WHEN L00 is toggled off
- THEN GUI shows L00 indicator as gray/inactive
- AND updates happen in real-time via IPC

**EARS Format**:
- WHILE GUI is running
- WHEN lock state changes
- THE system SHALL send IPC notification to GUI
- WHERE GUI displays lock status with color indicators
- AND shows lock number (L00-LFF)

### US9: 256 Modifiers and Locks (No Artificial Limits)

**As a** power user
**I want** 256 modifiers (M00-MFF) and 256 locks (L00-LFF)
**So that** I can organize my layers however I want

**Acceptance Criteria**:
- WHEN I define M00, M01, ... MFF
- THEN all work as modal modifiers
- WHEN I define L00, L01, ... LFF
- THEN all work as lock keys
- AND hex notation (00-FF) is used for clarity

**EARS Format**:
- WHILE parsing configuration
- WHEN M00-MFF or L00-LFF is referenced
- THE system SHALL support full range 0x00-0xFF (256 each)
- WHERE hex notation matches programming conventions

### US10: No Backward Compatibility - Remove !! Operator

**As a** developer
**I want** to implement clean design without legacy syntax
**So that** old broken modal modifier code can be removed

**Acceptance Criteria**:
- WHEN implementing virtual key system
- THEN old `mod mod0 = !!B` syntax is NOT supported
- AND `!!` operator is NOT parsed (removed completely)
- AND documentation explains migration to new syntax
- AND existing configs must be updated

**EARS Format**:
- WHILE implementing new system
- WHEN old syntax `mod modN = !!Key` or `!!` operator is encountered
- THE system SHALL show error message
- WHERE users must update configs to new syntax

**Rationale**:
- `!!` operator indicated "one-shot/toggle" behavior
- New M00-MFF modifiers are implicitly registered (no need for `mod mod0 = ...`)
- Simpler syntax: `def subst *B = *M00` replaces `mod mod0 = !!B`
- No ambiguity about modifier behavior (all work the same way)

## Non-Functional Requirements

### NFR1: Performance
- Modifier state check: <10μs
- Lock state check: <10μs
- Virtual key lookup: <5μs
- No performance degradation vs current system

### NFR2: Memory
- Modifier state: 32 bytes (256 bits)
- Lock state: 32 bytes (256 bits)
- Virtual key mapping: O(1) hash lookup
- Total overhead: <1KB

### NFR3: Latency
- Tap detection threshold: 200ms (configurable)
- GUI lock indicator update: <50ms via IPC
- Event processing: <1ms end-to-end

### NFR4: Usability
- Config syntax is clear and concise
- Error messages explain V_ vs physical key issues
- GUI lock indicators are visually obvious
- Documentation includes migration guide

## Out of Scope

- Custom tap threshold per modifier (future enhancement)
- Lock indicators in status bar (only in GUI)
- Modifier/lock naming/labeling (use hex numbers only)
- Tap-hold-tap patterns (future enhancement)
- Backward compatibility with old modal modifier syntax

## Success Metrics

1. All E2E tests pass with new virtual key system
2. M00 modal modifier activates correctly (current bug fixed)
3. V_ virtual keys resolve physical/virtual ambiguity
4. Lock indicators update in GUI within 50ms
5. Config files are more concise (fewer lines than old syntax)
6. Zero performance regression vs baseline

## Dependencies

- IPC system (already implemented)
- GUI framework (Qt, already implemented)
- Event processing pipeline (3-layer architecture exists)
- Parser for .mayu files (needs updates for new syntax)

## Risks

1. **Migration effort**: All existing configs need updating
   - Mitigation: Provide migration guide and examples

2. **Parser complexity**: New syntax for V_, M00, L00
   - Mitigation: Reuse existing parser patterns

3. **Testing coverage**: Need comprehensive E2E tests
   - Mitigation: Use existing test infrastructure from E2E testing framework

## References

- Current E2E testing status: `/home/rmondo/repos/yamy/CURRENT_E2E_STATUS.md`
- Architecture document: `/home/rmondo/repos/yamy/ARCHITECTURE_REFACTOR.md`
- Event processing: `src/core/engine/engine_event_processor.cpp`
- Modal modifiers (broken): `src/core/engine/modifier_key_handler.cpp`
