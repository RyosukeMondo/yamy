# Modal Modifier Testing Guide

## Current Configuration

The daemon is running with **config_clean.mayu** which has:
- **82 key substitutions** (Dvorak-like layout)
- **10 modal modifiers** (mod0 through mod9)
- **3 modal modifier test bindings**

## Test Cases

### Test 1: B (mod0) + W → 1

**Modal Modifier Definition:**
```
mod mod0 = !!B    # Hold B for >200ms activates mod0
```

**Key Binding:**
```
key M0-W = *_1    # When mod0 active, W outputs 1
```

**How to Test:**
1. **Hold B key** for longer than 200ms (keep it pressed)
2. While still holding B, **press W**
3. **Expected result:** Character "1" should be output
4. **Tap B quickly** (<200ms): Should output Enter (due to substitution)

**Debug Log Pattern:**
```
[MODIFIER] Hold detected: 0x0030 → modal mod0 ACTIVATE
[LAYER2:MODAL_MOD] 0x0030 HOLD → mod0 ACTIVATE
```

---

### Test 2: A (mod9) + E → 2

**Modal Modifier Definition:**
```
mod mod9 = !!A    # Hold A for >200ms activates mod9
```

**Key Binding:**
```
key M9-E = *_2    # When mod9 active, E outputs 2
```

**How to Test:**
1. **Hold A key** for longer than 200ms
2. While holding A, **press E**
3. **Expected result:** Character "2" should be output
4. **Tap A quickly**: Should output Tab (due to substitution)

---

### Test 3: 1 (mod4) + U → 3

**Modal Modifier Definition:**
```
mod mod4 = !!_1   # Hold 1 for >200ms activates mod4
```

**Key Binding:**
```
key M4-U = *_3    # When mod4 active, U outputs 3
```

**How to Test:**
1. **Hold 1 key** for longer than 200ms
2. While holding 1, **press U**
3. **Expected result:** Character "3" should be output
4. **Tap 1 quickly**: Should output Left Shift (due to substitution)

---

## Understanding the System

### Hold vs Tap Detection

- **Tap** (< 200ms): Key applies its substitution
  - B → Enter
  - A → Tab
  - W → A

- **Hold** (≥ 200ms): Key activates modal modifier
  - B → mod0 active
  - A → mod9 active
  - 1 → mod4 active

### Modal Modifiers vs Standard Modifiers

**Standard Modifiers** (inject VK codes):
- Shift, Ctrl, Alt, Win
- Example: `S-W` = Shift+W

**Modal Modifiers** (state-only, no VK injection):
- mod0 through mod19
- Example: `M0-W` = When mod0 active, press W

---

## Viewing Debug Logs

### Real-time Monitoring

```bash
tail -f ~/.local/share/YAMY/YAMY/yamy-daemon.log
```

### Check for Modal Modifier Activation

```bash
grep -i "MODAL_MOD\|Hold detected" ~/.local/share/YAMY/YAMY/yamy-daemon.log
```

### Expected Log Messages

**When holding B (mod0):**
```
[ModifierKeyHandler] [MODIFIER] Hold detected: 0x0030 → modal mod0 ACTIVATE
[EventProcessor] [LAYER2:MODAL_MOD] 0x0030 HOLD → mod0 ACTIVATE
```

**When tapping B:**
```
[ModifierKeyHandler] [MODIFIER] Tap detected: 0x0030
[EventProcessor] [LAYER1:SUBST] 0x0030 → 0x001c (substitution)
```

---

## Troubleshooting

### Daemon Not Responding to Keys

1. **Check if daemon is running:**
   ```bash
   ps aux | grep yamy
   ```

2. **Check input hook status:**
   ```bash
   grep "input hook" ~/.local/share/YAMY/YAMY/yamy-daemon.log
   ```

3. **Verify config loaded:**
   ```bash
   grep "successfully switched" ~/.local/share/YAMY/YAMY/yamy-daemon.log
   ```

### Modal Modifiers Not Activating

1. **Verify registration:**
   ```bash
   grep "Registered.*modal modifiers" /tmp/yamy-debug.log
   ```
   Should show: `Registered 10 modal modifiers`

2. **Check timing:** Hold key for **full 200ms** before pressing second key

3. **Enable debug logging:**
   ```bash
   export YAMY_DEBUG_KEYCODE=1
   ```

### Keys Output Wrong Characters

This is expected due to the Dvorak-like substitutions in config_clean.mayu.
The modal modifier system works **on top of** substitutions.

---

## Current Daemon Status

```bash
# Check daemon is running
ps aux | grep '[y]amy'

# View recent logs
tail -50 /tmp/yamy-debug.log

# Check loaded configuration
grep "switchConfiguration: successfully" /tmp/yamy-debug.log
```

**Expected Output:**
```
Registered 10 modal modifiers
EventProcessor debug logging enabled
switchConfiguration: successfully switched to: config_clean.mayu
```

---

## All Registered Modal Modifiers

| Modifier | Trigger Key | Scan Code | Hold Time |
|----------|-------------|-----------|-----------|
| mod0     | B           | 0x0030    | 200ms     |
| mod1     | V           | 0x002f    | 200ms     |
| mod2     | X           | 0x002d    | 200ms     |
| mod3     | C           | 0x002e    | 200ms     |
| mod4     | 1           | 0x0002    | 200ms     |
| mod5     | N           | 0x0031    | 200ms     |
| mod6     | M           | 0x0032    | 200ms     |
| mod7     | T           | 0x0014    | 200ms     |
| mod8     | R           | 0x0013    | 200ms     |
| mod9     | A           | 0x001e    | 200ms     |

**Note:** Only mod0, mod4, and mod9 have keymap bindings configured for testing.

---

## Next Steps

1. Try the three test cases above
2. Watch debug logs for activation messages
3. Verify output matches expected characters (1, 2, 3)
4. Report any issues or unexpected behavior

**Daemon Log Location:** `/tmp/yamy-debug.log`
**Config File:** `/home/rmondo/repos/yamy/keymaps/config_clean.mayu`
**Binary:** `/home/rmondo/repos/yamy/build/linux-debug/bin/yamy`
