# Migration Guide: .mayu to JSON

This guide will help you migrate your existing `.mayu` configuration files to the new JSON format introduced in yamy 2.0.

## Table of Contents

- [Why Migrate?](#why-migrate)
- [Breaking Changes](#breaking-changes)
- [JSON Configuration Structure](#json-configuration-structure)
- [Migration Examples](#migration-examples)
  - [Basic Key Remapping](#basic-key-remapping)
  - [Modifier Keys](#modifier-keys)
  - [Virtual Modifiers (M00-MFF)](#virtual-modifiers-m00-mff)
  - [Key Sequences](#key-sequences)
  - [Vim-Style Modal Editing](#vim-style-modal-editing)
  - [Emacs-Style Key Bindings](#emacs-style-key-bindings)
- [Removed Features](#removed-features)
- [Workarounds](#workarounds)
- [Common Patterns](#common-patterns)
- [Validation](#validation)

---

## Why Migrate?

The JSON configuration format provides several benefits:

- **Faster Configuration Loading**: <10ms (vs ~100ms for .mayu)
- **Better IDE Support**: JSON Schema enables autocomplete and validation in VSCode, IntelliJ, etc.
- **Simplified Event Processing**: ~50% reduction in event latency
- **Smaller Binary**: ~30% reduction in executable size
- **Industry Standard Format**: JSON is universally understood and tooling is excellent
- **Cleaner Codebase**: ~3,000 lines of parsing code removed

---

## Breaking Changes

The following features from `.mayu` are **removed** in JSON configuration:

1. **Per-Window Keymaps** - Only global keymaps are supported
2. **Window Class/Title Matching** - No `window` directives
3. **Include Directives** - No `include "file.mayu"` support
4. **Keymap Prefixes** - No `keymap2` or `&Prefix()` commands
5. **Action Commands** - No `&WindowMove()`, `&LoadSetting()`, `&HelpMessage()`, etc.
6. **Event Handlers** - No `event prefixed`, `event before-key-down`
7. **Thread Tracking** - No focus detection or thread attachment
8. **Clipboard Commands** - No clipboard manipulation commands
9. **Modifier Declarations** - No `mod control += CapsLock` (use virtual modifiers instead)
10. **Multiple Keymap Inheritance** - Single global keymap only

---

## JSON Configuration Structure

A JSON config has three main sections:

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "KeyName": "0xScanCode"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "KeyName",
      "tapAction": "KeyName"
    }
  },
  "mappings": [
    {
      "from": "KeySpec",
      "to": "KeySpec or [KeySeq]"
    }
  ]
}
```

### Version Field (Required)

Must be `"2.0"` to indicate JSON format version.

### Keyboard Section (Required)

Defines key names and their scan codes. You must define all keys you intend to use.

### Virtual Modifiers Section (Optional)

Defines M00-MFF virtual modifiers for modal editing (e.g., vim, emacs).

### Mappings Section (Required)

Defines key remappings from one key/combination to another.

---

## Migration Examples

### Basic Key Remapping

**.mayu format:**
```
keymap Global
    key A = B
    key CapsLock = Escape
```

**JSON format:**
```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "B": "0x30",
      "CapsLock": "0x3a",
      "Escape": "0x01"
    }
  },
  "mappings": [
    {
      "from": "A",
      "to": "B"
    },
    {
      "from": "CapsLock",
      "to": "Escape"
    }
  ]
}
```

---

### Modifier Keys

**.mayu format:**
```
keymap Global
    key C-A = C-Home      # Ctrl-A → Ctrl-Home
    key S-Tab = Tab       # Shift-Tab → Tab
    key C-S-A = C-End     # Ctrl-Shift-A → Ctrl-End
```

**JSON format:**
```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "Tab": "0x0f",
      "Home": "0xc7",
      "End": "0xcf"
    }
  },
  "mappings": [
    {
      "from": "Ctrl-A",
      "to": "Ctrl-Home"
    },
    {
      "from": "Shift-Tab",
      "to": "Tab"
    },
    {
      "from": "Ctrl-Shift-A",
      "to": "Ctrl-End"
    }
  ]
}
```

**Modifier Names:**
- `Ctrl` - Control key
- `Shift` - Shift key
- `Alt` - Alt key (Meta on Linux)
- `Win` - Windows/Super key

---

### Virtual Modifiers (M00-MFF)

Virtual modifiers enable modal editing like vim and emacs. They support tap-vs-hold behavior.

**.mayu format:**
```
# In .mayu, you'd use keymap2 with &Prefix() commands
keymap2 VimNormal : Global
    key H = Left
    key J = Down

keymap Global
    key CapsLock = &Prefix(VimNormal)
```

**JSON format:**
```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "CapsLock": "0x3a",
      "Escape": "0x01",
      "H": "0x23",
      "J": "0x24",
      "K": "0x25",
      "L": "0x26",
      "Left": "0xcb",
      "Down": "0xd0",
      "Up": "0xc8",
      "Right": "0xcd"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tapAction": "Escape"
    }
  },
  "mappings": [
    {
      "from": "M00-H",
      "to": "Left"
    },
    {
      "from": "M00-J",
      "to": "Down"
    },
    {
      "from": "M00-K",
      "to": "Up"
    },
    {
      "from": "M00-L",
      "to": "Right"
    }
  ]
}
```

**How Virtual Modifiers Work:**
- `trigger`: The physical key that activates the virtual modifier
- `tapAction`: What happens if you tap (not hold) the trigger key
- Hold threshold: Default 200ms (configurable via `holdThresholdMs`)

**When you tap CapsLock quickly**: Sends Escape
**When you hold CapsLock + H**: Sends Left arrow

You can define up to 256 virtual modifiers (M00-MFF in hexadecimal).

---

### Key Sequences

**.mayu format:**
```
keymap Global
    key C-A = Home S-End   # Select to beginning of line
    key C-B = Esc B        # Two key sequence
```

**JSON format:**
```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "B": "0x30",
      "Home": "0xc7",
      "End": "0xcf",
      "Escape": "0x01"
    }
  },
  "mappings": [
    {
      "from": "Ctrl-A",
      "to": ["Home", "Shift-End"]
    },
    {
      "from": "Ctrl-B",
      "to": ["Escape", "B"]
    }
  ]
}
```

**Note**: Use an array `[]` for key sequences. Each element is pressed and released in order.

---

### Vim-Style Modal Editing

**.mayu approach:**
Uses `keymap2` with `&Prefix()` to create modal states. Complex and requires deep understanding of yamy's keymap system.

**JSON approach:**
Use virtual modifiers as mode triggers. Much simpler but slightly different semantics.

**Example - CapsLock as vim mode trigger:**

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "CapsLock": "0x3a",
      "Escape": "0x01",
      "H": "0x23",
      "J": "0x24",
      "K": "0x25",
      "L": "0x26",
      "W": "0x11",
      "B": "0x30",
      "0": "0x0b",
      "4": "0x05",
      "D": "0x20",
      "X": "0x2d",
      "U": "0x16",
      "Left": "0xcb",
      "Right": "0xcd",
      "Up": "0xc8",
      "Down": "0xd0",
      "Home": "0xc7",
      "End": "0xcf",
      "Delete": "0xd3"
    }
  },
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tapAction": "Escape"
    }
  },
  "mappings": [
    {
      "from": "M00-H",
      "to": "Left",
      "description": "vim: h - left"
    },
    {
      "from": "M00-J",
      "to": "Down",
      "description": "vim: j - down"
    },
    {
      "from": "M00-K",
      "to": "Up",
      "description": "vim: k - up"
    },
    {
      "from": "M00-L",
      "to": "Right",
      "description": "vim: l - right"
    },
    {
      "from": "M00-W",
      "to": "Ctrl-Right",
      "description": "vim: w - forward word"
    },
    {
      "from": "M00-B",
      "to": "Ctrl-Left",
      "description": "vim: b - backward word"
    },
    {
      "from": "M00-0",
      "to": "Home",
      "description": "vim: 0 - beginning of line"
    },
    {
      "from": "M00-4",
      "to": "End",
      "description": "vim: $ - end of line"
    },
    {
      "from": "M00-X",
      "to": "Delete",
      "description": "vim: x - delete character"
    },
    {
      "from": "M00-D",
      "to": ["Shift-End", "Ctrl-X"],
      "description": "vim: dd - delete line"
    },
    {
      "from": "M00-U",
      "to": "Ctrl-Z",
      "description": "vim: u - undo"
    }
  ]
}
```

