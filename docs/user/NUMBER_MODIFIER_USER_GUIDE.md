# Number Modifier Keys User Guide

## Overview

YAMY's **Number Modifier** feature allows you to use number keys (1-9, 0) as hardware modifier keys (Shift, Ctrl, Alt, Win) when held down. This is especially useful for users with **small keyboards** (60%, 65%, 75% layouts) that lack dedicated modifier keys or a numpad.

### Key Benefits

- **More Modifiers**: Get up to 10 additional modifier keys without sacrificing functionality
- **Dual-Purpose Keys**: Number keys work normally when tapped, activate modifiers when held
- **System-Wide**: Modifiers work in all applications, not just YAMY
- **Customizable**: Configure which number maps to which modifier
- **Backward Compatible**: Existing number key remappings continue to work

## How It Works

### Hold vs Tap Detection

Number modifier keys use intelligent **hold-vs-tap detection** with a 200ms threshold:

```
TAP (< 200ms):  Quick press → Normal key behavior (substitution if configured)
HOLD (≥ 200ms): Long press  → Activates modifier key (system-wide)
```

**Example Timeline:**

```
Time: 0ms    → Press number 1
Time: 50ms   → Release number 1
             → TAP detected → Normal behavior (e.g., outputs F1 if substituted)

Time: 0ms    → Press number 1
Time: 250ms  → Still holding...
             → HOLD detected → LShift modifier activated
Time: 300ms  → Press 'A'
             → Outputs Shift+A
Time: 500ms  → Release number 1
             → LShift deactivated
```

## Configuration

### Basic Syntax

Add number modifier mappings to your `.mayu` configuration file:

```mayu
def numbermod *_1 = *LShift
def numbermod *_2 = *RShift
def numbermod *_3 = *LCtrl
def numbermod *_4 = *RCtrl
def numbermod *_5 = *LAlt
def numbermod *_6 = *RAlt
def numbermod *_7 = *LWin
def numbermod *_8 = *RWin
```

### Available Modifiers

You can map number keys to these hardware modifiers:

| Modifier   | Description           | Common Use                    |
|------------|-----------------------|-------------------------------|
| `*LShift`  | Left Shift            | Capitalization, symbols       |
| `*RShift`  | Right Shift           | Capitalization, symbols       |
| `*LCtrl`   | Left Control          | Shortcuts (Ctrl+C, Ctrl+V)    |
| `*RCtrl`   | Right Control         | Shortcuts                     |
| `*LAlt`    | Left Alt              | Shortcuts, Alt-Tab            |
| `*RAlt`    | Right Alt (AltGr)     | Special characters (on intl keyboards) |
| `*LWin`    | Left Windows/Super    | OS shortcuts, window manager  |
| `*RWin`    | Right Windows/Super   | OS shortcuts                  |

## Use Cases and Examples

### Example 1: 60% Keyboard Layout

Small keyboards often lack right-side modifiers. Map number keys to fill the gap:

```mayu
# Right-side modifiers for 60% keyboard
def numbermod *_7 = *RShift
def numbermod *_8 = *RCtrl
def numbermod *_9 = *RAlt
def numbermod *_0 = *RWin

# Keep left-side for quick access
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
```

**Usage:**
- Hold `7` + press `A` → Shift+A (capital A)
- Hold `8` + press `C` → Ctrl+C (copy)
- Hold `9` + press `Tab` → Alt+Tab (switch windows)

### Example 2: Vim Users

Free up Shift keys for other remappings while keeping them accessible:

```mayu
# Use number row for modifiers
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt

# Remap actual Shift keys to frequently-used Vim commands
*LShift = *Escape
*RShift = *Enter
```

**Usage:**
- Hold `1` + press `;` → Colon (in Vim command mode)
- Tap `LShift` → Escape (exit insert mode)

### Example 3: Programmers

Map numbers to modifiers while preserving F-key substitutions:

```mayu
# Number modifiers for when held
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt
def numbermod *_4 = *LWin

# F-key substitutions for when tapped (< 200ms)
*_1 = *F1
*_2 = *F2
*_3 = *F3
*_4 = *F4
*_5 = *F5
*_6 = *F6
*_7 = *F7
*_8 = *F8
*_9 = *F9
*_0 = *F10
```

**Usage:**
- **Tap** `1` quickly → F1 (help)
- **Hold** `1` for 250ms → LShift (modifier)
- **Hold** `1` + press `A` → Shift+A

