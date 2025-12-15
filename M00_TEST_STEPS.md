# M00 UAT - Step-by-Step Test Instructions

## Current Status

```
✅ YAMY Daemon:  Running
✅ Config:       master_m00.mayu (LOADED)
✅ YAMY GUI:     Running & Connected
✅ Engine:       Enabled
```

**GUI should now show:** "Active config: master_m00.mayu"

---

## Test 1: Verify Config Loaded

### Step 1.1: Check GUI
- Look at YAMY GUI window
- **Expected:** Shows "Config: master_m00.mayu" (not "(none)")
- **Status indicator:** Green (connected and running)

### Step 1.2: Check via CLI
```bash
./build/bin/yamy-ctl status
```
**Expected output:**
```
Engine: Running | Config: master_m00.mayu | Uptime: XXs | Keys: 0 | Keymap: Global
```

✅ If you see "master_m00.mayu" → Config loaded successfully!

---

## Test 2: TAP Action (B → Enter)

### What this tests:
- M00 tap action (quick press < 200ms)
- B key mapped to M00 virtual modifier
- TAP M00 → Enter

### Steps:

1. **Open a text editor** (any editor, even terminal with `cat`)

2. **Quick tap B key**
   - Press and release quickly (< 200ms)
   - Don't hold it

3. **Expected Result:**
   - ✅ New line created (Enter/Return)
   - ✅ Cursor moves to next line

4. **Watch logs** (optional):
   ```bash
   tail -f /tmp/yamy_daemon.log | grep -E "(M00|tap|Enter)"
   ```

### ✅ Test 2 PASSES if:
- Tapping B creates a new line

### ❌ Test 2 FAILS if:
- B outputs "B" character
- B outputs "Enter" from old config
- Nothing happens

---

## Test 3: HOLD Action - Numbers (B + A → 1)

### What this tests:
- M00 hold action (≥ 200ms)
- B+A key combination
- M00-*A → _1 (outputs "1")

### Steps:

1. **Open a text editor**

2. **Hold B key** (at least 200ms, count "one-Mississippi")

3. **While holding B, press A**

4. **Release both keys**

5. **Expected Result:**
   - ✅ Outputs the number "1"
   - ✅ NOT the letter "a" or "A"

6. **Try other combinations** (while holding B):
   - B + O → Should output "2"
   - B + U → Should output "4"
   - B + I → Should output "5"
   - B + D → Should output "6"

### ✅ Test 3 PASSES if:
- Holding B and pressing A outputs "1"
- Other number combinations work

### ❌ Test 3 FAILS if:
- Outputs "a" or "A" instead of "1"
- No output
- Wrong number

---

## Test 4: HOLD Action - Navigation (B + Colon → Left Arrow)

### What this tests:
- M00 + navigation keys
- Cursor movement

### Steps:

1. **Open a text editor with some text**
   ```bash
   echo "Hello World" > /tmp/test.txt
   nano /tmp/test.txt
   ```

2. **Move cursor to end of line**

3. **Hold B key**

4. **While holding B, press Colon key** (which is Z after substitution)

5. **Expected Result:**
   - ✅ Cursor moves LEFT
   - ✅ Each press moves left one position

6. **Try other navigation** (while holding B):
   - B + Comma → Cursor moves RIGHT
   - B + Period → Cursor moves DOWN
   - B + P → Cursor moves UP

### ✅ Test 4 PASSES if:
- Cursor navigation works as expected

### ❌ Test 4 FAILS if:
- Cursor doesn't move
- Wrong direction
- Outputs characters instead of moving

---

## Test 5: HOLD Action - Function Keys (B + Q → F2)

### What this tests:
- M00 + function keys
- F-key remapping

### Steps:

1. **Hold B key**

2. **While holding B, press Q** (which is Minus after substitution)

3. **Expected Result:**
   - ✅ F2 function triggered
   - ✅ Depends on your environment (might open help, rename, etc.)

4. **Try other F-keys** (while holding B):
   - B + Semicolon → F1
   - B + Q → F2
   - B + J → F3
   - B + K → F4

