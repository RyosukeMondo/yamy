# Parallel Refinement Plan - Yamy Modernization

**Generated:** 2025-12-10
**Status:** Ready for parallel execution
**Estimated Total Effort:** 5-8 work sessions (if done in parallel)

## Overview

This document breaks down the remaining modernization work into **independent, parallel tracks** that can be assigned to different team members or agents. Each section can be worked on simultaneously without conflicts.

---

## Track 1: Command Template System UTF-8 Migration

**Assignee:** `{ASSIGNEE_1}`
**Priority:** 🔴 HIGH (Affects all 63 commands)
**Estimated Effort:** 2-3 hours
**Dependencies:** None

### Objective
Migrate `src/core/commands/command_base.h` from `tstring`/`_T()` to `std::string` and UTF-8.

### Files to Modify
- `src/core/commands/command_base.h`

### Tasks
- [ ] Replace all `tstring` with `std::string` in template methods
- [ ] Replace all `_T()` macros with regular string literals
- [ ] Update `load()` method to use `std::string` instead of `tstring` (lines 74-76)
- [ ] Update `output()` method to eliminate `_T()` calls (lines 104, 110-112)
- [ ] Update `loadArgs()` helper to use `std::string` (line 144)
- [ ] Update `outputOneArg()` to use `std::string` (lines 176, 181)
- [ ] Ensure `getName()` returns `std::string` (already correct, line 48)
- [ ] Test: Build all 63 commands successfully
- [ ] Test: Run existing command tests

### Acceptance Criteria
- Zero `tstring` usages in `command_base.h`
- Zero `_T()` macros in `command_base.h`
- All 63 commands compile without errors
- No behavioral changes (output remains identical)

### Notes
- The template already has `getName()` returning `std::string` - use this consistently
- PAL conversion helpers (`to_tstring`, `from_tstring`) may still be needed temporarily at boundaries
- Consider using `to_UTF_8()` helper if TCHAR conversion is needed

---

## Track 2: Engine Class String Migration

**Assignee:** `{ASSIGNEE_2}`
**Priority:** 🔴 HIGH (Core class, affects focus tracking)
**Estimated Effort:** 2-3 hours
**Dependencies:** None

### Objective
Migrate `Engine` class from `tstring`/`tstringi` to `std::string` for internal string storage.

### Files to Modify
- `src/core/engine/engine.h`
- `src/core/engine/engine_focus.cpp`
- `src/core/engine/engine_window.cpp`
- `src/core/engine/engine_lifecycle.cpp`

### Tasks
- [ ] Migrate `FocusOfThread::m_className` from `tstringi` to `std::string` (engine.h:61)
- [ ] Migrate `FocusOfThread::m_titleName` from `tstringi` to `std::string` (engine.h:62)
- [ ] Update all usages of `m_className` and `m_titleName` in engine_focus.cpp
- [ ] Update window class/title retrieval code in engine_window.cpp
- [ ] Add UTF-8/UTF-16 conversions at PAL boundary (use `yamy::platform::wstring_to_utf8`)
- [ ] Update logging/output code in engine_lifecycle.cpp
- [ ] Test: Build engine successfully
- [ ] Test: Verify window focus detection still works

### Acceptance Criteria
- `FocusOfThread` uses `std::string` for both members
- All Engine files compile without tstring-related errors
- Window focus tracking maintains correct behavior
- Class names and titles correctly captured in UTF-8

### Notes
- Use `yamy::platform::wstring_to_utf8()` when getting strings from Win32 APIs
- Maintain case-insensitive comparisons using appropriate std::string methods
- The PAL `IWindowSystem::getClassName()` already returns `std::string` - leverage this!

---

## Track 3: Win32 Type Leakage Elimination (Commands)

**Assignee:** `{ASSIGNEE_3}`
**Priority:** 🟡 MEDIUM (PAL abstraction completeness)
**Estimated Effort:** 3-4 hours
**Dependencies:** None

### Objective
Eliminate Win32 types (HWND, DWORD, MSG, WPARAM, LPARAM) from command implementations.

