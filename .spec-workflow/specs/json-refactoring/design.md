# JSON Refactoring Technical Design

## Architecture Overview

### Current Architecture (Before Refactoring)
```
.mayu file (text)
    ↓
Parser::getLine() → Token[]
    ↓
SettingLoader::load_LINE()
    ├→ load_KEYBOARD_DEFINITION()
    ├→ load_KEYMAP_DEFINITION()
    └→ load_KEY_ASSIGN()
    ↓
Setting {Keyboard, Keymaps, KeySeqs}
    ↓
Engine (with FocusOfThread, window matching)
    ↓
EventProcessor (3-layer architecture)
    ↓
Output
```

**Complexity**:
- parser.cpp: 536 LOC (tokenizer)
- setting_loader.cpp: 2,141 LOC (semantic parser)
- engine_focus.cpp: 800 LOC (window/focus tracking)
- **Total**: ~3,500 LOC just for config + focus

### Target Architecture (After Refactoring)
```
config.json
    ↓
nlohmann/json parser (header-only library)
    ↓
JsonConfigLoader::load()
    ├→ parseKeyboard()
    ├→ parseVirtualModifiers()
    └→ parseMappings()
    ↓
Setting {Keyboard, Keymaps, KeySeqs} (simplified)
    ↓
Engine (single global keymap)
    ↓
EventProcessor (3-layer architecture) [UNCHANGED]
    ↓
Output
```

**Simplification**:
- json_config_loader.cpp: ~400 LOC (uses nlohmann/json)
- Engine: ~150 LOC removed (FocusOfThread, thread tracking)
- **Total**: ~550 LOC for config loading
- **Savings**: ~3,000 LOC removed

---

## Component Design

### 1. JSON Schema

#### Schema Definition
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["version", "keyboard", "mappings"],
  "properties": {
    "version": {
      "type": "string",
      "const": "2.0",
      "description": "Config format version"
    },
    "keyboard": {
      "type": "object",
      "properties": {
        "keys": {
          "type": "object",
          "patternProperties": {
            "^[A-Za-z0-9_]+$": {
              "type": "string",
              "pattern": "^0x[0-9A-Fa-f]{1,4}$"
            }
          }
        }
      }
    },
    "virtualModifiers": {
      "type": "object",
      "patternProperties": {
        "^M[0-9A-Fa-f]{2}$": {
          "type": "object",
          "required": ["trigger"],
          "properties": {
            "trigger": {"type": "string"},
            "tap": {"type": "string"},
            "holdThresholdMs": {"type": "integer", "default": 200}
          }
        }
      }
    },
    "mappings": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["from", "to"],
        "properties": {
          "from": {"type": "string"},
          "to": {
            "oneOf": [
              {"type": "string"},
              {"type": "array", "items": {"type": "string"}}
            ]
          }
        }
      }
    }
  }
}
```

---

### 2. JsonConfigLoader Class

#### Class Interface
```cpp
// json_config_loader.h
#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "setting.h"

namespace yamy::settings {

class JsonConfigLoader {
public:
    /**
     * @brief Construct loader with optional logging stream
     * @param log Output stream for warnings/errors (nullptr = no logging)
     */
    explicit JsonConfigLoader(std::ostream* log = nullptr);

    /**
     * @brief Load JSON configuration file
     * @param setting Output setting object (must not be nullptr)
     * @param json_path Path to JSON config file
     * @return true on success, false on error (see log for details)
     *
     * @pre setting != nullptr
     * @pre json_path is valid file path
     */
    bool load(Setting* setting, const std::string& json_path);

private:
    // Parsing methods
    bool parseKeyboard(const nlohmann::json& obj, Setting* setting);
    bool parseVirtualModifiers(const nlohmann::json& obj, Setting* setting);
    bool parseMappings(const nlohmann::json& obj, Setting* setting);

    // Helper methods
    Key* resolveKeyName(const std::string& name);
    ModifiedKey parseModifiedKey(const std::string& from_spec);
    bool validateSchema(const nlohmann::json& config);

    // Error reporting
    void logError(const std::string& message);
    void logWarning(const std::string& message);

