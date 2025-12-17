# JSON Refactoring Tasks

## Phase 1: Add JSON Loader

- [ ] 1.1. Add nlohmann/json dependency to build system
  - File: CMakeLists.txt
  - Add FetchContent declaration for nlohmann/json 3.11.3
  - Configure header-only library integration
  - Purpose: Enable JSON parsing capability
  - _Leverage: existing CMake configuration, FetchContent patterns_
  - _Requirements: FR-1_
  - _Prompt: Role: Build Engineer with expertise in CMake and dependency management | Task: Add nlohmann/json 3.11.3 to the project using CMake FetchContent, following requirement FR-1 for JSON configuration support | Restrictions: Must use FetchContent (not Conan for now), header-only library, do not modify existing dependencies | Success: Build succeeds with nlohmann/json available, can include <nlohmann/json.hpp> in code_

- [ ] 1.2. Create JsonConfigLoader class interface
  - File: src/core/settings/json_config_loader.h
  - Define class with load() method and private parsing methods
  - Add error logging support with std::ostream
  - Purpose: Establish JSON configuration loading interface
  - _Leverage: src/core/settings/setting.h, existing parser patterns_
  - _Requirements: FR-1, FR-6_
  - _Prompt: Role: C++ Developer specializing in configuration systems and API design | Task: Create JsonConfigLoader class interface following requirements FR-1 and FR-6, with proper error handling and logging support | Restrictions: Must use modern C++20, follow existing naming conventions (m_ prefix for members), use gsl::span where appropriate | Success: Header compiles cleanly, class is instantiable, interface is clear and well-documented with Doxygen comments_

- [ ] 1.3. Implement JsonConfigLoader class skeleton
  - File: src/core/settings/json_config_loader.cpp
  - Implement constructor and stub methods for all parsing functions
  - Add basic error logging helpers
  - Purpose: Create compilable implementation structure
  - _Leverage: src/core/settings/json_config_loader.h_
  - _Requirements: FR-1, FR-6_
  - _Prompt: Role: C++ Developer with expertise in implementation patterns | Task: Create JsonConfigLoader implementation skeleton with constructor and stub methods following requirements FR-1 and FR-6 | Restrictions: Use Expects/Ensures from Microsoft GSL for contracts, follow RAII patterns, do not use exceptions in stubs yet | Success: Implementation compiles, all methods have stubs, can instantiate JsonConfigLoader object_

- [x] 1.4. Implement keyboard key parsing (parseKeyboard)
  - File: src/core/settings/json_config_loader.cpp (continue from 1.3)
  - Implement parseKeyboard() to parse "keyboard.keys" section
  - Parse scan codes from hex strings (e.g., "0x1e" → 0x1e)
  - Create Key objects and populate Keyboard::m_table
  - Build m_keyLookup map for name resolution
  - Purpose: Load key definitions from JSON config
  - _Leverage: src/core/input/keyboard.h, src/core/settings/setting.h_
  - _Requirements: FR-1_
  - _Prompt: Role: C++ Developer with expertise in JSON parsing and data structures | Task: Implement parseKeyboard() method to parse key definitions from JSON following requirement FR-1, creating Key objects and populating Keyboard | Restrictions: Must validate scan code format (hex strings), handle missing "keys" section gracefully, use nlohmann/json API correctly | Success: Valid JSON key definitions load correctly, invalid scan codes are rejected with clear error messages, Key objects created and added to Keyboard_

- [x] 1.5. Implement resolveKeyName helper method
  - File: src/core/settings/json_config_loader.cpp (continue from 1.4)
  - Implement resolveKeyName() to lookup keys by name
  - Return nullptr for unknown keys with helpful error message
  - Purpose: Enable key name resolution for mappings
  - _Leverage: m_keyLookup map from parseKeyboard_
  - _Requirements: FR-1, FR-6_
  - _Prompt: Role: C++ Developer with expertise in lookup algorithms and error handling | Task: Implement resolveKeyName() helper to lookup keys by name following requirements FR-1 and FR-6 | Restrictions: Must check m_keyLookup first, return nullptr for unknown keys, log helpful error with suggestions | Success: Known keys resolve correctly, unknown keys return nullptr with error logged, performance is O(1) hash lookup_