### Example 4: Japanese Keyboard Layout

Use number modifiers for international input:

```mayu
# RAlt (AltGr) for special characters
def numbermod *_6 = *RAlt

# Standard modifiers
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
```

**Usage:**
- Hold `6` + press `E` → € (on keyboards with AltGr mappings)

## Combining with Regular Substitutions

Number modifiers work seamlessly with regular key substitutions:

```mayu
# When TAPPED, number 1 outputs F1
*_1 = *F1

# When HELD, number 1 activates LShift
def numbermod *_1 = *LShift
```

**Decision Tree:**

```
Press number 1
     │
     ├─ Release < 200ms → TAP  → Output F1 (substitution)
     │
     └─ Hold ≥ 200ms    → HOLD → Activate LShift (modifier)
                                 Other keys use LShift
                                 Release → Deactivate LShift
```

## Advanced Configuration

### Multiple Modifiers Simultaneously

You can hold multiple number modifiers at once:

```mayu
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
```

**Usage:**
- Hold `1` + hold `2` + press `A` → Ctrl+Shift+A

### Modifier Combinations for Shortcuts

Create complex shortcuts easily:

```mayu
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt
def numbermod *_4 = *LWin
```

**Common Shortcuts:**
- Hold `2` + press `C` → Ctrl+C (copy)
- Hold `2` + press `V` → Ctrl+V (paste)
- Hold `2` + hold `1` + press `Z` → Ctrl+Shift+Z (redo)
- Hold `4` + press `D` → Win+D (show desktop)

### Threshold Tuning

The default hold threshold is **200ms**. This is a good balance for most users:

- **Too low (< 150ms)**: Accidental modifier activation during fast typing
- **Too high (> 300ms)**: Feels sluggish, requires conscious holding

**Note:** Currently, the threshold is a compile-time constant. Future versions may support per-key or global threshold configuration via `.mayu` syntax:

```mayu
# Proposed future syntax (not yet implemented)
set NumberModifierThresholdMs = 150
```

## Troubleshooting

### Problem: Number key always acts as modifier, never taps

**Cause:** You're holding the key longer than 200ms before releasing.

**Solution:** Press and release more quickly (like normal typing). Practice with a timer app to get a feel for 200ms.

### Problem: Number key always taps, never holds

**Cause:** Releasing too quickly, or ModifierKeyHandler not integrated properly.

**Solution:**
1. Ensure you're holding for at least 250ms (to exceed threshold)
2. Check logs for `[LAYER2:MODIFIER]` messages:
   ```bash
   export YAMY_DEBUG_KEYCODE=1
   ./yamy
   ```
3. Verify number modifiers are registered:
   ```
   Number Modifier: 0x0002 → 0x002a  # _1 → LShift
   Registered N number modifiers
   ```

### Problem: Number modifier doesn't work in other applications

**Cause:** YAMY may not be running, or modifier isn't reaching Layer 3.

**Solution:**
1. Ensure YAMY is running (`yamy` or `yamy-ctl status`)
2. Check logs for complete event flow:
   ```
   [EVENT:START] evdev 2 PRESS
   [LAYER1:IN] evdev 2 (PRESS) → yamy 0x0002
   [LAYER2:MODIFIER] Hold detected: 0x0002 → modifier 0x002a
   [LAYER3:OUT] yamy 0x002a → evdev 42 (KEY_LEFTSHIFT)
   [EVENT:END] evdev 42 PRESS
   ```

### Problem: Substitution doesn't work with number modifiers

**Cause:** Both `def numbermod` and regular substitution configured, but tap timing is off.

**Solution:**
- Ensure you're **tapping quickly** (< 200ms)
- Verify substitution is configured: `*_1 = *F1`
- Check logs for `[LAYER2:MODIFIER] Tap detected, applying substitution`

### Problem: Accidentally trigger modifiers while typing numbers

**Cause:** Typing numbers slowly or holding them slightly too long.

**Solution:**
1. **Option 1**: Type numbers faster (most natural typists are under 150ms)
2. **Option 2**: Don't configure number modifiers for frequently-typed numbers
3. **Option 3**: Use only `_9` and `_0` for modifiers (less commonly typed)

## Best Practices

### Start Small

Don't configure all 10 number keys as modifiers immediately. Start with 2-3:

```mayu
# Minimal setup for experimentation
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
```

Test for a few days to build muscle memory, then expand.