    // State
    std::ostream* m_log;
    std::unordered_map<std::string, Key*> m_keyLookup;
    Keyboard* m_keyboard;  // Set during load()
};

} // namespace yamy::settings
```

#### Implementation Strategy

**parseKeyboard()**:
```cpp
bool JsonConfigLoader::parseKeyboard(const nlohmann::json& obj, Setting* setting) {
    Expects(setting != nullptr);

    if (!obj.contains("keys")) {
        logError("Missing 'keyboard.keys' section");
        return false;
    }

    const auto& keys = obj["keys"];
    for (auto& [name, scanCodeHex] : keys.items()) {
        // Parse scan code (e.g., "0x1e")
        uint16_t scanCode;
        if (!parseScanCode(scanCodeHex.get<std::string>(), &scanCode)) {
            logError("Invalid scan code for key '" + name + "': " + scanCodeHex.get<std::string>());
            return false;
        }

        // Create key object
        Key* key = new Key();
        key->addName(name);
        key->addScanCode(ScanCode(scanCode));

        // Add to keyboard
        setting->m_keyboard.addKey(key);
        m_keyLookup[name] = key;
    }

    return true;
}
```

**parseVirtualModifiers()**:
```cpp
bool JsonConfigLoader::parseVirtualModifiers(const nlohmann::json& obj, Setting* setting) {
    if (!obj.contains("virtualModifiers")) {
        return true;  // Optional section
    }

    const auto& vmods = obj["virtualModifiers"];
    for (auto& [modName, modDef] : vmods.items()) {
        // Extract M00-MFF number
        if (modName.length() != 3 || modName[0] != 'M') {
            logError("Invalid virtual modifier name: " + modName);
            return false;
        }

        uint8_t modNum = std::stoi(modName.substr(1), nullptr, 16);

        // Parse trigger key
        std::string triggerName = modDef["trigger"].get<std::string>();
        Key* triggerKey = resolveKeyName(triggerName);
        if (!triggerKey) {
            logError("Unknown trigger key for " + modName + ": " + triggerName);
            return false;
        }

        // Register virtual modifier trigger
        setting->m_virtualModTriggers[triggerKey->getScanCode()] = modNum;

        // Parse optional tap action
        if (modDef.contains("tap")) {
            std::string tapName = modDef["tap"].get<std::string>();
            Key* tapKey = resolveKeyName(tapName);
            if (!tapKey) {
                logError("Unknown tap key for " + modName + ": " + tapName);
                return false;
            }
            setting->m_modTapActions[modNum] = tapKey->getScanCode();
        }
    }

    return true;
}
```

**parseMappings()**:
```cpp
bool JsonConfigLoader::parseMappings(const nlohmann::json& obj, Setting* setting) {
    Expects(setting != nullptr);

    if (!obj.contains("mappings")) {
        logError("Missing 'mappings' section");
        return false;
    }

    const auto& mappings = obj["mappings"];

    // Get or create global keymap
    Keymap* globalKeymap = setting->m_keymaps.getGlobalKeymap();
    if (!globalKeymap) {
        globalKeymap = new Keymap("Global");
        setting->m_keymaps.add(globalKeymap);
    }

    for (const auto& mapping : mappings) {
        // Parse "from" key
        std::string from_spec = mapping["from"].get<std::string>();
        ModifiedKey fromKey = parseModifiedKey(from_spec);
        if (!fromKey.isValid()) {
            logError("Invalid 'from' key: " + from_spec);
            return false;
        }

        // Parse "to" action (single key or sequence)
        KeySeq* action = nullptr;
        if (mapping["to"].is_string()) {
            // Single key
            std::string to_spec = mapping["to"].get<std::string>();
            Key* toKey = resolveKeyName(to_spec);
            if (!toKey) {
                logError("Unknown 'to' key: " + to_spec);
                return false;
            }
            action = new KeySeq();
            action->addAction(new ActionKey(ModifiedKey(toKey)));
        } else if (mapping["to"].is_array()) {
            // Key sequence
            action = new KeySeq();
            for (const auto& keyName : mapping["to"]) {
                Key* key = resolveKeyName(keyName.get<std::string>());
                if (!key) {
                    logError("Unknown key in sequence: " + keyName.get<std::string>());
                    delete action;
                    return false;
                }
                action->addAction(new ActionKey(ModifiedKey(key)));
            }
        }

        // Add mapping to keymap
        globalKeymap->define(fromKey, action);
    }

    return true;
}
```

**parseModifiedKey()**:
```cpp
ModifiedKey JsonConfigLoader::parseModifiedKey(const std::string& from_spec) {
    // Parse modifier string like "Shift-M00-A" or "A"
    std::vector<std::string> parts = split(from_spec, '-');

    Modifier modifier;
    std::vector<uint8_t> virtualMods;
    std::string keyName;

    for (const auto& part : parts) {
        if (part == "Shift") {
            modifier.add(Modifier::SHIFT);
        } else if (part == "Ctrl") {
            modifier.add(Modifier::CONTROL);
        } else if (part == "Alt") {
            modifier.add(Modifier::ALT);
        } else if (part == "Win") {
            modifier.add(Modifier::WIN);
        } else if (part.length() == 3 && part[0] == 'M') {
            // Virtual modifier M00-MFF
            uint8_t modNum = std::stoi(part.substr(1), nullptr, 16);
            virtualMods.push_back(modNum);
        } else {
            // Assume it's the key name
            keyName = part;
        }
    }

    // Resolve key
    Key* key = resolveKeyName(keyName);
    if (!key) {
        return ModifiedKey();  // Invalid
    }

    // Create ModifiedKey
    ModifiedKey mk(key, modifier);
    for (uint8_t vmod : virtualMods) {
        mk.setVirtualMod(vmod, true);
    }

    return mk;
}
```

---

### 3. Engine Simplification

#### Current Engine Structure (engine.h)
```cpp
class Engine {
private:
    // REMOVE: FocusOfThread tracking
    class FocusOfThread {
        uint32_t m_threadId;
        WindowHandle m_hwndFocus;
        std::string m_className;
        std::string m_titleName;
        KeymapPtrList m_keymaps;
    };

