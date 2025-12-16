# Migration Guide: Virtual Key System

## Overview

This guide helps you migrate from the old modal modifier system to the new Virtual Key System.

**Important**: The new system is NOT backward compatible. Old `mod mod0 = !!Key` syntax no longer works.

## Breaking Changes

### 1. Modal Modifier Syntax Removed

**Old (REMOVED)**:
```mayu
mod mod0 = !!B
key mod0-H = *Left
```

**New**:
```mayu
def subst *B = *M00
key M00-H = *Left
```

### 2. No Automatic Modifier Registration

**Old**: Modifiers needed explicit declaration:
```mayu
mod mod0 = !!B
```

**New**: Modifiers work implicitly when referenced:
```mayu
# Just use M00 in substitution, no registration needed
def subst *B = *M00
key M00-H = *Left
```

### 3. Different Keycode Ranges

Old modal modifiers used arbitrary keycodes. New system uses:
- Virtual keys: 0xE000-0xEFFF
- Modifiers: 0xF000-0xF0FF
- Locks: 0xF100-0xF1FF

If you had hardcoded keycode values, these need updating.

### 4. Lock Keys Are New

Old system had no lock key concept. All toggle behavior was emulated with modifiers.

**New**: Use dedicated lock keys (L00-LFF) for persistent toggle state:
```mayu
def subst *CapsLock = *L00
key L00-A = *Home
```

## Migration Syntax Table

| Feature | Old Syntax | New Syntax | Notes |
|---------|------------|------------|-------|
| Modifier Definition | `mod mod0 = !!B` | `def subst *B = *M00` | Use substitution instead |
| Modifier Mapping | `key mod0-H = *Left` | `key M00-H = *Left` | Use M00-MFF notation |
| Tap Action | Not supported | `mod assign M00 = *Enter` | New feature |
| Virtual Keys | Not available | `V_A`, `V_Enter` | New feature |
| Lock Keys | Not available | `L00`, `L01` | New feature |
| Multiple Modifiers | `key mod0-mod1-A = *B` | `key M00-M01-A = *B` | Works similarly |
| Hardware Modifiers | `key S-A = *B` | `key S-A = *B` | No change (Shift, Ctrl, Alt, Win) |

## Step-by-Step Migration

### Step 1: Identify Old Modal Modifiers

Find all `mod modN = !!Key` declarations in your config.

**Example old config**:
```mayu
mod mod0 = !!Space
mod mod1 = !!Semicolon
```

### Step 2: Convert to Substitutions

Replace `mod modN = !!Key` with `def subst *Key = *M0N`:

**New config**:
```mayu
def subst *Space = *M00
def subst *Semicolon = *M01
```

### Step 3: Update Keymap Entries

Replace `modN` with `M0N` in all keymap entries:

**Old**:
```mayu
key mod0-H = *Left
key mod0-J = *Down
key mod0-mod1-A = *Home
```

**New**:
```mayu
key M00-H = *Left
key M00-J = *Down
key M00-M01-A = *Home
```

### Step 4: Add Tap Actions (Optional)

If you want tap behavior, add `mod assign` statements:

```mayu
# Make Space output Space on quick tap
mod assign M00 = *Space

# Make Semicolon output Semicolon on quick tap
mod assign M01 = *Semicolon
```

### Step 5: Convert Toggle Modifiers to Locks

If you used modifiers for persistent state (toggle on/off), use locks instead:

**Old (toggle emulation with mod0)**:
```mayu
mod mod0 = !!CapsLock
key mod0-A = *Home
# Required manual toggle logic or workarounds
```

**New (proper lock)**:
```mayu
def subst *CapsLock = *L00
key L00-A = *Home
# Toggles automatically on press
```

## Migration Examples

### Example 1: Simple Space Cadet

**Old Config**:
```mayu
mod mod0 = !!Space
key mod0-H = *Left
key mod0-J = *Down
key mod0-K = *Up
key mod0-L = *Right
```

**New Config**:
```mayu
def subst *Space = *M00
mod assign M00 = *Space

key M00-H = *Left
key M00-J = *Down
key M00-K = *Up
key M00-L = *Right
```

**Changes**:
1. `mod mod0 = !!Space` → `def subst *Space = *M00`
2. Added `mod assign M00 = *Space` for tap behavior
3. `mod0` → `M00` in all mappings

### Example 2: Multiple Modifiers

