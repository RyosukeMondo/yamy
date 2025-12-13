# Observed YAMY Mappings with config.mayu

## Test Results from E2E Suite

Based on actual E2E test execution with config.mayu enabled:

### Observed Substitutions

| Input Key | Input Code | config.mayu Says | Expected | Actual Output | Actual Code | Notes |
|-----------|------------|------------------|----------|---------------|-------------|-------|
| A | 30 | `def subst *A = *Tab` | Tab (15) | KEY_LEFTBRACE | 26 | ❌ Unexpected |
| B | 48 | `def subst *B = *Enter` | Enter (28) | KEY_TAB | 15 | ❌ Unexpected |
| R | 19 | `def subst *R = *E` | E (18) | KEY_R (passthrough) | 19 | ❌ Not remapped |
| Q | 16 | `def subst *Q = *Minus` | Minus (12) | KEY_Q (passthrough) | 16 | ❌ Not remapped |
| Tab | 15 | `def subst *Tab = *Space` | Space (57) | KEY_109 | 109 | ❌ Unexpected |
| W | 17 | `def subst *W = *A` | A (30) | KEY_TAB | 15 | ❌ Unexpected |

### Pattern Analysis

**Observation 1**: Some keys pass through unchanged (R, Q)
**Observation 2**: Some keys are remapped but to unexpected codes (A→26, B→15, W→15)
**Observation 3**: The mappings don't match the config.mayu declarations directly

### Hypotheses

**Hypothesis 1: Scan Code vs Evdev Mismatch**
- config.mayu uses YAMY virtual key names (*A, *Tab, etc.)
- These map to internal scan codes
- Scan codes are converted to evdev via yamyToEvdevKeyCode()
- The conversion might be incorrect for Japanese layout

**Hypothesis 2: Multiple Remapping Layers**
- Input → Scan code conversion
- Scan code → def subst remapping
- Remapped scan code → evdev output
- Each layer could introduce unexpected transformations

**Hypothesis 3: Japanese Layout Confusion**
- config.mayu might be written for US layout
- YAMY is detecting JP layout
- Scan code mappings differ between US and JP
- This causes mismatches in the substitution chain

### Next Steps

1. **Verify Layout Detection**
   - Check what layout YAMY detects
   - Confirm keymaps are for correct layout

2. **Trace the Conversion Chain**
   - Add debug logging to yamyToEvdevKeyCode()
   - Log: Input evdev → YAMY scan code → After subst → Output evdev
   - Identify where the mismatch occurs

3. **Test with US Layout**
   - Set keyboard layout to US
   - Re-run E2E tests
   - See if mappings match expectations

4. **Simplify Test Cases**
   - Start with keys that DO work (passthrough)
   - Identify which substitutions work correctly
   - Build test suite from known-good cases

## Working Test Case

✅ **Baseline (109.mayu only)**: abc → abc (PASS)
- Input: 30,48,46 (A,B,C)
- Output: 30,48,46 (A,B,C)
- This confirms the E2E framework works correctly!

## Recommendation

**SHORT TERM**: Create E2E tests for OBSERVED behavior, not EXPECTED behavior
- Document "A → KEY_LEFTBRACE" as a test case
- Mark as "needs investigation" but verify it's consistent
- This gives us regression testing even if mapping is unexpected

**LONG TERM**: Fix the scan code mapping
- Debug yamyToEvdevKeyCode()
- Verify config.mayu works on US layout
- Create JP-specific version if needed
