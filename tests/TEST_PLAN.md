# YAMY E2E Test Plan - MECE Feature Breakdown

## Configuration Strategy

### Two-Phase Approach

**Phase A: Simplified Configs (Baseline)**
- Test `config_clean.mayu` / `master_clean.mayu`
- Simplified for testing, guaranteed to work
- Establish baseline functionality
- **Goal**: Verify core engine works correctly

**Phase B: Original Configs (Ultimate Goal)**
- Test `config.mayu` / `master.mayu` / `dvorakY.mayu` / `hm.mayu`
- Original complex configurations
- More features, edge cases, and combinations
- **Goal**: Make original daily-use configs fully functional

---

## Configuration Analysis

### Simplified Configs (Phase A - Testing Baseline)

Based on `config_clean.mayu` (simplified for testing):

## Feature Categories (MECE)

### Level 1: Basic Features (Atomic)

#### 1.1 Simple Key Substitutions (No Modifiers)
Single input → single output, no modifier state required

**Priority: P0 (Critical)**
- Letter keys (A→Tab, D→Q, etc.)
- Number keys (0→R, 3→Comma, etc.)
- Special keys (Tab→Space, Enter→Yen, Esc→5)
- JP-specific keys (@→S, ;→V, etc.)
- Function keys (F1→LWin, F2→Esc, etc.)
- Navigation keys (,→F9, .→F10, etc.)

#### 1.2 Passthrough Keys
Input → same output (no transformation)

**Priority: P1 (High)**
- Letter passthroughs (Z→Z)
- Special passthroughs (Space→Space)
- Navigation passthroughs (Up→Up, Left→Left, etc.)
- Lock keys (NumLock→NumLock, etc.)

### Level 2: Modifier Features

#### 2.1 Hardware Modifiers (Standard)
Built-in keyboard modifiers

**Priority: P0 (Critical)**
- Shift keys (LShift, RShift)
- Ctrl keys (LCtrl→Space, LAlt→LCtrl reassignment)
- Alt keys (via F4, Hiragana reassignment)
- Win keys (via F1, LWin reassignment)

#### 2.2 Modal Modifiers (Custom - M0-M9)
User-defined layer switching

**Priority: P0 (Critical - daily use)**
- **M0 (mod0 = !!B)** - Primary layer
  - M0-A → 1 (Hold B, press W→A)
  - M0-T → 2 (Hold B, press O→T)
  - M0-O → X (Hold B, press E→O)
  - M0-D → Delete (Hold B, press U→D)
  - M0-Z → Y (Hold B, press Z)

- **M1 (mod1 = !!V)** - Secondary layer
- **M2 (mod2 = !!X)** - Tertiary layer
- **M3 (mod3 = !!C)** - Additional layer
- **M4 (mod4 = !!_1)** - Number-based layer
  - M4-U → 3 (Hold 1, press U)
- **M5 (mod5 = !!N)** - Additional layer
- **M6 (mod6 = !!M)** - Additional layer
- **M8 (mod8 = !!R)** - Additional layer
- **M9 (mod9 = !!A)** - DISABLED (conflicts)
  - M9-E → 2 (Hold A, press E) - CONFLICTING

### Level 3: Combined Features

#### 3.1 Chained Substitutions
Key A → Key B, Key B → Key C (transitive mappings)

**Priority: P1 (High)**
- Example: Physical X → Virtual 3, Virtual 3 → Comma
- Track full substitution chain

#### 3.2 Modal + Substitution
Modal modifier on substituted keys

**Priority: P0 (Critical - most complex daily use)**
- M0 operates on POST-substitution keys
- Physical W → Virtual A, then M0-A works
- This is the PRIMARY use case!

#### 3.3 Hardware Modifier + Substitution
Standard modifier + remapped key

**Priority: P1 (High)**
- Shift + remapped keys
- Ctrl + remapped keys
- Alt + remapped keys

#### 3.4 Multi-Modal Combinations
Multiple modal modifiers active

**Priority: P2 (Medium)**
- M0 + M1 combinations
- Nested modal states

### Level 4: Edge Cases

#### 4.1 Conflicting Definitions
**Priority: P2 (Medium)**
- B → Enter (DISABLED due to mod0 = !!B)
- _1 → LShift (DISABLED for M0 testing)
- _2 → Colon (DISABLED for M0 testing)
- mod9 = !!A (DISABLED - conflicts with M0-A)
- mod7 = !!T (DISABLED - conflicts with M0-T)

#### 4.2 Special Cases
**Priority: P2 (Medium)**
- Rapid key repeat
- Simultaneous key press
- Modal activation then release
- Key held during modal switch

---

## Test Priority Matrix