### Choose Logical Mappings

Match number positions to your hand position:

- **Left hand** (1-5): Left modifiers (LShift, LCtrl, LAlt)
- **Right hand** (6-0): Right modifiers (RShift, RCtrl, RAlt)

```mayu
# Ergonomic left-right split
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt

def numbermod *_8 = *RShift
def numbermod *_9 = *RCtrl
def numbermod *_0 = *RAlt
```

### Avoid Conflicts

Don't assign number modifiers to keys you frequently type:

- **Programmers:** Avoid `_0` if you type `0` often in code
- **Data entry:** Avoid all number keys if you input numbers constantly
- **Writers:** Numbers are rare, so all keys are safe to use

### Practice the Timing

The 200ms threshold is about the length of time it takes to say "one-two" quickly:

```
"one" (100ms) "two" (100ms) = 200ms
```

Practice this cadence to internalize the hold threshold.

### Combine with Layers

For advanced setups, use number modifiers with YAMY's modal layers:

```mayu
# Number modifier
def numbermod *_1 = *LShift

# Modal layer (different from hardware modifier)
mod mod4 = !!_2
key m4-*A = *Enter
```

**Usage:**
- Hold `1` → Hardware LShift (works system-wide)
- Hold `2` → Modal layer mod4 (A→Enter only in YAMY)

## Limitations and Caveats

### Threshold is Fixed (Currently)

The 200ms threshold is hardcoded. Future versions may support configuration:

```mayu
# Not yet supported
set NumberModifierThreshold = 150
```

**Workaround:** Adjust your typing speed to match the threshold.

### Maximum 10 Modifiers

Only number keys (0-9) are supported as modifier sources.

**Workaround:** This is usually sufficient. If you need more modifiers, use YAMY's modal layers (`mod modX`).

### No Visual Feedback

Unlike physical modifier keys, there's no LED or visual indicator when a number modifier is active.

**Workaround:**
- Trust the timing (200ms)
- Test in a text editor to verify modifier state
- Check YAMY debug logs for `[LAYER2:MODIFIER]` messages

### Modal Layers and Hardware Modifiers Interact

If you configure the same number key for both `def numbermod` and `mod modX`, **both** activate:

```mayu
def numbermod *_1 = *LShift
mod mod4 = !!_1
```

**Behavior:** Holding `1` activates **both** LShift (hardware) and mod4 (modal layer).

**Recommendation:** Use separate keys for each to avoid confusion.

## Testing Your Configuration

### Step 1: Create a Test Configuration

Create `test_numbermod.mayu`:

```mayu
# Test configuration
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl

# Optional: Add substitutions for tap behavior
*_1 = *F1
*_2 = *F2
```

### Step 2: Load Configuration

```bash
# Linux
yamy-ctl reload test_numbermod.mayu

# Or restart YAMY
killall yamy
yamy
```

### Step 3: Enable Debug Logging

```bash
export YAMY_DEBUG_KEYCODE=1
yamy 2>&1 | tee yamy.log
```

### Step 4: Test TAP Behavior

1. **Tap** number `1` quickly (< 200ms)
2. Expected: F1 output (if substitution configured)
3. Check logs for:
   ```
   [LAYER2:MODIFIER] Tap detected, applying substitution
   [LAYER2:SUBST] 0x0002 → 0x003B  # _1 → F1
   ```

### Step 5: Test HOLD Behavior

1. **Hold** number `1` for 300ms (count "one-two-three")
2. While holding, press `A`
3. Expected: Capital `A` appears
4. Release number `1`
5. Check logs for:
   ```
   [LAYER2:MODIFIER] Hold detected: 0x0002 → modifier 0x002a
   [LAYER3:OUT] yamy 0x002a → evdev 42 (KEY_LEFTSHIFT)
   ```

### Step 6: Test Combinations

1. Hold `1` for 300ms (LShift)
2. Hold `2` for 300ms (LCtrl)
3. Press `A`
4. Expected: Ctrl+Shift+A (e.g., opens something in your editor)

## Example Configurations

### Configuration 1: Minimal Setup for 60% Keyboard

```mayu
# Essential modifiers only
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt

# Keep numbers accessible via tap with substitutions to function keys
*_1 = *F1
*_2 = *F2
*_3 = *F3
*_4 = *F4
*_5 = *F5
*_6 = *F6
*_7 = *F7
*_8 = *F8
*_9 = *F9
*_0 = *F10
```