    FocusOfThreads m_focusOfThreads;        // DELETE
    FocusOfThread* m_currentFocusOfThread;  // DELETE
    FocusOfThread m_globalFocus;            // DELETE
    WindowHandle m_hwndFocus;               // DELETE
    ThreadIds m_attachedThreadIds;          // DELETE
    ThreadIds m_detachedThreadIds;          // DELETE

    // KEEP: Core state
    Setting* m_setting;
    EventProcessor* m_eventProcessor;
    ModifierState m_modifierState;
    // ...
};
```

#### Simplified Engine Structure
```cpp
class Engine {
private:
    // Core state (unchanged)
    Setting* m_setting;
    EventProcessor* m_eventProcessor;
    ModifierState m_modifierState;

    // NEW: Single global keymap
    Keymap* m_globalKeymap;

    // Input/output (unchanged)
    IInputInjector* m_inputInjector;
    IInputHook* m_inputHook;

    // Thread management (unchanged)
    std::thread m_keyboardThread;
    std::queue<InputEvent> m_inputQueue;

    // Remove methods:
    // - checkFocusWindow()
    // - setFocus()
    // - threadAttachNotify()
    // - threadDetachNotify()
};
```

#### Engine::setSetting() Simplification
```cpp
// BEFORE
void Engine::setSetting(Setting* setting) {
    m_setting = setting;

    // Complex focus/keymap setup
    m_globalFocus.m_keymaps.clear();
    for (auto* keymap : m_setting->m_keymaps) {
        if (keymap->matchesWindow("", "")) {
            m_globalFocus.m_keymaps.push_back(keymap);
        }
    }

    // Check current focus window
    checkFocusWindow();
    // ...
}

// AFTER
void Engine::setSetting(Setting* setting) {
    m_setting = setting;

    // Simple: get global keymap
    m_globalKeymap = m_setting->m_keymaps.getGlobalKeymap();

    if (!m_globalKeymap) {
        LOG_ERROR(logger, "No global keymap defined");
        return;
    }

    LOG_INFO(logger, "Loaded global keymap with {} mappings",
             m_globalKeymap->size());
}
```

---

### 4. Keymap Simplification

#### Current Keymap Class
```cpp
class Keymap {
public:
    enum Type {
        Type_keymap,      // DELETE
        Type_windowAnd,   // DELETE
        Type_windowOr     // DELETE
    };

private:
    Type m_type;                      // DELETE
    std::regex m_windowClass;         // DELETE
    std::regex m_windowTitle;         // DELETE
    Keymap* m_parentKeymap;           // KEEP (simplified)
    KeyAssignments m_assignments;     // KEEP
};
```

#### Simplified Keymap Class
```cpp
class Keymap {
private:
    std::string m_name;  // Always "Global" for Phase 1
    KeyAssignments m_assignments;
    Keymap* m_parentKeymap;  // Optional, for future extension

public:
    Keymap(const std::string& name) : m_name(name), m_parentKeymap(nullptr) {}

    void define(const ModifiedKey& key, KeySeq* action);
    KeySeq* lookup(const ModifiedKey& key) const;
    size_t size() const { return m_assignments.size(); }

