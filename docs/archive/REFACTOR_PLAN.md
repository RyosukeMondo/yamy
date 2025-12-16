# Refactoring Plan: Fix Modifier Matching & Remove m0-m19

**Commit:** 7901ef6 (safety checkpoint)

## Problem Statement

Modifier matching happens AFTER substitution, causing M20-M29 mappings to fail:

```
Current (BROKEN):
1. Physical W pressed (evdev 17, yamy 0x1100)
2. Substitution: W → A (yamy 0x1E00)
3. Modifier match: M20 + A ❌ (config has M20-*W)

Needed (FIXED):
1. Physical W pressed (evdev 17, yamy 0x1100)
2. Modifier check: M20 active? → Check keymap for M20-*W
3. If matched → Output result, SKIP substitution
4. If not matched → Apply substitution W → A
```

## Phase 1: Fix Modifier Matching Order

### Target Files
- `src/core/engine/engine_generator.cpp`
- `src/core/engine/engine_keyboard_handler.cpp`

### Changes Required

#### 1. Store Physical Scancode in Current/ModifiedKey
Add field to preserve original physical key before substitution:
```cpp
struct Current {
    uint16_t m_physical_yamy;  // NEW: Physical YAMY code before substitution
    uint16_t m_evdev_code;     // Already exists
    ModifiedKey m_mkey;
    // ...
};
```

#### 2. Check Keymap BEFORE Substitution (When Modifiers Active)
In `beginGeneratingKeyboardEvents()` around line 306:

```cpp
// NEW: Early keymap check if virtual modifiers are active
if (hasActiveVirtualModifiers()) {
    // Build ModifiedKey with PHYSICAL key + active modifiers
    ModifiedKey physical_mkey = buildPhysicalModifiedKey(i_c);

    // Try to find keymap match with physical key
    const Keymap::KeyAssignment *keyAssign =
        m_currentKeymap->searchAssignment(physical_mkey);

    if (keyAssign) {
        // MATCH FOUND! Execute action and skip substitution
        generateKeySeqEvents(i_c, keyAssign->m_keySeq, ...);
        return;  // Done, skip rest of processing
    }
}

// No match, proceed with normal flow (substitution, etc.)
// ... existing processEvent() call ...
```

#### 3. Helper Functions
```cpp
bool Engine::hasActiveVirtualModifiers() const {
    const uint32_t* modBits = m_modifierState.getModifierBits();
    for (int i = 0; i < 8; ++i) {
        if (modBits[i] != 0) return true;
    }
    return false;
}

ModifiedKey Engine::buildPhysicalModifiedKey(const Current& i_c) {
    ModifiedKey mkey;
    mkey.m_key = i_c.m_mkey.m_key;  // Physical key

    // Copy active modifiers
    const uint32_t* modBits = m_modifierState.getModifierBits();
    for (int i = 0; i < 8; ++i) {
        mkey.m_virtualMods[i] = modBits[i];
    }

    // Copy standard modifiers
    mkey.m_modifier = getCurrentModifiers(mkey.m_key, isPressed);

    return mkey;
}
```

## Phase 2: Remove Old m0-m19 Code

### Files to Modify (12 total)

#### 1. keyboard.h - Remove Type_Mod0-Mod19 enum
```cpp
// REMOVE:
enum Type {
    // ...
    Type_Mod0,   // Line ~181
    Type_Mod1,
    // ... through Type_Mod19
};
```

#### 2. modifier_state.cpp/h - Remove old modal modifier API
```cpp
// REMOVE old API (keep NEW API):
void activate(Modifier::Type type);           // REMOVE
void deactivate(Modifier::Type type);         // REMOVE
bool isActive(Modifier::Type type) const;     // REMOVE

// KEEP new API:
void activateModifier(uint8_t mod_num);       // KEEP
void deactivateModifier(uint8_t mod_num);     // KEEP
bool isModifierActive(uint8_t mod_num) const; // KEEP
```

