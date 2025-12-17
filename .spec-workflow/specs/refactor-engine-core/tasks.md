# Tasks Document: Core Engine Refactoring

- [x] 1. Refactor `SettingLoader::load_MODIFIER`
  - File: `src/core/settings/setting_loader.cpp`
  - Extract M00-MFF and L00-LFF parsing into helper functions.
  - Replace `goto` logic with structured loops.
  - Use `std::map` for basic modifier lookup.
  - _Status: Completed (See recent git history)_

- [x] 2. Remove Thread-Local Storage Hack
  - File: `src/core/settings/setting_loader.h` / `.cpp`
  - **Implementation Detail:** Define `struct ParserContext` (see `design.md` Tech Specs) inside `SettingLoader`.
  - **Action:** Update `parseMxxModifier`, `load_KEY_SEQUENCE`, and `load_KEY_NAME` to accept `ParserContext& ctx` as an argument.
  - **Action:** Move `s_pendingVirtualMod` and `s_hasVirtualMod` from the anonymous namespace into this `ParserContext` struct instance (passed down from `load_MODIFIER` -> `parseMxxModifier`).
  - _Requirements: 4.1, 4.2_

- [x] 3. Introduce ConfigAST
  - File: `src/core/settings/config_ast.h`
  - **Implementation Detail:** Copy the struct definitions from `design.md` (Technical Specifications section) into this new header file.
  - **Action:** Ensure namespaces match `yamy::ast`.
  - _Requirements: 1.1_

- [x] 4. Update SettingLoader to use AST (Phase A)
  - File: `src/core/settings/setting_loader.h`
  - Add `ConfigAST` member.
  - Update `load_KEY_ASSIGN` etc. to populate AST *in addition to* legacy structures (Validation step).
  - _Requirements: 1.1_

- [x] 5. Implement Compiler Phase
  - File: `src/core/engine/config_compiler.cpp`
  - Create class `ConfigCompiler` that takes `ConfigAST` and produces `Keyboard` and `Keymaps`.
  - _Requirements: 1.3_

- [ ] 6. Refactor EventProcessor Lookup
  - File: `src/core/engine/engine_event_processor.cpp`
  - Implement `LookupTable` structure.
  - Build `LookupTable` from `Keyboard::Substitutes`.
  - Replace linear scan in `layer2_applySubstitution` with map lookup.
  - _Requirements: 3.1, 3.3_

- [ ] 7. Unify Modifier State
  - File: `src/core/input/modifier_state.h`
  - Expand to support 256+ bits.
  - Integrate Hardware, Virtual, and Lock states into one vector.
  - _Requirements: 3.2_
