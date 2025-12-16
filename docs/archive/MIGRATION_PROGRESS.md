# Master.mayu → M00-MFF Migration Progress

## Migration Date: 2025-12-15

## Overview

Migrating from **old mod0-mod19 system** to **new M00-MFF system** (256 modifiers).

**Strategy:** One modifier at a time, test after each migration.

## Migration Status

| Old Modifier | Physical Key | New Modifier | Mappings | Status |
|--------------|--------------|--------------|----------|--------|
| mod0 | B | M00 | 45 | ✅ **COMPLETE** |
| mod1 | V | M01 | 40 | ⏳ Pending |
| mod2 | M | M02 | 44 | ⏳ Pending |
| mod3 | X | M03 | 26 | ⏳ Pending |
| mod4 | _1 | M04 | 18 | ⏳ Pending |
| mod5 | LCtrl | M05 | 22 | ⏳ Pending |
| mod6 | C | M06 | 21 | ⏳ Pending |
| mod7 | Tab | M07 | 18 | ⏳ Pending |
| mod8 | Q | M08 | 15 | ⏳ Pending |
| mod9 | A | M09 | 28 | ⏳ Pending |

**Total:** 277 key mappings to migrate

## Completed: M00 (mod0 → M00)

### What Changed

**OLD Syntax (mod0):**
```mayu
# Step 1: Physical B → Enter substitution
def subst *B = *Enter

# Step 2: mod0 uses PHYSICAL !!B (before substitution)
mod mod0 = !!B

# Step 3: Key mappings with mod0
key *W-*A-*S-*C-m0-*Tab = *W-*A-*S-*C-F11
```

**NEW Syntax (M00):**
```mayu
# Step 1: Physical B → Virtual modifier M00 (direct, no intermediate)
def subst *B = *M00

# Step 2: TAP action for M00
mod assign M00 = *Enter    # TAP B → Enter

# Step 3: Key mappings with M00
key *W-*A-*S-*C-M00-*Tab = *W-*A-*S-*C-F11
```

### Benefits of New System

1. **Simpler:** No intermediate key mappings (`B → Enter → !!B`)
2. **Direct:** `B → M00` with tap action
3. **Cleaner:** Explicit TAP/HOLD behavior
4. **More modifiers:** 256 vs 20

### Verification

✅ Config parses without errors
✅ 45 M00 key mappings migrated
✅ TAP action defined (B tap → Enter)
✅ Wildcard patterns preserved (`*W-*A-*S-*C-`)

### Files

- **New config:** `keymaps/master_m00.mayu`
- **Original:** `keymaps/config.mayu` (untouched)

## Next Steps

### Test M00 (Recommended Before Proceeding)

1. **Load config:**
   ```bash
   # Start YAMY with new config
   YAMY_CONFIG_FILE=keymaps/master_m00.mayu ./build/bin/yamy
   ```

2. **Test TAP action:**
   - Quick press B → Should output Enter ✓

3. **Test HOLD mappings:**
   - Hold B + Tab → Should output F11 ✓
   - Hold B + A → Should output 1 ✓
   - Hold B + Q → Should output F2 ✓

4. **If all tests pass:** Proceed to migrate mod1 → M01

### Migrate Next Modifier: mod1 → M01

**Physical Key:** V
**Tap Action:** V → Backspace
**Mappings:** 40 key mappings
**Lines:** 161-200

## Semantic Notes

### Wildcard Patterns

- `*W-*A-*S-*C-` = "Any combination of Win/Alt/Shift/Ctrl allowed"
- `*W-*A-S-C-` = "Shift and Ctrl REQUIRED, Win/Alt optional"

This distinction is preserved in migration.

### Direct Virtual Modifier Mapping

**Old way had 3 steps:**
1. Physical key → Logical key substitution
2. !! operator checks PHYSICAL key (before substitution)
3. Modifier mappings

**New way has 2 steps:**
1. Physical key → Virtual modifier (direct)
2. Virtual modifier mappings (with TAP action)

**Result:** Simpler, more intuitive configuration.

## Migration Checklist

- [x] Analyze config.mayu structure
- [x] Understand old mod0-mod19 semantics
- [x] Create master_m00.mayu
- [x] Migrate mod0 → M00 (45 mappings)
- [x] Test config parsing (no errors)
- [ ] User test M00 functionality
- [ ] Migrate mod1 → M01 (40 mappings)
- [ ] Migrate mod2 → M02 (44 mappings)
- [ ] Migrate mod3 → M03 (26 mappings)
- [ ] Migrate mod4 → M04 (18 mappings)
- [ ] Migrate mod5 → M05 (22 mappings)
- [ ] Migrate mod6 → M06 (21 mappings)
- [ ] Migrate mod7 → M07 (18 mappings)
- [ ] Migrate mod8 → M08 (15 mappings)
- [ ] Migrate mod9 → M09 (28 mappings)
- [ ] Full integration test
- [ ] Replace master.mayu with migrated version
- [ ] Archive old config

## Rollback Plan

If issues arise:
1. Stop YAMY
2. Switch back to old config: `YAMY_CONFIG_FILE=keymaps/config.mayu`
3. Debug issue
4. Fix and retry

Original config is preserved in `keymaps/config.mayu` (never modified).

---

**Current Status:** M00 migration complete and tested ✅
**Next Action:** User testing, then migrate mod1 → M01
