# Design: Virtual Key System

**Spec Name**: virtual-key-system
**Created**: 2025-12-15
**Status**: Draft

## Architecture Overview

### Virtual Key Hierarchy

```
Physical Keys → Substitution Layer → Virtual Keys → Output Layer
                                    ↓
                          ┌─────────┴─────────┐
                          │                   │
                     Regular Keys      Special Keys
                      (V_ prefix)      ┌─────┴─────┐
                                       │           │
                                   Modifiers    Locks
                                   (M00-MFF)  (L00-LFF)
```

### Three Virtual Key Types

1. **Virtual Regular Keys (V_*)**: Virtual versions of any key
   - Purpose: Distinguish virtual from physical in keymaps
   - Example: `V_B`, `V_Enter`, `V_A`
   - Keycode range: 0xE000-0xEFFF (4096 keys)

2. **Modal Modifiers (M00-MFF)**: Hold-to-activate layers
   - Purpose: Layer switching, tap/hold behavior
   - Count: 256 modifiers (0x00-0xFF)
   - Keycode range: 0xF000-0xF0FF

3. **Lock Keys (L00-LFF)**: Toggle-able persistent state
   - Purpose: Mode locking, combinatorial logic
   - Count: 256 locks (0x00-0xFF)
   - Keycode range: 0xF100-0xF1FF

## Keycode Allocation

```cpp
// keycode_mapping.h

// Virtual regular keys: V_A, V_B, etc.
#define YAMY_VIRTUAL_KEY_BASE    0xE000
#define YAMY_VIRTUAL_KEY_MAX     0xEFFF
// Example: V_A = 0xE000 + offset_of_A

// Modal modifiers: M00-MFF
#define YAMY_MOD_00              0xF000
#define YAMY_MOD_FF              0xF0FF

// Lock keys: L00-LFF
#define YAMY_LOCK_00             0xF100
#define YAMY_LOCK_FF             0xF1FF

// Helper functions
inline bool isVirtualKey(uint16_t code) {
    return (code >= YAMY_VIRTUAL_KEY_BASE && code <= YAMY_VIRTUAL_KEY_MAX);
}

inline bool isModifier(uint16_t code) {
    return (code >= YAMY_MOD_00 && code <= YAMY_MOD_FF);
}

inline bool isLock(uint16_t code) {
    return (code >= YAMY_LOCK_00 && code <= YAMY_LOCK_FF);
}

inline uint8_t getModifierNumber(uint16_t code) {
    return static_cast<uint8_t>(code - YAMY_MOD_00);
}

inline uint8_t getLockNumber(uint16_t code) {
    return static_cast<uint8_t>(code - YAMY_LOCK_00);
}
```

## Configuration Syntax

### Virtual Keys

```mayu
# Virtual regular keys
def subst *A = *V_B              # A outputs virtual B
def subst *Q = *V_Quote          # Q outputs virtual '

# Use in keymaps
key M00-V_B = *Left              # M00 + virtual B
key M00-B = *Right               # M00 + physical B
```

### Modifiers

```mayu
# Map physical key to modifier
def subst *Space = *M00

# Assign tap action (optional)
mod assign M00 = *Space          # Tap M00 outputs Space

# Use in keymaps
key M00-H = *Left                # Single modifier
key M00-M01-H = *Home            # Multiple modifiers
```

### Locks

```mayu
# Map physical key to lock
def subst *CapsLock = *L00

# Use in keymaps
key L00-H = *Left                # Single lock
key L00-L01-H = *Home            # Multiple locks (combinatorial!)
key M00-L00-H = *End             # Mix modifiers and locks
```

## State Management

### ModifierState Class

```cpp
// modifier_state.h
class ModifierState {
public:
    // 256 modifiers (M00-MFF)
    void activateModifier(uint8_t mod_num);
    void deactivateModifier(uint8_t mod_num);
    bool isModifierActive(uint8_t mod_num) const;

    // Get bitmask of active modifiers
    const uint32_t* getModifierBits() const { return m_modifiers; }

private:
    uint32_t m_modifiers[8];  // 256 bits = 8 * 32-bit words
};
```

### LockState Class

```cpp
// lock_state.h
class LockState {
public:
    // 256 locks (L00-LFF)
    void toggleLock(uint8_t lock_num);
    bool isLockActive(uint8_t lock_num) const;

    // Get bitmask of active locks
    const uint32_t* getLockBits() const { return m_locks; }

    // IPC notification
    void notifyGUI();

private:
    uint32_t m_locks[8];  // 256 bits = 8 * 32-bit words

    void setBit(uint8_t lock_num, bool value);
};
```

