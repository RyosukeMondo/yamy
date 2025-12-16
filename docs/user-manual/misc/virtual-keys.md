# Virtual Key System Documentation

## Overview

The Virtual Key System provides a powerful way to create complex keyboard layouts using three types of virtual keys:

- **Virtual Regular Keys (V_*)**: Virtual versions of physical keys
- **Modal Modifiers (M00-MFF)**: 256 hold-to-activate layer switches with tap/hold behavior
- **Lock Keys (L00-LFF)**: 256 toggle-able persistent mode switches

This system eliminates ambiguity between physical and virtual keys, supports unlimited layers, and provides visual feedback for active modes.

## Quick Start

### Space Cadet Example

Create a dual-function Space key that outputs Space on tap but acts as a modifier when held:

```mayu
# Make Space key into M00 modifier
def subst *Space = *M00

# Configure M00 to output Space on quick tap
mod assign M00 = *Space

# Add Vim-style navigation when holding Space
key M00-H = *Left
key M00-J = *Down
key M00-K = *Up
key M00-L = *Right
```

**How it works**:
- Quick tap Space (<200ms) → outputs Space
- Hold Space + H/J/K/L → navigation arrows

## Virtual Key Types

### 1. Virtual Regular Keys (V_*)

Virtual regular keys distinguish between physical key presses and keys generated through substitution.

#### Syntax

```mayu
V_KeyName
```

Examples: `V_A`, `V_B`, `V_Enter`, `V_Space`

#### Use Case: Physical vs Virtual Distinction

```mayu
# Substitute physical B to virtual V_B
def subst *B = *V_B

# Different behavior for physical B vs virtual V_B
key M00-B = *C        # Physical B with M00 → C
key M00-V_B = *D      # Virtual V_B with M00 → D
```

**Result**:
- Press M00 + physical B → outputs C
- Press M00 + A (if A→V_B is defined) → outputs D

#### Visual Diagram

```
Physical Keyboard:    [A] [B] [C]
                       |   |
Substitution Layer:    |   └──> V_B
                       |
                       └──> V_A

Keymap Layer:
  - Physical B recognized as B
  - Substituted B recognized as V_B
  - Different mappings possible for each
```

### 2. Modal Modifiers (M00-MFF)

Modal modifiers create temporary layers that activate while held. Supports 256 modifiers (M00 through MFF in hexadecimal).

#### Syntax

```mayu
M00, M01, M02, ..., M0F, M10, M11, ..., MFF
```

#### Basic Modifier Definition

```mayu
# Substitute a key to become a modifier
def subst *CapsLock = *M00

# Define mappings that activate when M00 is held
key M00-A = *Home
key M00-E = *End
key M00-F = *PageDown
key M00-B = *PageUp
```

#### Tap/Hold Behavior

Configure what happens on quick tap vs hold:

```mayu
# Make B into modifier M00
def subst *B = *M00

# Set tap action (outputs Enter on quick tap <200ms)
mod assign M00 = *Enter

# Hold behavior automatically activates M00 modifier
key M00-H = *Left
key M00-J = *Down
```

**Behavior**:
- **Tap** (<200ms): Outputs Enter
- **Hold** (≥200ms): Activates M00, no output
- **Hold + Key**: Activates M00 mapping

#### Multiple Modifier Combinations

Combine multiple modifiers for complex layouts:

```mayu
# Define two modifiers
def subst *F = *M00      # Base navigation layer
def subst *D = *M01      # Extended layer

# Single modifier mappings
key M00-H = *Left
key M00-L = *Right

# Combo mapping (both M00 and M01 active)
key M00-M01-H = *Home
key M00-M01-L = *End
```

**Specificity Rule**: More specific mappings (more modifiers) take priority.

- Press M00 + H → outputs Left
- Press M00 + M01 + H → outputs Home (more specific wins)

#### Visual Diagram

```
┌─────────────────────────────────────┐
│ Normal Layer                         │
│   A→A  B→B  C→C  H→H                │
└─────────────────────────────────────┘
           │
           │ Hold M00 (e.g., Space)
           ↓
┌─────────────────────────────────────┐
│ M00 Layer                            │
│   A→A  B→B  C→C  H→Left             │
└─────────────────────────────────────┘
           │
           │ Also Hold M01 (e.g., D)
           ↓
┌─────────────────────────────────────┐
│ M00-M01 Layer (most specific)       │
│   A→A  B→B  C→C  H→Home             │
└─────────────────────────────────────┘
```

### 3. Lock Keys (L00-LFF)

Lock keys are toggle switches that maintain state until toggled again. Supports 256 locks (L00 through LFF).