    // Remove:
    // - doesSameWindow()
    // - Type enum
    // - window regex matching
};
```

---

## Data Flow

### Configuration Loading
```
1. User creates config.json
2. Engine::setSetting() calls JsonConfigLoader::load()
3. JsonConfigLoader::load()
   a. Read file into string
   b. Parse JSON (nlohmann/json)
   c. Validate schema (version, required fields)
   d. parseKeyboard() → populate Keyboard::m_table
   e. parseVirtualModifiers() → populate Setting::m_modTapActions, m_virtualModTriggers
   f. parseMappings() → create KeyAssignments in global Keymap
4. Engine stores m_globalKeymap pointer
5. Ready for event processing
```

### Event Processing (Unchanged)
```
1. Input device → evdev → InputEvent
2. Engine::keyboardHandler() receives event
3. EventProcessor::processEvent()
   a. Layer 1: evdev → YAMY scan code
   b. Layer 2: Lookup in RuleLookupTable (m_globalKeymap compiled rules)
   c. Layer 3: YAMY scan code → evdev
4. Output → uinput → system
```

---

## Testing Strategy

### Unit Tests

**test_json_loader.cpp** (New):
```cpp
TEST_CASE("JsonConfigLoader basic parsing") {
    JsonConfigLoader loader;
    Setting setting;

    // Valid config
    REQUIRE(loader.load(&setting, "test_configs/valid.json"));
    REQUIRE(setting.m_keyboard.size() > 0);
}

TEST_CASE("JsonConfigLoader virtual modifiers") {
    JsonConfigLoader loader;
    Setting setting;

    loader.load(&setting, "test_configs/vim-mode.json");

    // Check M00 is registered
    REQUIRE(setting.m_virtualModTriggers.size() == 1);
    REQUIRE(setting.m_modTapActions[0] == /* Escape scan code */);
}

TEST_CASE("JsonConfigLoader modifier parsing") {
    JsonConfigLoader loader;

    // Test "Shift-M00-A"
    ModifiedKey mk = loader.parseModifiedKey("Shift-M00-A");
    REQUIRE(mk.isValid());
    REQUIRE(mk.hasShift());
    REQUIRE(mk.isVirtualModActive(0));
}

TEST_CASE("JsonConfigLoader error handling") {
    JsonConfigLoader loader;
    Setting setting;

    // Invalid JSON syntax
    REQUIRE_FALSE(loader.load(&setting, "test_configs/invalid_syntax.json"));

    // Missing required field
    REQUIRE_FALSE(loader.load(&setting, "test_configs/missing_version.json"));

    // Unknown key
    REQUIRE_FALSE(loader.load(&setting, "test_configs/unknown_key.json"));
}
```

### Integration Tests

**test_engine_integration.cpp** (Existing - Modified):
```cpp
TEST_CASE("Engine with JSON config") {
    Engine engine;
    JsonConfigLoader loader;
    Setting setting;

    // Load JSON config
    REQUIRE(loader.load(&setting, "test_configs/basic.json"));
    engine.setSetting(&setting);

    // Verify global keymap set
    REQUIRE(engine.getGlobalKeymap() != nullptr);

    // Test basic mapping: A → Tab
    InputEvent press{EV_KEY, KEY_A, 1};
    InputEvent release{EV_KEY, KEY_A, 0};

    auto outputs = engine.processEvent(press);
    REQUIRE(outputs.size() == 1);
    REQUIRE(outputs[0].code == KEY_TAB);
}

TEST_CASE("Engine with M00 virtual modifier") {
    // Test hold-vs-tap detection
    // Test M00-A → Left arrow
    // ...
}
```

### Property-Based Tests

**property_json_loader.cpp** (New):
```cpp
rc::check("Valid JSON always parses without crash",
  [](const std::string& jsonContent) {
    JsonConfigLoader loader;
    Setting setting;

    // Should never crash, even on invalid JSON
    try {
        loader.load(&setting, jsonContent);
    } catch (...) {
        // Expected for invalid JSON
    }

    RC_ASSERT(true);  // No crash
  });
