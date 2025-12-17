# YAMY JSON Refactoring Implementation Plan

## Overview

Transform YAMY from complex .mayu text parser to minimal JSON-based key remapper. Focus on M00-MFF virtual modifiers and basic key remapping, removing ~3,000 LOC of window/focus/thread complexity.

**User Requirements:**
- Enable key remapping including M00-MFF virtual modifiers
- Enable M00-MFF combinations (e.g., M00-A → Left)
- Support key sequences (e.g., M00-B → ['Escape', 'B'])
- Keep standard modifiers (Shift, Ctrl, Alt, Win)
- Drop window matching, thread tracking, complex features
- Use JSON configuration (nlohmann/json library)
- No backward compatibility required

**Strategy:** Incremental 5-phase implementation with passing tests at each step.

---

## JSON Configuration Schema

### Example Configuration

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "B": "0x30",
      "Tab": "0x0f",
      "Enter": "0x1c",
      "Escape": "0x01",
      "CapsLock": "0x3a",
      "Left": "0x69",
      "Right": "0x6a"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tap": "Escape",
      "holdThresholdMs": 200
    },
    "M01": {
      "trigger": "Semicolon",
      "tap": "Semicolon"
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
    },
    {
      "from": "Shift-M01-A",
      "to": "End"
    }
  ]
}
```

### Schema Structure

- **keyboard.keys**: Map key names → scan codes (hex strings)
- **virtualModifiers**: M00-MFF definitions with trigger keys and tap actions
- **mappings**: Array of from→to rules with modifier support

---

## What Gets Kept vs. Removed

### ✅ KEEP (Working, Well-Tested)

**Core Event Processing:**
- `EventProcessor` (engine_event_processor.cpp) - 3-layer architecture verified by tests
- `ModifierState` (modifier_state.h/cpp) - 528-bit state tracking
- `ModifierKeyHandler` (modifier_key_handler.cpp) - hold-vs-tap detection
- `RuleLookupTable` (lookup_table.h) - O(1) rule matching

**Data Structures:**
- `Keyboard` class - key definitions and scan codes
- `Keymap` class - simplified to single global keymap
- `KeySeq`, `Action` classes - for key sequence support
- `Setting` class - simplified container

### ❌ REMOVE (Overly Complex)

**Window/Focus System (~800 LOC):**
- `engine_focus.cpp` - window focus tracking
- `FocusOfThread` class - per-thread keymaps
- `checkFocusWindow()` - regex window matching
- Thread attach/detach notifications

**Parser (~2,700 LOC):**
- `parser.cpp` - tokenizer (536 LOC)
- `setting_loader.cpp` - semantic parser (2,141 LOC)
- All `load_*()` methods

**Window Commands (~40 files):**
- `src/core/commands/cmd_window_*.cpp`
- `src/core/commands/cmd_clipboard_*.cpp`
- `src/core/commands/cmd_emacs_*.cpp`
- `src/core/window/*.cpp`

**Total Removal: ~5,000 LOC**

---

## Implementation Phases

### Phase 1: Add JSON Loader (Additive, No Breaking Changes)

**Goal:** Load JSON configs alongside .mayu without breaking existing tests

**Steps:**

1. **Add nlohmann/json dependency**
   - Update `CMakeLists.txt`:
     ```cmake
     include(FetchContent)
     FetchContent_Declare(json
       URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
     )
     FetchContent_MakeAvailable(json)
     ```

2. **Create JsonConfigLoader**
   - File: `src/core/settings/json_config_loader.h` (~150 lines)
   - File: `src/core/settings/json_config_loader.cpp` (~400 lines)
   - Methods:
     - `bool load(Setting* setting, const std::string& json_path)`
     - `bool parseKeyboard(const json& obj, Setting* setting)`
     - `bool parseVirtualModifiers(const json& obj, Setting* setting)`
     - `bool parseMappings(const json& obj, Setting* setting)`
     - `ModifiedKey parseModifiedKey(const std::string& from_spec)`

3. **Implementation details**
   - Parse keyboard.keys → populate `Keyboard::m_table`
   - Parse virtualModifiers → populate `Setting::m_modTapActions` and `m_virtualModTriggers`
   - Parse mappings → create `CompiledRule` objects for `RuleLookupTable`
   - Modifier parsing: support "Shift-M00-A", "M01-B" format
   - Key sequence parsing: handle both single keys and arrays

4. **Testing**
   - Create `tests/test_json_loader.cpp`
   - Test valid JSON parsing
   - Test invalid JSON error handling
   - Test M00-MFF modifier parsing
   - Verify all existing .mayu tests still pass

**Files Modified:**
- `CMakeLists.txt` (add nlohmann/json, add test target)

**Files Created:**
- `src/core/settings/json_config_loader.h`
- `src/core/settings/json_config_loader.cpp`
- `tests/test_json_loader.cpp`
- `keymaps/config.json` (example config)

**Success Criteria:**
- All existing tests pass
- JSON config loads successfully
- M00-MFF modifiers work identically to .mayu version

---

### Phase 2: Simplify Engine (Remove Window/Focus Logic)

**Goal:** Remove window matching, thread tracking, focus detection

**Steps:**

1. **Simplify Engine class (engine.h)**
   - Remove `FocusOfThread` class (lines 67-81)
   - Remove members:
     ```cpp
     // DELETE these members:
     FocusOfThreads m_focusOfThreads;
     FocusOfThread *m_currentFocusOfThread;
     FocusOfThread m_globalFocus;
     HWND m_hwndFocus;
     ThreadIds m_attachedThreadIds;
     ThreadIds m_detachedThreadIds;
     ```
   - Add single keymap reference:
     ```cpp
     Keymap* m_globalKeymap;  // Single global keymap
     ```

2. **Remove methods (engine.h)**
   - Delete: `checkFocusWindow()`, `setFocus()`
   - Delete: `threadAttachNotify()`, `threadDetachNotify()`
   - Delete: `queryKeymapForWindow()`

3. **Simplify keyboard handler (engine_keyboard_handler.cpp)**
   - Remove focus change detection
   - Remove window class/title queries
   - Directly use `m_globalKeymap` instead of `m_currentFocusOfThread->m_keymaps`

4. **Update setSetting() (engine.cpp)**
   - Remove keymap search logic
   - Set `m_globalKeymap = m_setting->m_keymaps.getGlobalKeymap()`

5. **Delete engine_focus.cpp**
   - Remove entire file (~800 LOC)

**Files Modified:**
- `src/core/engine/engine.h` (~150 lines removed)
- `src/core/engine/engine.cpp` (~100 lines removed)
- `src/core/engine/engine_keyboard_handler.cpp` (~50 lines simplified)

**Files Deleted:**
- `src/core/engine/engine_focus.cpp`

**Success Criteria:**
- `test_event_processor_ut.cpp` still passes
- `test_engine_integration.cpp` still passes
- Build succeeds

---

### Phase 3: Simplify Keymap (Single Global Only)

**Goal:** Single global keymap, remove window regex matching

**Steps:**

1. **Simplify Keymap class (keymap.h)**
   - Remove `Type` enum (windowAnd/windowOr/keymap)
   - Remove members:
     ```cpp
     // DELETE:
     std::regex m_windowClass;
     std::regex m_windowTitle;
     Keymap *m_parentKeymap;  // No inheritance
     ```
   - Keep only:
     ```cpp
     std::string m_name;  // Always "Global"
     KeyAssignments m_assignments;
     ```

2. **Simplify Keymaps class (keymap.cpp)**
   - Remove `searchWindow()` method
   - Remove `doesSameWindow()` method
   - Add `getGlobalKeymap()` method - returns single global keymap
   - Remove keymap parent traversal in `searchAssignment()`

3. **Update JsonConfigLoader**
   - Create single "Global" keymap
   - Add all mappings to this keymap
   - No window/context logic

**Files Modified:**
- `src/core/input/keymap.h` (~80 lines removed)
- `src/core/input/keymap.cpp` (~150 lines removed)
- `src/core/settings/json_config_loader.cpp` (use single global keymap)

**Success Criteria:**
- Key mappings work correctly
- Virtual modifiers integrate properly
- Tests pass

---

### Phase 4: Delete Legacy Parser & Unused Features

**Goal:** Remove .mayu parser and window manipulation features

**Steps:**

1. **Delete parser files**
   - `src/core/settings/parser.h`
   - `src/core/settings/parser.cpp` (536 LOC)
   - `src/core/settings/setting_loader.h`
   - `src/core/settings/setting_loader.cpp` (2,141 LOC)

2. **Delete window manipulation**
   - `src/core/window/focus.cpp`
   - `src/core/window/target.cpp`
   - `src/core/window/layoutmanager.cpp`
   - All `src/core/commands/cmd_window_*.cpp` files
   - All `src/core/commands/cmd_clipboard_*.cpp` files
   - All `src/core/commands/cmd_emacs_*.cpp` files

3. **Update CMakeLists.txt**
   - Remove all deleted files from build
   - Remove parser test targets

4. **Update main.cpp**
   - Remove .mayu loading code
   - Use only `JsonConfigLoader`

5. **Simplify Setting class**
   - Remove unused members:
     ```cpp
     // DELETE if not used by EventProcessor:
     Symbols m_symbols;
     bool m_correctKanaLockHandling;
     bool m_sts4mayu;
     bool m_cts4mayu;
     bool m_mouseEvent;
     int32_t m_dragThreshold;
     unsigned int m_oneShotRepeatableDelay;
     ```
   - Keep only:
     ```cpp
     Keyboard m_keyboard;
     Keymaps m_keymaps;
     KeySeqs m_keySeqs;
     std::unordered_map<uint8_t, uint16_t> m_modTapActions;
     std::unordered_map<uint16_t, uint8_t> m_virtualModTriggers;
     ```

**Files Deleted:**
- `src/core/settings/parser.h`
- `src/core/settings/parser.cpp`
- `src/core/settings/setting_loader.h`
- `src/core/settings/setting_loader.cpp`
- `src/core/engine/engine_focus.cpp`
- `src/core/window/*.cpp` (3 files)
- `src/core/commands/cmd_window_*.cpp` (~37 files)
- `src/core/commands/cmd_clipboard_*.cpp` (~5 files)
- `src/core/commands/cmd_emacs_*.cpp` (~3 files)

**Files Modified:**
- `CMakeLists.txt` (remove ~50 files)
- `src/core/settings/setting.h` (simplify)
- `src/main.cpp` (use JsonConfigLoader only)

**Success Criteria:**
- Clean build with no parser references
- All tests pass with JSON configs
- Binary size reduced significantly

---

### Phase 5: Testing, Documentation, Example Configs

**Goal:** Comprehensive validation and user documentation

**Steps:**

1. **Create JSON example configs**
   - `keymaps/config.json` - basic example
   - `keymaps/vim-mode.json` - vim-style M00 modal editing
   - `keymaps/emacs-mode.json` - emacs-style M01 meta key

2. **Run full test suite**
   - All unit tests pass
   - Integration tests pass
   - E2E tests pass with JSON config

3. **Performance benchmarks**
   - Measure event processing latency (should improve ~50%)
   - Measure config load time (should be <10ms)
   - Compare binary size (should reduce ~30%)

4. **Documentation**
   - Update README with JSON schema
   - Document M00-MFF virtual modifier system
   - Create migration guide (.mayu → JSON)
   - Add JSON schema file for IDE validation

5. **Cleanup**
   - Remove unused test configs (.mayu files)
   - Update .gitignore
   - Final code review

**Files Created:**
- `keymaps/config.json`
- `keymaps/vim-mode.json`
- `keymaps/emacs-mode.json`
- `docs/json-schema.md`
- `docs/migration-guide.md`
- `schema/config.schema.json` (JSON Schema for IDE validation)

**Success Criteria:**
- All tests pass
- Documentation complete
- Example configs work
- Performance improved

---

## Critical Files Reference

### Core Files to Modify

1. **src/core/engine/engine.h** (680 lines)
   - Remove FocusOfThread, window/thread tracking (~150 lines)

2. **src/core/engine/engine.cpp** (8.7K)
   - Simplify keyboard handler, remove focus detection (~100 lines)

3. **src/core/engine/engine_keyboard_handler.cpp**
   - Remove window focus logic (~50 lines)

4. **src/core/input/keymap.h** (417 lines)
   - Remove window regex, inheritance (~80 lines)

5. **src/core/input/keymap.cpp**
   - Remove searchWindow(), parent traversal (~150 lines)

6. **src/core/settings/setting.h** (78 lines)
   - Remove unused members (~20 lines)

7. **CMakeLists.txt**
   - Add nlohmann/json dependency
   - Remove ~50 deleted files
   - Add json_config_loader

### Files to Create

1. **src/core/settings/json_config_loader.h** (~150 lines)
2. **src/core/settings/json_config_loader.cpp** (~400 lines)
3. **tests/test_json_loader.cpp** (~200 lines)
4. **keymaps/config.json** (example)
5. **keymaps/vim-mode.json** (example)

### Files to Delete (~50 files, ~5,000 LOC)

1. **Parser**: parser.h, parser.cpp, setting_loader.h, setting_loader.cpp
2. **Focus**: engine_focus.cpp
3. **Window**: window/focus.cpp, window/target.cpp, window/layoutmanager.cpp
4. **Commands**: cmd_window_*.cpp, cmd_clipboard_*.cpp, cmd_emacs_*.cpp

---

## Risk Mitigation

### High Risk: EventProcessor Integration

**Mitigation:**
- Don't modify EventProcessor, ModifierState, ModifierKeyHandler
- Test after every phase:
  ```bash
  ./build/tests/test_event_processor_ut
  ./build/tests/test_engine_integration
  ```

### Medium Risk: Build System

**Mitigation:**
- Use nlohmann/json (header-only, MIT license)
- Test build after each file removal
- Incremental CMakeLists.txt updates

### Medium Risk: M00-MFF Functionality

**Mitigation:**
- Keep ModifierKeyHandler untouched
- Test hold-vs-tap timing (200ms threshold)
- Verify virtual modifier activation/deactivation

---

## Success Criteria (All Phases)

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

---

## Estimated Timeline

- **Phase 1** (JSON Loader): 2-3 days
- **Phase 2** (Engine Simplification): 1-2 days
- **Phase 3** (Keymap Simplification): 1 day
- **Phase 4** (Delete Parser): 1 day
- **Phase 5** (Testing/Docs): 1-2 days

**Total: 6-9 days**

---

## Notes

- **M00-MFF system is well-designed** - keep it completely intact
- **EventProcessor is verified** - don't touch it
- **Incremental approach** - tests pass at each phase, can stop early if needed
- **No backward compatibility** - clean break from .mayu format
- **Focus on simplicity** - global keymap only, no per-window configurations
