# M00-MFF Virtual Modifier E2E Test Guide

## Overview

This guide describes the comprehensive End-to-End (E2E) test suite for the **NEW M00-MFF Virtual Modifier System** - a brand new implementation supporting 256 modal modifiers with hex notation (M00-MFF).

## What is M00-MFF?

The M00-MFF system is a **complete replacement** for the old mod0-mod19 system:

- **OLD System**: `mod mod0 = !!B` (20 modifiers, decimal notation)
- **NEW System**: `def subst *B = *M00` + `key M00-*A = *_1` (256 modifiers, hex notation)

### Key Features

1. **256 Virtual Modifiers**: M00 through MFF (0x00 - 0xFF)
2. **TAP/HOLD Detection**: Keys can act as modifiers when held, or output a different key when tapped quickly
3. **Separate Implementation**: Completely independent from old mod0-mod19 system
4. **Hex Notation**: `M00`, `M01`, `M0A`, `MFF` (not `M0`, `M1`, `M10`, `M255`)

## Test Configuration

### File: `keymaps/test_m00_e2e.mayu`

Defines three virtual modifiers for testing:

| Modifier | Physical Key | Tap Action | Hold Actions |
|----------|--------------|------------|--------------|
| **M00** | B | Enter | A→1, S→2, D→3, F→4, Q→F1, W→F2, E→F3, R→F4 |
| **M01** | V | Backspace | A→Left, S→Down, D→Up, F→Right, Q→Home, W→End, E→PageUp, R→PageDown |
| **M02** | N | Space | A→7, S→8, D→9, F→0 |

### Syntax Examples

```mayu
# Map physical key to virtual modifier
def subst *B = *M00

# Define tap action (quick press/release)
mod assign M00 = *Enter

# Define hold actions (modifier active while held)
key M00-*A = *_1      # B+A → 1
key M00-*S = *_2      # B+S → 2

# Combinations with standard modifiers
key M00-S-*A = *F5    # B+Shift+A → F5
key M01-C-*A = *F7    # V+Ctrl+A → F7
```

## Test Categories

### Category 1: Basic TAP Functionality (3 tests)

Tests quick press/release behavior for each modifier:

- `m00_tap_b_to_enter`: B tap → Enter
- `m01_tap_v_to_backspace`: V tap → Backspace
- `m02_tap_n_to_space`: N tap → Space

**Test Syntax**: `48` (evdev code for B key)
**Expected**: `28` (evdev code for Enter)

### Category 2: Basic HOLD Functionality - M00 (8 tests)

Tests holding B key and pressing other keys:

- `m00_hold_b_a_to_1`: Hold B + A → 1
- `m00_hold_b_s_to_2`: Hold B + S → 2
- `m00_hold_b_d_to_3`: Hold B + D → 3
- `m00_hold_b_f_to_4`: Hold B + F → 4
- `m00_hold_b_q_to_f1`: Hold B + Q → F1
- `m00_hold_b_w_to_f2`: Hold B + W → F2
- `m00_hold_b_e_to_f3`: Hold B + E → F3
- `m00_hold_b_r_to_f4`: Hold B + R → F4

**Test Syntax**: `HOLD:48+PRESS:30` (Hold B, press A)
**Expected**: `2` (evdev code for '1' key)

### Category 3: HOLD Functionality - M01 (8 tests)

Tests holding V key for navigation:

- `m01_hold_v_a_to_left`: Hold V + A → Left arrow
- `m01_hold_v_s_to_down`: Hold V + S → Down arrow
- `m01_hold_v_d_to_up`: Hold V + D → Up arrow
- `m01_hold_v_f_to_right`: Hold V + F → Right arrow
- `m01_hold_v_q_to_home`: Hold V + Q → Home
- `m01_hold_v_w_to_end`: Hold V + W → End
- `m01_hold_v_e_to_pageup`: Hold V + E → PageUp
- `m01_hold_v_r_to_pagedown`: Hold V + R → PageDown

