# Handoff Document - YAMY M20-MFF Migration

**Date:** 2025-12-16 01:20
**From:** Claude Sonnet 4.5
**To:** Gemini
**Status:** Phase 1 Complete, Testing Issues

---

## ✅ What Was Accomplished

### Phase 1: Modifier Matching Order Fix (COMPLETED)

**Commit:** 40f4ac9 - "feat: Phase 1 - Fix modifier matching order (check BEFORE substitution)"

**Files Modified:**
- `src/core/engine/engine.h` (lines 360-364): Added helper function declarations
- `src/core/engine/engine_generator.cpp`:
  - Lines 287-316: Implemented `hasActiveVirtualModifiers()` and `buildPhysicalModifiedKey()`
  - Lines 333-358: Added early keymap check in `beginGeneratingKeyboardEvents()`

**Implementation:**
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
        return;  // Skip rest of processing
    }
}
```

**Config Fix:**
- Removed duplicate M20 mappings in `keymaps/master_m00.mayu`:
  - Line 178: ~~M20-Q = F2~~ (kept F1)
  - Line 183: ~~M20-M = F7~~ (kept F5)
  - Line 184: ~~M20-W = F8~~ (kept _1 from line 167)

---

## ❌ Current Problem

**Symptom:** User reports B+W outputs "A" instead of "1"

**Log Evidence:**
```
[GEN:EARLY] Virtual modifiers active, checking keymap with PHYSICAL key BEFORE substitution
[GEN:EARLY] MATCH FOUND with physical key! Executing action and skipping substitution
[GEN:EARLY] Action executed, returning early
```

**Logs show early check IS working**, but something is wrong with the output.

**Additional Strange Behavior:**
1. Config reloads happening repeatedly after each key press
2. Logs show: `[ModState::isActive] type=19, bit=0, m_modal[0]=0x00000000, active=0`
3. W key events show: `M00_active=0, virtualModsMatch=0` even when modifiers should be active

**Key Finding from Logs:**
```
[EVENT] Read from /dev/input/event24: scancode=0x11 DOWN  # W key
[EVENT] Callback returned BLOCK
[ModState::isActive] type=19, bit=0, m_modal[0]=0x00000000, active=0
  loading: /home/rmondo/repos/yamy/keymaps/master_m00.mayu  # Config reload!
  loading: /home/rmondo/repos/yamy/keymaps/109.mayu
  loading: /home/rmondo/repos/yamy/keymaps/key_seq.mayu
```

---

## 🔍 Root Cause Hypothesis

**Possible Issues:**

1. **Config Reload Loop:** The early keymap check might be triggering a config reload, which causes modifiers to be cleared
   - Evidence: Config reloads after every MATCH FOUND

2. **ModifierState Not Synced:** The early check uses `m_modifierState.getModifierBits()` but the actual keymap matching might use a different state
   - Evidence: Logs show `M00_active=0` when checking assignments

3. **Early Return Skips Modifier Activation:** The early return might prevent M20 from being activated in the first place
   - Evidence: B (M20 trigger) might not be setting the modifier bit

4. **Action Being Executed Incorrectly:** The matched action might not be the right one
   - Evidence: User sees "A" which suggests substitution IS happening

---

## 📁 Current System State

**Processes Running:**
- YAMY Daemon: PID 1137325
- YAMY GUI: PID 1137702
- Config: `/home/rmondo/repos/yamy/keymaps/master_m00.mayu`
- Engine: Running

**Log Files:**
- Daemon: `/tmp/yamy_daemon.log`
- GUI: `/tmp/yamy_gui.log`

**Recent Commits:**
```
40f4ac9 - Phase 1: Fix modifier matching order (check BEFORE substitution)
7901ef6 - WIP: M20-M29 migration with physical key mappings (pre-refactor checkpoint)
```

---

## 🎯 Next Steps (Pending)

### Immediate Investigation Needed:

1. **Debug Why Config Reloads After Match:**
   - Search for `switchConfiguration` calls in engine_generator.cpp
   - Check if `generateKeySeqEvents()` triggers a reload

2. **Debug Modifier Activation:**
   - Add logs to show when M20 is activated/deactivated
   - Check if B hold (>200ms) actually sets the modifier bit
   - Verify `m_modifierState.getModifierBits()` returns correct values

3. **Debug Early Return:**
   - The early return at line 354 might be preventing normal modifier processing
   - May need to only return early for NON-MODIFIER keys

4. **Check buildPhysicalModifiedKey():**
   - Lines 298-316 in engine_generator.cpp
   - Verify it's correctly copying modifier state
   - The `const_cast` on line 312 is suspicious

### Phase 2 (Not Started):
- Remove old m0-m19 implementation (12 files)
- See `REFACTOR_PLAN.md` for details

---

## 🔧 Key Files to Investigate

1. **src/core/engine/engine_generator.cpp**
   - Line 335-358: Early keymap check (NEW CODE)
   - Line 298-316: `buildPhysicalModifiedKey()` (NEW CODE)
   - Look for config reload triggers

2. **src/core/engine/modifier_key_handler.cpp**
   - TAP/HOLD detection logic
   - M20-M29 activation/deactivation

3. **keymaps/master_m00.mayu**
   - Line 167: `M20-*W = _1` (should output "1")
   - Lines 18-27: M20-M29 substitution definitions

4. **src/core/input/modifier_state.cpp**
   - `getModifierBits()` implementation
   - Virtual modifier state tracking

---

## 📝 Configuration Details

**M20-M29 Mappings (keymaps/master_m00.mayu):**
```mayu
# Virtual modifier substitutions
def subst *B = *M20          # B → M20 (mod0)
def subst *V = *M21          # V → M21 (mod1)
# ... through M29