**Old Config**:
```mayu
mod mod0 = !!F
mod mod1 = !!D

key mod0-H = *Left
key mod1-H = *Home
key mod0-mod1-H = *C-Home
```

**New Config**:
```mayu
def subst *F = *M00
def subst *D = *M01

key M00-H = *Left
key M01-H = *Home
key M00-M01-H = *C-Home
```

**Changes**:
1. Both `mod` declarations → `def subst`
2. `mod0` → `M00`, `mod1` → `M01`

### Example 3: Vim Mode with Lock

**Old Config** (using mod0 with manual toggle):
```mayu
mod mod0 = !!CapsLock

# Vim navigation
key mod0-H = *Left
key mod0-J = *Down
key mod0-K = *Up
key mod0-L = *Right

# Had to manually track toggle state
```

**New Config** (using proper lock):
```mayu
def subst *CapsLock = *L00

# Vim navigation (automatically persistent)
key L00-H = *Left
key L00-J = *Down
key L00-K = *Up
key L00-L = *Right

# Lock state persists automatically
```

**Changes**:
1. Use L00 (lock) instead of M00 (modifier)
2. No manual toggle tracking needed
3. GUI shows lock state indicator

### Example 4: Physical vs Virtual Key Distinction

This is a new feature with no old equivalent.

**New Config**:
```mayu
# Physical A → Virtual V_B
def subst *A = *V_B

# Different mappings for physical vs virtual
key M00-B = *C        # Physical B
key M00-V_B = *D      # Virtual B (from substituted A)
```

**Use Case**: Distinguish between a key pressed physically vs generated through substitution.

### Example 5: Symbol Layer

**Old Config**:
```mayu
mod mod0 = !!Semicolon

key mod0-1 = *S-1    # !
key mod0-2 = *S-2    # @
key mod0-A = *LeftBracket
```

**New Config**:
```mayu
def subst *Semicolon = *M00
mod assign M00 = *Semicolon

key M00-1 = *S-1
key M00-2 = *S-2
key M00-A = *LeftBracket
```

**Changes**:
1. Added tap action to preserve semicolon functionality
2. Updated notation

## Common Migration Pitfalls

### Pitfall 1: Forgetting to Remove `mod` Declarations

**Wrong**:
```mayu
mod mod0 = !!Space           # Old syntax - will cause error
def subst *Space = *M00      # New syntax
```

**Correct**:
```mayu
# Remove old mod declaration entirely
def subst *Space = *M00
```

### Pitfall 2: Using Wrong Notation in Keymaps

**Wrong**:
```mayu
def subst *Space = *M00
key mod0-H = *Left           # Still using old mod0 notation
```

**Correct**:
```mayu
def subst *Space = *M00
key M00-H = *Left            # Use M00 notation
```

### Pitfall 3: Tap Action Syntax Errors

**Wrong**:
```mayu
mod assign *M00 = Enter      # Wrong: * on modifier, missing * on key
mod assign M00 = Enter       # Wrong: missing * on key
```

**Correct**:
```mayu
mod assign M00 = *Enter      # No * on modifier, * on output key
```

### Pitfall 4: Using Modifier When Lock Is Needed

**Problem**: Modifier state doesn't persist after release.

**Wrong** (for persistent mode):
```mayu
def subst *CapsLock = *M00   # Modifier only active while held
key M00-A = *Home
```

**Correct** (for persistent mode):
```mayu
def subst *CapsLock = *L00   # Lock persists after toggle
key L00-A = *Home
```

### Pitfall 5: Not Updating Hex Notation

If you had more than 10 modifiers, notation changes:

**Old**:
```mayu
mod mod10 = !!Key    # Decimal notation
```

**New**:
```mayu
def subst *Key = *M0A    # Hex notation (10 decimal = 0A hex)
```

**Conversion**:
- mod10 → M0A
- mod11 → M0B
- mod15 → M0F
- mod16 → M10
- etc.

## Automated Migration Script

For large configs, use this search-and-replace approach:

### Pattern 1: Modifier Declarations

```bash
# Find: mod mod(\d+) = !!(\w+)
# Replace: def subst *$2 = *M0$1
```

Example:
- `mod mod0 = !!Space` → `def subst *Space = *M00`
- `mod mod5 = !!F` → `def subst *F = *M05`

### Pattern 2: Keymap Entries

```bash
# Find: key mod(\d+)-
# Replace: key M0$1-
```