### Files to Modify
- `src/core/commands/cmd_mouse_hook.cpp`
- `src/core/commands/cmd_direct_sstp.cpp`
- `src/core/commands/cmd_window_resize_to.cpp`
- `src/core/commands/cmd_window_identify.cpp`
- `src/core/commands/cmd_window_move_to.cpp`
- `src/core/commands/cmd_window_move_visibly.cpp`
- `src/core/commands/cmd_window_move.cpp`
- `src/core/commands/cmd_window_hv_maximize.cpp`
- `src/core/commands/cmd_set_foreground_window.cpp`
- `src/core/commands/cmd_load_setting.cpp`
- `src/core/commands/cmd_mayu_dialog.cpp`

### Tasks
- [ ] Replace `HWND` with `yamy::platform::WindowHandle` in all command files
- [ ] Replace `DWORD` with `uint32_t` or appropriate sized integer
- [ ] Replace `MSG` with platform-agnostic message structure
- [ ] Replace `WPARAM`/`LPARAM` with `uintptr_t` where used
- [ ] Replace `RECT*` with `yamy::platform::Rect*` (already done in some files)
- [ ] Use `static_cast<HWND>()` only at PAL implementation boundary (not in commands)
- [ ] Verify all casts are to PAL types, not Win32 types
- [ ] Test: Build all modified commands
- [ ] Test: Verify window operations still work correctly

### Acceptance Criteria
- Zero Win32 type references in all 11 command files
- All commands use `yamy::platform::*` types exclusively
- Commands compile cleanly on both Windows and Linux (stub) builds
- No behavioral regressions in window manipulation

### Notes
- Pattern: Commands should use `WindowHandle`, implementations cast to `HWND`
- Check if new PAL types need to be added (e.g., platform::Message)
- Some files may already be partially fixed - complete the migration

---

## Track 4: Function System String Migration

**Assignee:** `{ASSIGNEE_4}`
**Priority:** 🟡 MEDIUM (Legacy system, contained scope)
**Estimated Effort:** 2 hours
**Dependencies:** None

### Objective
Migrate `function.cpp` type tables and related code from `tstring` to `std::string`.

### Files to Modify
- `src/core/functions/function.cpp`
- `src/core/functions/function.h`
- `src/core/functions/function_data.h`

### Tasks
- [ ] Migrate `TypeTable<T>::m_name` from `const _TCHAR*` to `const char*` (function.cpp:27)
- [ ] Update `getTypeName()` template to use `std::string*` instead of `tstring*` (function.cpp:32)
- [ ] Update `getTypeValue()` template to use `const std::string&` instead of `const tstringi&` (function.cpp:44)
- [ ] Replace all `_T()` macros in type table initializers with regular string literals
- [ ] Update `operator<<` overloads to work with `std::string`
- [ ] Update function.h if it exposes any tstring types
- [ ] Check function_data.h for tstring dependencies
- [ ] Test: Build function system
- [ ] Test: Verify enum serialization/deserialization works

### Acceptance Criteria
- `TypeTable` uses `const char*` for names
- All helper functions use `std::string`
- Zero `_T()` macros in type tables
- VKey, ToWindowType serialization maintains compatibility

### Notes
- Type tables are static data - simple string literal replacement
- Maintain backward compatibility for serialized .mayu files if needed
- This is a contained subsystem - low risk of breaking other code

---

## Track 5: Settings & Keymap String Migration

**Assignee:** `{ASSIGNEE_5}`
**Priority:** 🟡 MEDIUM (User-facing configuration)
**Estimated Effort:** 3-4 hours
**Dependencies:** None

### Objective
Migrate Settings, SettingLoader, and Keymap classes from `tstring` to `std::string`.

### Files to Modify
- `src/core/settings/setting.h`
- `src/core/settings/setting.cpp`
- `src/core/settings/setting_loader.h`
- `src/core/settings/setting_loader.cpp`
- `src/core/settings/parser.h`
- `src/core/settings/parser.cpp`
- `src/core/input/keymap.h`
- `src/core/input/keymap.cpp`

