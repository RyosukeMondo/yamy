# Number Modifier Syntax Documentation

## Overview

The `.mayu` parser has been extended to support a new `def numbermod` syntax that allows mapping number keys to hardware modifier keys. This feature is part of the number-to-modifier mapping system designed for small keyboard users.

## Syntax

```
def numbermod <number_key> = <modifier_key>
```

Where:
- `<number_key>`: A number key from `*_1` to `*_0` (the number row keys)
- `<modifier_key>`: A valid hardware modifier key

### Valid Modifier Keys

The following hardware modifier keys are supported:
- `*LShift` - Left Shift
- `*RShift` - Right Shift
- `*LCtrl` - Left Control
- `*RCtrl` - Right Control
- `*LAlt` - Left Alt
- `*RAlt` - Right Alt
- `*LWin` - Left Windows/Super
- `*RWin` - Right Windows/Super

## Examples

### Basic Usage

```mayu
# Map number 1 to Left Shift
def numbermod *_1 = *LShift

# Map number 2 to Right Shift
def numbermod *_2 = *RShift

# Map number 3 to Left Control
def numbermod *_3 = *LCtrl

# Map number 4 to Right Control
def numbermod *_4 = *RCtrl

# Map number 5 to Left Alt
def numbermod *_5 = *LAlt

# Map number 6 to Right Alt
def numbermod *_6 = *RAlt

# Map number 7 to Left Windows
def numbermod *_7 = *LWin

# Map number 8 to Right Windows
def numbermod *_8 = *RWin
```

### Complete Configuration Example

```mayu
# Number modifier mappings for small keyboards
# This allows using number keys as modifiers when held

def numbermod *_1 = *LShift
def numbermod *_2 = *RShift
def numbermod *_3 = *LCtrl
def numbermod *_4 = *RCtrl
def numbermod *_5 = *LAlt
def numbermod *_6 = *RAlt
def numbermod *_7 = *LWin
def numbermod *_8 = *RWin

# You can still use regular key substitutions
*_9 = *ESC
*_0 = *BackSpace
```

## How It Works

### Parser Integration

The `.mayu` parser recognizes the `def numbermod` keyword and:

1. **Parses the number key name**: Validates that it's a valid key (e.g., `*_1`)
2. **Parses the modifier key name**: Validates that it's a valid hardware modifier
3. **Registers the mapping**: Stores the number-to-modifier mapping in the keyboard configuration
4. **Reports errors**: Provides clear error messages if the syntax is invalid

### Implementation Details

The parser implementation consists of:

1. **Token parsing** (`setting_loader.cpp:240-286`):
   - Tokenizes the line: `def numbermod *_1 = *LShift`
   - Validates the `=` separator
   - Validates the modifier key against a whitelist
   - Creates a `NumberModifier` entry in the keyboard configuration

2. **Parser integration** (`setting_loader.cpp:373`):
   ```cpp
   else if (*t == "numbermod") load_DEFINE_NUMBER_MODIFIER();
   ```

3. **Keyboard storage** (`keyboard.h:383-392`, `keyboard.cpp:324-327`):
   - `NumberModifier` class stores the mapping
   - `addNumberModifier()` method adds entries to the list
   - `getNumberModifiers()` method retrieves the list for processing

4. **Engine integration** (`engine_setting.cpp:261-299`):
   - Reads number modifiers from keyboard configuration
   - Converts key names to YAMY scan codes
   - Registers mappings with the `EventProcessor` via `registerNumberModifier()`
   - Logs registration for debugging

## Error Handling

The parser provides clear error messages for common mistakes:

### Invalid Number Key

```mayu
def numbermod *A = *LShift  # Error: 'A' is not a valid number key
```

Error: `'*A': invalid number key name.`

### Missing Equals Sign

```mayu
def numbermod *_1 *LShift  # Error: missing '='
```

Error: `there must be '=' after number key name in 'def numbermod'.`

### Invalid Modifier Key

```mayu
def numbermod *_1 = *A  # Error: 'A' is not a valid modifier
```

Error: `'*A': invalid modifier key. Valid modifiers: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin.`

### Unknown Key Name

```mayu
def numbermod *_99 = *LShift  # Error: '_99' doesn't exist
```

Error: `'*_99': invalid number key name.`

## Backward Compatibility

The new syntax is **fully backward compatible** with existing `.mayu` files:

- Files without `def numbermod` work exactly as before
- Regular key substitutions still work normally
- Number keys can still be remapped using regular substitution syntax
- The parser gracefully handles mixed configurations

### Example: Mixed Configuration

```mayu
# Old syntax (still works)
*_1 = *F1
*_2 = *F2

# New syntax (number modifiers)
def numbermod *_3 = *LCtrl
def numbermod *_4 = *RCtrl

# Both can coexist in the same file
```

## Integration with Hold/Tap Detection

When number modifiers are registered, they are processed by the `ModifierKeyHandler` with hold/tap detection:

- **HOLD** (>200ms): Activates the modifier (e.g., holding `1` = `LShift`)
- **TAP** (<200ms): Performs normal substitution (if configured)

This allows number keys to serve dual purposes:
- Quick tap: Normal key function
- Long hold: Modifier key function

See `docs/NUMBER_MODIFIER_DESIGN.md` for complete hold/tap detection details.

## Debugging

Enable debug logging to see number modifier registration:

```bash
export YAMY_DEBUG_KEYCODE=1
./yamy
```

Debug output shows:
```
Number Modifier: 0x0002 → 0x002a  # _1 → LShift
Number Modifier: 0x0003 → 0x0036  # _2 → RShift
Registered 2 number modifiers
```

## File Locations

- **Parser implementation**: `src/core/settings/setting_loader.cpp` (lines 239-286)
- **Parser integration**: `src/core/settings/setting_loader.cpp` (line 373)
- **Keyboard storage**: `src/core/input/keyboard.h` (lines 383-392), `keyboard.cpp` (lines 324-327)
- **Engine integration**: `src/core/engine/engine_setting.cpp` (lines 261-299)
- **Header declaration**: `src/core/settings/setting_loader.h` (line 87)

## Testing

Test your number modifier configuration:

1. Create a test `.mayu` file with `def numbermod` entries
2. Load the configuration in YAMY
3. Check the logs for "Number Modifier: ..." messages
4. Test hold/tap behavior (once ModifierKeyHandler is fully integrated)

## See Also

- `docs/NUMBER_MODIFIER_DESIGN.md` - Complete design specification
- `docs/NUMBER_MODIFIER_USER_GUIDE.md` - User-facing guide (to be created in task 4.7)
- Task 4.1: Design number-to-modifier mapping system
- Task 4.2: Implement ModifierKeyHandler class
- Task 4.3: Create number-to-modifier mapping table
- Task 4.4: Integrate ModifierKeyHandler into EventProcessor
- **Task 4.5**: Extend .mayu parser to support number modifier syntax (this document)
