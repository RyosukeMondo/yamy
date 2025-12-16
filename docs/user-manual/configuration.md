# YAMY Configuration Guide

Complete reference for the `.mayu` configuration file syntax.

---

## Table of Contents

1. [Overview](#overview)
2. [Basic Syntax](#basic-syntax)
   - [Comments](#comments)
   - [File Structure](#file-structure)
3. [Key Names](#key-names)
   - [Alphabetic Keys](#alphabetic-keys)
   - [Number Keys](#number-keys)
   - [Function Keys](#function-keys)
   - [Modifier Keys](#modifier-keys)
   - [Navigation Keys](#navigation-keys)
   - [Special Keys](#special-keys)
   - [Numpad Keys](#numpad-keys)
   - [Mouse Buttons](#mouse-buttons)
4. [Modifier Prefixes](#modifier-prefixes)
5. [Key Remapping](#key-remapping)
   - [Simple Key Mapping](#simple-key-mapping)
   - [Mapping with Modifiers](#mapping-with-modifiers)
   - [Key Sequences](#key-sequences)
6. [Modifier Configuration](#modifier-configuration)
7. [Keymaps](#keymaps)
   - [Defining Keymaps](#defining-keymaps)
   - [Keymap Inheritance](#keymap-inheritance)
   - [Prefix Keymaps](#prefix-keymaps)
8. [Window Matching](#window-matching)
   - [Window Directive Syntax](#window-directive-syntax)
   - [Regex Patterns](#regex-patterns)
9. [Built-in Functions](#built-in-functions)
   - [Window Functions](#window-functions)
   - [Mouse Functions](#mouse-functions)
   - [System Functions](#system-functions)
   - [Dialog Functions](#dialog-functions)
10. [Conditionals and Defines](#conditionals-and-defines)
11. [Include Directive](#include-directive)
12. [Advanced Features](#advanced-features)
13. [Example Configurations](#example-configurations)

---

## Overview

YAMY configuration files (`.mayu`) define keyboard remappings using a declarative syntax. The configuration supports:

- Simple key-to-key remappings
- Modifier key reassignment
- Application-specific keymaps
- Built-in functions for window management and more
- Conditional compilation with `if`/`endif`
- File includes for modular configurations

---

## Basic Syntax

### Comments

Lines starting with `#` are comments:

```mayu
# This is a comment
key C-A = Home  # This is an inline comment
```

### File Structure

A typical configuration file has this structure:

```mayu
# 1. Includes (optional)
include "104.mayu"

# 2. Defines (optional)
define MY_OPTION

# 3. Global keymap
keymap Global
  # key definitions...

# 4. Application-specific keymaps
window AppName /pattern/ : ParentKeymap
  # app-specific keys...
```

---

## Key Names

### Alphabetic Keys

Standard letter keys: `A` through `Z`

```mayu
key A = B       # A becomes B
key C-A = Home  # Ctrl+A becomes Home
```

### Number Keys

Number row keys use underscore prefix:

| Key | Name |
|-----|------|
| 1 | `_1` |
| 2 | `_2` |
| 3 | `_3` |
| 4 | `_4` |
| 5 | `_5` |
| 6 | `_6` |
| 7 | `_7` |
| 8 | `_8` |
| 9 | `_9` |
| 0 | `_0` |

```mayu
key C-_1 = F1   # Ctrl+1 becomes F1
```

### Function Keys

`F1` through `F12`

```mayu
key F12 = &LoadSetting  # F12 reloads config
```

### Modifier Keys

| Key | Names |
|-----|-------|
| Left Control | `LControl`, `LCtrl` |
| Right Control | `RControl`, `RCtrl` |
| Left Shift | `LShift` |
| Right Shift | `RShift` |
| Left Alt | `LAlt`, `LMenu` |
| Right Alt | `RAlt`, `RMenu` |
| Left Windows | `LWindows`, `LWin` |
| Right Windows | `RWindows`, `RWin` |
| CapsLock | `CapsLock`, `Capital`, `Caps` |

### Navigation Keys

| Key | Names |
|-----|-------|
| Up Arrow | `Up` |
| Down Arrow | `Down` |
| Left Arrow | `Left` |
| Right Arrow | `Right` |
| Home | `Home` |
| End | `End` |
| Page Up | `PageUp`, `Prior` |
| Page Down | `PageDown`, `Next` |
| Insert | `Insert` |
| Delete | `Delete`, `Del` |

### Special Keys

| Key | Names |
|-----|-------|
| Space | `Space` |
| Enter | `Enter`, `Return` |
| Tab | `Tab` |
| Backspace | `BackSpace`, `BS`, `Back` |
| Escape | `Escape`, `Esc` |
| Print Screen | `PrintScreen`, `Snapshot` |
| Scroll Lock | `ScrollLock`, `Scroll` |
| Pause | `Pause` |
| Break | `Break` |
| Applications | `Applications`, `Apps` |

### Punctuation Keys

| Key | Names |
|-----|-------|
| ` | `GraveAccent`, `BackQuote` |
| - | `HyphenMinus`, `Hyphen`, `Minus` |
| = | `EqualsSign`, `Equal` |
| [ | `LeftSquareBracket`, `OpenBracket` |
| ] | `RightSquareBracket`, `CloseBracket` |
| \ | `ReverseSolidus`, `BackSlash` |
| ; | `Semicolon` |
| ' | `Apostrophe`, `Quote` |
| , | `Comma` |
| . | `FullStop`, `Period` |
| / | `Solidus`, `Slash` |

### Numpad Keys

| Key | Names |
|-----|-------|
| Numpad 0-9 | `Num0` through `Num9` |
| Numpad + | `NumPlusSign`, `NumPlus` |
| Numpad - | `NumHyphenMinus`, `NumMinus` |
| Numpad * | `NumAsterisk`, `NumMultiply` |
| Numpad / | `NumSolidus`, `NumSlash` |
| Numpad . | `NumFullStop`, `NumPeriod` |
| Numpad Enter | `NumEnter`, `NumReturn` |
| Num Lock | `NumLock` |

### Mouse Buttons

| Button | Name |
|--------|------|
| Left Button | `LButton` |
| Right Button | `RButton` |
| Middle Button | `MButton` |
| Scroll Forward | `WheelForward` |
| Scroll Backward | `WheelBackward` |
| Extra Button 1 | `XButton1` |
| Extra Button 2 | `XButton2` |

---

## Modifier Prefixes

Modifier prefixes are added before key names:

| Prefix | Modifier | Alternative |
|--------|----------|-------------|
| `C-` | Control | |
| `S-` | Shift | |
| `A-` | Alt | `M-` (Meta) |
| `W-` | Windows/Super | |

### Combining Modifiers

Combine multiple modifiers by chaining prefixes:

```mayu
key C-A = Home           # Ctrl+A
key C-S-A = C-Home       # Ctrl+Shift+A -> Ctrl+Home
key C-A-S-Delete = &Exit # Ctrl+Alt+Shift+Delete
```

### Special Modifier Prefixes

| Prefix | Meaning |
|--------|---------|
| `*` | Match any state of the modifier |
| `~` | Ensure modifier is NOT pressed |
| `!` | "One-shot" modifier (release after key) |

```mayu
key *C-A = Home          # Matches C-A regardless of other modifiers
key ~S-Space = Space     # Only when Shift is NOT held
```

### IME State Prefixes

| Prefix | Meaning |
|--------|---------|
| `IC-` | IME conversion mode active |
| `IL-` | IME input mode active |

---

## Key Remapping

### Simple Key Mapping

The basic syntax maps one key to another:

```mayu
key SourceKey = TargetKey
```

Examples:

```mayu
key CapsLock = LControl  # CapsLock becomes Control
key A = B                # A becomes B
```

### Mapping with Modifiers

Include modifier prefixes in the mapping:

```mayu
key C-A = Home           # Ctrl+A -> Home
key A-F4 = &WindowClose  # Alt+F4 -> Close window (function)
```

**Passing through modifiers:**

Use `*` prefix to pass through the modifier state:

```mayu
key *CapsLock = *LControl  # CapsLock acts as Control key
```

### Key Sequences

Map a key to multiple keys or a sequence:

```mayu
key C-K = S-End Delete   # Ctrl+K: Select to end of line, then delete
```

### Keyseq Directive

Define reusable key sequences:

```mayu
keyseq $WindowClose = A-F4
key C-S-Q = $WindowClose  # Use the defined sequence
```

---

## Modifier Configuration

### Adding Modifier Capability

Make a key act as a modifier:

```mayu
mod control += CapsLock   # CapsLock also acts as Control
```

### Modifier Syntax

```mayu
mod <modifier> += <key>   # Add key as modifier
mod <modifier> -= <key>   # Remove key from modifier
mod <modifier> = <keys>   # Set exactly which keys are this modifier
```

### Custom Modifiers

Define custom modifier groups (mod0 through mod9):

```mayu
mod mod0 = !!CapsLock     # Custom modifier when CapsLock is held
```

The `!!` prefix means the key acts as a modifier while held, not just pressed.

---

## Keymaps

### Defining Keymaps

Keymaps group related key definitions:

```mayu
keymap Global
  key C-A = Home
  key C-E = End

keymap MyKeymap
  key C-S = C-F   # Search
```

The `Global` keymap applies everywhere unless overridden.

### Keymap Inheritance

Keymaps can inherit from parent keymaps:

```mayu
keymap EmacsMove : Global
  key C-F = Right
  key C-B = Left

keymap EmacsEdit : EmacsMove
  key C-D = Delete
  key C-K = S-End Delete
```

### Prefix Keymaps

Create keymaps activated by a prefix key:

```mayu
keymap2 EmacsC-X : EmacsEdit
  event prefixed = &HelpMessage("C-x", "C-x prefix active")
  key C-S = C-S              # C-x C-s -> Save
  key C-F = C-O              # C-x C-f -> Open
  key C-C = A-F4             # C-x C-c -> Exit

keymap EmacsEdit
  key C-X = &Prefix(EmacsC-X)  # C-x activates prefix keymap
```

---

## Window Matching

### Window Directive Syntax

Create application-specific keymaps:

```mayu
window <name> /<regex-pattern>/ : <parent-keymap>
  # key definitions for this window
```

Example:

```mayu
window Firefox /:firefox:/ : Global
  key C-L = F6   # Focus address bar
```

### Regex Patterns

Patterns match against: `<executable>:<window-class>:<window-title>`

```mayu
# Match by executable name
window Terminal /gnome-terminal/ : Global

# Match by window class
window Firefox /:Navigator:/ : Global

# Match by title
window VSCode /Visual Studio Code$/ : Global

# Multiple conditions (AND)
window NotepadEdit /notepad\.exe:.*:Edit$/ : EmacsEdit
```

### Pattern Operators

| Operator | Meaning |
|----------|---------|
| `.` | Any character |
| `*` | Zero or more of previous |
| `+` | One or more of previous |
| `?` | Zero or one of previous |
| `^` | Start of string |
| `$` | End of string |
| `\` | Escape special character |
| `[...]` | Character class |

### Combining Patterns

Use `&&` to require multiple patterns:

```mayu
window Dialog ( /:Dialog:/ && /Save/ ) : Global
  # Matches windows with class "Dialog" AND title containing "Save"
```

---

## Built-in Functions

Functions start with `&` and perform actions.

### Window Functions

| Function | Description |
|----------|-------------|
| `&WindowClose` | Close the current window |
| `&WindowMaximize` | Maximize window |
| `&WindowMinimize` | Minimize window |
| `&WindowRestore` | Restore window from maximized/minimized |
| `&WindowVMaximize` | Maximize vertically only |
| `&WindowHMaximize` | Maximize horizontally only |
| `&WindowMove(x, y)` | Move window by offset |
| `&WindowMoveVisibly` | Move window to visible area |
| `&WindowRaise` | Bring window to front |
| `&WindowLower` | Send window to back |
| `&WindowToggleTopMost` | Toggle always-on-top |
| `&WindowClingToLeft` | Snap window to left edge |
| `&WindowClingToRight` | Snap window to right edge |
| `&WindowClingToTop` | Snap window to top edge |
| `&WindowClingToBottom` | Snap window to bottom edge |
| `&WindowIdentify` | Show window info in log |
| `&WindowRedraw` | Force window redraw |
| `&WindowSetAlpha(n)` | Set window transparency (0-100, -1 to disable) |

Example:

```mayu
key C-S-Left  = &WindowMove(-16, 0)
key C-S-Right = &WindowMove(16, 0)
key C-S-Z     = &WindowMaximize
key C-S-I     = &WindowMinimize
```

### Mouse Functions

| Function | Description |
|----------|-------------|
| `&MouseMove(x, y)` | Move mouse cursor by offset |
| `&VK(button)` | Simulate virtual key/mouse button |

Example:

```mayu
key W-Left  = &MouseMove(-16, 0)
key W-Right = &MouseMove(16, 0)
```

### System Functions

| Function | Description |
|----------|-------------|
| `&LoadSetting` | Reload configuration file |
| `&Default` | Pass key through unmodified |
| `&Ignore` | Ignore the key (do nothing) |
| `&Undefined` | Mark key as undefined (cancels prefix) |
| `&Prefix(keymap)` | Enter prefix keymap mode |
| `&KeymapParent` | Switch to parent keymap |
| `&KeymapWindow` | Switch to window's default keymap |
| `&Wait(ms)` | Wait for milliseconds |
| `&Sync` | Synchronize key processing |
| `&Repeat(action, count)` | Repeat action n times |
| `&PostMessage(...)` | Send Windows message |
| `&ShellExecute(...)` | Execute shell command |
| `&SetImeStatus(n)` | Set IME state |
| `&SetImeString(str)` | Send string to IME |

Example:

```mayu
key C-S-S = &LoadSetting            # Reload config
key C-Q = &Prefix(KeymapDefault)    # Enter default keymap
```

### Dialog Functions

| Function | Description |
|----------|-------------|
| `&HelpMessage(title, text)` | Show help message |
| `&MayuDialog(dialog, action)` | Control YAMY dialogs |
| `&InvestigateCommand` | Start command investigation |

Example:

```mayu
event prefixed = &HelpMessage("Prefix", "Waiting for next key...")
```

---

## Conditionals and Defines

### Define Directive

Set compilation flags:

```mayu
define MY_OPTION
define KBD104
```

### If/Endif Blocks

Conditionally include configuration:

```mayu
if ( KBD104 )
  # US keyboard layout
  key CapsLock = LControl
endif

if ( KBD109 )
  # Japanese keyboard layout
  key 英数 = LControl
endif

if ( !MY_OPTION )
  # Runs if MY_OPTION is NOT defined
endif
```

### Logical Operators

| Operator | Meaning |
|----------|---------|
| `( condition )` | Grouping |
| `!condition` | NOT |
| `cond1 and cond2` | AND |
| `cond1 or cond2` | OR |

Example:

```mayu
if ( KBD104 ) and ( EMACS_MODE )
  include "emacs-104.mayu"
endif
```

---

## Include Directive

Include other configuration files:

```mayu
include "path/to/file.mayu"
include "emacsedit.mayu"
```

Include paths are relative to the current file or YAMY's keymap directory.

---

## Advanced Features

### Event Handlers

Handle keymap events:

```mayu
keymap2 MyPrefix : Global
  event prefixed = &HelpMessage("Prefix", "Active")
  event before-key-down = &HelpMessage
```

### Key Alias

Define key aliases:

```mayu
def alias ↑ = Up
def alias ↓ = Down
def alias ← = Left
def alias → = Right
```

### Key Substitution

Replace keys at scan code level:

```mayu
def subst *CapsLock = *LControl
```

### Options

Set global options:

```mayu
def option mouse-event = true
def option drag-threshold = 30
```

### Variables

Use variables for repeat counts:

```mayu
key C-U = &Variable(0, 4) &Prefix(RepeatKeymap)
```

---

## Example Configurations

### Minimal Configuration

```mayu
# CapsLock as Control
keymap Global
mod control += CapsLock
key *CapsLock = *LControl
```

### Emacs-Style Navigation

```mayu
keymap EmacsMove : Global
key C-F = Right
key C-B = Left
key C-N = Down
key C-P = Up
key C-A = Home
key C-E = End
key C-V = Next
key M-V = Prior
key C-G = Escape

keymap EmacsEdit : EmacsMove
key C-D = Delete
key C-H = BackSpace
key C-K = S-End Delete
key C-W = C-X
key M-W = C-C
key C-Y = C-V

window EditControl /:(Edit|RichEdit)$/ : EmacsEdit
```

### Vim-Style Modal Editing

```mayu
keymap Global
key CapsLock = Escape

keymap2 VimNormal : Global
event prefixed = &HelpMessage("VIM", "-- NORMAL --")
key H = Left
key J = Down
key K = Up
key L = Right
key W = C-Right
key B = C-Left
key _0 = Home
key S-_4 = End
key I = &Undefined
key A = Right &Undefined

keymap Global
key Escape = &Prefix(VimNormal)
```

### Application-Specific

```mayu
# Firefox
window Firefox /:firefox:/ : Global
key C-L = F6
key C-K = C-E

# Terminal - pass through Ctrl keys
window Terminal /:gnome-terminal:/ : Global
key C-A = &Default
key C-E = &Default
key C-K = &Default

# VS Code
window VSCode /code:/ : Global
key C-S-P = F1
```

### Complete Configuration Template

```mayu
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# My YAMY Configuration
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Keyboard type
if ( !KBD109 ) and ( !KBD104 )
  include "104.mayu"
endif

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Global Settings
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

# CapsLock as Control
mod control += CapsLock
key *CapsLock = *LControl

# Window management
key C-S-Left   = &WindowMove(-16, 0)
key C-S-Right  = &WindowMove(16, 0)
key C-S-Up     = &WindowMove(0, -16)
key C-S-Down   = &WindowMove(0, 16)
key C-S-Z      = &WindowMaximize
key C-S-I      = &WindowMinimize

# Utility
key C-S-S = &LoadSetting
key C-S-D = &WindowIdentify

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Emacs-style editing
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap EmacsMove : Global
key C-F = Right
key C-B = Left
key C-N = Down
key C-P = Up
key C-A = Home
key C-E = End
key C-G = Escape

keymap EmacsEdit : EmacsMove
key C-D = Delete
key C-H = BackSpace
key C-K = S-End Delete

window EditControl /:(Edit|RichEdit(20[AW])?)$/ : EmacsEdit

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Application-specific
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Terminal - don't override standard shortcuts
window Terminal /:(gnome-terminal|konsole|xterm):/ : Global
key C-A = &Default
key C-E = &Default

# Dialog boxes - Cancel with C-g
window DialogBox /:#32770:/ : Global
key C-G = Escape
```

---

## See Also

- [User Guide](user-guide.md) - Getting started with YAMY
- [Troubleshooting](troubleshooting.md) - Common issues and solutions
- Example configurations in `keymaps/` directory