- [x] 1.6. Implement virtual modifier parsing (parseVirtualModifiers)
  - File: src/core/settings/json_config_loader.cpp (continue from 1.5)
  - Implement parseVirtualModifiers() to parse "virtualModifiers" section
  - Extract M00-MFF numbers from modifier names
  - Register trigger keys in Setting::m_virtualModTriggers
  - Register tap actions in Setting::m_modTapActions
  - Purpose: Configure M00-MFF virtual modifiers with tap actions
  - _Leverage: src/core/settings/setting.h, resolveKeyName()_
  - _Requirements: FR-2_
  - _Prompt: Role: C++ Developer with expertise in modifier key systems and state management | Task: Implement parseVirtualModifiers() to register M00-MFF modifiers following requirement FR-2, supporting trigger keys and tap actions | Restrictions: Validate modifier name format (M00-MFF), ensure trigger keys exist, tap action is optional, extract hex number correctly | Success: M00-MFF modifiers register correctly, trigger keys linked to modifier numbers, tap actions stored, invalid modifier names rejected_

- [x] 1.7. Implement parseModifiedKey helper method
  - File: src/core/settings/json_config_loader.cpp (continue from 1.6)
  - Implement parseModifiedKey() to parse modifier strings
  - Parse standard modifiers (Shift, Ctrl, Alt, Win)
  - Parse virtual modifiers (M00-MFF)
  - Extract key name and resolve to Key*
  - Build ModifiedKey object with all modifiers
  - Purpose: Parse complex modifier expressions like "Shift-M00-A"
  - _Leverage: src/core/input/keyboard.h (ModifiedKey, Modifier), resolveKeyName()_
  - _Requirements: FR-2, FR-5_
  - _Prompt: Role: C++ Developer with expertise in string parsing and modifier systems | Task: Implement parseModifiedKey() to parse modifier expressions following requirements FR-2 and FR-5, supporting standard and virtual modifiers | Restrictions: Split by '-' delimiter, validate each component, set modifier bits correctly, handle virtual mod bitset properly | Success: Simple keys parse (A), standard modifiers parse (Shift-A), virtual modifiers parse (M00-A), combinations work (Shift-M00-A)_

- [x] 1.8. Implement mapping parsing (parseMappings)
  - File: src/core/settings/json_config_loader.cpp (continue from 1.7)
  - Implement parseMappings() to parse "mappings" array
  - Parse "from" field using parseModifiedKey()
  - Parse "to" field (single key or array for sequences)
  - Create KeySeq with ActionKey objects
  - Add mappings to global Keymap
  - Purpose: Create key remapping rules from JSON config
  - _Leverage: src/core/input/keymap.h, parseModifiedKey(), resolveKeyName()_
  - _Requirements: FR-1, FR-4_
  - _Prompt: Role: C++ Developer with expertise in action systems and rule compilation | Task: Implement parseMappings() to create key remapping rules following requirements FR-1 and FR-4, supporting single keys and sequences | Restrictions: Must get or create global Keymap, validate "from" and "to" fields, handle array "to" for sequences, call Keymap::define() correctly | Success: Simple mappings work (A→Tab), modifier mappings work (Shift-A→Tab), sequences work (M00-B→[Esc,B]), all mappings added to global keymap_

- [x] 1.9. Implement main load() orchestration method
  - File: src/core/settings/json_config_loader.cpp (continue from 1.8)
  - Implement load() to orchestrate parsing workflow
  - Read JSON file into string
  - Parse with nlohmann::json with error handling
  - Validate schema (version field required, must be "2.0")
  - Call parseKeyboard(), parseVirtualModifiers(), parseMappings() in order
  - Purpose: Complete JSON configuration loading pipeline
  - _Leverage: all parseXXX() methods, nlohmann/json API_
  - _Requirements: FR-1, FR-6_
  - _Prompt: Role: C++ Developer with expertise in orchestration and error handling | Task: Implement main load() method to orchestrate JSON loading following requirements FR-1 and FR-6, with comprehensive error handling | Restrictions: Use try-catch for nlohmann::json::parse_error, validate version field first, call parsers in correct order, return false on any error | Success: Valid JSON loads successfully, invalid JSON syntax caught with line numbers, missing version rejected, all sections parse correctly_