### Configuration 2: Maximum Modifiers (All 8 Hardware Modifiers)

```mayu
# Full modifier mapping (use all 10 number keys)
def numbermod *_1 = *LShift
def numbermod *_2 = *RShift
def numbermod *_3 = *LCtrl
def numbermod *_4 = *RCtrl
def numbermod *_5 = *LAlt
def numbermod *_6 = *RAlt
def numbermod *_7 = *LWin
def numbermod *_8 = *RWin

# Optionally, keep _9 and _0 for tap-only substitutions
*_9 = *Escape
*_0 = *BackSpace
```

### Configuration 3: Ergonomic Left-Hand Modifiers

```mayu
# Only left-hand numbers (1-5) for left-handed modifier access
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt
def numbermod *_4 = *LWin

# Right hand numbers remain normal
*_6 = *F6
*_7 = *F7
*_8 = *F8
*_9 = *F9
*_0 = *F10
```

### Configuration 4: Vim Power User

```mayu
# Modifiers for shortcuts
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl

# Remap physical modifiers to Vim keys
*LShift = *Escape
*RShift = *Enter
*CapsLock = *Escape

# Function keys for tap behavior
*_3 = *F3
*_4 = *F4
*_5 = *F5
*_6 = *F6
*_7 = *F7
*_8 = *F8
*_9 = *F9
*_0 = *F10
```

### Configuration 5: Japanese Keyboard with AltGr

```mayu
# International character support
def numbermod *_6 = *RAlt  # AltGr for special characters

# Standard modifiers
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt

# Japanese-specific substitutions
*_7 = *Hiragana
*_8 = *Katakana
*_9 = *Muhenkan
*_0 = *Henkan
```

## Performance and Timing

### Event Processing Overhead

Number modifier detection adds minimal overhead:

- **Check if number modifier**: < 10µs (hash map lookup)
- **Process hold/tap state**: < 50µs (timestamp comparison)
- **Total overhead**: < 100µs (0.1ms)

This is well within YAMY's **< 1ms** event processing budget.

### Threshold Accuracy

The 200ms threshold is **deterministic** but not nanosecond-precise:

- Threshold check happens on **next event** (not via timer callback)
- Typical accuracy: ±10ms
- User-imperceptible difference

**Example:**
```
Time: 0ms    → Press number 1 (start timer)
Time: 205ms  → Press letter A (trigger threshold check)
             → Threshold exceeded (205ms > 200ms)
             → Activate modifier retroactively
             → Process letter A with modifier active
```

The modifier activates before processing the `A` keypress, so the user sees `Shift+A` as expected.

## Migration from Other Tools

### From QMK/Via (Keyboard Firmware)

If you're used to QMK's hold-tap behavior:

**QMK:**
```c
LT(1, KC_1)  // Layer-Tap: Layer 1 when held, 1 when tapped
```

**YAMY Equivalent:**
```mayu
def numbermod *_1 = *LShift  # Modifier when held
*_1 = *F1                    # F1 when tapped (optional)
```

**Differences:**
- YAMY uses **200ms** threshold (QMK default: 200ms, configurable)
- YAMY modifiers are **hardware keys**, QMK layers are **firmware layers**
- YAMY works at OS level, QMK at firmware level

### From AutoHotkey (Windows)

**AutoHotkey:**
```ahk
1::
    KeyWait, 1, T0.2
    If ErrorLevel
        Send {LShift Down}
    Else
        Send {F1}
Return

1 up::Send {LShift Up}
```

**YAMY Equivalent:**
```mayu
def numbermod *_1 = *LShift
*_1 = *F1
```

**Advantages:**
- Much simpler syntax
- No scripting required
- Integrated with YAMY's event processing pipeline
- Works cross-platform (Windows + Linux)

### From xmodmap/xkb (Linux)

**xmodmap** cannot do hold-tap detection (only simple remapping).

**YAMY Equivalent:**
```mayu
# xmodmap can only do this:
# xmodmap -e "keycode 10 = Shift_L"

# YAMY does both:
def numbermod *_1 = *LShift  # Hold
*_1 = *F1                    # Tap
```

**Advantages:**
- Hold-tap detection (impossible with xmodmap/xkb alone)
- Unified configuration for Windows + Linux
- Works with existing YAMY substitutions and modal layers

## FAQ

### Q: Can I use letters as modifier keys instead of numbers?