# TAP actions
mod assign M20 = *Enter      # TAP: B → Enter
mod assign M21 = *BackSpace  # TAP: V → Backspace
# ... through M29

# Key mappings (using PHYSICAL keys)
key *W-*A-*S-*C-M20-*W = *W-*A-*S-*C-_1   # M20+W → "1"
key *W-*A-*S-*C-M20-*E = *W-*A-*S-*C-_2   # M20+E → "2"
key *W-*A-*S-*C-M20-*R = *W-*A-*S-*C-_3   # M20+R → "3"
key *W-*A-*S-*C-M20-*T = *W-*A-*S-*C-_4   # M20+T → "4"
```

---

## 🐛 Debug Commands

**Watch logs in real-time:**
```bash
tail -f /tmp/yamy_daemon.log | grep -E "(GEN:EARLY|M20|MATCH|scancode=0x30|scancode=0x11)"
```

**Check modifier state:**
```bash
tail -f /tmp/yamy_daemon.log | grep -E "(ModState|m_modal|virtualMods|active)"
```

**Restart system:**
```bash
killall -9 yamy yamy-gui
./build/bin/yamy > /tmp/yamy_daemon.log 2>&1 &
sleep 2
./build/bin/yamy-ctl reload --config /home/rmondo/repos/yamy/keymaps/master_m00.mayu
./build/bin/yamy-ctl start
./build/bin/yamy-gui > /tmp/yamy_gui.log 2>&1 &
```

---

## 📚 Reference Documents

- `REFACTOR_PLAN.md` - Complete 4-phase refactoring plan
- `PHASE1_MODIFIER_MATCHING_FIX.md` - Phase 1 implementation details
- `YAMY_RUNNING_STATUS.md` - Current system status
- `M20_MIGRATION_COMPLETE.md` - M20-M29 migration details

---

## 💡 Suggested Fix Direction

The early keymap check logic might need modification:

**Current Issue:** Early return happens for ALL keys when modifiers are active, including the modifier trigger key itself (B).

**Possible Fix:** Only do early check for NON-MODIFIER keys:
```cpp
if (hasActiveVirtualModifiers() && !i_isModifier) {
    // Early keymap check
    // ...
}
```

This would ensure:
1. B press/release still goes through normal flow (activates M20)
2. W press (when M20 active) uses early check (matches M20-W before substitution)

---

## ✅ What Works

- Build: No compilation errors
- M20-M29 parsing: All 10 modifiers registered
- Config loading: No syntax errors
- Early check detection: Logs show "Virtual modifiers active" correctly
- TAP actions: Not tested but likely working based on logs

## ❌ What Doesn't Work

- M20 + W → outputs "A" instead of "1"
- Config reloads happening unexpectedly
- Modifier state seems inconsistent between early check and actual keymap lookup

---

**Good luck, Gemini! 🚀**
