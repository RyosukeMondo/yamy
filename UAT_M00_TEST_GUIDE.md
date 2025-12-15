# M00 UAT Test Guide

## YAMY Daemon Status

✅ **RUNNING** with config: `keymaps/master_m00.mayu`

**PIDs:** 1032945, 1033452
**Log file:** `/tmp/yamy_m00_test.log`

## Watch Logs (Real-time)

```bash
# Terminal 1: Watch YAMY logs in real-time
tail -f /tmp/yamy_m00_test.log
```

## Test Cases for M00

### Test 1: TAP Action (B → Enter)

**Expected:** Quick tap of B key → Outputs Enter

**How to test:**
1. In any text editor
2. Quick tap B (press and release < 200ms)
3. Should produce: **Enter/newline**

**Success criteria:** ✓ B tap creates new line

---

### Test 2: HOLD Action - Basic (B + A → 1)

**Expected:** Hold B + press A → Outputs 1

**How to test:**
1. Hold down B key (≥ 200ms)
2. While holding B, press A
3. Should produce: **1**

**Success criteria:** ✓ B+A outputs "1"

---

### Test 3: HOLD Action - Navigation (B + Colon → Left)

**Expected:** Hold B + press Colon → Outputs Left arrow

**How to test:**
1. Hold down B key
2. While holding B, press Colon (Z key after substitution)
3. Cursor should move left

**Success criteria:** ✓ Cursor moves left

---

### Test 4: HOLD Action - Function Keys (B + Q → F2)

**Expected:** Hold B + press Q → Outputs F2

**How to test:**
1. Hold down B key
2. While holding B, press Q (becomes Minus after subst)
3. Should trigger F2 function

**Success criteria:** ✓ F2 is triggered

---

### Test 5: Wildcard Modifiers (B + Shift + A → Shift + 1)

**Expected:** Hold B + Shift + press A → Outputs Shift+1 (which is !)

**How to test:**
1. Hold down B key
2. Hold Shift
3. Press A
4. Should produce: **!** (Shift+1)

**Success criteria:** ✓ Outputs "!"

---

### Test 6: TAP vs HOLD Timing

**Test 6a - Fast tap (< 200ms):**
- Quick B tap → Should output Enter ✓

**Test 6b - Slow hold (≥ 200ms):**
- Hold B (wait 300ms), press A → Should output 1 ✓

**Success criteria:** ✓ Timing threshold works correctly

---

## Quick Test Table

| Test | Input | Expected Output | Result |
|------|-------|----------------|--------|
| 1. TAP | Quick B tap | Enter | ☐ |
| 2. HOLD basic | Hold B + A | 1 | ☐ |
| 3. Navigation | Hold B + Colon | Left arrow | ☐ |
| 4. Function | Hold B + Q | F2 | ☐ |
| 5. With Shift | Hold B + Shift + A | ! | ☐ |
| 6a. Fast tap | B (< 200ms) | Enter | ☐ |
| 6b. Slow hold | B (≥ 200ms) + A | 1 | ☐ |

## Complete M00 Mapping Reference

All 45 M00 mappings:

```
Hold B + Tab → F11
Hold B + Kanji → Enter
Hold B + Colon → Left arrow
Hold B + Comma → Right arrow
Hold B + LAlt → Tab
Hold B + E → _3
Hold B + L → Ctrl+Alt+Shift+B
Hold B + Slash → @
Hold B + S → _0
Hold B + V → F9
Hold B + Z → F10
Hold B + _5 → F9
Hold B + Esc → F9
Hold B + Period → Down arrow
Hold B + P → Up arrow
Hold B + Y → Ctrl+Alt+Shift+Y
Hold B + F → @
Hold B + G → Home
Hold B + C → End
Hold B + R → Ctrl+Alt+Shift+A
Hold B + Space → F12
Hold B + Minus → F12
Hold B + A → 1
Hold B + O → 2
Hold B + U → 4
Hold B + I → 5
Hold B + D → 6
Hold B + H → 7
Hold B + T → 8
Hold B + N → 9
Hold B + Yen → F11
Hold B + Eisuu → F11
Hold B + Semicolon → F1
Hold B + Q → F2
Hold B + J → F3
Hold B + K → F4
Hold B + X → F5
Hold B + B → F6
Hold B + M → F7
Hold B + W → F8
Hold B + Del → Ctrl+W
Hold B + BS → Ctrl+F2
Hold B + LCtrl → Shift+Tab
```

## Troubleshooting

### Issue: B outputs Enter immediately (no hold)

**Cause:** TAP threshold might be too long, or key is being released

**Fix:** Check timing - hold B for at least 200ms before pressing second key

### Issue: B doesn't output anything when tapped

**Cause:** Config not loaded or M00 tap action not defined

**Fix:**
```bash
grep "mod assign M00" keymaps/master_m00.mayu
# Should show: mod assign M00 = *Enter
```

### Issue: Wrong key output

**Cause:** Check substitution mapping

**Fix:** Remember physical keys are substituted:
- Physical Q → Minus (then M08)
- Physical A → Tab (then M09)
- Physical B → M00 (direct)
- etc.

## Stop/Restart YAMY

**Stop:**
```bash
killall -9 yamy
```

**Restart with M00 config:**
```bash
YAMY_CONFIG_FILE=keymaps/master_m00.mayu ./build/bin/yamy > /tmp/yamy_m00_test.log 2>&1 &
```

**Restart with OLD config (rollback):**
```bash
killall -9 yamy
YAMY_CONFIG_FILE=keymaps/config.mayu ./build/bin/yamy > /tmp/yamy_old.log 2>&1 &
```

## After Testing

**If all tests pass:**
- ✅ M00 migration successful!
- Ready to migrate mod1 → M01

**If issues found:**
- Document the issue
- Check logs: `tail -100 /tmp/yamy_m00_test.log`
- Rollback to old config if needed
- Debug and fix

---

**Current Status:** YAMY running, ready for UAT
**Log file:** `/tmp/yamy_m00_test.log`
**Config:** `keymaps/master_m00.mayu`