```
┌─────────────────────────────────────────────────────────────┐
│ P0 - Critical (Must Test First)                             │
├─────────────────────────────────────────────────────────────┤
│ 1. Simple letter substitutions (20+ keys)                   │
│ 2. M0 modal layer (5 bindings - DAILY USE)                  │
│ 3. M0 + Substitution combinations (THE KEY FEATURE)         │
│ 4. Hardware modifiers (Shift, Ctrl, Alt, Win reassignments) │
├─────────────────────────────────────────────────────────────┤
│ P1 - High (Test After P0)                                   │
├─────────────────────────────────────────────────────────────┤
│ 1. Passthrough keys                                          │
│ 2. M1-M8 modal layers (less frequently used)                │
│ 3. Hardware modifier + substitution combos                   │
│ 4. Chained substitutions                                     │
├─────────────────────────────────────────────────────────────┤
│ P2 - Medium (Test for completeness)                         │
├─────────────────────────────────────────────────────────────┤
│ 1. Edge cases (rapid repeat, simultaneous press)            │
│ 2. Multi-modal combinations                                  │
│ 3. Disabled/conflicting definitions (verify they're off)    │
└─────────────────────────────────────────────────────────────┘
```

---

## Test Scenarios to Create

### Phase 1: Atomic Tests (P0 - Simple Substitutions)

1. **Letter substitutions** (`letter_substitutions.json`)
   - Test each active letter mapping individually
   - A→Tab, D→Q, E→O, F→J, etc.
   - ~20 test cases

2. **Number substitutions** (`number_substitutions.json`)
   - Test each active number mapping
   - 0→R, 3→Comma, 5→P, etc.
   - ~7 test cases

3. **Special keys** (`special_keys.json`)
   - Tab→Space, Enter→Yen, Esc→5
   - ~5 test cases

4. **JP-specific keys** (`jp_keys.json`)
   - @→S, ;→V, :→Z, etc.
   - ~8 test cases

5. **Function keys** (`function_keys.json`)
   - F1→LWin, F2→Esc, etc.
   - ~12 test cases

6. **Passthrough keys** (`passthrough_keys.json`)
   - Z→Z, Space→Space, Arrow keys
   - ~10 test cases

### Phase 2: Modal Modifiers (P0 - Critical Daily Use)

7. **M0 basic activation** (`m0_basic.json`)
   - Verify M0 activates when B is held
   - Verify M0 deactivates when B is released
   - ~2 test cases

8. **M0 layer mappings** (`m0_layer.json`)
   - M0-A → 1 (Physical: Hold B, press W)
   - M0-T → 2 (Physical: Hold B, press O)
   - M0-O → X (Physical: Hold B, press E)
   - M0-D → Delete (Physical: Hold B, press U)
   - M0-Z → Y (Physical: Hold B, press Z)
   - ~5 test cases

9. **M4 layer mappings** (`m4_layer.json`)
   - M4-U → 3 (Physical: Hold 1, press U)
   - ~1 test case

10. **Other modal layers** (`m1_m8_layers.json`)
    - M1 (!!V), M2 (!!X), M3 (!!C), M5 (!!N), M6 (!!M), M8 (!!R)
    - Test activation/deactivation
    - ~6 test cases

### Phase 3: Combined Features (P0 - Complex Daily Use)

11. **Modal + Substitution** (`modal_substitution_combo.json`)
    - THE CRITICAL TEST: M0 works on substituted keys
    - Physical W → Virtual A, M0-A should work
    - Physical O → Virtual T, M0-T should work
    - Verify substitution happens BEFORE modal check
    - ~5 test cases

12. **Hardware modifier reassignments** (`hardware_modifier_remap.json`)
    - LCtrl → Space
    - LAlt → LCtrl
    - F4 → Alt
    - F3 → Ctrl
    - F2 → Shift
    - F1 → Win
    - ~6 test cases

### Phase 4: Advanced Combinations (P1)

13. **Chained substitutions** (`chained_substitutions.json`)
    - Physical X → Virtual 3, Virtual 3 → Comma
    - Test full chain
    - ~3 test cases

14. **Hardware + Substitution** (`hardware_mod_substitution.json`)
    - Shift + (A→Tab)
    - Ctrl + (D→Q)
    - ~4 test cases

15. **Multi-modal** (`multi_modal.json`)
    - M0 + M1 combinations (if supported)
    - ~2 test cases

### Phase 5: Edge Cases (P2)

16. **Rapid input** (`rapid_input.json`)
    - Fast key sequences
    - Key repeat
    - ~2 test cases

17. **Simultaneous press** (`simultaneous_press.json`)
    - Multiple keys pressed at once
    - ~2 test cases