**A:** Not currently. The feature is designed for number keys (0-9) only. However, the underlying `ModifierKeyHandler` supports any key, so this could be extended in the future if there's demand.

**Workaround:** Use YAMY's existing modal layers for letter-based modifiers.

### Q: Why 200ms? Can I change it?

**A:** 200ms is a well-tested threshold used by QMK, Karabiner, and other tools. It's fast enough to not feel sluggish, but slow enough to avoid accidental triggers.

Currently, the threshold is hardcoded. Future versions may support:
```mayu
set NumberModifierThreshold = 150  # Proposed syntax
```

### Q: What happens if I press another key before the threshold?

**A:** The threshold check happens on the **next event**:

```
Time: 0ms    → Press number 1 (WAITING state)
Time: 100ms  → Press letter A (< 200ms threshold)
             → Check: number 1 still WAITING (not held long enough)
             → Process A normally (no modifier)
Time: 150ms  → Release number 1
             → TAP detected → Apply substitution (if configured)
```

The modifier **does not activate retroactively** if you press another key before 200ms.

### Q: Can I use number modifiers for triple-modifier shortcuts (Ctrl+Shift+Alt)?

**A:** Yes! Hold multiple number modifiers simultaneously:

```mayu
def numbermod *_1 = *LShift
def numbermod *_2 = *LCtrl
def numbermod *_3 = *LAlt
```

**Usage:**
- Hold `1` + hold `2` + hold `3` + press `A` → Ctrl+Shift+Alt+A

### Q: Do number modifiers work in games?

**A:** **Generally yes**, but with caveats:

- **Single-player games**: Should work fine
- **Multiplayer/anti-cheat**: YAMY uses input injection, which some anti-cheat systems flag
- **Latency-sensitive games**: The 200ms threshold means you can't use number keys as fast-tap inputs

**Recommendation:** Disable YAMY or use a separate `.mayu` profile for gaming.

### Q: Can I have different thresholds for different keys?

**A:** Not currently. All number modifiers use the same 200ms threshold.

**Future:** This could be supported via syntax like:
```mayu
def numbermod *_1 = *LShift threshold=150
def numbermod *_2 = *LCtrl threshold=250
```

### Q: What's the difference between number modifiers and modal layers?

**A:**

| Feature                  | Number Modifiers (`def numbermod`) | Modal Layers (`mod modX = !!_Y`) |
|--------------------------|-----------------------------------|-----------------------------------|
| **Scope**                | System-wide (all apps)            | YAMY-only                         |
| **Output**               | Hardware modifier keys            | Engine-level layer switching      |
| **Use Case**             | System shortcuts, modifiers       | Complex YAMY-specific remapping   |
| **Examples**             | Shift+A, Ctrl+C, Win+D            | Vim-style layers, custom modes    |

You can use **both** for different purposes:
- **Number modifiers** for system-level shortcuts
- **Modal layers** for application-specific complex remapping

## Changelog and Version History

### Version 1.0 (Initial Release)

- **Feature:** Number keys as hardware modifiers with hold-tap detection
- **Syntax:** `def numbermod *_X = *Modifier`
- **Threshold:** 200ms (hardcoded)
- **Supported Modifiers:** LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin
- **Platform:** Linux + Windows (via YAMY cross-platform architecture)

### Future Enhancements (Planned)

- Configurable threshold per-key or globally
- Support for non-number keys as modifiers (letters, symbols)
- Visual indicator for active modifiers (system tray icon)
- Per-application profiles (different thresholds for games vs productivity)

## Additional Resources

- **Design Document**: `docs/NUMBER_MODIFIER_DESIGN.md` - Technical architecture and implementation details
- **Syntax Reference**: `docs/NUMBER_MODIFIER_SYNTAX.md` - Parser implementation and error handling
- **Testing Guide**: `tests/test_number_modifiers.cpp`, `tests/test_number_modifiers_e2e.py`
- **Implementation Tasks**: `.spec-workflow/specs/key-remapping-consistency/tasks.md` (Phase 4)

## Support and Feedback

If you encounter issues or have questions:

1. **Check logs**: Enable debug logging with `export YAMY_DEBUG_KEYCODE=1`
2. **Review this guide**: Troubleshooting section covers common issues
3. **Test configuration**: Use the testing steps to verify setup
4. **Report bugs**: Open an issue on GitHub with logs and configuration

Happy typing with your new number modifiers!