- [x] 1.10. Create unit tests for JsonConfigLoader
  - File: tests/test_json_loader.cpp
  - Write test cases for all parsing methods
  - Test valid JSON configurations
  - Test error handling (syntax errors, missing fields, unknown keys)
  - Test M00-MFF modifier parsing
  - Test key sequence parsing
  - Purpose: Ensure JsonConfigLoader reliability and correctness
  - _Leverage: tests/googletest framework, src/core/settings/json_config_loader.h_
  - _Requirements: FR-1, FR-2, FR-4, FR-6_
  - _Prompt: Role: QA Engineer with expertise in C++ unit testing and Google Test framework | Task: Create comprehensive unit tests for JsonConfigLoader covering all requirements, using Google Test framework | Restrictions: Must test both success and failure paths, use test fixtures for JSON files, mock Setting object where appropriate, achieve >80% code coverage | Success: All test cases pass, error paths tested, M00-MFF parsing tested, key sequences tested, code coverage >80%_

- [x] 1.11. Create test JSON configuration files
  - Files: tests/configs/valid.json, tests/configs/vim-mode.json, tests/configs/invalid_*.json
  - Create valid.json with basic key definitions and mappings
  - Create vim-mode.json with M00 CapsLock vim bindings
  - Create error test configs (invalid syntax, missing fields, unknown keys)
  - Purpose: Provide test data for JsonConfigLoader unit tests
  - _Leverage: JSON schema from design.md_
  - _Requirements: FR-1, FR-2_
  - _Prompt: Role: QA Engineer with expertise in test data creation and JSON formats | Task: Create comprehensive test JSON configuration files covering valid and invalid scenarios following requirements FR-1 and FR-2 | Restrictions: Follow JSON schema exactly, include comments where JSON allows, cover edge cases, ensure files are syntactically correct (except error cases) | Success: valid.json loads successfully, vim-mode.json demonstrates M00 usage, error configs trigger correct error messages_

- [x] 1.12. Create example JSON configuration files
  - Files: keymaps/config.json, keymaps/vim-mode.json
  - Create config.json with well-commented basic examples
  - Create vim-mode.json with comprehensive vim-style bindings
  - Purpose: Provide user-facing example configurations
  - _Leverage: JSON schema from design.md, user stories from requirements.md_
  - _Requirements: FR-1, FR-2, FR-4_
  - _Prompt: Role: Technical Writer with expertise in configuration examples and user documentation | Task: Create user-friendly example JSON configs following requirements FR-1, FR-2, FR-4 with clear comments and explanations | Restrictions: Must be valid JSON, include inline comments via description fields if needed, cover common use cases, keep examples simple and clear | Success: Examples are clear and well-documented, vim-mode.json demonstrates M00 modal editing, users can easily adapt examples_

- [x] 1.13. Verify existing .mayu tests still pass
  - Verify all existing unit tests still pass with .mayu configs
  - Ensure no regressions in EventProcessor tests
  - Confirm M00-MFF functionality unchanged
  - Purpose: Ensure backward compatibility during Phase 1
  - _Leverage: existing test suite (test_event_processor_ut, test_engine_integration)_
  - _Requirements: NFR-3_
  - _Prompt: Role: QA Engineer with expertise in regression testing and test automation | Task: Verify all existing tests still pass with .mayu configurations following requirement NFR-3, ensuring no regressions | Restrictions: Do not modify existing tests, run full test suite, check EventProcessor tests specifically, verify M00-MFF system unchanged | Success: All existing .mayu tests pass, EventProcessor tests pass unchanged, M00-MFF functionality identical, no regressions detected_

## Phase 2: Simplify Engine

- [x] 2.1. Remove FocusOfThread class from engine.h
  - File: src/core/engine/engine.h
  - Delete FocusOfThread class definition (lines 67-81)
  - Delete related members (m_focusOfThreads, m_currentFocusOfThread, m_globalFocus, m_hwndFocus, m_attachedThreadIds, m_detachedThreadIds)
  - Add m_globalKeymap member
  - Remove method declarations (checkFocusWindow, setFocus, threadAttachNotify, threadDetachNotify)
  - Purpose: Simplify Engine to use single global keymap
  - _Leverage: existing Engine class structure_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Refactoring Specialist with expertise in class simplification | Task: Remove FocusOfThread and related members from Engine class following requirement FR-3 for global keymap only | Restrictions: Do not modify EventProcessor integration, keep core input/output members, remove only focus/window related code | Success: FocusOfThread class deleted, all focus members removed, m_globalKeymap added, method declarations removed, compiles without errors_