**Key Differences:**
- In `.mayu`, you could have true modal states with `&Prefix()` that persist across keystrokes
- In JSON, virtual modifiers require holding the trigger key (more like a "layer" than a "mode")
- This is actually more similar to QMK/ZMK keyboard firmware behavior

---

### Emacs-Style Key Bindings

**.mayu approach:**
```
keymap EmacsEdit : Global
    key C-F = Right       # Forward char
    key C-B = Left        # Backward char
    key C-N = Down        # Next line
    key C-P = Up          # Previous line
    key C-A = Home        # Beginning of line
    key C-E = End         # End of line
    key M-F = C-Right     # Forward word (Meta = Alt)
    key M-B = C-Left      # Backward word
```

**JSON approach:**
```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "F": "0x21",
      "B": "0x30",
      "N": "0x31",
      "P": "0x19",
      "A": "0x1e",
      "E": "0x12",
      "Left": "0xcb",
      "Right": "0xcd",
      "Up": "0xc8",
      "Down": "0xd0",
      "Home": "0xc7",
      "End": "0xcf"
    }
  },
  "mappings": [
    {
      "from": "Ctrl-F",
      "to": "Right",
      "description": "emacs: forward-char"
    },
    {
      "from": "Ctrl-B",
      "to": "Left",
      "description": "emacs: backward-char"
    },
    {
      "from": "Ctrl-N",
      "to": "Down",
      "description": "emacs: next-line"
    },
    {
      "from": "Ctrl-P",
      "to": "Up",
      "description": "emacs: previous-line"
    },
    {
      "from": "Ctrl-A",
      "to": "Home",
      "description": "emacs: beginning-of-line"
    },
    {
      "from": "Ctrl-E",
      "to": "End",
      "description": "emacs: end-of-line"
    },
    {
      "from": "Alt-F",
      "to": "Ctrl-Right",
      "description": "emacs: forward-word"
    },
    {
      "from": "Alt-B",
      "to": "Ctrl-Left",
      "description": "emacs: backward-word"
    }
  ]
}
```

