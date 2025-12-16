# Scan Code Mapping Fix

## Problem

Key remapping wasn't working correctly. For example:
- config.mayu: `def subst *A = *Tab` (A → Tab)
- **Expected**: KEY_TAB (evdev 15)
- **Got**: KEY_LEFTBRACE (evdev 26)

## Root Cause

**VK Code vs Scan Code Mismatch**

YAMY uses two different key code systems:

1. **VK Codes** (Virtual Key Codes): Windows-style codes
   - VK_A = 0x41
   - VK_TAB = 0x09

2. **Scan Codes** (Hardware Scan Codes): From 109.mayu
   - A scan code = 0x1E
   - Tab scan code = 0x0F
   - CommercialAt (@) scan code = 0x1A

### The Bug

`g_evdevToYamyMap` was using VK codes instead of scan codes:

```cpp
// WRONG (before fix):
{KEY_A, VK_A}  // KEY_A → 0x41 (VK code)

// RIGHT (after fix):
{KEY_A, 0x1E}  // KEY_A → 0x1E (scan code from 109.mayu)
```

### Data Flow

**Before Fix (BROKEN):**
```
1. Physical A key → evdev KEY_A (30)
2. Layer 1: KEY_A (30) → VK_A (0x41) ❌ Wrong! Engine expects scan codes
3. Layer 2 (engine): Can't find 0x41 in substitution table
4. Layer 3: Outputs mangled code → wrong key
```

**After Fix (CORRECT):**
```
1. Physical A key → evdev KEY_A (30)
2. Layer 1: KEY_A (30) → scan 0x1E ✓ Correct scan code for A
3. Layer 2 (engine): Applies def subst *A = *Tab → scan 0x1E becomes 0x0F
4. Layer 3: scan 0x0F → KEY_TAB (15) ✓ Correct!
```

## The Fix

Changed `g_evdevToYamyMap` in `src/platform/linux/keycode_mapping.cpp` to use scan codes from 109.mayu instead of VK codes:

### Letters (A-Z)

**Before:**
```cpp
{KEY_A, VK_A}, {KEY_B, VK_B}, {KEY_C, VK_C}, ...  // VK codes
```

**After:**
```cpp
{KEY_A, 0x1E}, {KEY_B, 0x30}, {KEY_C, 0x2E}, ...  // Scan codes from 109.mayu
```

### Numbers (0-9)

**Before:**
```cpp
{KEY_0, VK_0}, {KEY_1, VK_1}, ...  // VK codes
```

**After:**
```cpp
{KEY_0, 0x0B}, {KEY_1, 0x02}, ...  // Scan codes from 109.mayu
```

### Special Keys

**Before:**
```cpp
{KEY_TAB, VK_TAB},  // VK_TAB = 0x09
```

**After:**
```cpp
{KEY_TAB, 0x0F},    // Tab scan code from 109.mayu
```

## Verification

### Check Mappings

```bash
# Verify KEY_A maps to scan code 0x1E (not VK_A = 0x41)
grep "KEY_A," src/platform/linux/keycode_mapping.cpp | head -1
# Should show: {KEY_A, 0x1E}

# Verify KEY_TAB maps to scan code 0x0F (not VK_TAB = 0x09)
grep "KEY_TAB," src/platform/linux/keycode_mapping.cpp | head -1
# Should show: {KEY_TAB, 0x0F}
```

### Test Substitution

```bash
# Test A → Tab substitution
./build/bin/yamy-test e2e-auto 30 15

# Expected result:
# Input:    30 (KEY_A)
# Output:   15 (KEY_TAB)
# Status:   ✓ PASSED
```

## Files Modified

- **src/platform/linux/keycode_mapping.cpp**: Changed `g_evdevToYamyMap` to use scan codes
- **src/platform/linux/input_injector_linux.cpp**: Added Layer 2 debug logging

## Debug Logging

Layer 2 debug output now shows what the engine outputs:

```bash
export YAMY_DEBUG_KEYCODE=1
./build/bin/yamy

# Log output shows:
[LAYER1:IN] Input evdev code = 30 (KEY_A)
[LAYER1:IN] Mapped to YAMY code = 0x001E (30)  # Scan code 0x1E ✓

[LAYER2:OUT] Engine output code = 0x000F (15)  # Tab scan code 0x0F ✓

[LAYER3:OUT] Input YAMY code = 0x000F (15)
[LAYER3:OUT] Found in US scan map → evdev 15 (KEY_TAB)  # Correct! ✓
```

## Reference: Scan Code Mappings (109.mayu)

From `keymaps/109.mayu`:

```
Esc         = 0x01
_1          = 0x02
_2          = 0x03
...
_9          = 0x0a
_0          = 0x0b
Minus       = 0x0c
Caret       = 0x0d
BackSpace   = 0x0e
Tab         = 0x0f
Q           = 0x10
...
A           = 0x1e
S           = 0x1f
...
```

These are the codes the YAMY engine expects and uses for substitutions defined in config.mayu.