- [x] 2.2. Simplify Engine::setSetting() method
  - File: src/core/engine/engine.cpp
  - Replace complex focus/keymap setup with simple global keymap assignment
  - Get global keymap from Setting::m_keymaps
  - Log keymap loading with Quill logger
  - Purpose: Simplify setting loading for single keymap model
  - _Leverage: src/core/input/keymap.h (Keymaps::getGlobalKeymap)_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Developer with expertise in refactoring and simplification | Task: Simplify Engine::setSetting() to use single global keymap following requirement FR-3 | Restrictions: Remove all focus/window logic, call Keymaps::getGlobalKeymap(), update EventProcessor if needed, use Quill logging | Success: Method simplified from ~100 lines to ~10 lines, m_globalKeymap set correctly, EventProcessor updated, no focus logic_

- [x] 2.3. Remove focus detection methods from engine.cpp
  - File: src/core/engine/engine.cpp
  - Delete checkFocusWindow() implementation
  - Delete setFocus() implementation
  - Delete threadAttachNotify() implementation
  - Delete threadDetachNotify() implementation
  - Remove all calls to these methods
  - Purpose: Remove unused focus tracking code
  - _Leverage: existing engine.cpp_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Refactoring Specialist with expertise in dead code elimination | Task: Delete focus detection method implementations from engine.cpp following requirement FR-3 | Restrictions: Find and remove all calls to deleted methods, ensure no dangling references, maintain other Engine functionality | Success: All focus methods deleted, no calls to deleted methods remain, engine.cpp compiles, functionality preserved_

- [x] 2.4. Simplify keyboardHandler to use global keymap
  - File: src/core/engine/engine_keyboard_handler.cpp
  - Remove focus change detection logic
  - Remove window class/title queries
  - Use m_globalKeymap directly instead of m_currentFocusOfThread->m_keymaps
  - Purpose: Simplify event processing to use single keymap
  - _Leverage: m_globalKeymap member, EventProcessor_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Developer with expertise in event processing and refactoring | Task: Simplify keyboardHandler() to use m_globalKeymap directly following requirement FR-3 | Restrictions: Do not modify EventProcessor integration, remove window focus checks only, maintain event processing flow, use m_globalKeymap consistently | Success: keyboardHandler() simplified by ~50 lines, uses m_globalKeymap directly, no focus logic, event processing works correctly_

- [x] 2.5. Delete engine_focus.cpp file
  - File: src/core/engine/engine_focus.cpp (delete)
  - Delete entire file (~800 LOC)
  - Update CMakeLists.txt to remove from build
  - Purpose: Remove unused focus tracking implementation
  - _Leverage: CMakeLists.txt_
  - _Requirements: FR-3_
  - _Prompt: Role: Build Engineer with expertise in CMake and code cleanup | Task: Delete engine_focus.cpp and remove from CMakeLists.txt following requirement FR-3 | Restrictions: Only delete engine_focus.cpp, update CMakeLists.txt correctly, ensure build succeeds after deletion | Success: File deleted, CMakeLists.txt updated, build succeeds without engine_focus.cpp, ~800 LOC removed_

- [x] 2.6. Verify EventProcessor tests still pass
  - Run test_event_processor_ut
  - Run test_engine_integration
  - Verify M00-MFF system unchanged
  - Purpose: Ensure Engine simplification doesn't break core functionality
  - _Leverage: existing test suite_
  - _Requirements: NFR-3_
  - _Prompt: Role: QA Engineer with expertise in regression testing and core system validation | Task: Verify EventProcessor tests pass after Engine simplification following requirement NFR-3 | Restrictions: Do not modify EventProcessor code, run full test suite, verify M00-MFF functionality, check for regressions | Success: test_event_processor_ut passes, test_engine_integration passes, M00-MFF works identically, no regressions in core event processing_

## Phase 3: Simplify Keymap

