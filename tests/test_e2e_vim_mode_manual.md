# Manual E2E Test Procedure: vim-mode.json Virtual Modifier Verification

## Purpose

Verify M00-MFF virtual modifier system end-to-end functionality using the real vim-mode.json configuration with CapsLock as the modifier trigger.

## Prerequisites

1. **Build yamy daemon** (if not already built):
   ```bash
   cd /home/rmondo/repos/yamy
   cmake --build build --target yamy
   ```

2. **Configuration file**: `keymaps/vim-mode.json`

3. **Test editor**: Any text editor (gedit, kate, vim, VSCode, etc.)

4. **Test document**: Create a test file with sample text for navigation testing

## Test Environment Setup

### 1. Stop any running yamy instance
```bash
pkill -9 yamy 2>/dev/null || true
```

### 2. Start yamy daemon with vim-mode.json
```bash
./build/bin/yamy --config keymaps/vim-mode.json --daemon
```

### 3. Verify daemon is running
```bash
pgrep -a yamy
# Expected output: Process ID and path to yamy binary
```

### 4. Open test editor
Create a test file with the following content:
```
Line 1: The quick brown fox jumps over the lazy dog.
Line 2: ABCDEFGHIJKLMNOPQRSTUVWXYZ
Line 3: 1234567890
Line 4: Special characters: !@#$%^&*()
Line 5: End of test document.
```

---

## Test Cases

### Test Category 1: TAP Behavior (CapsLock → Escape)

#### Test 1.1: Quick Tap Outputs Escape
**Objective**: Verify that tapping CapsLock quickly (<200ms) outputs Escape

**Procedure**:
1. Place cursor at the end of Line 1
2. Quickly tap CapsLock (press and release within ~100ms)
3. Observe behavior

**Expected Result**:
- Escape key is triggered
- In vim/vi mode editors: Enter command mode
- In other editors: May trigger cancel/close behavior (depending on application)

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 1.2: Multiple Rapid Taps
**Objective**: Verify repeated quick taps all output Escape

**Procedure**:
1. Tap CapsLock 3 times rapidly (each tap <200ms)
2. Observe behavior

**Expected Result**:
- Each tap triggers one Escape event
- No "stuck" modifier state

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

### Test Category 2: HOLD Behavior - Basic Navigation (h/j/k/l)

#### Test 2.1: Hold CapsLock + H → Left Arrow
**Objective**: Verify vim-style left movement

**Procedure**:
1. Place cursor in middle of Line 2
2. Hold CapsLock (>200ms)
3. While holding CapsLock, press H
4. Release H, then release CapsLock

**Expected Result**:
- Cursor moves one position left
- Equivalent to pressing Left arrow key

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 2.2: Hold CapsLock + J → Down Arrow
**Objective**: Verify vim-style down movement

**Procedure**:
1. Place cursor on Line 2
2. Hold CapsLock + press J
3. Release J, then release CapsLock

**Expected Result**:
- Cursor moves down one line to Line 3

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 2.3: Hold CapsLock + K → Up Arrow
**Objective**: Verify vim-style up movement

**Procedure**:
1. Place cursor on Line 3
2. Hold CapsLock + press K
3. Release K, then release CapsLock

**Expected Result**:
- Cursor moves up one line to Line 2

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 2.4: Hold CapsLock + L → Right Arrow
**Objective**: Verify vim-style right movement

**Procedure**:
1. Place cursor in middle of Line 2
2. Hold CapsLock + press L
3. Release L, then release CapsLock

**Expected Result**:
- Cursor moves one position right

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

### Test Category 3: HOLD Behavior - Word Movement

#### Test 3.1: Hold CapsLock + W → Ctrl-Right (Word Forward)
**Objective**: Verify vim 'w' command (next word)

**Procedure**:
1. Place cursor at start of "quick" in Line 1
2. Hold CapsLock + press W
3. Release W, then release CapsLock

**Expected Result**:
- Cursor jumps to start of next word ("brown")
- Equivalent to Ctrl-Right arrow

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 3.2: Hold CapsLock + B → Ctrl-Left (Word Backward)
**Objective**: Verify vim 'b' command (previous word)

**Procedure**:
1. Place cursor at "fox" in Line 1
2. Hold CapsLock + press B
3. Release B, then release CapsLock

**Expected Result**:
- Cursor jumps to start of previous word ("brown")

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

### Test Category 4: HOLD Behavior - Line Movement

#### Test 4.1: Hold CapsLock + 0 → Home (Line Start)
**Objective**: Verify vim '0' command

**Procedure**:
1. Place cursor in middle of Line 1
2. Hold CapsLock + press 0 (number key)
3. Release 0, then release CapsLock

**Expected Result**:
- Cursor jumps to beginning of current line

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 4.2: Hold CapsLock + 4 → End (Line End)
**Objective**: Verify vim '$' command (Shift-4)