### Implementation

```cpp
// lock_state.cpp
void LockState::toggleLock(uint8_t lock_num) {
    if (lock_num > 0xFF) return;

    uint32_t word_idx = lock_num / 32;
    uint32_t bit_idx = lock_num % 32;
    uint32_t mask = 1u << bit_idx;

    m_locks[word_idx] ^= mask;  // XOR to toggle

    notifyGUI();  // Update visual indicators
}

bool LockState::isLockActive(uint8_t lock_num) const {
    if (lock_num > 0xFF) return false;

    uint32_t word_idx = lock_num / 32;
    uint32_t bit_idx = lock_num % 32;

    return (m_locks[word_idx] & (1u << bit_idx)) != 0;
}

void LockState::notifyGUI() {
    // Send IPC message with lock state
    LockStatusMessage msg;
    memcpy(msg.lockBits, m_locks, sizeof(m_locks));
    // Send via existing IPC channel
    notifyGUI(MessageType::LockStatusUpdate, &msg);
}
```

## Event Processing Flow

### Layer 1: evdev → YAMY

```
Physical key event → evdev code → YAMY internal code
```

No changes needed (existing layer works).

### Layer 2a: Substitution

```cpp
// Apply substitutions FIRST
uint16_t yamy_code = applySubstitution(yamy_in);
// Now yamy_code might be:
// - Physical key (original)
// - Virtual key (V_*)
// - Modifier (M00-MFF)
// - Lock (L00-LFF)
```

### Layer 2b: Modifier/Lock Processing

```cpp
// Check if result is modifier
if (isModifier(yamy_code)) {
    return processModifier(yamy_code, eventType);
}

// Check if result is lock
if (isLock(yamy_code)) {
    return processLock(yamy_code, eventType);
}

// Check if keymap entry exists with active modifiers/locks
return lookupKeymap(yamy_code, modifierState, lockState);
```

### Layer 3: YAMY → evdev

```cpp
// Suppress virtual keys (they don't have evdev codes)
if (isVirtualKey(yamy_code) || isModifier(yamy_code) || isLock(yamy_code)) {
    return 0;  // Don't output
}

return yamyToEvdevKeyCode(yamy_code);
```

## Tap/Hold Detection for Modifiers

### Timing

```cpp
class ModifierKeyHandler {
    struct ModifierState {
        std::chrono::steady_clock::time_point press_time;
        bool is_pressed;
        uint16_t tap_output;  // From "mod assign M00 = *Enter"
    };

    std::unordered_map<uint8_t, ModifierState> m_modStates;

    const int TAP_THRESHOLD_MS = 200;
};
```

### State Machine

```
┌─────────┐
│  IDLE   │
└────┬────┘
     │ Key DOWN
     ▼
┌─────────────┐
│ PRESSED     │◄───────── Timer not expired
│ (waiting)   │
└────┬────┬───┘
     │    │
     │    └── Timer expired → HELD (activate modifier)
     │
     │ Key UP (before timer)
     ▼
┌─────────────┐
│ TAP         │
│ (output)    │
└─────────────┘
```

### Implementation

```cpp
ProcessingAction ModifierKeyHandler::processModifier(
    uint16_t mod_code,
    EventType type,
    ModifierState& io_modState
) {
    uint8_t mod_num = getModifierNumber(mod_code);

    if (type == EventType::PRESS) {
        m_modStates[mod_num].press_time = std::chrono::steady_clock::now();
        m_modStates[mod_num].is_pressed = true;

        // Wait to determine tap vs hold
        return ProcessingAction::WAITING_FOR_THRESHOLD;

    } else {  // RELEASE
        auto duration = std::chrono::steady_clock::now() -
                        m_modStates[mod_num].press_time;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            duration).count();

        if (ms < TAP_THRESHOLD_MS) {
            // TAP detected
            uint16_t tap_output = m_modStates[mod_num].tap_output;
            if (tap_output != 0) {
                return {ProcessingAction::OUTPUT_TAP, tap_output};
            }
            return ProcessingAction::SUPPRESS;  // No tap action defined
        } else {
            // HOLD - deactivate modifier
            io_modState.deactivateModifier(mod_num);
            return ProcessingAction::DEACTIVATE_MODIFIER;
        }
    }
}
```

## Keymap Matching with Specificity

### Priority Rules

More specific matches win:
1. Most modifiers + most locks + key
2. Fewer modifiers + most locks + key
3. Most modifiers + fewer locks + key
4. No modifiers/locks + key

### Example