- [x] 3.1. Remove window matching from Keymap class
  - File: src/core/input/keymap.h
  - Delete Type enum (Type_keymap, Type_windowAnd, Type_windowOr)
  - Delete m_windowClass member (std::regex)
  - Delete m_windowTitle member (std::regex)
  - Update constructor to not take window regex parameters
  - Purpose: Simplify Keymap for global-only usage
  - _Leverage: existing Keymap class structure_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Refactoring Specialist with expertise in class simplification and API design | Task: Remove window matching members from Keymap class following requirement FR-3 | Restrictions: Do not modify KeyAssignments or lookup logic, keep parent keymap support for future, remove only window-related members | Success: Type enum deleted, regex members removed, constructor simplified, class compiles, API cleaner_

- [x] 3.2. Delete doesSameWindow method from keymap.cpp
  - File: src/core/input/keymap.cpp
  - Delete doesSameWindow() implementation
  - Remove any calls to this method
  - Purpose: Remove unused window matching logic
  - _Leverage: existing keymap.cpp_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Developer with expertise in refactoring and dead code elimination | Task: Delete doesSameWindow() method from keymap.cpp following requirement FR-3 | Restrictions: Find all calls and remove them, ensure no dangling references, maintain other Keymap functionality | Success: doesSameWindow() deleted, no calls remain, keymap.cpp compiles, functionality preserved_

- [x] 3.3. Simplify Keymaps::getGlobalKeymap method
  - File: src/core/input/keymap.cpp
  - Simplify getGlobalKeymap() to return keymap named "Global"
  - Remove window matching logic from search
  - Purpose: Direct lookup for single global keymap
  - _Leverage: existing Keymaps class, m_keymaps list_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Developer with expertise in collection operations and simplification | Task: Simplify Keymaps::getGlobalKeymap() to direct lookup following requirement FR-3 | Restrictions: Search by name "Global" only, no window regex, return first match or nullptr, maintain simple iteration | Success: Method simplified to ~5 lines, returns keymap with name "Global", no window logic, works correctly_

- [x] 3.4. Remove searchWindow method from Keymaps class
  - File: src/core/input/keymap.cpp
  - Delete searchWindow() implementation
  - Remove method declaration from keymap.h
  - Remove any calls to this method
  - Purpose: Remove unused window search functionality
  - _Leverage: existing Keymaps class_
  - _Requirements: FR-3_
  - _Prompt: Role: C++ Refactoring Specialist with expertise in API cleanup | Task: Delete searchWindow() method from Keymaps class following requirement FR-3 | Restrictions: Remove from both .h and .cpp files, find and remove all calls, ensure no references remain | Success: searchWindow() deleted from header and implementation, no calls remain, Keymaps class compiles_

- [x] 3.5. Verify key mappings work with simplified Keymap
  - Test simple mappings (A→Tab)
  - Test modifier mappings (Shift-A→Tab)
  - Test M00-MFF mappings (M00-A→Left)
  - Test key sequences (M00-B→[Esc,B])
  - Purpose: Ensure Keymap simplification doesn't break functionality
  - _Leverage: test_json_loader tests, example JSON configs_
  - _Requirements: FR-1, FR-2, FR-4_
  - _Prompt: Role: QA Engineer with expertise in functional testing and validation | Task: Verify all key mapping types work after Keymap simplification following requirements FR-1, FR-2, FR-4 | Restrictions: Test all mapping types, verify M00-MFF modifiers, check key sequences, use JSON configs for testing | Success: Simple mappings work, modifier mappings work, M00-MFF mappings work, sequences work, all tests pass_

## Phase 4: Delete Parser

- [x] 4.1. Delete parser source files
  - Files: src/core/settings/parser.h, src/core/settings/parser.cpp (delete)
  - Delete parser.h header file
  - Delete parser.cpp implementation file
  - Purpose: Remove legacy .mayu parser
  - _Leverage: file system operations_
  - _Requirements: NFR-2_
  - _Prompt: Role: DevOps Engineer with expertise in codebase cleanup | Task: Delete parser.h and parser.cpp files following requirement NFR-2 for clean break from .mayu | Restrictions: Delete only parser files, do not delete JsonConfigLoader, ensure files are actually deleted | Success: parser.h deleted, parser.cpp deleted, files no longer in repository, ~536 LOC removed_