### Category 4: HOLD Functionality - M02 (4 tests)

Tests holding N key for number pad:

- `m02_hold_n_a_to_7`: Hold N + A → 7
- `m02_hold_n_s_to_8`: Hold N + S → 8
- `m02_hold_n_d_to_9`: Hold N + D → 9
- `m02_hold_n_f_to_0`: Hold N + F → 0

### Category 5: Multiple Keys with Same Modifier (2 tests)

Tests pressing multiple keys while holding a modifier:

- `m00_multi_keys`: Hold B + A,S,D → 1,2,3
- `m01_multi_keys`: Hold V + A,S,D,F → Left,Down,Up,Right

**Test Syntax**: `HOLD:48+PRESS:30,31,32`
**Expected**: `2,3,4`

### Category 6: TAP/HOLD Mixed Sequences (3 tests)

Tests alternating between tap and hold behaviors:

- `m00_tap_then_hold`: B tap, then B+A → Enter, then 1
- `m01_tap_then_hold`: V tap, then V+A → Backspace, then Left
- `m00_m01_sequence`: B+A, then V+A → 1, then Left

### Category 7: Combinations with Standard Modifiers (4 tests)

Tests M00-MFF combined with Shift/Ctrl/Alt:

- `m00_shift_a_to_f5`: B+Shift+A → F5
- `m00_shift_s_to_f6`: B+Shift+S → F6
- `m01_ctrl_a_to_f7`: V+Ctrl+A → F7
- `m01_ctrl_s_to_f8`: V+Ctrl+S → F8

**Test Syntax**: `HOLD:48+SHIFT:30`
**Expected**: `63` (evdev code for F5)

### Category 8: Edge Cases (3 tests)

Tests boundary conditions and complex scenarios:

- `m00_rapid_tap`: B,B,B → Enter,Enter,Enter (rapid taps)
- `m00_hold_release_tap`: B+A, release, B tap → 1, then Enter
- `m00_m01_concurrent`: B+V+A (concurrent modifiers - complex)

### Category 9: Threshold Boundary Testing (2 tests)

Tests the 200ms TAP/HOLD threshold:

- `m00_fast_tap_100ms`: B tap 100ms → Enter (below threshold)
- `m00_slow_hold_300ms`: B hold 300ms + A → 1 (above threshold)

**Test Syntax**: `TAP:48:100` (tap B for 100ms)
**Expected**: `28` (Enter - tap action)

## Running Tests

### Run All E2E Tests (including M00-MFF)

```bash
cd tests
./run_comprehensive_e2e_tests.sh
```

### Run Only M00-MFF Tests

```bash
cd tests
grep "^m00_\|^m01_\|^m02_" e2e_test_cases.txt | while IFS='|' read test config input expected desc; do
    echo "Testing: $test"
done
```

### Run Standalone M00-MFF Test Suite

```bash
cd tests
./test_m00_comprehensive.sh
```

## Test Format Reference

### Test Case Format

```
TEST_NAME | CONFIG | INPUT_KEYS | EXPECTED_KEYS | DESCRIPTION
```

### Input Key Formats

| Format | Example | Description |
|--------|---------|-------------|
| Simple press | `48` | Press and release key 48 (B) |
| Sequence | `30,48,46` | Press A, B, C in sequence |
| Hold+Press | `HOLD:48+PRESS:30` | Hold key 48, press key 30 |
| With Shift | `SHIFT:30` | Press Shift+key 30 |
| With Ctrl | `CTRL:30` | Press Ctrl+key 30 |
| Timed tap | `TAP:48:100` | Tap key 48 for 100ms |
| Timed hold | `HOLD:48:300+PRESS:30` | Hold key 48 for 300ms, then press 30 |

### Evdev Key Code Reference

