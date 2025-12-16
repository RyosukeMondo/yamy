# M00 Hold Issue - Debug Summary

## Current Status

✅ **Config loaded:** master_m00.mayu
✅ **M00 registered:** Physical key 0x0030 (B) → M00, tap=Enter
✅ **Parser working:** M00 mappings parsed correctly
✅ **TAP working:** B tap → Enter ✓
❌ **HOLD NOT working:** B+A should → 1, but doesn't ❌

## Analysis

From logs:
```
[MODIFIER] Registered virtual modifier trigger: physical key 0x0030 → M00, tap_output=0x001C
```

This shows:
- Physical B (0x0030 = evdev 48) is registered
- M00 tap output = 0x001C (Enter)
- Registration is CORRECT ✅

## The Problem

When you HOLD B + press A:
- Expected: Output "1"
- Actual: Nothing (or passthrough)

This means M00 modifier is NOT being activated when B is held.

## Possible Causes

1. **Hold threshold not reached** - Need to hold B > 200ms
2. **Modifier state not updating** - M00 not being added to modifier state
3. **Keymap not matching** - M00 mappings not being found
4. **Event processing issue** - Hold detection not triggering

## Debug Steps

### Step 1: Hold B and check logs

**Do this:**
1. Open terminal: `tail -f /tmp/yamy_daemon.log | grep -E "(WAIT|ACTIVE|M00|modifier|0x0030)"`
2. Press and HOLD B (count to 3 seconds)
3. While holding, press A
4. Release both

**What to look for:**
- "WAITING" - B pressed, waiting for threshold
- "ACTIVE" - Threshold passed, M00 activated
- "M00" - M00 modifier state change

### Step 2: Check if any modifier activates

Try holding B for a LONG time (5 seconds) before pressing A.

If still nothing → Modifier activation code not running

### Step 3: Check substitution

The issue might be that B is being substituted to M00 BEFORE the hold detection checks it.

**Flow should be:**
1. Physical B (evdev 48) pressed
2. ModifierHandler checks: "Is this a modifier trigger?"
3. If yes, start HOLD timer
4. If held > 200ms, activate M00
5. Process next key with M00 active

**But might be happening:**
1. Physical B pressed
2. Substitution: B → M00 (too early!)
3. ModifierHandler checks M00 (not registered as trigger!)
4. No hold detection

## Quick Test

Let me check if this is a substitution order issue...

The problem is likely that **substitution happens BEFORE hold detection**!

## Solution Needed

The modifier detection needs to happen on the PHYSICAL evdev scancode (48) BEFORE any substitution.

Current flow might be:
```
evdev 48 (B) → [SUBST] → M00 (0xF000) → [MOD CHECK] → Not a trigger!
```

Should be:
```
evdev 48 (B) → [MOD CHECK] → Is trigger! Start timer → [SUBST if tap] → M00
```

## Next Steps

I need to check the event processing order to see where substitution happens relative to modifier detection.

---

**Current diagnosis:** Substitution might be happening too early, preventing hold detection from recognizing B as a modifier trigger.