- [x] 4.2. Delete setting_loader source files
  - Files: src/core/settings/setting_loader.h, src/core/settings/setting_loader.cpp (delete)
  - Delete setting_loader.h header file
  - Delete setting_loader.cpp implementation file
  - Purpose: Remove legacy .mayu semantic parser
  - _Leverage: file system operations_
  - _Requirements: NFR-2_
  - _Prompt: Role: DevOps Engineer with expertise in codebase cleanup | Task: Delete setting_loader.h and setting_loader.cpp files following requirement NFR-2 | Restrictions: Delete only setting_loader files, preserve Setting class (setting.h/cpp), ensure complete deletion | Success: setting_loader.h deleted, setting_loader.cpp deleted, ~2,141 LOC removed, Setting class preserved_

- [ ] 4.3. Delete window manipulation command files
  - Files: src/core/commands/cmd_window_*.cpp, src/core/commands/cmd_clipboard_*.cpp, src/core/commands/cmd_emacs_*.cpp (delete)
  - Delete all window command files (~37 files)
  - Delete clipboard command files (~5 files)
  - Delete emacs command files (~3 files)
  - Purpose: Remove unused feature implementations
  - _Leverage: file system operations, shell scripting_
  - _Requirements: NFR-2_
  - _Prompt: Role: DevOps Engineer with expertise in bulk file operations and cleanup | Task: Delete unused command files following requirement NFR-2 for simplified feature set | Restrictions: Delete only cmd_window_*, cmd_clipboard_*, cmd_emacs_* files, preserve core command infrastructure, use careful pattern matching | Success: ~45 command files deleted, ~2,000 LOC removed, core commands preserved_

