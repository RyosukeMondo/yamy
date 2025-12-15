# M00 Hold Detection - Root Cause Analysis

## Summary

**Status:** TAP works ✅, HOLD doesn't work ❌

**Root Cause Hypothesis:** Keymap entries parsed from `key M00-*A = *_1` are not storing the M00 bit in `ModifiedKey::m_virtualMods[8]` during config loading.

## Detailed Analysis

### What Works ✅

1. **TAP Action (B → Enter)**
   - Event: B pressed → WAITING state
   - Event: B released < 200ms → TAP detected
   - Output: Enter key
   - **Why it works:** ModifierKeyHandler correctly returns tap_output (0x001C = Enter)

2. **M00 Activation**
   - When B is held > 200ms: `m_modifierState.activateModifier(0)` is called
   - M00 bit is set in `m_modifierState.m_modal[0]` (bit 0)
   - Logs confirm: `[MODIFIER] Auto-activating M00 (0x0030) - threshold exceeded`

3. **M00 Propagation to ModifiedKey**
   - In `beginGeneratingKeyboardEvents()` lines 534-538:
     ```cpp
     const uint32_t* modBits = m_modifierState.getModifierBits();
     for (int i = 0; i < 8; ++i) {
         cnew.m_mkey.m_virtualMods[i] = modBits[i];
     }
     ```
   - M00 bit is correctly copied from ModifierState to ModifiedKey

### What Doesn't Work ❌

4. **Keymap Matching (M00 + A → 1)**
   - Event: A pressed while M00 is active
   - Expected: Match `key *W-*A-*S-*C-M00-*A = *W-*A-*S-*C-_1`
   - Actual: No match found
   - **Why it fails:** Unknown - need to check if keymap entries have M00 bit set

## Event Flow

```
User Action: Hold B + Press A
↓
1. B PRESS → ModifierKeyHandler → WAITING state
↓
2. Wait 200ms → ModifierKeyHandler → M00 ACTIVE in m_modifierState ✅
↓
3. A PRESS → EventProcessor → Substitution (A → A, no change)
↓
4. beginGeneratingKeyboardEvents(A)
   ├─ Apply substitution (A → A)
   ├─ Copy M00 from m_modifierState to cnew.m_mkey.m_virtualMods[0] ✅
   └─ Call generateKeyboardEvents(cnew)
↓
5. generateKeyboardEvents()
   └─ m_keymap->searchAssignment(cnew.m_mkey)
      ├─ Input: key=A, M00_active=1
      ├─ Search: key M00-*A in keymap
      └─ Result: NOT FOUND ❌ (Why?)
↓
6. Fallback: Default key sequence (passthrough A)
```

## Hypothesis: Config Parser Issue

When `key M00-*A = *_1` is parsed, the KeyAssignment might not be setting the M00 bit in `KeyAssignment.m_modifiedKey.m_virtualMods[8]`.

### Expected vs Actual

**Expected:** When parser sees `M00-*A`:
```cpp
KeyAssignment assignment;
assignment.m_modifiedKey.m_key = A;
assignment.m_modifiedKey.setVirtualModActive(0, true);  // Set M00 bit ✅
```

**Actual (suspected):** Parser might be setting OLD mod0 instead of NEW M00:
```cpp
KeyAssignment assignment;
assignment.m_modifiedKey.m_key = A;
assignment.m_modifiedKey.m_modifier.press(Modifier::Type_Mod0, true);  // WRONG! ❌
// m_virtualMods[8] is NOT set ❌
```

## Verification Steps

1. **Check searchAssignment logs:**
   ```bash
   tail -f /tmp/yamy_daemon.log | grep "searchAssignment"
   ```
   - Hold B + Press A
   - Check if any assignments show `M00_required=1`

2. **Check if keymap entries have M00 bit set:**
   - Add logging in setting_loader.cpp where KeyAssignment is created
   - Check if `m_virtualMods[8]` is being set during parsing

3. **Compare OLD vs NEW system:**
   - OLD: `key m0-*A = *_1` → sets `Modifier::Type_Mod0`
   - NEW: `key M00-*A = *_1` → should set `m_virtualMods[0] bit 0`

## Next Steps

1. Run YAMY with verbose logging and test B+A
2. Check if searchAssignment finds ANY assignments with M00_required=1
3. If not, check setting_loader.cpp to see how M00-*A is parsed
4. Fix parser to set m_virtualMods[8] instead of Modifier::Type_Mod0

## Files to Check

- `src/core/settings/setting_loader.cpp` - Parser that creates KeyAssignment entries
- `src/core/input/keymap.cpp:303` - searchAssignment() with M00 matching logic
- `src/core/engine/engine_generator.cpp:534-538` - M00 propagation (WORKS ✅)

## Expected Fix Location

The parser in setting_loader.cpp needs to:
1. Recognize `M00` prefix in key definitions
2. Call `KeyAssignment.m_modifiedKey.setVirtualModActive(mod_num, true)`
3. NOT call `KeyAssignment.m_modifiedKey.m_modifier.press(Type_Mod0, true)`

This is different from:
- `def subst *B = *M00` - which IS working (substitution table entry)
- `mod assign M00 = *Enter` - which IS working (tap action registration)

The issue is specifically with:
- `key M00-*A = *_1` - keymap entry creation during parsing
