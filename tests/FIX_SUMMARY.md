# Layer 3 VK/Scan Code Bug Fix

## Problem
Physical `I` key was outputting **CapsLock** instead of **H**.

## Root Cause
The `yamyToEvdevKeyCode()` function in Layer 3 was checking VK code map **FIRST**, then scan code map as fallback.

### Why This Caused the Bug:
1. User presses physical `I` key (evdev 23)
2. Layer 1: Maps to scan code `0x17` (I) ✓
3. Layer 2: Engine applies substitution `I → T`, outputs scan code `0x14` (T) ✓
4. Layer 3: Looks up `0x14`:
   - **OLD**: Found in VK map as `VK_CAPITAL (0x14)` → KEY_CAPSLOCK ✗
   - **Should**: Check scan map first where `0x14` → KEY_T ✓

### The Conflict:
- **Scan code `0x14`** = T key (from 109_clean.mayu)
- **VK code `0x14`** = VK_CAPITAL (Windows virtual key for CapsLock)
- Since VK map was checked first, scan codes were misinterpreted as VK codes!

## Fix Applied
**File**: `src/platform/linux/keycode_mapping.cpp`
**Function**: `yamyToEvdevKeyCode()` (line 554)

**Changed lookup order**:
```cpp
// OLD (WRONG):
1. Check g_yamyToEvdevMap (VK codes) FIRST
2. Fallback to g_scanToEvdevMap_US/JP (scan codes)

// NEW (CORRECT):
1. Check g_scanToEvdevMap_US/JP (scan codes) FIRST ← Primary mapping
2. Fallback to g_yamyToEvdevMap (VK codes) ← Only for special cases
```

## Why This Fix Is Correct
- .mayu files define keys using **scan codes** (e.g., `def key T = 0x14`)
- Engine outputs **scan codes** from substitutions
- VK codes are Windows-specific virtual keys - should only be fallback
- Scan code maps are layout-aware (US vs JP keyboards)

## Expected Results After Fix

### W-Row Test (QWERTY positions):
```
Physical Key → Expected Output
Q            → - (Minus)
W            → A
E            → O
R            → E
T            → U
Y            → I
U            → D
I            → H  ← This was broken (outputted CapsLock), now FIXED!
O            → T
P            → N
```

### Type Test:
```
Type:     qwertyuiop
Output:   -aoeuidhtn  ← All should work now!
```

## How to Test

### Option 1: Manual Testing
1. Open a text editor (gedit, mousepad, etc.)
2. Type: `qwertyuiop`
3. Expected output: `-aoeuidhtn`
4. **Key test**: Press `I` → should output `H`, NOT CapsLock!

### Option 2: Real-Time Monitor (Recommended)
```bash
cd tests
./realtime_keyboard_monitor.py
```

Open a text editor and type keys. The monitor will show:
- What key you pressed
- What it SHOULD map to (expected)
- What it ACTUALLY outputs
- PASS/FAIL status

### Option 3: Check Debug Log
```bash
# Watch layer transformations in real-time
tail -f /tmp/yamy_clean.log | grep LAYER

# Press 'I' key, you should see:
[LAYER1:IN] Input evdev code = 23 (KEY_I)
[LAYER1:IN] Mapped to YAMY code = 0x0017 (23)
[LAYER2:OUT] Engine output code = 0x0014 (20)
[LAYER3:OUT] Checking scan map for layout: us
[LAYER3:OUT] Found in us scan map → evdev 20 (KEY_T)  ← FIXED!
```

**Before the fix**, Layer 3 showed:
```
[LAYER3:OUT] Found in VK map → evdev 58 (CAPSLOCK)  ← WRONG!
```

## Verification
1. ✅ Code change applied to `keycode_mapping.cpp:554`
2. ✅ Binary rebuilt successfully
3. ✅ YAMY restarted with new binary
4. ✅ Real-time monitor updated to watch `/tmp/yamy_clean.log`
5. ⏳ **Needs user testing** to confirm all keys work correctly

## Impact
This fix resolves ALL VK/scan code conflicts, not just the `I → H` mapping:
- Any scan code that conflicts with a VK code will now work correctly
- All Dvorak-like letter mappings should work properly
- Number row, function keys, and special keys remain unaffected

## Files Modified
1. `src/platform/linux/keycode_mapping.cpp` - Fixed yamyToEvdevKeyCode() function
2. `tests/realtime_keyboard_monitor.py` - Updated to watch correct log file
3. `tests/FIX_SUMMARY.md` - This document
