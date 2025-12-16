# YAMY Configuration Analysis

## Configuration Breakdown

Your YAMY setup has 4 configuration files with distinct behaviors:

### 1. **109.mayu** (Baseline)
- **Purpose**: Japanese 109-key keyboard layout definitions
- **Behavior**: Passthrough (no remapping)
- **Test**: `abc → abc` ✅ Already passing

### 2. **config.mayu** (Modal Layers - VERY COMPLEX!)
- **Lines**: 434 lines
- **Behaviors**:
  1. **Simple Substitution** (lines 2-91): 90 key substitutions
  2. **Modal Modifiers** (lines 93-112): 10 custom modal layers
  3. **Modal Key Mappings** (lines 115-428): 300+ modal bindings

#### config.mayu Behavior Categories:

**A. Simple Substitution (def subst)**
- Direct 1:1 key replacement
- Examples:
  ```
  def subst *A = *Tab       # A → Tab
  def subst *B = *Enter     # B → Enter
  def subst *V = *BS        # V → Backspace
  def subst *W = *A         # W → A
  ```

**B. Modal Layers (mod definitions)**
Creates 10 modal layers activated by holding keys:
- `mod mod9 = !!A` - Hold A to activate mod9 layer
- `mod mod0 = !!B` - Hold B to activate mod0 layer
- `mod mod1 = !!V` - Hold V to activate mod1 layer
- `mod mod2 = !!M` - Hold M to activate mod2 layer
- `mod mod3 = !!X` - Hold X to activate mod3 layer
- `mod mod4 = !!_1` - Hold _1 (number 1) to activate mod4 layer
- `mod mod5 = !!LCtrl` - Hold LCtrl to activate mod5 layer
- `mod mod6 = !!C` - Hold C to activate mod6 layer
- `mod mod7 = !!Tab` - Hold Tab to activate mod7 layer
- `mod mod8 = !!Q` - Hold Q to activate mod8 layer

**C. Modal Key Mappings**
When a modal layer is active, keys map to different outputs:
```
# When mod0 (B) is held:
key *W-*A-*S-*C-m0-*Tab = *W-*A-*S-*C-F11    # Tab → F11
key *W-*A-*S-*C-m0-*A = *W-*A-*S-*C-_1       # A → _1

# When mod1 (V) is held:
key *W-*A-*S-*C-m1-*Colon = *W-*A-*S-*C-L    # Colon → L
key *W-*A-*S-*C-m1-*A = *W-*A-*S-*C-S        # A → S
```

**D. Modifier Redefinitions**
```
mod Alt += F4           # F4 becomes Alt
mod Shift += N          # N becomes Shift (additional)
mod Ctrl += LAlt        # LAlt becomes Ctrl
```

### 3. **hm.mayu** (Symbol Remapping)
- **Lines**: 24 lines
- **Purpose**: Make typing special characters easier
- **Examples**:
  ```
  key s-_2 = $COMMERCIAL_AT       # Shift+2 → @
  key colon = s-_7                # : → Shift+7 (&)
  key Atmark = $GRAVE_ACCENT      # @ → `
  key s-semicolon = colon         # Shift+; → :
  ```

### 4. **dvorakY.mayu** (Layout Transformation)
- **Lines**: 12 lines
- **Purpose**: Dvorak layout with Japanese extensions
- **Features**:
  - Toggle Lock0 with _4 and _5 keys
  - Multi-character output sequences
  - Japanese character support
- **Examples**:
  ```
  key L0-IL-~KL-*IC-SEMICOLON = y a    # ; → "ya" (2 keys!)
  key L0-IL-~KL-*IC-q = y o            # q → "yo"
  key L0-IL-~KL-*IC-x = y i            # x → "yi"
  ```

## Test Categories

Based on this analysis, we need these test categories:

### Category 1: Simple Substitution ✅ (Priority: HIGH)
**Test Focus**: Single key → single key remapping
**Config**: `config.mayu` (def subst only)
**Test Cases**:
1. `A → Tab` (KEY_A → KEY_TAB)
2. `B → Enter` (KEY_B → KEY_ENTER)
3. `V → Backspace` (KEY_V → KEY_BACKSPACE)
4. `W → A` (KEY_W → KEY_A)
5. Multi-key sequence: `WAV → ABS` (after substitution)

### Category 2: Modal Layer - Single Modifier (Priority: HIGH)
**Test Focus**: Hold modifier + press key
**Config**: `config.mayu` (mod + key mappings)
**Test Cases**:
1. Hold B (mod0) + press Tab → F11
2. Hold V (mod1) + press A → S
3. Hold M (mod2) + press A → $LEFT_PARENTHESIS
4. Release modifier → return to normal

### Category 3: Symbol Remapping (Priority: MEDIUM)
**Test Focus**: Character → symbol transformations
**Config**: `config.mayu` + `hm.mayu`
**Test Cases**:
1. `:` → `&` (with hm.mayu)
2. Shift+`;` → `:` (with hm.mayu)
3. `@` → `` ` `` (with hm.mayu)