Example:
- `key mod0-H` → `key M00-H`
- `key mod5-J` → `key M05-J`

### Pattern 3: Multiple Modifiers

```bash
# Find: key mod(\d+)-mod(\d+)-
# Replace: key M0$1-M0$2-
```

Example:
- `key mod0-mod1-A` → `key M00-M01-A`

**Note**: These patterns work for modifiers 0-9. For mod10+ you'll need to manually convert to hex.

## Testing Your Migration

### Step 1: Syntax Check

Run yamy daemon with your new config:

```bash
./yamy --config /path/to/new/config.mayu --test
```

Check for parser errors.

### Step 2: Basic Function Test

Test each converted modifier:

1. Verify substitution works (key activates modifier)
2. Verify modifier mappings work (modifier + key outputs expected result)
3. Verify tap actions work (quick tap outputs tap key)

### Step 3: Combination Test

Test multi-modifier combinations:

1. Hold first modifier, verify its mappings
2. Hold both modifiers, verify combo mappings
3. Verify specificity (more specific mapping wins)

### Step 4: Lock Test

If using locks:

1. Toggle lock on, verify mappings work
2. Verify lock persists across multiple key presses
3. Toggle lock off, verify normal behavior returns
4. Check GUI indicator shows correct lock state

## Rollback Plan

If migration causes issues, you can temporarily revert:

1. Keep old config file as backup: `config.mayu.old`
2. Keep old yamy binary: `yamy.old`
3. Switch back: `./yamy.old --config config.mayu.old`

**Note**: Old system is deprecated and will not receive updates.

## Getting Help

If you encounter migration issues:

1. Check this migration guide
2. Check [VIRTUAL_KEYS.md](VIRTUAL_KEYS.md) for syntax reference
3. Review [troubleshooting section](VIRTUAL_KEYS.md#troubleshooting)
4. Search GitHub issues: https://github.com/your-repo/issues
5. Post a new issue with:
   - Old config snippet
   - New config snippet
   - Error message or unexpected behavior

## Migration Checklist

Use this checklist to track your migration progress:

- [ ] Read breaking changes section
- [ ] Backup old config file
- [ ] Identify all `mod modN = !!Key` declarations
- [ ] Convert to `def subst *Key = *M0N`
- [ ] Update all `modN` to `M0N` in keymaps
- [ ] Add `mod assign` for desired tap actions
- [ ] Convert toggle modifiers to lock keys (L00-LFF)
- [ ] Test basic modifier functionality
- [ ] Test tap actions
- [ ] Test modifier combinations
- [ ] Test lock keys (if used)
- [ ] Verify GUI lock indicators (if used)
- [ ] Run full E2E tests
- [ ] Document any custom patterns used

## FAQ

### Q: Can I keep using the old syntax?

**A**: No. The old `mod mod0 = !!Key` syntax has been removed entirely. You must migrate to the new system.

### Q: Will my old config file work?

**A**: No. The new parser does not recognize old syntax. You must update your config.

### Q: How long does migration take?

**A**: For simple configs (<50 lines), about 15 minutes. For complex configs (500+ lines), plan 1-2 hours.

### Q: Can I migrate incrementally?

**A**: No. The old and new systems are incompatible. You must migrate the entire config at once.

### Q: What if I have 20+ modifiers?

**A**: Convert carefully to hex notation:
- mod0-mod9 → M00-M09
- mod10 → M0A
- mod11 → M0B
- mod15 → M0F
- mod16 → M10
- mod20 → M14
- etc.

### Q: Do hardware modifiers (Shift, Ctrl) change?

**A**: No. Shift, Ctrl, Alt, Win modifiers work exactly as before.

### Q: What happens to my keymap specificity rules?

**A**: Specificity works the same way but now counts modifiers + locks. More specific entries still win.

### Q: Can I use both old and new modifiers?

**A**: No. Old syntax is completely removed.

## Summary

Migration steps:

1. Replace `mod modN = !!Key` with `def subst *Key = *M0N`
2. Replace `modN` with `M0N` in all keymaps
3. Add `mod assign M0N = *Key` for tap actions
4. Convert toggle modifiers to locks (L00-LFF)
5. Test thoroughly

The new system provides:
- Cleaner syntax
- More features (tap/hold, locks, 256 modifiers)
- Better error messages
- GUI feedback

While migration requires work, the new system is more powerful and maintainable.