```

---

## Migration Plan

### Phase 1: Add JSON Loader (Week 1-2)
**Goal**: Add JSON support alongside .mayu (no breaking changes)

**Tasks**:
1. Add nlohmann/json to CMakeLists.txt (FetchContent)
2. Create json_config_loader.h with class interface
3. Implement JsonConfigLoader::load()
4. Implement parseKeyboard()
5. Implement parseVirtualModifiers()
6. Implement parseMappings()
7. Implement parseModifiedKey() helper
8. Write unit tests (test_json_loader.cpp)
9. Create example config.json
10. Verify all existing .mayu tests still pass

**Success Criteria**:
- [ ] JSON configs load successfully
- [ ] M00-MFF modifiers work identically
- [ ] All existing .mayu tests pass

---

### Phase 2: Simplify Engine (Week 3)
**Goal**: Remove window/focus/thread tracking

**Tasks**:
1. Remove FocusOfThread class from engine.h
2. Remove m_focusOfThreads, m_currentFocusOfThread members
3. Add m_globalKeymap member
4. Simplify Engine::setSetting()
5. Remove checkFocusWindow() method
6. Remove setFocus() method
7. Remove threadAttachNotify/Detach methods
8. Update keyboardHandler() to use m_globalKeymap
9. Delete engine_focus.cpp
10. Update CMakeLists.txt

**Success Criteria**:
- [ ] test_event_processor_ut passes
- [ ] test_engine_integration passes
- [ ] Build succeeds

---

### Phase 3: Simplify Keymap (Week 4)
**Goal**: Single global keymap only

**Tasks**:
1. Remove Type enum from Keymap class
2. Remove m_windowClass, m_windowTitle members
3. Remove doesSameWindow() method
4. Simplify Keymaps::getGlobalKeymap()
5. Remove searchWindow() method
6. Update JsonConfigLoader to use single global keymap
7. Run integration tests

**Success Criteria**:
- [ ] Key mappings work correctly
- [ ] Virtual modifiers integrate properly
- [ ] Tests pass

---

### Phase 4: Delete Parser (Week 5)
**Goal**: Remove .mayu support entirely

**Tasks**:
1. Delete src/core/settings/parser.h
2. Delete src/core/settings/parser.cpp
3. Delete src/core/settings/setting_loader.h
4. Delete src/core/settings/setting_loader.cpp
5. Delete src/core/commands/cmd_window_*.cpp (37 files)
6. Delete src/core/commands/cmd_clipboard_*.cpp (5 files)
7. Delete src/core/window/*.cpp (3 files)
8. Update CMakeLists.txt to remove deleted files
9. Update main.cpp to use JsonConfigLoader only
10. Clean build verification

**Success Criteria**:
- [ ] Clean build with no parser references
- [ ] All tests pass with JSON configs
- [ ] Binary size reduced by ~30%

---

### Phase 5: Documentation (Week 6)
**Goal**: Complete documentation and examples

**Tasks**:
1. Write docs/json-schema.md
2. Write docs/migration-guide.md (.mayu → JSON)
3. Create keymaps/vim-mode.json
4. Create keymaps/emacs-mode.json
5. Create schema/config.schema.json (JSON Schema)
6. Run performance benchmarks
7. Update README.md
8. Final code review

**Success Criteria**:
- [ ] All documentation complete
- [ ] Example configs work
- [ ] Performance improved

---

## Performance Considerations

### Config Loading
**Target**: <10ms (vs ~100ms for .mayu)

**Optimizations**:
- nlohmann/json is highly optimized
- No tokenization overhead (JSON native)
- No regex compilation for window matching
- Direct object construction (no AST)

### Event Processing
**Target**: ~50% reduction in latency

**Optimizations**:
- No window regex matching on every event
- No focus change detection
- Direct m_globalKeymap lookup
- RuleLookupTable unchanged (already O(1))

### Memory Usage
**Target**: <10MB resident set

**Savings**:
- No window class/title strings
- No thread-specific keymap copies
- No focus tracking state
- Simpler Setting object

---

## Error Handling

### JSON Parsing Errors
```cpp
try {
    auto config = nlohmann::json::parse(file_stream);
} catch (const nlohmann::json::parse_error& e) {
    logError("JSON syntax error at line " + std::to_string(e.byte) + ": " + e.what());
    return false;
}
```

### Schema Validation Errors
```cpp
if (!config.contains("version")) {
    logError("Missing required field: 'version'");
    return false;
}

if (config["version"] != "2.0") {
    logError("Unsupported version: " + config["version"].get<std::string>());
    return false;
}
```

### Key Resolution Errors
```cpp
Key* key = resolveKeyName("UnknownKey");
if (!key) {
    // Suggest similar names (Levenshtein distance)
    auto suggestions = findSimilarKeys("UnknownKey");
    logError("Unknown key 'UnknownKey'. Did you mean: " + suggestions[0] + "?");
    return false;
}
```

---

## Dependencies

### New Dependencies
- **nlohmann/json**: 3.11.3 (header-only, MIT license)
  - Add to conanfile.txt: `nlohmann_json/3.11.3`
  - Or use CMake FetchContent

### Removed Dependencies
- None (all existing dependencies remain)

---

**Document Version**: 1.0
**Created**: 2025-12-17
**Status**: Draft