### Category 4: Layout Transformation (Priority: LOW)
**Test Focus**: Multi-character output
**Config**: `dvorakY.mayu`
**Test Cases**:
1. `;` → `ya` (2-key output)
2. `q` → `yo`
3. Toggle Lock0 behavior

### Category 5: Combined Behaviors (Priority: MEDIUM)
**Test Focus**: All configs together
**Config**: All 4 files
**Test Cases**:
1. Verify modal layers work with dvorakY
2. Verify symbol remapping works with substitution
3. Full integration test

## Testing Strategy

### Phase 1: Individual Layer Testing
Test each config file independently:
1. ✅ 109.mayu only (baseline) - DONE
2. 109 + config.mayu (substitution only)
3. 109 + config.mayu (with mod0 layer)
4. 109 + hm.mayu
5. 109 + dvorakY.mayu

### Phase 2: Combined Testing
Test combinations:
1. 109 + config + hm
2. 109 + config + dvorakY
3. 109 + hm + dvorakY
4. All together

### Phase 3: Stress Testing
1. Rapid modifier switching
2. Multiple keys in sequence
3. Modal nesting (if supported)

## Challenges for E2E Testing

### Challenge 1: Modal Layers
- **Problem**: Need to hold one key while pressing another
- **Solution**: Inject key press (hold), inject second key, release both
- **Implementation**: Extend VirtualKeyboard to support key hold

### Challenge 2: Multi-character Output
- **Problem**: dvorakY outputs multiple keys for one input
- **Solution**: Expect array of keys [KEY_Y, KEY_A] for semicolon
- **Implementation**: Modify E2E test to accept variable-length output

### Challenge 3: Symbol Keys
- **Problem**: Need to map symbols to evdev codes
- **Solution**: Add symbol → evdev mapping table
- **Implementation**: Extend keyCodeToName with all symbols

### Challenge 4: Complex Modifier States
- **Problem**: `*W-*A-*S-*C-m0-*Tab` has many modifier combinations
- **Solution**: Test with and without each modifier
- **Implementation**: Generate test matrix for modifier combinations

## Recommended Test Prioritization

1. **High Priority** (Do First):
   - Simple substitution (A→Tab, B→Enter, V→BS)
   - Single modal layer (Hold B, press Tab → F11)

2. **Medium Priority** (Do Next):
   - Symbol remapping (hm.mayu)
   - Combined config+hm

3. **Low Priority** (Do Later):
   - Multi-character output (dvorakY)
   - Complex modal combinations
   - Full integration with all configs

## Next Steps

1. Create E2E tests for simple substitution
2. Extend VirtualKeyboard to support key hold (for modal layers)
3. Create modal layer E2E tests
4. Add symbol mapping and test hm.mayu
5. Handle multi-character output for dvorakY
6. Run full integration suite