**For CapsLock as Control:**

You can use a virtual modifier to make CapsLock act as Control when held:

```json
{
  "virtualModifiers": {
    "M01": {
      "trigger": "CapsLock",
      "tapAction": "Escape"
    }
  },
  "mappings": [
    {
      "from": "M01-F",
      "to": "Right",
      "description": "CapsLock-F as Ctrl-F (forward-char)"
    },
    {
      "from": "M01-B",
      "to": "Left"
    }
  ]
}
```

---

## Removed Features

### 1. Per-Window Keymaps

**.mayu had:**
```
window EditControl /:(Edit|TEdit)$/ : EmacsEdit
window GVim /gvim.*:Vim$/ : VimExclude
```

**Workaround:**
Use a single global keymap that works across all applications. Most vim/emacs keybindings don't conflict with application shortcuts because they use modifier keys or modal states.

If you need application-specific behavior, consider:
- Using the application's built-in key remapping
- Creating separate JSON configs and loading them per-application
- Using desktop environment tools (e.g., KDE's window rules)

---

### 2. Action Commands

**.mayu had:**
```
key C-S-S = &LoadSetting           # Reload configuration
key C-W H = &WindowMove(-16, 0)   # Move window left
key Escape = &HelpMessage("VIM", "-- NORMAL --")
```

**Workaround:**
These advanced features are not available in JSON configuration. The JSON format focuses on core key remapping functionality.

For window management, use:
- Your desktop environment's built-in shortcuts (e.g., Super+Arrow in GNOME)
- Dedicated window management tools (e.g., i3, sway, hammerspoon)

For configuration reload, restart yamy or use system signals if supported.

---

### 3. Include Directives

**.mayu had:**
```
include "109.mayu"
include "base-keys.mayu"
```

**Workaround:**
JSON doesn't support includes. Instead:
- Copy all key definitions into your config.json
- Use the provided example configs as starting points
- Use external tools to generate/merge JSON files if needed

Example with `jq`:
```bash
jq -s '.[0] * .[1]' base.json custom.json > config.json
```

---

### 4. Keymap Inheritance and Prefixes

**.mayu had:**
```
keymap2 EmacsEdit : EmacsMove      # Inheritance
key C-X = &Prefix(EmacsC-X)        # Prefix chains
```

**Workaround:**
Use virtual modifiers (M00-MFF) to create layers instead of prefix chains.

Instead of `C-x C-s` (prefix chain), use:
- Direct shortcuts: `Ctrl-S` for save
- Virtual modifier combinations: `M00-S` for save (if M00 is your mode key)

---

## Common Patterns

### Pattern 1: CapsLock to Escape/Control

**.mayu:**
```
mod control += CapsLock
key *CapsLock = *LControl
```

**JSON (tap for Escape, hold for layer):**
```json
{
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tapAction": "Escape"
    }
  },
  "mappings": []
}
```

---

### Pattern 2: Home Row Modifiers

**.mayu:**
Complex setup with multiple keymaps.

**JSON:**
```json
{
  "virtualModifiers": {
    "M00": {
      "trigger": "A",
      "tapAction": "A"
    },
    "M01": {
      "trigger": "S",
      "tapAction": "S"
    },
    "M02": {
      "trigger": "D",
      "tapAction": "D"
    },
    "M03": {
      "trigger": "F",
      "tapAction": "F"
    }
  },
  "mappings": [
    {
      "from": "M00-J",
      "to": "Left"
    },
    {
      "from": "M01-K",
      "to": "Down"
    }
  ]
}
```

---

### Pattern 3: Arrow Keys on Right Hand

**Common pattern: IJKL as arrows when holding a modifier**

```json
{
  "virtualModifiers": {
    "M00": {
      "trigger": "Semicolon",
      "tapAction": "Semicolon"
    }
  },
  "mappings": [
    {
      "from": "M00-J",
      "to": "Left"
    },
    {
      "from": "M00-K",
      "to": "Down"
    },
    {
      "from": "M00-I",
      "to": "Up"
    },
    {
      "from": "M00-L",
      "to": "Right"
    }
  ]
}
```

---

## Validation

### JSON Schema

yamy provides a JSON Schema file (`schema/config.schema.json`) for validation and IDE autocomplete.

**In VSCode**, add to your config.json:
```json
{
  "$schema": "../schema/config.schema.json",
  "version": "2.0",
  ...
}
```

This enables:
- Autocomplete for key names
- Real-time validation
- Hover documentation

### Manual Validation

Use `jq` to validate JSON syntax:
```bash
jq empty config.json && echo "Valid JSON" || echo "Invalid JSON"
```

### Common Errors

**Error: Missing version field**
```
Missing required field: 'version'
```
**Fix:** Add `"version": "2.0"` as the first field.

**Error: Invalid scan code format**
```
Invalid scan code for key 'A': '1e'
```
**Fix:** Use hex format with `0x` prefix: `"0x1e"`

**Error: Unknown key name**
```
Unknown key 'UnknownKey'. Did you mean: 'Enter'?
```
**Fix:** Define the key in the `keyboard.keys` section first.

**Error: Invalid modifier name**
```
Invalid virtual modifier name: 'M0'
```
**Fix:** Use two hex digits: `"M00"`, `"M01"`, ..., `"MFF"`

---

## Migration Checklist

- [ ] Review your .mayu config and identify used features
- [ ] Check the [Removed Features](#removed-features) section
- [ ] Start with an example config (vim-mode.json or emacs-mode.json)
- [ ] Define all keys you need in the `keyboard.keys` section
- [ ] Convert simple mappings to the `mappings` array
- [ ] Convert keymap2 prefixes to virtual modifiers (M00-MFF)
- [ ] Test your config with `yamy --config config.json`
- [ ] Validate with JSON Schema in your IDE
- [ ] Remove or find workarounds for removed features
- [ ] Document any custom patterns for future reference

---

## Getting Help

- **JSON Schema Documentation**: `docs/json-schema.md`
- **Example Configs**: `keymaps/vim-mode.json`, `keymaps/config.json`
- **GitHub Issues**: Report migration issues or ask questions
- **Community**: Share your configs and learn from others

---

## Conclusion

While the JSON format removes some advanced features from .mayu, it provides a cleaner, faster, and more maintainable configuration system. Most common use cases (vim/emacs modal editing, key remapping, modifier customization) are fully supported through the virtual modifier system (M00-MFF).

The migration requires rewriting your config, but the result is a simpler, more portable configuration that's easier to understand and maintain.

Happy hacking!