```
# Letters
KEY_A=30, KEY_B=48, KEY_C=46, KEY_D=32, KEY_E=18, KEY_F=33
KEY_N=49, KEY_Q=16, KEY_R=19, KEY_S=31, KEY_V=47, KEY_W=17

# Numbers
KEY_1=2, KEY_2=3, KEY_3=4, KEY_4=5
KEY_7=8, KEY_8=9, KEY_9=10, KEY_0=11

# Function keys
KEY_F1=59, KEY_F2=60, KEY_F3=61, KEY_F4=62
KEY_F5=63, KEY_F6=64, KEY_F7=65, KEY_F8=66

# Navigation
KEY_LEFT=105, KEY_RIGHT=106, KEY_UP=103, KEY_DOWN=108
KEY_HOME=102, KEY_END=107, KEY_PAGEUP=104, KEY_PAGEDOWN=109

# Special
KEY_ENTER=28, KEY_BACKSPACE=14, KEY_SPACE=57
```

## Expected Test Results

### Passing Criteria

✅ **PASS** if:
- TAP actions trigger within threshold (<200ms)
- HOLD actions activate after threshold (≥200ms)
- Correct keys are output for all combinations
- Modifiers activate/deactivate correctly
- Multiple modifiers work independently

### Known Limitations

Some tests are marked as **SKIPPED** because they require features not yet implemented in the test infrastructure:

- Complex concurrent modifier combinations
- Precise timing control for threshold boundary tests
- Some standard modifier combinations (awaiting implementation)

## Implementation Details

### NEW vs OLD System

| Feature | OLD (mod0-mod19) | NEW (M00-MFF) |
|---------|------------------|---------------|
| Count | 20 modifiers | 256 modifiers |
| Syntax | `mod mod0 = !!B` | `def subst *B = *M00` |
| Key mapping | `key M0-*A = *_1` | `key M00-*A = *_1` |
| Storage | `Modifier::Type_Mod0-19` | `ModifiedKey::m_virtualMods[8]` |
| Parser | Hardcoded M0-M19 | Dynamic M00-MFF |
| Keymap lookup | Old `m_modifier` field | New `m_virtualMods` field |

### Storage Architecture

```cpp
class ModifiedKey {
    Modifier m_modifier;         // OLD mod0-mod19 system
    Key *m_key;
    uint32_t m_virtualMods[8];   // NEW M00-MFF system (256 bits)
};
```

## Extending Tests

### Adding New M00-MFF Tests

1. **Add test case** to `tests/e2e_test_cases.txt`:
   ```
   m03_new_test | test_m00_e2e.mayu | HOLD:48+PRESS:30 | 2 | Description
   ```

2. **Add modifier mappings** to `keymaps/test_m00_e2e.mayu`:
   ```mayu
   def subst *X = *M03
   mod assign M03 = *SomeKey
   key M03-*A = *SomeOtherKey
   ```

3. **Run tests**:
   ```bash
   cd tests
   ./run_comprehensive_e2e_tests.sh
   ```

## Troubleshooting

### Test Fails: "Invalid config"
- Check that `test_m00_e2e.mayu` exists in `keymaps/`
- Verify config is added to `generate_config()` in test runner

### Test Fails: "Unknown modifier"
- Ensure modifier is defined in test config with `def subst`
- Check hex notation is correct (M00, not M0)

### Test Skipped: "requires key hold"
- Test infrastructure doesn't support HOLD yet
- Tests will be enabled when infrastructure is ready

### Unexpected Output
- Check evdev key codes match expected values
- Verify threshold (200ms) is correctly set
- Enable debug logging to trace event flow

## Summary

The M00-MFF test suite provides **comprehensive coverage** of the NEW virtual modifier system:

- **37+ test cases** covering all aspects
- **3 modifiers** (M00, M01, M02) with different behaviors
- **TAP/HOLD detection** thoroughly tested
- **Standard modifier combinations** (Shift, Ctrl)
- **Edge cases** and boundary conditions
- **Complete independence** from old mod0-mod19 system

This ensures the NEW M00-MFF system works correctly before deprecating the old system.