```mayu
key A = *1                    # Specificity: 0
key M00-A = *2                # Specificity: 1 modifier
key M00-M01-A = *3            # Specificity: 2 modifiers
key M00-L00-A = *4            # Specificity: 1 mod + 1 lock
key M00-M01-L00-L01-A = *5    # Specificity: 2 mod + 2 lock (highest!)
```

### Matching Algorithm

```cpp
struct KeymapEntry {
    uint32_t required_mods[8];   // Bitmask of required M00-MFF
    uint32_t required_locks[8];  // Bitmask of required L00-LFF
    uint16_t input_key;
    uint16_t output_key;
    uint8_t specificity;  // Precomputed: popcount(mods) + popcount(locks)
};

std::vector<KeymapEntry> m_keymap;  // Sorted by specificity DESC

uint16_t lookupKeymap(
    uint16_t key,
    const ModifierState& mods,
    const LockState& locks
) {
    const uint32_t* active_mods = mods.getModifierBits();
    const uint32_t* active_locks = locks.getLockBits();

    // Check most specific entries first
    for (const auto& entry : m_keymap) {
        if (entry.input_key != key) continue;

        // Check if all required modifiers are active
        bool mods_match = true;
        for (int i = 0; i < 8; i++) {
            if ((active_mods[i] & entry.required_mods[i]) != entry.required_mods[i]) {
                mods_match = false;
                break;
            }
        }
        if (!mods_match) continue;

        // Check if all required locks are active
        bool locks_match = true;
        for (int i = 0; i < 8; i++) {
            if ((active_locks[i] & entry.required_locks[i]) != entry.required_locks[i]) {
                locks_match = false;
                break;
            }
        }
        if (!locks_match) continue;

        // Found match!
        return entry.output_key;
    }

    // No match - passthrough
    return key;
}
```

## Parser Changes

### Token Recognition

```cpp
// Parser must recognize new token types

enum TokenType {
    // ... existing tokens
    TOK_VIRTUAL_KEY,    // V_A, V_B, V_Enter, etc.
    TOK_MODIFIER,       // M00, M01, ... MFF
    TOK_LOCK,           // L00, L01, ... LFF
    TOK_MOD_ASSIGN,     // "mod assign"
};

Token parseKeyToken(const std::string& str) {
    if (str.starts_with("V_")) {
        // Virtual key: V_A, V_Enter, etc.
        std::string key_name = str.substr(2);  // Remove "V_"
        uint16_t base_code = lookupKeyCode(key_name);
        return {TOK_VIRTUAL_KEY, YAMY_VIRTUAL_KEY_BASE + base_code};
    }

    if (str.starts_with("M") && str.length() == 3) {
        // Modifier: M00, M01, ... MFF
        std::string hex = str.substr(1);
        uint16_t num = std::stoi(hex, nullptr, 16);
        if (num <= 0xFF) {
            return {TOK_MODIFIER, YAMY_MOD_00 + num};
        }
    }

    if (str.starts_with("L") && str.length() == 3) {
        // Lock: L00, L01, ... LFF
        std::string hex = str.substr(1);
        uint16_t num = std::stoi(hex, nullptr, 16);
        if (num <= 0xFF) {
            return {TOK_LOCK, YAMY_LOCK_00 + num};
        }
    }

    // Fallback to existing key lookup
    return {TOK_KEY, lookupKeyCode(str)};
}
```

### Grammar Extensions

```
# New statement type: mod assign
mod_assign_stmt ::= "mod" "assign" MODIFIER "=" "*" KEY

# Examples:
# mod assign M00 = *Enter
# mod assign M01 = *Space
```

## GUI Lock Indicators

### IPC Message

```cpp
// MessageType enum (add to existing)
enum class MessageType : uint32_t {
    // ... existing types
    LockStatusUpdate = 0x0200,
};

// Lock status message
struct LockStatusMessage {
    uint32_t lockBits[8];  // 256 bits for L00-LFF
};
```

### GUI Component (Qt)

```cpp
// lock_indicator_widget.h
class LockIndicatorWidget : public QWidget {
    Q_OBJECT

public:
    void updateLockStatus(const uint32_t lockBits[8]);

private:
    struct LockIndicator {
        QLabel* label;      // "L00"
        QLabel* status;     // Green dot or gray dot
    };

    std::vector<LockIndicator> m_indicators;

    void setLockActive(uint8_t lock_num, bool active);
};

// Implementation
void LockIndicatorWidget::updateLockStatus(const uint32_t lockBits[8]) {
    for (uint8_t i = 0; i <= 0xFF; i++) {
        uint32_t word_idx = i / 32;
        uint32_t bit_idx = i % 32;
        bool active = (lockBits[word_idx] & (1u << bit_idx)) != 0;

        setLockActive(i, active);
    }
}

void LockIndicatorWidget::setLockActive(uint8_t lock_num, bool active) {
    if (lock_num >= m_indicators.size()) return;

    // Update visual indicator
    QString color = active ? "green" : "gray";
    m_indicators[lock_num].status->setStyleSheet(
        QString("background-color: %1; border-radius: 6px;").arg(color)
    );
}
```

