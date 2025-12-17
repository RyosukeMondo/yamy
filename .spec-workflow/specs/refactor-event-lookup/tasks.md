# Tasks Document: EventProcessor Lookup Refactoring

- [ ] 1. Define CompiledRule and LookupTable
  - File: `src/core/engine/compiled_rule.h`, `src/core/engine/lookup_table.h`
  - **Implementation Detail:** Copy the struct and class definitions from `design.md` exactly.
  - **Action:** Ensure `ModifierState` header is included correctly for `TOTAL_BITS`.
  - _Requirements: 2.1, 2.2_

- [ ] 2. Implement Rule Compilation
  - File: `src/core/engine/engine.cpp` (or `engine_compiler.cpp` if created)
  - **Implementation Detail:** Create a helper method `compileSubstitute(const Substitute& sub)` that returns a `CompiledRule`.
  - **Action:**
    - Iterate legacy `Modifier` object.
    - Map `Modifier::Type` to `ModifierState::StdModifier` bit indices.
    - Set `requiredOn`/`requiredOff` based on `isOn` vs `isDontcare`.
    - Handle Virtual/Lock modifiers (if exposed in legacy Substitute) similarly.
  - **Action:** Update `buildSubstitutionTable` to populate `m_eventProcessor->getLookupTable()`.
  - _Requirements: 3.1_

- [ ] 3. Update EventProcessor to use LookupTable
  - File: `src/core/engine/engine_event_processor.cpp`
  - **Implementation Detail:** Modify `layer2_applySubstitution`.
  - **Action:**
    - Get current state: `const auto& state = io_modState->getFullState();`
    - Call `m_lookupTable.findMatch(yamy_in, state);`
    - If match found: return `match->outputScanCode`.
    - If no match: return `yamy_in` (pass-through).
  - _Requirements: 1.1, 1.2_

- [ ] 4. Cleanup Legacy Code
  - File: `src/core/engine/engine_event_processor.cpp`
  - **Action:** Remove the old linear scan loop over `m_substitutesList`.
  - **Action:** Remove `setSubstitutesList` method if no longer needed.