### Tasks
- [ ] Migrate Setting class string members to `std::string`
- [ ] Migrate SettingLoader parsing methods to `std::string`
- [ ] Update Parser regex from `tregex` to `std::regex`
- [ ] Migrate Keymap name storage from `tstring` to `std::string`
- [ ] Update file I/O to use UTF-8 encoding
- [ ] Replace `_T()` macros with regular string literals
- [ ] Add UTF-8/UTF-16 conversion at file system boundary (Windows)
- [ ] Test: Load existing .mayu configuration files
- [ ] Test: Parse complex keymaps correctly
- [ ] Test: Verify regex patterns still work

### Acceptance Criteria
- All Setting/SettingLoader/Keymap classes use `std::string`
- Parser uses `std::regex` instead of `tregex`
- Existing .mayu files load correctly (backward compatibility)
- No parsing regressions

### Notes
- .mayu files are likely UTF-8 or ASCII already - verify encoding
- Regex patterns should remain identical (just change from tregex to std::regex)
- File paths on Windows need UTF-8 → UTF-16 conversion for Win32 APIs
- Use `yamy::platform::utf8_to_wstring()` for file operations

---

## Track 6: Document & Automation Updates

**Assignee:** `{ASSIGNEE_6}`
**Priority:** 🟢 LOW (Documentation, no code changes)
**Estimated Effort:** 1-2 hours
**Dependencies:** Tracks 1-5 completion (to verify final counts)

### Objective
Update documentation to reflect actual implementation status and create automation for tracking.

### Files to Modify
- `docs/keynote-level-productivity.md`
- `scripts/track_legacy_strings.sh` (NEW)
- `README.md` (if needed)

### Tasks
- [ ] Update Phase 3 status line with actual count: 974 → {FINAL_COUNT} usages
- [ ] Update Phase 4 status to "ZERO windows.h includes" (celebrate!)
- [ ] Add Win32 type leakage note: "15 files → {FINAL_COUNT} files leak Win32 types"
- [ ] Create tracking script: `scripts/track_legacy_strings.sh`
  - Count `tstring`, `_T()`, `_TCHAR` usages
  - Count Win32 type leakage (HWND, DWORD, MSG, etc.)
  - Output progress metrics
- [ ] Add "String Migration Progress Tracking" section to keynote doc
- [ ] Remove `.vcxproj` files from `proj/` directory (cleanup)
- [ ] Update README.md with current build instructions (if needed)
- [ ] Create pre-commit hook to track string migration progress (optional)

### Acceptance Criteria
- keynote-level-productivity.md reflects actual current state
- Tracking script accurately counts legacy string usages
- Documentation is clear and up-to-date
- Old Visual Studio project files removed

### Script Template
```bash
#!/bin/bash
# scripts/track_legacy_strings.sh
echo "=== Legacy String Usage Tracking ==="
echo "tstring usages:"
grep -r "tstring" src/core | wc -l
echo "_T() macro usages:"
grep -r "_T(" src/core | wc -l
echo "_TCHAR usages:"
grep -r "_TCHAR" src/core | wc -l
echo ""
echo "=== Win32 Type Leakage ==="
grep -r "HWND\|DWORD\|MSG\|WPARAM\|LPARAM" src/core --include="*.cpp" --include="*.h" | wc -l
echo "Files with Win32 types:"
grep -rl "HWND\|DWORD\|MSG\|WPARAM\|LPARAM" src/core --include="*.cpp" --include="*.h" | wc -l
```

---

## Track 7: CI & Linux Build Verification

**Assignee:** `{ASSIGNEE_7}`
**Priority:** 🟢 LOW (Quality assurance)
**Estimated Effort:** 2-3 hours
**Dependencies:** Tracks 1, 3 (to ensure portability)

### Objective
Set up CI to build Linux stubs and verify no Windows dependencies leak into core.

### Files to Create/Modify
- `.github/workflows/ci.yml` (NEW)
- `CMakeLists.txt` (add Linux build option)
- `src/platform/linux/README.md` (update)

### Tasks
- [ ] Create GitHub Actions workflow for Linux build
- [ ] Configure CMake to support Linux target build
- [ ] Add CMake option: `BUILD_LINUX_STUB` (default: OFF)
- [ ] Verify Linux stubs compile on Ubuntu latest
- [ ] Add static analysis step to detect Windows.h includes in src/core
- [ ] Add step to detect Win32 types in src/core (HWND, DWORD, etc.)
- [ ] Configure workflow to run on every PR
- [ ] Test: Trigger workflow manually and verify it passes
- [ ] Document Linux build process in README

