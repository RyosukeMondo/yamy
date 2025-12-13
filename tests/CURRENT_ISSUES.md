# Current Issues After Engine Start

## ✅ What's Working
- Engine is running and hooking keyboard (3 devices hooked)
- Layer transformations are happening
- W → A mapping works correctly
- Most letter keys work

## ❌ Issues Found

### 1. Keys 'E' and 'U' Only Work on Key UP (Not Key Down)
**User Report**: Physical 'R' and 'T' keys (which should output 'E' and 'U') only produce output when releasing the key, not when pressing it.

**Expected Behavior**:
- Press physical R → Output E immediately
- Press physical T → Output U immediately

**Actual Behavior**:
- Press physical R → No output
- Release physical R → Output E
- Press physical T → No output
- Release physical T → Output U

**Possible Cause**: Key event timing issue - the engine might be only processing key release events for these keys, or there's a delay/buffering issue.

### 2. Number 4 Key Has No Mapping (4→4)
**User Report**: Pressing the number 4 key outputs '4' (no remapping)

**Config Check**:
```
Number key mappings in config_clean.mayu:
0 → R
1 → LShift
2 → Colon
3 → Comma
5 → P
6 → Y
7 → F
8 → G
9 → C
4 → (NOT MAPPED)
```

**Cause**: The number 4 key was not mapped in the original configuration. This appears to be intentional or was missed.

**Solution**: If 4 should be mapped, add a line to config_clean.mayu:
```
def subst *_4 = *[TARGET_KEY]
```

### 3. Hardware N Should Be Shift Modifier (Not Working)
**User Report**: Physical N key should act as Left Shift (modifier), but it's not working.

**Config Says**: `def subst *N = *LShift`

**Log Evidence**:
```
[LAYER1:IN] Input evdev code = 49 (N)
[LAYER1:IN] Mapped to YAMY code = 0x0031 (49)
← NO LAYER2 OUTPUT!
← NO LAYER3 OUTPUT!
```

**Cause**: The engine captures the N key in Layer 1 but doesn't transform it in Layer 2. The key substitution is not being applied.

**Possible Reasons**:
1. Modifier key substitutions might require special handling
2. The engine might not be processing the N key substitution correctly
3. There could be an issue with how LShift is being applied

## 🔍 Next Steps to Investigate

### For Issue 1 (E/U Key Up Only):
1. Check if there's a key repeat or timing issue in the engine
2. Look at how key press vs key release events are handled
3. Check if there's special handling for certain scan codes

### For Issue 2 (Number 4 Not Mapped):
- Decide if this should be mapped and add to config if needed

### For Issue 3 (N → LShift Not Working):
1. Check if modifier key substitutions work at all
2. Look at engine logs to see why N key doesn't produce LAYER2/LAYER3 output
3. Check if there's special handling required for modifier keys

## 📊 Summary

| Key | Expected | Actual | Status |
|-----|----------|--------|--------|
| W | A | A | ✅ Works |
| E (physical R) | E | E (on key up only) | ⚠️ Partial |
| U (physical T) | U | U (on key up only) | ⚠️ Partial |
| 4 | ??? | 4 | ❌ Not mapped |
| N | LShift | N | ❌ Not working |

## 🎯 Priority
1. **High**: Fix N → LShift (modifier keys are critical)
2. **High**: Fix E/U key up issue (affects usability)
3. **Low**: Add mapping for number 4 (if needed)
