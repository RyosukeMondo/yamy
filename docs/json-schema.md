# YAMY JSON Configuration Schema

This document describes the JSON configuration format for YAMY (Yet Another Modifier YAMY), a keyboard remapping tool for Linux.

## Table of Contents

- [Overview](#overview)
- [Schema Structure](#schema-structure)
- [Field Reference](#field-reference)
  - [version](#version)
  - [keyboard](#keyboard)
  - [virtualModifiers](#virtualmodifiers)
  - [mappings](#mappings)
- [Modifier Syntax](#modifier-syntax)
- [Key Sequences](#key-sequences)
- [Validation Rules](#validation-rules)
- [Complete Examples](#complete-examples)
- [Common Patterns](#common-patterns)

## Overview

YAMY uses JSON configuration files to define keyboard key mappings. The JSON format provides:

- Simple, human-readable syntax
- IDE autocomplete support (with JSON Schema)
- Fast parsing (<10ms config load time)
- Clear validation and error messages

The configuration file defines three main components:

1. **Keyboard keys**: Map symbolic key names to scan codes
2. **Virtual modifiers**: Define M00-MFF modal layer keys with tap actions
3. **Mappings**: Define key remapping rules (from → to)

## Schema Structure

A minimal valid configuration:

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "Escape": "0x01"
    }
  },
  "mappings": [
    {
      "from": "A",
      "to": "Escape"
    }
  ]
}
```

## Field Reference

### version

**Type**: `string`
**Required**: Yes
**Valid values**: `"2.0"`

Specifies the configuration format version. Currently, only version `"2.0"` is supported.

**Example**:
```json
{
  "version": "2.0"
}
```

**Validation**:
- Field must be present
- Value must be exactly `"2.0"`

**Errors**:
```
Missing required field: 'version'
Unsupported version: '1.0' (expected '2.0')
```

---

### keyboard

**Type**: `object`
**Required**: Yes

Defines keyboard keys and their scan codes. This section maps symbolic key names (like `"A"`, `"CapsLock"`) to hardware scan codes.

#### keyboard.keys

**Type**: `object`
**Required**: Yes (can be empty object `{}`)

Maps key names to scan code hex strings.

**Format**:
```json
{
  "keyboard": {
    "keys": {
      "KeyName": "0xSCANCODE"
    }
  }
}
```

**Key Name Rules**:
- Must match pattern: `^[A-Za-z0-9_]+$`
- Case-sensitive (recommended: PascalCase for multi-word keys)
- Examples: `"A"`, `"CapsLock"`, `"F1"`, `"LeftShift"`

**Scan Code Rules**:
- Must be hex string: `0x` prefix followed by 1-4 hex digits
- Examples: `"0x1e"`, `"0x3a"`, `"0xcb"`
- Valid range: `0x00` to `0xFFFF`

**Example**:
```json
{
  "keyboard": {
    "keys": {
      "A": "0x1e",
      "B": "0x30",
      "CapsLock": "0x3a",
      "Escape": "0x01",
      "Tab": "0x0f",
      "Left": "0xcb",
      "Right": "0xcd",
      "F1": "0x3b"
    }
  }
}
```

**Common Scan Codes**:

| Key | Scan Code | Key | Scan Code |
|-----|-----------|-----|-----------|
| A-Z | 0x1e-0x2c | 0-9 | 0x0b, 0x02-0x0a |
| Escape | 0x01 | Tab | 0x0f |
| CapsLock | 0x3a | Enter | 0x1c |
| LeftShift | 0x2a | RightShift | 0x36 |
| LeftCtrl | 0x1d | RightCtrl | 0x9d |
| LeftAlt | 0x38 | RightAlt | 0xb8 |
| Space | 0x39 | Backspace | 0x0e |
| Left | 0xcb | Right | 0xcd |
| Up | 0xc8 | Down | 0xd0 |
| Home | 0xc7 | End | 0xcf |
| PageUp | 0xc9 | PageDown | 0xd1 |
| Delete | 0xd3 | Insert | 0xd2 |
| F1-F12 | 0x3b-0x58 | | |

**Validation**:
- Key names must not be empty
- Scan codes must match pattern `^0x[0-9A-Fa-f]{1,4}$`
- Duplicate key names are allowed (last definition wins)

**Errors**:
```
Invalid scan code for key 'A': '1e' (missing '0x' prefix)
Invalid scan code for key 'F1': '0xGG' (invalid hex digits)
Missing 'keyboard.keys' section
```

---

### virtualModifiers

**Type**: `object`
**Required**: No (optional)

Defines M00-MFF virtual modifier keys for modal editing. Virtual modifiers enable vim/emacs-style layer keys with hold-vs-tap detection.

**Format**:
```json
{
  "virtualModifiers": {
    "M00": {
      "trigger": "KeyName",
      "tapAction": "KeyName",
      "holdThresholdMs": 200
    }
  }
}
```

#### Modifier Names

**Pattern**: `^M[0-9A-Fa-f]{2}$`
**Range**: `M00` to `MFF` (256 modifiers)

Examples: `"M00"`, `"M01"`, `"MAA"`, `"MFF"`

#### trigger

**Type**: `string`
**Required**: Yes

The physical key that activates this virtual modifier.

**Example**:
```json
{
  "M00": {
    "trigger": "CapsLock"
  }
}
```

**Validation**:
- Must reference a key defined in `keyboard.keys`
- Key name is case-sensitive

**Errors**:
```
Unknown trigger key for M00: 'CapsLck' (typo)
Unknown trigger key for M01: 'Semicolon' (not defined in keyboard.keys)
```

#### tapAction

**Type**: `string`
**Required**: No (optional)

The key to output when the trigger is quickly tapped (not held).

**Example**:
```json
{
  "M00": {
    "trigger": "CapsLock",
    "tapAction": "Escape"
  }
}
```

**Behavior**:
- If key is pressed and released within `holdThresholdMs`: outputs `tapAction`
- If key is held longer than `holdThresholdMs`: activates modifier (no tap action)

**Validation**:
- Must reference a key defined in `keyboard.keys`
- Can be omitted (no tap action, modifier-only behavior)

#### holdThresholdMs

**Type**: `integer`
**Required**: No
**Default**: `200`

Time threshold in milliseconds to distinguish tap from hold.

**Example**:
```json
{
  "M00": {
    "trigger": "CapsLock",
    "tapAction": "Escape",
    "holdThresholdMs": 150
  }
}
```

**Validation**:
- Must be positive integer
- Recommended range: 100-500ms
- Default is 200ms if omitted

**Complete Example**:
```json
{
  "virtualModifiers": {
    "M00": {
      "trigger": "CapsLock",
      "tapAction": "Escape"
    },
    "M01": {
      "trigger": "Semicolon",
      "tapAction": "Semicolon"
    }
  }
}
```

**Use Cases**:
- **Vim mode**: CapsLock as layer key, taps to Escape
- **Emacs mode**: Semicolon as Meta key, taps to Semicolon
- **Custom layers**: Any key can become a modal modifier

**Errors**:
```
Invalid virtual modifier name: 'M0' (must be M00-MFF format)
Invalid virtual modifier name: 'M100' (only M00-MFF supported)
Unknown tap key for M00: 'Esc' (not defined in keyboard.keys)
```

---

### mappings

**Type**: `array`
**Required**: Yes

Defines key remapping rules. Each mapping specifies a source key combination (`from`) and a target action (`to`).

**Format**:
```json
{
  "mappings": [
    {
      "from": "KeySpec",
      "to": "KeySpec or Array"
    }
  ]
}
```

#### from

**Type**: `string`
**Required**: Yes

The source key combination to match. Supports:
- Simple keys: `"A"`
- Standard modifiers: `"Shift-A"`, `"Ctrl-A"`, `"Alt-A"`, `"Win-A"`
- Virtual modifiers: `"M00-A"`, `"M01-H"`
- Combinations: `"Shift-Ctrl-A"`, `"M00-Shift-G"`

**Syntax**: `[Modifier-]...[Modifier-]KeyName`

See [Modifier Syntax](#modifier-syntax) for details.

**Example**:
```json
{
  "mappings": [
    {"from": "A", "to": "B"},
    {"from": "Shift-A", "to": "Tab"},
    {"from": "M00-H", "to": "Left"},
    {"from": "M00-Shift-G", "to": "Ctrl-End"}
  ]
}
```

**Validation**:
- Must contain at least one key name (after modifiers)
- All modifier names must be valid
- Key name must be defined in `keyboard.keys`
- Virtual modifiers (M00-MFF) must be defined in `virtualModifiers`

**Errors**:
```
Invalid 'from' key: 'Shift-' (missing key name)
Unknown 'from' key: 'Shift-Unknown' (key not defined)
Unknown virtual modifier in 'from': 'M99-A' (M99 not defined)
```

#### to

**Type**: `string` or `array of strings`
**Required**: Yes

The target action to perform. Can be:
- **Single key**: `"KeyName"` or `"Modifier-KeyName"`
- **Key sequence**: `["Key1", "Key2", ...]`

See [Key Sequences](#key-sequences) for details.

**Single Key Example**:
```json
{
  "mappings": [
    {"from": "A", "to": "Tab"},
    {"from": "CapsLock", "to": "Escape"},
    {"from": "M00-H", "to": "Left"},
    {"from": "M00-W", "to": "Ctrl-Right"}
  ]
}
```

**Key Sequence Example**:
```json
{
  "mappings": [
    {
      "from": "M00-G",
      "to": ["Ctrl-Home"]
    },
    {
      "from": "M00-Y",
      "to": ["Shift-End", "Ctrl-C"]
    },
    {
      "from": "M00-O",
      "to": ["End", "Enter", "Insert"]
    }
  ]
}
```

**Validation**:
- String: must be valid key specification
- Array: each element must be valid key specification
- All key names must be defined in `keyboard.keys`

**Errors**:
```
Unknown 'to' key: 'Tabulator' (not defined)
Unknown key in sequence: 'LeftArrow' (not defined, use 'Left')
```

---

## Modifier Syntax

Modifiers are specified using hyphen-separated format: `Modifier1-Modifier2-Key`

### Standard Modifiers

| Modifier | Example | Description |
|----------|---------|-------------|
| `Shift` | `Shift-A` | Shift modifier |
| `Ctrl` | `Ctrl-A` | Control modifier |
| `Alt` | `Alt-A` | Alt modifier |
| `Win` | `Win-A` | Windows/Super modifier |

### Virtual Modifiers

| Format | Example | Description |
|--------|---------|-------------|
| `M00-MFF` | `M00-A` | Virtual modifier 00-FF (hex) |

### Modifier Combinations

Modifiers can be combined in any order:

```json
{
  "mappings": [
    {"from": "Shift-Ctrl-A", "to": "F1"},
    {"from": "M00-Shift-G", "to": "Ctrl-End"},
    {"from": "Ctrl-Alt-Delete", "to": "F12"}
  ]
}
```

**Rules**:
- Modifier order doesn't matter: `Shift-Ctrl-A` equals `Ctrl-Shift-A`
- Virtual modifiers can combine with standard modifiers
- The last component is always the base key
- Use hyphen `-` as separator (not underscore or space)

**Examples**:

```json
{
  "from": "A"              // Simple key
  "from": "Shift-A"        // Shift + A
  "from": "Ctrl-Alt-A"     // Ctrl + Alt + A
  "from": "M00-H"          // M00 (CapsLock) + H
  "from": "M00-Shift-G"    // M00 + Shift + G
  "from": "Shift-M00-A"    // Shift + M00 + A (same as above)
}
```

---

## Key Sequences

Key sequences allow outputting multiple keys in order for a single input.

**Syntax**: Array of key specifications

```json
{
  "to": ["Key1", "Key2", "Key3"]
}
```

### Use Cases

#### 1. Vim-style Commands

```json
{
  "mappings": [
    {
      "comment": "M00-G: Jump to top of file (Ctrl-Home)",
      "from": "M00-G",
      "to": ["Ctrl-Home"]
    },
    {
      "comment": "M00-Y: Yank to end of line (Shift-End, Ctrl-C)",
      "from": "M00-Y",
      "to": ["Shift-End", "Ctrl-C"]
    }
  ]
}
```

#### 2. Multi-Step Actions

```json
{
  "mappings": [
    {
      "comment": "M00-O: Insert line below (End, Enter, Insert)",
      "from": "M00-O",
      "to": ["End", "Enter", "Insert"]
    },
    {
      "comment": "M00-I: Insert at line start (Home, Insert)",
      "from": "M00-I",
      "to": ["Home", "Insert"]
    }
  ]
}
```

#### 3. Copy/Paste Shortcuts

```json
{
  "mappings": [
    {
      "comment": "M00-X: Cut (Shift-Delete)",
      "from": "M00-X",
      "to": ["Shift-Delete"]
    },
    {
      "comment": "M00-P: Paste (Shift-Insert)",
      "from": "M00-P",
      "to": ["Shift-Insert"]
    }
  ]
}
```

### Timing

- Keys are output in array order
- Default timing between keys: system default
- Press-and-release for each key in sequence

### Modifiers in Sequences

Each element can include modifiers:

```json
{
  "to": ["Ctrl-Home", "Shift-End", "Ctrl-C"]
}
```

This outputs:
1. Ctrl-Home (jump to start)
2. Shift-End (select to end)
3. Ctrl-C (copy)

---

## Validation Rules

### Schema Validation

The configuration is validated on load:

1. **JSON Syntax**: File must be valid JSON
2. **Required Fields**: `version`, `keyboard`, `mappings` must be present
3. **Version Check**: `version` must be `"2.0"`
4. **Key Definitions**: All referenced keys must be defined in `keyboard.keys`
5. **Virtual Modifiers**: M00-MFF referenced in mappings must be defined
6. **Scan Codes**: Must match hex pattern `0x[0-9A-Fa-f]{1,4}`

### Error Messages

YAMY provides clear error messages with context:

```
JSON syntax error at byte 245: unexpected end of input
Missing required field: 'version'
Unsupported version: '1.0' (expected '2.0')
Invalid scan code for key 'A': '1e' (expected '0x' prefix)
Unknown key in mapping: 'UnknownKey'
Unknown virtual modifier: 'M99' (not defined in virtualModifiers)
```

### Warnings

Non-fatal issues that don't prevent loading:

```
Virtual modifier M01 defined but never used in mappings
Key 'F13' defined but never used in mappings
Duplicate key definition: 'A' (last definition used)
```

---

## Complete Examples

### Example 1: Simple Remapping

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "CapsLock": "0x3a",
      "Escape": "0x01"
    }
  },
  "mappings": [
    {
      "from": "CapsLock",
      "to": "Escape"
    }
  ]
}
```

**Effect**: Remap CapsLock to Escape

---

### Example 2: Vim Mode (Modal Editing)

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "H": "0x23",
      "J": "0x24",
      "K": "0x25",
      "L": "0x26",
      "CapsLock": "0x3a",
      "Escape": "0x01",
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

**Effect**:
- Hold CapsLock: Activates vim navigation mode (HJKL → arrows)
- Tap CapsLock: Outputs Escape

---

### Example 3: Emacs Mode (Meta Key)

```json
{
  "version": "2.0",
  "keyboard": {
    "keys": {
      "Semicolon": "0x27",
      "W": "0x11",
      "Y": "0x15"
    }
  },
  "virtualModifiers": {
    "M01": {
      "trigger": "Semicolon",
      "tapAction": "Semicolon"
    }
  },
  "mappings": [
    {
      "from": "M01-W",
      "to": "Ctrl-C"
    },
    {
      "from": "M01-Y",
      "to": "Ctrl-V"
    }
  ]
}
```

**Effect**:
- Hold Semicolon + W: Copy (Ctrl-C)
- Hold Semicolon + Y: Paste (Ctrl-V)
- Tap Semicolon: Outputs semicolon

---

## Common Patterns

### Pattern 1: Swap Keys

```json
{
  "mappings": [
    {"from": "CapsLock", "to": "LeftCtrl"},
    {"from": "LeftCtrl", "to": "CapsLock"}
  ]
}
```

### Pattern 2: Layer Key Navigation

```json
{
  "virtualModifiers": {
    "M00": {"trigger": "Space", "tapAction": "Space"}
  },
  "mappings": [
    {"from": "M00-H", "to": "Left"},
    {"from": "M00-J", "to": "Down"},
    {"from": "M00-K", "to": "Up"},
    {"from": "M00-L", "to": "Right"}
  ]
}
```

### Pattern 3: Home Row Mods (Advanced)

```json
{
  "virtualModifiers": {
    "M00": {"trigger": "F", "tapAction": "F"},
    "M01": {"trigger": "D", "tapAction": "D"},
    "M02": {"trigger": "S", "tapAction": "S"},
    "M03": {"trigger": "A", "tapAction": "A"}
  },
  "mappings": [
    {"from": "M00-J", "to": "Ctrl-J"},
    {"from": "M01-K", "to": "Alt-K"},
    {"from": "M02-L", "to": "Shift-L"}
  ]
}
```

### Pattern 4: Application Shortcuts

```json
{
  "mappings": [
    {"from": "Win-1", "to": "Ctrl-Alt-1"},
    {"from": "Win-2", "to": "Ctrl-Alt-2"},
    {"from": "Win-3", "to": "Ctrl-Alt-3"}
  ]
}
```

### Pattern 5: Text Editing Shortcuts

```json
{
  "virtualModifiers": {
    "M00": {"trigger": "CapsLock", "tapAction": "Escape"}
  },
  "mappings": [
    {"from": "M00-U", "to": "Ctrl-Z"},
    {"from": "M00-Ctrl-R", "to": "Ctrl-Y"},
    {"from": "M00-D", "to": "Delete"},
    {"from": "M00-X", "to": ["Shift-Delete"]},
    {"from": "M00-P", "to": ["Shift-Insert"]}
  ]
}
```

---

## Performance Notes

- **Load Time**: JSON configs load in <10ms
- **Parse Overhead**: Minimal (nlohmann/json is highly optimized)
- **Memory Usage**: <10MB for typical configs
- **Event Latency**: No performance impact on key events

---

## Best Practices

1. **Key Names**: Use descriptive, consistent names (PascalCase recommended)
2. **Comments**: JSON doesn't support comments, but you can add `"_comment"` fields
3. **Organization**: Group related mappings together
4. **Virtual Modifiers**: Use M00-M0F for frequently used layers
5. **Testing**: Test tap vs. hold threshold to find comfortable timing

---

## Migration from .mayu

If you're migrating from .mayu format, see the [Migration Guide](migration-guide.md) for detailed conversion instructions.

Key differences:
- JSON syntax instead of text format
- No per-window keymaps (global only)
- Simplified modifier syntax
- Array notation for key sequences

---

## See Also

- [Migration Guide](migration-guide.md) - Converting from .mayu to JSON
- [Example Configurations](../keymaps/) - Ready-to-use configs
- JSON Schema File: [config.schema.json](../schema/config.schema.json)

---

**Document Version**: 1.0
**Last Updated**: 2025-12-18
**YAMY Version**: 2.0+
