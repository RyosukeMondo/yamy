# YAMY Running Status

## Current Status: ✅ RUNNING (M20-M29 System)

**Date:** 2025-12-16 00:14
**Config:** `keymaps/master_m00.mayu` (M20-M29 migration with PHYSICAL key mappings)

## Processes

| Component | PID | Status | Log File |
|-----------|-----|--------|----------|
| **YAMY Daemon** | 1082908 | ✅ Running | `/tmp/yamy_daemon.log` |
| **YAMY GUI** | 1083332 | ✅ Running | `/tmp/yamy_gui.log` |

## Registered Virtual Modifiers (M20-M29)

All 10 modifiers successfully registered:

| Modifier | Physical Key | TAP Action |
|----------|--------------|------------|
| M20 | B (0x0030) | Enter |
| M21 | V (0x002F) | Backspace |
| M22 | M (0x0032) | Backspace |
| M23 | X (0x002D) | Comma |
| M24 | _1 (0x0002) | LShift |
| M25 | LCtrl (0x001D) | Space |
| M26 | C (0x002E) | Delete |
| M27 | Tab (0x000F) | Space |
| M28 | Q (0x0010) | Minus |
| M29 | A (0x001E) | Tab |

## Key Changes from Old System

### 1. Renamed M00-M09 → M20-M29
- Avoids collision with old m0-m9 notation
- Clear separation from legacy system

### 2. PHYSICAL Key Mappings (CRITICAL FIX!)

**OLD (ambiguous):**
```mayu
def subst *W = *A         # W substituted to A
key M00-*A = *_1          # Does this mean physical A or logical A (from W)?
```

**NEW (unambiguous):**
```mayu
def subst *W = *A         # W substituted to A
key M20-*W = *_1          # Clearly means physical W key!
```

### 3. Physical → Logical Mapping Reference

Based on your Dvorak-like layout:

| Physical | Logical | M20 Mapping |
|----------|---------|-------------|
| W | A | M20-*W → _1 |
| E | O | M20-*E → _2 |
| R | E | M20-*R → _3 |
| T | U | M20-*T → _4 |
| Y | I | M20-*Y → _5 |
| U | D | M20-*U → _6 |
| I | H | M20-*I → _7 |
| O | T | M20-*O → _8 |
| P | N | M20-*P → _9 |
| D | S | M20-*D → _0 |

## Testing Instructions

### Test 1: TAP Action
- Quick tap **B** → Should output **Enter** ✅

### Test 2: HOLD Action with PHYSICAL Keys
**IMPORTANT:** Use PHYSICAL keys (W, E, R, T), NOT logical keys (A, O, E, U)!

- Hold **B** + press **W** → Should output **"1"** ✅
- Hold **B** + press **E** → Should output **"2"** ✅
- Hold **B** + press **R** → Should output **"3"** ✅
- Hold **B** + press **T** → Should output **"4"** ✅

### Test 3: Navigation
- B + L (physical) → Cursor LEFT
- B + Comma (physical) → Cursor RIGHT
- B + X (physical) → Cursor DOWN
- B + J (physical) → Cursor UP

## Watch Logs

### Terminal 1: Watch All Events
```bash
tail -f /tmp/yamy_daemon.log
```

### Terminal 2: Watch M20 Events Only
```bash
tail -f /tmp/yamy_daemon.log | grep -E "(M20|M2[0-9]|searchAssignment|KEYMAP)"
```

### Terminal 3: Check Status Anytime
```bash
./build/bin/yamy-ctl status
```

## Control Commands

### View Process Status
```bash
ps aux | grep -E "(yamy|yamy-gui)" | grep -v grep
```

### Stop Everything
```bash
killall -9 yamy yamy-gui
```

### Restart Daemon Only
```bash
killall -9 yamy
./build/bin/yamy > /tmp/yamy_daemon.log 2>&1 &
sleep 2
./build/bin/yamy-ctl reload --config "$(pwd)/keymaps/master_m00.mayu"
```

### Restart GUI Only
```bash
killall -9 yamy-gui
./build/bin/yamy-gui > /tmp/yamy_gui.log 2>&1 &
```

### Restart Both
```bash
killall -9 yamy yamy-gui
./build/bin/yamy > /tmp/yamy_daemon.log 2>&1 &
sleep 2
./build/bin/yamy-ctl reload --config "$(pwd)/keymaps/master_m00.mayu"
./build/bin/yamy-gui > /tmp/yamy_gui.log 2>&1 &
```

## What's Loaded

**Current Configuration:** `keymaps/master_m00.mayu`
- Base: `109.mayu` (keyboard layout)
- Key sequences: `key_seq.mayu`
- Main config: M20-M29 migration
  - ✅ 10 virtual modifier substitutions (M20-M29)
  - ✅ 10 mod assign statements (TAP actions)
  - ✅ All key mappings use PHYSICAL keys
  - ✅ Wildcard modifier patterns (`*W-*A-*S-*C-`)

**Active Features:**
- M20-M29 virtual modifiers (was: mod0-mod9)
- TAP/HOLD detection (200ms threshold)
- PHYSICAL key mapping (unambiguous!)
- Mouse event handling
- IPC communication (daemon ↔ GUI)

## Migration Status

| Old Modifier | New Modifier | Status |
|--------------|--------------|--------|
| mod0 | M20 | ✅ Complete |
| mod1 | M21 | ✅ Complete |
| mod2 | M22 | ✅ Complete |
| mod3 | M23 | ✅ Complete |
| mod4 | M24 | ✅ Complete |
| mod5 | M25 | ✅ Complete |
| mod6 | M26 | ✅ Complete |
| mod7 | M27 | ✅ Complete |
| mod8 | M28 | ✅ Complete |
| mod9 | M29 | ✅ Complete |

## Documentation

- `M20_MIGRATION_COMPLETE.md` - Complete migration details
- `M00_HOLD_FIX_SUMMARY.md` - Original M00 issue and fix
- `M00_ROOT_CAUSE_ANALYSIS.md` - Technical deep dive
- `M00_TEST_STEPS.md` - Old test guide (outdated, use this file instead)

---

**Ready for UAT Testing! 🚀**

**Next Steps:**
1. Test M20 functionality with PHYSICAL keys (W, E, R, T)
2. Verify all TAP and HOLD actions work correctly
3. If successful, consider removing old m0-m19 code from engine (cleanup)

**Key Insight:** The ambiguity between physical and logical keys is now resolved by using PHYSICAL key names in all M20-M29 mappings!