### Acceptance Criteria
- CI workflow successfully builds Linux stub
- CI fails if `#include <windows.h>` added to src/core
- CI fails if Win32 types leak into src/core
- Build matrix includes: Windows (MinGW), Linux (GCC)
- Documentation updated with CI status badge

### Notes
- Linux build will NOT be functional, just verify architectural separation
- Use `nm` or `objdump` to verify no Windows symbols in core object files
- Consider adding sanitizer builds (UBSan, ASan) for quality

---

## Coordination & Integration

### Work Assignment Template
```
Track {N}: {Track Name}
Assignee: {NAME/AGENT_ID}
Branch: feature/track-{N}-{short-name}
Status: [ ] Not Started | [ ] In Progress | [ ] Review | [ ] Complete
```

### Branch Strategy
Each track should work on its own branch:
- `feature/track-1-command-template-utf8`
- `feature/track-2-engine-string-migration`
- `feature/track-3-win32-type-elimination`
- `feature/track-4-function-string-migration`
- `feature/track-5-settings-keymap-migration`
- `feature/track-6-documentation-updates`
- `feature/track-7-ci-linux-verification`

### Merge Order (After All Complete)
1. Track 6 (Documentation) - No code changes, merge first
2. Track 7 (CI) - Sets up verification
3. Track 1 (Command Template) - Highest impact
4. Track 2 (Engine) - Core class
5. Track 3 (Win32 Types) - Enables portability
6. Track 4 (Function System) - Contained subsystem
7. Track 5 (Settings/Keymap) - User-facing

### Testing Strategy
After each track merges:
- Run full build (32-bit + 64-bit)
- Run existing tests
- Manual smoke test: Load .mayu file, test key remapping
- Check tracking script for progress

### Conflict Resolution
Tracks are designed to be independent, but if conflicts occur:
- Track 1 takes precedence (command template is foundation)
- Track 2 takes precedence over 4, 5 (Engine is core)
- Tracks 3, 4, 5 are peer - resolve by file ownership

---

## Success Metrics

### Overall Progress Tracking
```
Phase 3: String Unification
├─ Current: 974 legacy usages
├─ Track 1: -150 (command template)
├─ Track 2: -100 (engine)
├─ Track 4: -80 (function system)
├─ Track 5: -300 (settings/keymap)
└─ Target: ~344 remaining (65% complete)

Phase 4: PAL Completion
├─ Current: 15 files with Win32 leakage
├─ Track 3: -11 (command files)
├─ Track 2: -3 (engine files)
└─ Target: ~1 file remaining (93% complete)
```

### Definition of Done
- [ ] All 7 tracks completed and merged
- [ ] Build succeeds cleanly (zero warnings)
- [ ] All tests pass
- [ ] CI pipeline green
- [ ] Documentation updated
- [ ] Tracking script shows <400 legacy string usages
- [ ] Less than 2 files with Win32 type leakage

---

## Notes for Assignees

### Before Starting
1. Pull latest `master` branch
2. Create your feature branch: `git checkout -b feature/track-{N}-{name}`
3. Read the "Files to Modify" section carefully
4. Check dependencies (most tracks have none!)

### During Work
1. Commit frequently with clear messages
2. Run build after each logical change
3. Use `scripts/cmake_package.sh` to verify full build
4. Update your track status in this document

### Before Submitting
1. Ensure all tasks checked off
2. Run full build: `rm -rf build && bash scripts/cmake_package.sh`
3. Run tracking script: `bash scripts/track_legacy_strings.sh`
4. Update this document with final counts
5. Create PR with:
   - Clear description of changes
   - Before/After metrics (string count, type leakage count)
   - Screenshots of successful build (if applicable)

### Getting Help
- Check existing PAL implementations in `src/platform/windows/` for patterns
- UTF conversion: Use `yamy::platform::utf8_to_wstring()` and `wstring_to_utf8()`
- Type replacements: See `src/core/platform/types.h` for platform-agnostic types
- Questions: Ask in #yamy-modernization channel

---

**Ready to start? Assign tracks and begin parallel work!**