#### 3. engine_modifier.cpp - Remove getCurrentModifiers() old code
```cpp
// REMOVE lines 86-88:
for (int i = Modifier::Type_Mod0; i <= Modifier::Type_Mod19; ++ i)
    cmods.press(static_cast<Modifier::Type>(i),
                isPressed(static_cast<Modifier::Type>(i)));
```

#### 4. engine_modifier.cpp - Remove isPressed() mod0-mod19 check
```cpp
// REMOVE lines 21-28:
if (i_mt >= Modifier::Type_Mod0 && i_mt <= Modifier::Type_Mod19) {
    bool active = m_modifierState.isActive(i_mt);
    if (active) {
        Acquire a(&m_log, 0);
        m_log << "[DEBUG] isPressed: mod" << (i_mt - Modifier::Type_Mod0) << " = ACTIVE" << std::endl;
    }
    return active;
}
```

#### 5. setting_loader.cpp - Remove M0-M19 map entries
```cpp
// REMOVE lines 457-476:
{ "M0-", Modifier::Type_Mod0 },
{ "M1-", Modifier::Type_Mod1 },
// ... through M19-
```

#### 6. setting_loader.cpp - Remove mod0-mod19 keywords
```cpp
// REMOVE lines 1385-1404:
else if (*t == "mod0" ) mt = Modifier::Type_Mod0;
else if (*t == "mod1" ) mt = Modifier::Type_Mod1;
// ... through mod19
```

#### 7. engine_event_processor.cpp - Remove mod0-mod19 comments
Clean up comments referencing old system.

#### 8-12. Other files
Clean up remaining references in:
- engine_generator.cpp
- modifier_key_handler.cpp/h
- keymap.cpp
- engine_setting.cpp

## Phase 3: Testing

### Test Cases
1. **TAP:** B tap → Enter ✅
2. **HOLD:** B + W → "1" ✅ (using PHYSICAL W before substitution)
3. **HOLD:** B + E → "2" ✅
4. **HOLD:** B + R → "3" ✅
5. **HOLD:** B + T → "4" ✅
6. **Substitution:** W (no modifier) → A ✅ (normal substitution still works)

### Validation
```bash
# Start daemon
./build/bin/yamy > /tmp/yamy_daemon.log 2>&1 &

# Load config
./build/bin/yamy-ctl reload --config "$(pwd)/keymaps/master_m00.mayu"

# Start engine
./build/bin/yamy-ctl start

# Test
# 1. Tap W → Should output "a" (substituted)
# 2. Hold B + tap W → Should output "1" (M20-*W match, no substitution)
```

## Phase 4: Commit

After successful testing:
```bash
git add -A
git commit -m "feat: Fix modifier matching order and remove m0-m19 legacy code

- Modifier matching now happens BEFORE substitution
- Physical key used for M00-MFF combinations
- Removed all Type_Mod0-Mod19 enum and legacy code
- Simplified to M00-MFF only (256 virtual modifiers)
- M20-*W now correctly matches physical W, not substituted A

BREAKING CHANGE: Old mod0-mod19 syntax no longer supported.
Use M00-MFF hex notation instead.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

## Rollback Plan

If refactoring breaks things:
```bash
git reset --hard 7901ef6  # Return to safety checkpoint
```

## Estimated Effort

- Phase 1 (Fix matching order): 2-3 hours
- Phase 2 (Remove m0-m19): 1-2 hours
- Phase 3 (Testing): 30 minutes
- Phase 4 (Cleanup & commit): 30 minutes

**Total:** 4-6 hours of focused work

## Success Criteria

✅ M20-M29 hold combinations work with physical keys (W, E, R, T)
✅ Normal substitution still works (W → A when no modifiers)
✅ No references to Type_Mod0-Mod19 in codebase
✅ All tests pass
✅ Code metrics under limits (max 500 lines/file)