- [ ] 4.4. Delete window system source files
  - Files: src/core/window/focus.cpp, src/core/window/target.cpp, src/core/window/layoutmanager.cpp (delete)
  - Delete focus.cpp implementation
  - Delete target.cpp implementation
  - Delete layoutmanager.cpp implementation
  - Purpose: Remove window abstraction layer
  - _Leverage: file system operations_
  - _Requirements: NFR-2_
  - _Prompt: Role: DevOps Engineer with expertise in codebase architecture cleanup | Task: Delete window system implementation files following requirement NFR-2 | Restrictions: Delete only window/*.cpp files, preserve platform window interfaces if still needed, ensure complete deletion | Success: 3 window system files deleted, ~500 LOC removed, platform interfaces preserved if needed_

- [ ] 4.5. Update CMakeLists.txt to remove deleted files
  - File: CMakeLists.txt
  - Remove parser.cpp from SOURCES
  - Remove setting_loader.cpp from SOURCES
  - Remove engine_focus.cpp from SOURCES (already done in Phase 2)
  - Remove all cmd_window_*.cpp files from SOURCES
  - Remove all cmd_clipboard_*.cpp files from SOURCES
  - Remove all cmd_emacs_*.cpp files from SOURCES
  - Remove all window/*.cpp files from SOURCES
  - Purpose: Update build configuration for deleted files
  - _Leverage: existing CMakeLists.txt structure_
  - _Requirements: NFR-2_
  - _Prompt: Role: Build Engineer with expertise in CMake and build configuration | Task: Update CMakeLists.txt to remove all deleted files following requirement NFR-2 | Restrictions: Remove only deleted files from SOURCES lists, preserve json_config_loader.cpp, maintain build structure, test build after changes | Success: All deleted files removed from CMakeLists.txt, build configuration updated, no references to deleted files remain_

- [ ] 4.6. Update main.cpp to use JsonConfigLoader only
  - File: src/app/main_linux.cpp (or appropriate entry point)
  - Replace SettingLoader with JsonConfigLoader
  - Update config file path to use .json extension
  - Remove .mayu loading code paths
  - Purpose: Switch to JSON-only configuration loading
  - _Leverage: JsonConfigLoader, existing main.cpp structure_
  - _Requirements: NFR-2_
  - _Prompt: Role: Application Developer with expertise in initialization and configuration | Task: Update main entry point to use JsonConfigLoader only following requirement NFR-2 | Restrictions: Remove all SettingLoader references, use .json config path, maintain other initialization logic, ensure proper error handling | Success: main.cpp uses JsonConfigLoader only, loads .json configs, no .mayu code paths, application starts correctly_

- [ ] 4.7. Clean build verification
  - Clean build directory completely
  - Run cmake configuration
  - Build project from scratch
  - Verify no compilation errors
  - Check binary size reduction
  - Purpose: Ensure clean build without deleted files
  - _Leverage: CMake build system_
  - _Requirements: NFR-1, NFR-2_
  - _Prompt: Role: Build Engineer with expertise in build verification and testing | Task: Verify clean build succeeds after parser deletion following requirements NFR-1 and NFR-2 | Restrictions: Clean build directory first, run full build, check for errors, measure binary size, ensure no references to deleted code | Success: Clean build succeeds, no compilation errors, no linker errors, binary size reduced by ~30%, build time acceptable_

- [ ] 4.8. Run full test suite with JSON configs
  - Run all unit tests with JSON configurations
  - Run integration tests
  - Run E2E tests if available
  - Verify M00-MFF functionality
  - Purpose: Ensure all functionality works with JSON-only system
  - _Leverage: test suite, example JSON configs_
  - _Requirements: All requirements_
  - _Prompt: Role: QA Engineer with expertise in comprehensive testing and validation | Task: Run full test suite with JSON configs verifying all requirements | Restrictions: Use JSON configs exclusively, test all mapping types, verify M00-MFF system, check for regressions | Success: All unit tests pass, integration tests pass, E2E tests pass, M00-MFF works correctly, no regressions detected_

## Phase 5: Documentation

- [ ] 5.1. Write JSON schema documentation
  - File: docs/json-schema.md
  - Document complete JSON schema structure
  - Explain each section (version, keyboard, virtualModifiers, mappings)
  - Provide examples for each field
  - Document validation rules
  - Purpose: Help users understand JSON configuration format
  - _Leverage: JSON schema from design.md, requirements from requirements.md_
  - _Requirements: FR-1, FR-2, FR-4, FR-5, FR-6_
  - _Prompt: Role: Technical Writer with expertise in API documentation and configuration formats | Task: Write comprehensive JSON schema documentation covering all requirements | Restrictions: Must be accurate and complete, include examples for every section, explain validation rules, keep language clear and accessible | Success: Documentation is complete and accurate, all schema sections explained, examples provided, validation rules documented, users can understand format_

- [ ] 5.2. Create JSON Schema validation file
  - File: schema/config.schema.json
  - Create formal JSON Schema (draft-07) for validation
  - Define all required and optional fields
  - Add pattern validation for scan codes, modifier names
  - Include descriptions for IDE support
  - Purpose: Enable IDE autocomplete and validation for JSON configs
  - _Leverage: JSON Schema specification, design.md schema definition_
  - _Requirements: FR-1, FR-6_
  - _Prompt: Role: JSON Schema Expert with expertise in schema design and validation | Task: Create formal JSON Schema file for config validation following requirements FR-1 and FR-6 | Restrictions: Use JSON Schema draft-07, include all fields from design, add helpful descriptions, enable IDE features | Success: Schema is valid JSON Schema, validates config correctly, enables IDE autocomplete, rejects invalid configs_

- [ ] 5.3. Write migration guide from .mayu to JSON
  - File: docs/migration-guide.md
  - Document conversion process for common .mayu patterns
  - Provide side-by-side examples (.mayu vs JSON)
  - Explain removed features (per-window keymaps, etc.)
  - Suggest workarounds for missing features
  - Purpose: Help users migrate existing .mayu configs to JSON
  - _Leverage: .mayu format knowledge, JSON schema, user stories_
  - _Requirements: NFR-2_
  - _Prompt: Role: Technical Writer with expertise in migration guides and user documentation | Task: Write comprehensive migration guide from .mayu to JSON following requirement NFR-2 | Restrictions: Cover common patterns, provide clear examples, explain removed features honestly, suggest workarounds, maintain helpful tone | Success: Guide is comprehensive and helpful, examples are clear, removed features explained, users can successfully migrate configs_

- [ ] 5.4. Create comprehensive example configurations
  - File: keymaps/emacs-mode.json
  - Create emacs-mode.json with Semicolon as meta key
  - Add common emacs bindings (M01-W→Ctrl-C, M01-Y→Ctrl-V, etc.)
  - Include inline documentation via descriptions
  - Purpose: Provide emacs users with ready-to-use config
  - _Leverage: JSON schema, user stories, emacs keybindings knowledge_
  - _Requirements: FR-1, FR-2, FR-4_
  - _Prompt: Role: Power User and Technical Writer with expertise in Emacs and configuration examples | Task: Create comprehensive emacs-mode.json config following requirements FR-1, FR-2, FR-4 | Restrictions: Must be valid JSON, include common emacs bindings, add helpful comments, keep bindings authentic to emacs | Success: Config is valid and loads correctly, demonstrates M01 meta key, includes essential emacs bindings, well-documented_

- [ ] 5.5. Enhance vim-mode example configuration
  - File: keymaps/vim-mode.json (modify)
  - Add more vim bindings (M00-W→Ctrl-Right for word navigation, etc.)
  - Include inline documentation
  - Demonstrate key sequences for complex vim operations
  - Purpose: Provide vim users with comprehensive config example
  - _Leverage: existing vim-mode.json, JSON schema, vim keybindings knowledge_
  - _Requirements: FR-1, FR-2, FR-4_
  - _Prompt: Role: Vim Power User and Technical Writer with expertise in modal editing | Task: Enhance vim-mode.json with comprehensive vim bindings following requirements FR-1, FR-2, FR-4 | Restrictions: Must be valid JSON, add common vim bindings, demonstrate sequences, maintain vim philosophy, add helpful comments | Success: Config demonstrates comprehensive vim usage, M00 modal editing clear, key sequences shown, well-documented_

- [ ] 5.6. Run performance benchmarks
  - Measure JSON config load time (<10ms target)
  - Measure event processing latency (50% reduction target)
  - Measure binary size reduction (30% target)
  - Document results in docs/performance.md
  - Purpose: Verify performance improvements from refactoring
  - _Leverage: benchmark tools, example configs_
  - _Requirements: NFR-1_
  - _Prompt: Role: Performance Engineer with expertise in benchmarking and profiling | Task: Run comprehensive performance benchmarks following requirement NFR-1 | Restrictions: Measure config load time accurately, benchmark event latency with various key types, measure binary size before/after, document methodology | Success: Config loads in <10ms, event latency reduced ~50%, binary size reduced ~30%, results documented with methodology_

- [ ] 5.7. Update README.md with JSON information
  - File: README.md
  - Update configuration section to describe JSON format
  - Remove or mark .mayu references as deprecated
  - Add link to JSON schema documentation
  - Add link to example configurations
  - Purpose: Update main project documentation for JSON system
  - _Leverage: existing README.md, docs/json-schema.md_
  - _Requirements: NFR-2_
  - _Prompt: Role: Technical Writer with expertise in README documentation and project presentation | Task: Update README.md for JSON configuration system following requirement NFR-2 | Restrictions: Update configuration section, deprecate .mayu references, add JSON documentation links, maintain README structure and tone | Success: README accurately reflects JSON system, .mayu marked deprecated, links to documentation added, clear and helpful_

- [ ] 5.8. Final code review and cleanup
  - Review all modified files for code quality
  - Check code metrics (max 500 lines/file, 50 lines/function)
  - Run valgrind to check for memory leaks
  - Fix any remaining issues
  - Purpose: Ensure code quality and reliability
  - _Leverage: code metrics tools, valgrind, GSL contracts_
  - _Requirements: NFR-3_
  - _Prompt: Role: Senior Developer with expertise in code quality, review, and best practices | Task: Perform final code review and cleanup following requirement NFR-3 | Restrictions: Check all metrics, verify no memory leaks, ensure GSL contracts used, maintain coding standards, fix all issues found | Success: All files meet metrics (500 lines/file, 50 lines/function), no memory leaks, code quality high, GSL contracts used properly_

- [ ] 5.9. Create final implementation summary
  - Document total LOC removed (~5,000)
  - Document performance improvements achieved
  - List all deleted files (parser, focus, commands)
  - List all created files (JsonConfigLoader, examples, docs)
  - Summarize success criteria met
  - Purpose: Provide clear summary of refactoring accomplishments
  - _Leverage: git diff statistics, performance benchmarks, file lists_
  - _Requirements: All requirements_
  - _Prompt: Role: Technical Writer and Project Manager with expertise in project summaries | Task: Create comprehensive implementation summary documenting all refactoring accomplishments | Restrictions: Use actual statistics (git diff), include all files changed, document performance gains accurately, list success criteria met | Success: Summary is accurate and complete, statistics are real, accomplishments clearly stated, success criteria verification included_