**Procedure**:
1. Place cursor at start of Line 1
2. Hold CapsLock + press 4 (number key - represents '$')
3. Release 4, then release CapsLock

**Expected Result**:
- Cursor jumps to end of current line

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

### Test Category 5: Combined Modifiers (CapsLock + Shift + Key)

#### Test 5.1: Hold CapsLock + Shift + G → Ctrl-End (Document End)
**Objective**: Verify vim 'G' command (Shift-G) works with virtual modifier

**Procedure**:
1. Place cursor on Line 1
2. Hold CapsLock
3. While holding CapsLock, hold Shift
4. While holding both, press G
5. Release all keys

**Expected Result**:
- Cursor jumps to end of document (Line 5)
- Equivalent to Ctrl-End

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 5.2: Hold CapsLock + Shift + X → Backspace
**Objective**: Verify vim 'X' command (delete before cursor)

**Procedure**:
1. Place cursor in middle of Line 2, after 'D'
2. Hold CapsLock + Shift + X
3. Release all keys

**Expected Result**:
- Character before cursor ('D') is deleted
- Equivalent to Backspace key

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

### Test Category 6: Timing Threshold Verification (200ms)

#### Test 6.1: Tap at 190ms Boundary (Below Threshold)
**Objective**: Verify taps slightly below 200ms still trigger tap action

**Procedure**:
1. Press CapsLock
2. Count "one-mississip" slowly (approximately 190ms)
3. Release CapsLock before completing the count

**Expected Result**:
- Escape is triggered (tap action)
- Modifier is NOT activated

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 6.2: Hold at 250ms (Above Threshold)
**Objective**: Verify holds above 200ms activate modifier

**Procedure**:
1. Press CapsLock
2. Count "one-mississippi" fully (approximately 250ms)
3. While still holding CapsLock, press H
4. Release H, then CapsLock

**Expected Result**:
- Left arrow is triggered (hold action)
- Escape is NOT triggered

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 6.3: Rapid Tap-then-Hold Sequence
**Objective**: Verify state machine handles quick transitions

**Procedure**:
1. Quickly tap CapsLock (should output Escape)
2. Wait 500ms
3. Hold CapsLock + press H (should output Left)
4. Release all keys

**Expected Result**:
- First action: Escape triggered
- Second action: Left arrow triggered
- No interference between actions

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

### Test Category 7: Additional Vim Commands

#### Test 7.1: Hold CapsLock + X → Delete (Forward Delete)
**Objective**: Verify vim 'x' command

**Procedure**:
1. Place cursor on 'A' in Line 2
2. Hold CapsLock + press X
3. Release all keys

**Expected Result**:
- Character under cursor ('A') is deleted

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

#### Test 7.2: Hold CapsLock + U → Ctrl-Z (Undo)
**Objective**: Verify vim 'u' command

**Procedure**:
1. Make a text change (e.g., type some text)
2. Hold CapsLock + press U
3. Release all keys

**Expected Result**:
- Last change is undone (Ctrl-Z triggered)

**Actual Result**: [ ] Pass [ ] Fail
**Notes**: _________________________________

---

## Test Results Summary

### Overall Results

| Category | Total | Passed | Failed | Pass Rate |
|----------|-------|--------|--------|-----------|
| Tap Behavior | 2 | ___ | ___ | ___% |
| Basic Navigation (hjkl) | 4 | ___ | ___ | ___% |
| Word Movement | 2 | ___ | ___ | ___% |
| Line Movement | 2 | ___ | ___ | ___% |
| Combined Modifiers | 2 | ___ | ___ | ___% |
| Timing Threshold | 3 | ___ | ___ | ___% |
| Additional Commands | 2 | ___ | ___ | ___% |
| **TOTAL** | **17** | **___** | **___** | **___%** |

### Critical Issues Found

1. _____________________________________________
2. _____________________________________________
3. _____________________________________________

### Minor Issues Found

1. _____________________________________________
2. _____________________________________________

### Success Criteria

- [ ] All tap behaviors work correctly (CapsLock → Escape)
- [ ] All hold behaviors work correctly (CapsLock + Key → Action)
- [ ] Combined modifiers work (CapsLock + Shift + Key)
- [ ] 200ms threshold timing is accurate
- [ ] No "stuck" modifier states
- [ ] Pass rate ≥ 85% (15 of 17 tests)

---

## Cleanup

```bash
# Stop yamy daemon
pkill yamy

# Verify stopped
pgrep yamy
# Expected: No output
```

---

## Notes

- **Tester Name**: _________________________________
- **Date**: _________________________________
- **vim-mode.json Version**: _________________________________
- **yamy Build**: _________________________________
- **Platform**: Linux x86_64
- **Desktop Environment**: _________________________________

## Additional Observations

_____________________________________________
_____________________________________________
_____________________________________________
