# Code Metrics Violations Report

This document tracks code metrics violations (files >500 lines, functions >50 lines, CCN >15) and refactoring status.

## Summary

- **Refactored**: engine.cpp (680→20 lines) ✅
- **Remaining File Violations**: 8 files
- **Remaining Function Violations**: ~40 functions

## Refactoring Status

### ✅ Completed

| File | Original Size | Status | Notes |
|------|---------------|--------|-------|
| src/core/engine/engine.cpp | 680 lines | FIXED | Split into engine_keyboard_handler.cpp and engine_ipc_handler.cpp |

### ⚠️  Deferred (Architectural Complexity)

These files require significant architectural changes and extensive testing:

| File | Lines | Functions >50 | Complexity | Reason for Deferral |
|------|-------|---------------|------------|---------------------|
| setting_loader.cpp | 1747 | 8 | Critical parser | Core configuration parsing, complex interdependencies |
| config_manager.cpp | 1868 | Unknown | Configuration | Central configuration management |
| config_validator.cpp | 668 | Unknown | Validation | Configuration validation logic |
| parser.cpp | 536 | 3 (154 lines max) | UTF-8 parsing | Complex UTF-8 and tokenization logic |

### 📋 Candidates for Future Refactoring

These files have more straightforward refactoring paths:

| File | Lines | Key Violations | Suggested Approach |
|------|-------|----------------|-------------------|
| engine.h | 644 | Header file | Split into separate headers by concern |
| keymap.cpp | 646 | 2 functions (88, 65 lines) | Extract helper functions |
| function.cpp | 586 | 1 function (50 lines) | Extract window management logic |
| session_manager.cpp | 604 | Unknown | Split session logic |
| engine_generator.cpp | 491 | 1 function (156 lines, CCN 54) | Extract event generation logic |

## Detailed Function Violations

### High Priority (CCN >20 or Lines >100)

1. **setting_loader.cpp:load_MODIFIER** - 162 lines, CCN 28
2. **setting_loader.cpp:readFile** - 160 lines, CCN 38
3. **parser.cpp:Parser::getLine** - 178 lines, CCN 48
4. **engine_generator.cpp:beginGeneratingKeyboardEvents** - 156 lines, CCN 54
5. **setting_loader.cpp:load_MODIFIER_ASSIGNMENT** - 80 lines, CCN 44
6. **setting_loader.cpp:load_KEYMAP_DEFINITION** - 83 lines, CCN 22

### Medium Priority (50-100 lines or CCN 15-20)

- keymap.cpp:describe - 88 lines, CCN 22
- keymap.cpp:adjustModifier - 65 lines, CCN 17
- Various command handlers in src/core/commands/

## Recommendations

### Short Term (Current Sprint)
- ✅ Complete: Refactored engine.cpp
- Document exceptions for complex parser/config files
- Add TODO comments in files earmarked for future refactoring

### Medium Term (Next Quarter)
- Refactor engine_generator.cpp:beginGeneratingKeyboardEvents
- Split engine.h into logical headers
- Refactor keymap.cpp functions

### Long Term (Future Releases)
- Redesign configuration parsing architecture
- Consider using parser generator for setting_loader
- Implement incremental parser refactoring with comprehensive test coverage

## Exceptions Policy

Files deferred due to:
1. **Critical infrastructure** - Core parsing/config logic
2. **High risk** - Extensive testing required
3. **Architectural debt** - Requires design changes, not just code splitting

These will be addressed in future refactoring sprints with:
- Comprehensive test coverage first
- Incremental refactoring approach
- Feature flags for new implementations