#### Syntax

```mayu
L00, L01, L02, ..., L0F, L10, L11, ..., LFF
```

#### Basic Lock Definition

```mayu
# Substitute CapsLock to L00
def subst *CapsLock = *L00

# Define mappings active when L00 is toggled on
key L00-A = *Home
key L00-S = *End
```

**Behavior**:
- First press CapsLock → toggles L00 ON
- Press A → outputs Home (while L00 is on)
- Press CapsLock again → toggles L00 OFF
- Press A → outputs A (normal)

#### Lock Combinations

Combine multiple locks for "treasure hunting" (combinatorial modes):

```mayu
# Two lock keys
def subst *CapsLock = *L00
def subst *ScrollLock = *L01

# Single lock mappings
key L00-A = *1
key L01-A = *2

# Combo lock mapping (both active)
key L00-L01-A = *3
```

**Result**:
- L00 on, press A → outputs 1
- L01 on, press A → outputs 2
- Both on, press A → outputs 3 (most specific wins)

#### Visual Lock Indicators

Lock states are shown in the GUI with colored indicators:

```
GUI Lock Indicator:
┌─────────────────┐
│ Active Locks:    │
│ L00 [●] ← green │
│ L01 [○] ← gray  │
└─────────────────┘
```

- Green (●): Lock active
- Gray (○): Lock inactive

Lock indicators update in real-time when locks are toggled.

#### Visual Diagram

```
State Diagram:

         Press L00
OFF ────────────────> ON
         ↑            │
         │            │
         └────────────┘
         Press L00

Lock State Persistence:

Time →   t0    t1    t2    t3    t4    t5
L00:     OFF   ON    ON    ON    OFF   OFF
         ↑     ↑                 ↑
      Press  [Stays active]  Press again
```

## Common Patterns

### Pattern 1: Space Cadet (Dual-Function Keys)

Make any key output one thing on tap, activate layer on hold:

```mayu
# Space acts as navigation layer
def subst *Space = *M00
mod assign M00 = *Space

key M00-H = *Left
key M00-J = *Down
key M00-K = *Up
key M00-L = *Right

# Enter acts as symbol layer
def subst *Enter = *M01
mod assign M01 = *Enter

key M01-H = *Home
key M01-L = *End
```

### Pattern 2: Vim Mode with Locks

Create a persistent Vim-like navigation mode:

```mayu
# Toggle Vim mode with CapsLock
def subst *CapsLock = *L00

# Vim navigation (active when L00 is on)
key L00-H = *Left
key L00-J = *Down
key L00-K = *Up
key L00-L = *Right
key L00-W = *C-Right    # Word forward
key L00-B = *C-Left     # Word backward

# Vim editing
key L00-D = *Delete
key L00-X = *Delete
key L00-I = *L00        # Toggle vim mode off (press CapsLock)
```

### Pattern 3: Multi-Layer Navigation

Create base and extended navigation layers:

```mayu
# F key = base navigation
def subst *F = *M00

# D key = extended navigation
def subst *D = *M01

# Base navigation (M00)
key M00-H = *Left
key M00-J = *Down
key M00-K = *Up
key M00-L = *Right

# Extended navigation (M01)
key M01-H = *Home
key M01-J = *PageDown
key M01-K = *PageUp
key M01-L = *End

# Combo navigation (M00 + M01)
key M00-M01-H = *C-Home    # Start of document
key M00-M01-L = *C-End     # End of document
```

### Pattern 4: Symbol Layers

Create easy access to symbols:

```mayu
# Semicolon as symbol layer
def subst *Semicolon = *M00
mod assign M00 = *Semicolon

# Number row becomes symbols
key M00-1 = *S-1    # !
key M00-2 = *S-2    # @
key M00-3 = *S-3    # #
key M00-4 = *S-4    # $
key M00-5 = *S-5    # %

# Home row becomes brackets
key M00-A = *LeftBracket      # [
key M00-S = *RightBracket     # ]
key M00-D = *S-9              # (
key M00-F = *S-0              # )
```

### Pattern 5: Gaming Layers with Locks

Create game-specific layouts:

```mayu
# ScrollLock toggles gaming mode
def subst *ScrollLock = *L00

# Gaming WASD layout (L00 active)
key L00-W = *Up
key L00-A = *Left
key L00-S = *Down
key L00-D = *Right

# Skill shortcuts
key L00-1 = *F1
key L00-2 = *F2
key L00-3 = *F3
```

## Hexadecimal Notation

Modifiers and locks use hexadecimal notation (00-FF) to support 256 of each:

```
Decimal  Hex
0        00
1        01
...
10       0A
11       0B
...
15       0F
16       10
...
255      FF
```

Examples:
- M00 = Modifier 0
- M0A = Modifier 10
- M10 = Modifier 16
- MFF = Modifier 255

## Specificity and Priority

When multiple keymap entries could match, the most specific one wins:

```mayu
key A = *1              # Specificity: 0 (no modifiers/locks)
key M00-A = *2          # Specificity: 1
key M00-M01-A = *3      # Specificity: 2
key L00-M00-A = *4      # Specificity: 2
key L00-L01-M00-A = *5  # Specificity: 3 (most specific)
```

**Specificity = number of required modifiers + number of required locks**

The system always matches the entry with the highest specificity.

## Troubleshooting

### Modifier Not Activating

**Problem**: Key mapped as modifier doesn't activate layer.

**Solution**: Ensure you've defined the substitution:

```mayu
# Wrong: just using M00 without substitution
key M00-H = *Left     # M00 never activates!

# Correct: substitute a physical key to M00
def subst *Space = *M00
key M00-H = *Left     # Now works when holding Space
```

### Tap Action Not Working

**Problem**: Quick tap doesn't output the expected key.

**Solution**: Verify `mod assign` syntax:

```mayu
# Wrong
mod assign *M00 = Enter    # Missing * on output

# Correct
mod assign M00 = *Enter
```

### Lock Not Persisting

**Problem**: Lock state doesn't persist across key presses.

**Solution**: Lock keys (L00-LFF) persist automatically. Check that:
1. You're using L (lock) not M (modifier)
2. Physical key is substituted to lock key:

```mayu
def subst *CapsLock = *L00
```

### Multiple Mappings Not Working

**Problem**: Combo mapping (M00-M01-A) doesn't work.

**Checklist**:
1. Both modifiers must be held simultaneously
2. More specific mapping must be defined:

```mayu
key M00-A = *1        # Less specific
key M00-M01-A = *2    # More specific (this wins when both active)
```

### Virtual Key Not Recognized

**Problem**: V_A not recognized in config.

**Solution**: Use exact key name with V_ prefix:

```mayu
# Wrong
def subst *A = *Virtual_A    # Not recognized

# Correct
def subst *A = *V_A
```

## FAQ

### Q: How many modifiers and locks can I use?

**A**: 256 modifiers (M00-MFF) and 256 locks (L00-LFF). That's 512 special keys total.

### Q: Can I combine modifiers and locks?

**A**: Yes! Example:

```mayu
key M00-L00-A = *Home    # Requires M00 held AND L00 toggled on
```

### Q: What's the tap/hold threshold?

**A**: 200 milliseconds. Release before 200ms = tap action. Hold 200ms or more = activates modifier.

### Q: Can I nest virtual keys?

**A**: Yes, but keep it simple:

```mayu
def subst *A = *V_B
def subst *V_B = *V_C    # Works but can be confusing
```

### Q: Do virtual keys get output to the system?

**A**: No. V_*, M00-MFF, and L00-LFF are internal only. Only physical keys and their mappings are output.

### Q: Can I use V_ with physical modifier keys?

**A**: Yes:

```mayu
# This works
key S-V_A = *B    # Shift + virtual A → B
```

### Q: How do I see which locks are active?

**A**: Check the GUI lock indicator widget. It shows all active locks in real-time with color coding (green = active, gray = inactive).

### Q: Can I have more than one tap action per modifier?

**A**: No. Each modifier can have only one tap action:

```mayu
mod assign M00 = *Enter     # M00 tap action
# mod assign M00 = *Space   # This would override above
```

### Q: What happens if I press multiple modifiers quickly?

**A**: Each modifier tracks its own timing independently. You can tap M00 then tap M01 - both execute their tap actions.

### Q: Can I map a lock key to toggle another lock?

**A**: Yes:

```mayu
def subst *CapsLock = *L00
def subst *ScrollLock = *L01

# Pressing L00 can toggle L01
key L00-Space = *L01    # When L00 is on, Space toggles L01
```

## Summary

The Virtual Key System provides:

- **Virtual Regular Keys (V_*)**: Distinguish physical from virtual
- **Modal Modifiers (M00-MFF)**: 256 tap/hold layers
- **Lock Keys (L00-LFF)**: 256 persistent toggle modes
- **Unlimited Combinations**: Mix modifiers and locks freely
- **Visual Feedback**: GUI indicators for lock state
- **Clean Syntax**: Short, readable notation

Start with simple examples (Space Cadet) and build up to complex multi-layer configurations as needed.