### Note:
F-key effects depend on your application. Just verify the F-key is being pressed, not the original key.

---

## Test 6: Wildcard Modifiers (B + Shift + A)

### What this tests:
- M00 with standard modifiers (Shift)
- Wildcard pattern `*W-*A-*S-*C-`

### Steps:

1. **Open text editor**

2. **Hold B key**

3. **Hold Shift key**

4. **Press A**

5. **Expected Result:**
   - ✅ Outputs "!" (Shift+1, because B+A→1, and Shift+1→!)
   - ✅ OR outputs Shift+1 combination

### ✅ Test 6 PASSES if:
- Standard modifiers (Shift/Ctrl) are preserved
- Key combinations work with modifiers

---

## Test 7: TAP vs HOLD Timing

### What this tests:
- 200ms threshold between TAP and HOLD

### Test 7a: Fast Tap (< 200ms)

1. **Quick tap B** (very fast)
2. **Expected:** Enter/new line

### Test 7b: Slow Hold (≥ 200ms)

1. **Hold B** (count "one-Mississippi" = ~300ms)
2. **Press A while holding**
3. **Expected:** Outputs "1"

### ✅ Test 7 PASSES if:
- Fast tap triggers Enter
- Slow hold triggers modifier layer

---

## Test Results Checklist

| Test | Description | Result |
|------|-------------|--------|
| 1 | Config loaded (GUI shows master_m00.mayu) | ☐ PASS ☐ FAIL |
| 2 | TAP: B → Enter | ☐ PASS ☐ FAIL |
| 3 | HOLD: B+A → 1 | ☐ PASS ☐ FAIL |
| 4 | HOLD: B+Colon → Left arrow | ☐ PASS ☐ FAIL |
| 5 | HOLD: B+Q → F2 | ☐ PASS ☐ FAIL |
| 6 | B+Shift+A works | ☐ PASS ☐ FAIL |
| 7 | Timing: Fast tap vs slow hold | ☐ PASS ☐ FAIL |

---

## Watch Logs While Testing

**Terminal 1: Watch all events**
```bash
tail -f /tmp/yamy_daemon.log
```

**Terminal 2: Watch M00 events only**
```bash
tail -f /tmp/yamy_daemon.log | grep -E "(M00|Enter|tap|hold)"
```

**Terminal 3: Check status anytime**
```bash
./build/bin/yamy-ctl status
```

---

## If Tests Fail

### Restart Everything
```bash
# Stop
killall -9 yamy yamy-gui

# Start daemon
./build/bin/yamy > /tmp/yamy_daemon.log 2>&1 &
sleep 2

# Load config
./build/bin/yamy-ctl reload --config "$(pwd)/keymaps/master_m00.mayu"

# Start GUI
./build/bin/yamy-gui > /tmp/yamy_gui.log 2>&1 &
```

### Rollback to Old Config
```bash
./build/bin/yamy-ctl reload --config "$(pwd)/keymaps/config.mayu"
```

### Check Logs for Errors
```bash
tail -100 /tmp/yamy_daemon.log | grep -i error
```

---

## After Testing

### ✅ If All Tests Pass:
- M00 migration successful!
- Ready to proceed with mod1 → M01

### ⚠️ If Any Test Fails:
- Document which test failed
- Share the error/unexpected behavior
- Check logs for clues
- We'll debug together

---

## Quick Reference: M00 Mappings

```
TAP B → Enter

HOLD B + A → 1
HOLD B + O → 2
HOLD B + U → 4
HOLD B + I → 5
HOLD B + D → 6
HOLD B + H → 7
HOLD B + T → 8
HOLD B + N → 9

HOLD B + Colon → Left
HOLD B + Comma → Right
HOLD B + Period → Down
HOLD B + P → Up

HOLD B + Q → F2
HOLD B + Semicolon → F1
HOLD B + J → F3
HOLD B + K → F4
```

See `UAT_M00_TEST_GUIDE.md` for complete list of 45 mappings.

---

**Ready to test! Start with Test 1 and work through Test 7. Good luck! 🚀**