18. **Disabled features verification** (`disabled_features.json`)
    - Verify B→Enter is OFF (should do mod0 instead)
    - Verify _1→LShift is OFF
    - Verify _2→Colon is OFF
    - ~3 test cases

---

## Test Suite Organization

```
tests/
├── scenarios/
│   ├── phase1_atomic/
│   │   ├── letter_substitutions.json          (P0)
│   │   ├── number_substitutions.json          (P0)
│   │   ├── special_keys.json                  (P0)
│   │   ├── jp_keys.json                       (P0)
│   │   ├── function_keys.json                 (P0)
│   │   └── passthrough_keys.json              (P1)
│   ├── phase2_modal/
│   │   ├── m0_basic.json                      (P0)
│   │   ├── m0_layer.json                      (P0)
│   │   ├── m4_layer.json                      (P0)
│   │   └── m1_m8_layers.json                  (P1)
│   ├── phase3_combined/
│   │   ├── modal_substitution_combo.json      (P0) ← MOST IMPORTANT!
│   │   └── hardware_modifier_remap.json       (P0)
│   ├── phase4_advanced/
│   │   ├── chained_substitutions.json         (P1)
│   │   ├── hardware_mod_substitution.json     (P1)
│   │   └── multi_modal.json                   (P2)
│   └── phase5_edge_cases/
│       ├── rapid_input.json                   (P2)
│       ├── simultaneous_press.json            (P2)
│       └── disabled_features.json             (P2)
└── suites/
    ├── p0_critical.json                        ← Run first
    ├── p1_high.json                            ← Run after P0
    ├── p2_complete.json                        ← Run for full coverage
    └── daily_use.json                          ← Real-world workflow test
```

---

## Key Insights from Config Analysis

### 1. Modal Before or After Substitution?

**CRITICAL**: Config shows `mod mod0 = !!B` with comment:
> "Hold physical B keycap for mod0 (!! checks BEFORE substitution)"

**BUT**: Keymap bindings use POST-substitution names:
```
key M0-A = *_1    # Hold B, press W (→A) → 1
key M0-T = *_2    # Hold B, press O (→T) → 2
```

**CONCLUSION**:
- `!!` in modifier definition = CHECK PHYSICAL KEY
- Keymap bindings = MATCH AFTER SUBSTITUTION
- **This is the COMPLEX behavior we must test!**

### 2. Disabled Features (Must NOT work)

These are intentionally disabled due to conflicts:
```
#def subst *B = *Enter           # DISABLED - conflicts with mod0 = !!B
#def subst *_1 = *LShift          # DISABLED for M0 testing
#def subst *_2 = *Colon           # DISABLED for M0 testing
#mod mod9 = !!A                   # DISABLED - conflicts with M0-A
#mod mod7 = !!T                   # DISABLED - conflicts with M0-T
```

**Test**: Verify these do NOT activate

### 3. Most Complex Features (Daily Use)

The user's actual workflow involves:
1. **M0 layer** (Hold B) for numbers and navigation
2. **Dvorak-like substitutions** for all letters
3. **Combination**: M0 works on substituted keys

**Example**:
- Physical: Hold B, press W
- Processing: W → A (substitution), then M0-A → 1 (modal binding)
- Expected output: 1

This is THE CORE FEATURE to test thoroughly!

---

## Recommended Test Creation Order

1. ✅ **P0 Phase 1**: Letter substitutions (simple, foundational)
2. ✅ **P0 Phase 2**: M0 basic + M0 layer (daily use)
3. ✅ **P0 Phase 3**: Modal + Substitution combo (CRITICAL!)
4. ⬜ **P0 Phase 1**: Number/Special/JP/Function keys
5. ⬜ **P0 Phase 3**: Hardware modifier reassignments
6. ⬜ **P1 Phase 1**: Passthrough keys
7. ⬜ **P1 Phase 2**: M1-M8 layers
8. ⬜ **P1 Phase 4**: Advanced combinations
9. ⬜ **P2 Phase 5**: Edge cases

---

## Success Criteria

### P0 Tests Must Pass
- All basic letter substitutions work
- M0 layer activates/deactivates correctly
- **M0 + Substitution combo works** (THE KEY TEST!)
- Hardware modifier reassignments work

### P1 Tests Should Pass
- Passthrough keys work
- Other modal layers work
- Advanced combinations work

### P2 Tests May Fail
- Edge cases are for stress testing
- Acceptable to have some failures here

---

## Next Steps

1. Create P0 Phase 1 tests (letter_substitutions.json)
2. Create P0 Phase 2 tests (m0_basic.json, m0_layer.json)
3. Create P0 Phase 3 tests (modal_substitution_combo.json)
4. Create P0 test suite
5. Expand to P1 and P2

Ready to start implementation?