## Migration Guide

### Old Syntax → New Syntax

```mayu
# OLD (broken):
def subst *B = *Enter
mod mod0 = !!B
keymap Global : GLOBAL
    key Mod0-A = *_1

# NEW (working):
def subst *B = *M00
mod assign M00 = *Enter
keymap Global : GLOBAL
    key M00-A = *_1
```

### Virtual Key Migration

```mayu
# OLD (ambiguous):
def subst *A = *B
key Mod0-B = *C    # Does this trigger when A is pressed?

# NEW (explicit):
def subst *A = *V_B
key M00-B = *C     # Physical B only
key M00-V_B = *D   # Virtual B (from A substitution)
```

## File Structure

```
src/
├── core/
│   ├── input/
│   │   ├── modifier_state.h/cpp       # ModifierState class (update)
│   │   └── lock_state.h/cpp           # LockState class (new)
│   ├── engine/
│   │   ├── engine_event_processor.cpp # Update layer 2 logic
│   │   ├── modifier_key_handler.cpp   # Update for M00-MFF
│   │   └── engine.h                   # Add LockState member
│   └── settings/
│       └── setting_loader.cpp         # Parser updates
├── platform/
│   └── linux/
│       └── keycode_mapping.h          # Add virtual key ranges
└── ui/
    └── qt/
        └── lock_indicator_widget.h/cpp # GUI indicators (new)
```

## Testing Strategy

### Unit Tests

1. **ModifierState**: Activate/deactivate, bitmask operations
2. **LockState**: Toggle, persistence, IPC notification
3. **Parser**: V_, M00, L00 token recognition
4. **Keymap matching**: Specificity, combinatorial logic

### E2E Tests

Use existing test infrastructure:

```mayu
# Test config: tests/scenarios/virtual-keys.mayu
def subst *A = *V_B
def subst *B = *M00
mod assign M00 = *Enter

keymap Global : GLOBAL
    key M00-H = *Left
    key M00-V_B = *Right
```

Test cases:
1. Press A → outputs nothing (waiting for threshold)
2. Quick tap A → outputs Enter (tap action)
3. Hold A + press H → outputs Left
4. Hold A + press A → outputs Right (V_B mapping)

## Performance Considerations

### Memory Overhead

```
ModifierState: 32 bytes (256 bits)
LockState:     32 bytes (256 bits)
Total:         64 bytes (negligible)
```

### Lookup Complexity

- Modifier check: O(1) - array index
- Lock check: O(1) - array index
- Keymap lookup: O(n) where n = number of keymap entries
  - Optimization: Sort by specificity, early exit on first match
  - Typical n < 1000 entries → <1ms

### Timing Critical Paths

- Modifier activation: 1 bitwise OR operation (<1μs)
- Lock toggle: 1 bitwise XOR + IPC call (<50μs)
- Keymap match: O(n) loop with bitmask checks (<1ms)

## Error Handling

### Parser Errors

```cpp
// Invalid modifier number
"M100" → Error: Modifier must be M00-MFF (hex)

// Invalid lock number
"LGG" → Error: Lock must be L00-LFF (hex)

// Undefined virtual key
"V_INVALID" → Error: Key INVALID not found
```

### Runtime Errors

```cpp
// Modifier overflow (shouldn't happen with uint8_t)
assert(mod_num <= 0xFF);

// Lock overflow
assert(lock_num <= 0xFF);
```

## Dependencies

- Existing: IPC system, Qt GUI, event processing pipeline
- New: None (self-contained feature)

## Backward Compatibility

**Breaking change**: Old modal modifier syntax removed.

Migration required for:
- `mod mod0 = !!Key` → `def subst *Key = *M00`
- `key Mod0-A` → `key M00-A`
- `key Shift-Mod0-A` → `key S-M00-A`

Provide migration script:
```bash
# migrate_config.sh
sed -i 's/mod mod0 = !!\(.*\)/def subst *\1 = *M00/' config.mayu
sed -i 's/Mod0-/M00-/g' config.mayu
# ... etc
```

## Open Questions

None - design is crystal clear from discussion.
