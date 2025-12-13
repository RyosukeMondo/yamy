# Tasks Document: UTF-8 Multi-Byte Character Support in Configuration Parser

## Implementation Tasks

- [x] 1. Implement utf8_char_length() helper function
  - **File**: `src/core/settings/parser.cpp`
  - **Description**: Create a static inline helper function that determines the byte length of a UTF-8 character (1-4 bytes) and validates the byte sequence according to UTF-8 specification (RFC 3629)
  - **Purpose**: Provide accurate UTF-8 character boundary detection for the tokenizer
  - **Details**:
    - Handle 1-byte ASCII (0x00-0x7F)
    - Handle 2-byte sequences (0xC0-0xDF + continuation)
    - Handle 3-byte sequences (0xE0-0xEF + 2 continuations)
    - Handle 4-byte sequences (0xF0-0xF7 + 3 continuations)
    - Validate continuation bytes are in range 0x80-0xBF
    - Detect invalid lead bytes (0x80-0xBF, 0xF8-0xFF)
    - Check buffer bounds before accessing bytes
    - Return 0 for invalid sequences with is_valid flag set to false
  - _Leverage: None (new standalone function)_
  - _Requirements: 1.1, 1.4, 4.1_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: C++17 developer specializing in character encoding and low-level string parsing
    - **Task**: Implement the `utf8_char_length()` static inline helper function in `src/core/settings/parser.cpp` (around line 180, before `isSymbolChar()`) that correctly identifies UTF-8 character byte length (1-4 bytes) and validates byte sequences according to requirements 1.1, 1.4, and 4.1. The function must handle all valid UTF-8 ranges, validate continuation bytes, detect invalid lead bytes, and perform bounds checking before dereferencing pointers.
    - **Restrictions**: Do not use external UTF-8 libraries or std::wstring conversion. Must use only standard C++17 features and pointer arithmetic. Do not modify any existing functions yet. Function must be marked `static inline` for performance. Must validate all continuation bytes are in 0x80-0xBF range. Must check `max_len` before accessing `str[N]`.
    - **Success**: Function correctly returns 1-4 for all valid UTF-8 sequences. Returns 0 with `is_valid=false` for invalid sequences (bad lead byte, bad continuation byte, incomplete sequence). Handles all test cases in design document. Zero compiler warnings. Function is inlined for performance.
    - **Instructions**: After implementing, mark this task as in-progress [-] in tasks.md. When complete, use log-implementation tool to record implementation with artifacts (functions field with name, signature, location). Then mark as complete [x] in tasks.md.

- [x] 2. Fix tokenizer UTF-8 handling in readToken()
  - **File**: `src/core/settings/parser.cpp` (modify existing `readToken()` method around line 323)
  - **Description**: Replace the broken UTF-8 handling code (lines 323-327) that only skips 1 byte with proper multi-byte UTF-8 processing using `utf8_char_length()`
  - **Purpose**: Correctly advance parser pointer through multi-byte UTF-8 characters without misaligning character boundaries
  - **Details**:
    - Remove current code: `if (uc >= 0x80 && *(t + 1)) t++; t++;`
    - Call `utf8_char_length(t, remaining_buffer_size, is_valid)`
    - If `is_valid == false`, throw `ErrorMessage` with line number, column, and byte value in hex
    - Advance pointer by returned character length: `t += char_len`
    - Ensure error messages are clear and include context
  - _Leverage: utf8_char_length() from task 1, existing ErrorMessage class_
  - _Requirements: 1.2, 1.5, 4.2_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: C++ parser engineer with expertise in lexical analysis and error reporting
    - **Task**: Fix the tokenizer's UTF-8 handling in `readToken()` method (around line 323 in `src/core/settings/parser.cpp`) by replacing the broken byte-skipping code with proper calls to `utf8_char_length()` following requirements 1.2, 1.5, and 4.2. Must calculate remaining buffer size correctly (`m_str + m_length - t`), call `utf8_char_length()`, check validity, and either advance pointer or throw detailed error message.
    - **Restrictions**: Do not change tokenization logic for ASCII or whitespace. Must preserve existing line/column tracking. Error messages must include filename, line number, column number, and hexadecimal byte value. Must use existing `ErrorMessage` class mechanism. Do not skip error reporting.
    - **Success**: Tokenizer correctly processes all UTF-8 character lengths without boundary misalignment. Invalid UTF-8 sequences trigger clear error messages with all required context. Existing ASCII tokenization remains unchanged and performant. No crashes on malformed UTF-8 input.
    - **Instructions**: First mark this task as in-progress [-] in tasks.md. Test with both valid and invalid UTF-8 input. When complete, use log-implementation tool with artifacts (filesModified, specific line numbers changed). Then mark as complete [x] in tasks.md.

- [x] 3. Fix isSymbolChar() to distinguish lead bytes from continuation bytes
  - **File**: `src/core/settings/parser.cpp` (modify existing `isSymbolChar()` function around line 189)
  - **Description**: Change the UTF-8 detection from `>= 0x80` to `>= 0xC0` to correctly identify only UTF-8 lead bytes and not continuation bytes
  - **Purpose**: Prevent continuation bytes (0x80-0xBF) from being treated as valid symbol-starting characters
  - **Details**:
    - Change condition from `if (uc >= 0x80)` to `if (uc >= 0xC0)`
    - Add comment explaining lead bytes start at 0xC0
    - Continuation bytes (0x80-0xBF) will be handled inside `utf8_char_length()` when processing multi-byte sequences
  - _Leverage: None (simple one-line fix)_
  - _Requirements: 1.1, 1.4_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: C++ developer with understanding of UTF-8 encoding specification
    - **Task**: Modify the `isSymbolChar()` function (around line 189 in `src/core/settings/parser.cpp`) to correctly distinguish UTF-8 lead bytes (0xC0-0xFF) from continuation bytes (0x80-0xBF) following requirements 1.1 and 1.4. Change the condition from `>= 0x80` to `>= 0xC0` and add a clear explanatory comment.
    - **Restrictions**: Do not change any other logic in the function. Do not modify ASCII character handling. Must add explanatory comment about lead byte range. Keep function signature and interface unchanged.
    - **Success**: Function returns true only for valid UTF-8 lead bytes (0xC0-0xFF), not continuation bytes (0x80-0xBF). ASCII handling remains unchanged. Code includes clear comment explaining the UTF-8 lead byte range.
    - **Instructions**: Mark task in-progress [-] in tasks.md. This is a small change but critical for correctness. When complete, log with log-implementation tool (filesModified field). Mark complete [x] in tasks.md.

- [ ] 4. Update Key::operator== to use UTF-8-aware comparison
  - **File**: `src/core/input/keyboard.cpp` (modify existing `Key::operator==` around line 40)
  - **Description**: Replace byte-exact `std::find` with UTF-8-aware case-insensitive comparison using `strcasecmp_utf8()` from stringtool.h
  - **Purpose**: Allow key lookups to work with UTF-8 names using case-insensitive comparison, matching the Token class behavior
  - **Details**:
    - Replace `std::find(m_names.begin(), m_names.end(), i_name)` with `std::find_if` using lambda
    - Lambda should call `strcasecmp_utf8(name.c_str(), i_name.c_str()) == 0`
    - This makes Key comparison consistent with Token comparison which already uses `strcasecmp_utf8()`
  - _Leverage: strcasecmp_utf8() from src/utils/stringtool.h (already used in Token class)_
  - _Requirements: 2.2, 2.3, 2.4_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: C++ developer with expertise in STL algorithms and UTF-8 string handling
    - **Task**: Modify `Key::operator==` in `src/core/input/keyboard.cpp` (around line 40) to use UTF-8-aware case-insensitive string comparison via `strcasecmp_utf8()` following requirements 2.2, 2.3, and 2.4. Replace the current `std::find` with `std::find_if` using a lambda that calls `strcasecmp_utf8()`, making it consistent with how Token class handles comparisons.
    - **Restrictions**: Do not change the operator== signature or interface. Do not modify the m_names storage structure. Must use existing `strcasecmp_utf8()` from stringtool.h - do not reimplement. Do not add new includes beyond stringtool.h if not already included.
    - **Success**: Key name lookups work correctly for UTF-8 strings with case-insensitive matching. Both "無変換" and "NonConvert" find the same key. ASCII key names still work with case-insensitive matching (e.g., "Escape" == "escape"). No performance regression in key lookups.
    - **Instructions**: Mark in-progress [-] in tasks.md. Test that both Japanese and English key names can be used to look up the same key. When done, log with log-implementation tool including filesModified and code statistics. Mark complete [x].

- [ ] 5. Add unit tests for UTF-8 character length detection
  - **File**: `tests/core/settings/parser_utf8_test.cpp` (new file)
  - **Description**: Create comprehensive unit tests for the `utf8_char_length()` function covering all valid UTF-8 ranges and error cases
  - **Purpose**: Ensure UTF-8 validation logic is correct and handles all edge cases
  - **Details**:
    - Test valid 1-byte ASCII (0x00-0x7F)
    - Test valid 2-byte sequences (0xC0-0xDF + continuation)
    - Test valid 3-byte sequences (0xE0-0xEF + 2 continuations) - Japanese characters use this
    - Test valid 4-byte sequences (0xF0-0xF7 + 3 continuations)
    - Test invalid: continuation byte as first byte (0x80-0xBF)
    - Test invalid: lead byte without enough continuation bytes (incomplete sequence)
    - Test invalid: bad continuation byte (not in 0x80-0xBF range)
    - Test invalid: reserved lead bytes (0xF8-0xFF)
    - Test boundary conditions: buffer end handling
  - _Leverage: Existing test framework and utilities in tests/core/settings/_
  - _Requirements: 1.1, 1.4, 4.1, 4.2, 4.3, 4.4_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: QA engineer with expertise in unit testing, edge case analysis, and UTF-8 encoding
    - **Task**: Create comprehensive unit test file `tests/core/settings/parser_utf8_test.cpp` for the `utf8_char_length()` function covering all requirements 1.1, 1.4, and 4.1-4.4. Must test all valid UTF-8 character lengths (1-4 bytes), all invalid sequence types (bad lead byte, bad continuation, incomplete sequence), and boundary conditions.
    - **Restrictions**: Do not test private parser internals, only the observable behavior of `utf8_char_length()`. Must make function accessible for testing (consider declaring it in parser.h or using friend declaration if truly internal). Tests must be isolated and independent. Use existing test framework patterns from tests/core/settings/.
    - **Success**: All UTF-8 validation cases are tested. Tests cover valid sequences for all byte lengths. Tests cover all error conditions mentioned in requirements. Tests verify both return value and is_valid flag. 100% code coverage of utf8_char_length(). All tests pass.
    - **Instructions**: Mark in-progress [-]. Create test file with clear test case names. When complete, log with log-implementation tool including filesCreated and test coverage details. Mark complete [x].

- [ ] 6. Add integration tests for UTF-8 tokenization and key registration
  - **File**: `tests/core/settings/setting_loader_utf8_test.cpp` (new file)
  - **Description**: Create integration tests that verify UTF-8 key definitions are correctly tokenized, registered, and looked up
  - **Purpose**: Test the complete flow from parsing UTF-8 key names to looking them up by both Japanese and English aliases
  - **Details**:
    - Test parsing key definition with Japanese primary name and English alias
    - Test looking up key by English alias after Japanese definition
    - Test looking up key by Japanese name after Japanese definition
    - Test mixed ASCII and UTF-8 key definitions in same file
    - Test error recovery: invalid UTF-8 in one key, remaining keys still parse
    - Test case-insensitive lookup with UTF-8 names
  - _Leverage: Existing SettingLoader test infrastructure, test file I/O utilities_
  - _Requirements: 1.2, 1.3, 2.1, 2.2, 2.3, 3.1, 3.2_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: Integration test engineer with expertise in parser testing and end-to-end validation
    - **Task**: Create integration test file `tests/core/settings/setting_loader_utf8_test.cpp` that tests the complete UTF-8 parsing flow from tokenization to key registration to lookup, covering requirements 1.2, 1.3, 2.1-2.3, and 3.1-3.2. Must test Japanese key definitions, English alias lookups, case-insensitive matching, and error recovery.
    - **Restrictions**: Do not test implementation details of tokenizer or Key class. Test observable behavior only. Must create temporary .mayu test files for parsing. Clean up test files after tests. Use existing test patterns from tests/core/settings/. Do not modify existing test infrastructure.
    - **Success**: Integration tests verify complete UTF-8 key definition flow works end-to-end. Tests confirm Japanese key names can be defined and looked up. Tests verify English aliases work after Japanese primary names. Tests confirm error recovery when one definition is invalid. All tests pass consistently.
    - **Instructions**: Mark in-progress [-]. Create realistic test .mayu files with Japanese key names. When complete, log with log-implementation tool including filesCreated, test scenarios covered. Mark complete [x].

- [ ] 7. Add end-to-end test with actual Japanese keyboard layout (109.mayu)
  - **Files**: `tests/e2e/japanese_keyboard_layout_test.cpp` (new file), test with actual `keymaps/109.mayu`
  - **Description**: Create end-to-end test that loads the real 109.mayu file with all 169 Japanese key definitions and verifies they all parse correctly
  - **Purpose**: Validate the fix works with the actual production Japanese keyboard layout file
  - **Details**:
    - Load actual `keymaps/109.mayu` file (not a test fixture)
    - Verify all expected keys are registered (count should match)
    - Test looking up common Japanese keys: 無変換, 変換, 英数, 半角/全角, ひらがな
    - Test looking up those keys by English aliases: NonConvert, Convert, Eisuu, Kanji, Hiragana
    - Verify no parsing errors occurred
    - Test that arrow keys (Up, Down, Left, Right) still work after removing Unicode symbols
  - _Leverage: Existing e2e test infrastructure, real keymaps/109.mayu file_
  - _Requirements: All requirements (comprehensive validation)_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: E2E test engineer with expertise in real-world validation and system testing
    - **Task**: Create end-to-end test file `tests/e2e/japanese_keyboard_layout_test.cpp` that loads the actual production `keymaps/109.mayu` file and validates all Japanese key definitions parse correctly and are accessible by both Japanese and English names, covering all requirements comprehensively.
    - **Restrictions**: Must use actual keymaps/109.mayu file, not test fixtures. Do not modify 109.mayu during test (read-only). Test must be idempotent and safe to run multiple times. Must verify sufficient keys were registered (at least 169 from 109.mayu). Do not hard-code all key names - test representative sample.
    - **Success**: E2E test successfully loads actual 109.mayu without errors. Test verifies key count is correct (169+ keys). Test confirms Japanese key names are accessible (無変換, 変換, 英数, etc.). Test confirms English aliases work (NonConvert, Convert, Eisuu, etc.). Test confirms arrow keys (Up, Down, Left, Right) work. Test runs consistently and passes.
    - **Instructions**: Mark in-progress [-]. This test validates the entire fix works in production conditions. When complete, log with log-implementation tool including test coverage and validation results. Mark complete [x]. This is the final validation before considering the spec complete.

- [ ] 8. Update error logging to include UTF-8 context in engine log
  - **File**: `src/core/settings/parser.cpp` (extend error reporting in `readToken()`)
  - **Description**: Ensure UTF-8 parsing errors are written to the engine log with sufficient context for debugging
  - **Purpose**: Help users diagnose UTF-8 encoding issues in their configuration files
  - **Details**:
    - Verify ErrorMessage throws are caught and logged by engine
    - Ensure UTF-8 errors include: filename, line number, column, hex byte value, error type
    - Test that errors appear in `/tmp/yamy-engine.log` on Linux
    - Ensure error messages are user-friendly (explain what UTF-8 is if needed)
  - _Leverage: Existing ErrorMessage and logging infrastructure from msgstream.h fix_
  - _Requirements: 4.4, Non-functional: Usability_
  - _Prompt: **Implement the task for spec utf8-parser-support**, first run spec-workflow-guide to get the workflow guide then implement the task:
    - **Role**: Developer with expertise in error handling, logging, and user experience
    - **Task**: Verify and enhance UTF-8 error logging in the parser (src/core/settings/parser.cpp) to ensure all UTF-8 parsing errors are written to the engine log (/tmp/yamy-engine.log on Linux) with complete context following requirement 4.4 and usability non-functional requirements. Ensure error messages are clear and helpful for non-technical users.
    - **Restrictions**: Do not modify the existing msgstream.h logging infrastructure. Must use existing ErrorMessage class mechanism. Do not add console output (logging goes to engine log only). Error messages must be understandable to users who may not know what UTF-8 is.
    - **Success**: All UTF-8 parsing errors appear in engine log with filename, line, column, byte value. Error messages are clear and actionable. Test by creating .mayu file with invalid UTF-8 and verifying error appears in log. Error messages help users understand what went wrong and how to fix it.
    - **Instructions**: Mark in-progress [-]. Test error logging with various invalid UTF-8 scenarios. When complete, log with log-implementation tool including error message examples. Mark complete [x].

## Task Ordering and Dependencies

**Phase 1: Core Implementation (Tasks 1-4)**
- Task 1 must be completed first (utf8_char_length is needed by tasks 2)
- Tasks 2, 3, 4 can be done in parallel after task 1
- Task 8 can be done alongside task 2

**Phase 2: Testing (Tasks 5-7)**
- Task 5 (unit tests) can start after task 1 is complete
- Task 6 (integration tests) requires tasks 1-4 to be complete
- Task 7 (E2E tests) should be done last, after all other tasks are complete

**Suggested Implementation Order:**
1. Task 1 (utf8_char_length helper)
2. Task 5 (unit tests for utf8_char_length - validates task 1)
3. Tasks 2, 3, 4 in parallel (tokenizer fix, isSymbolChar fix, Key comparison fix)
4. Task 8 (error logging - goes with task 2)
5. Task 6 (integration tests - validates tasks 2-4 together)
6. Task 7 (E2E test with 109.mayu - final validation)

## Completion Criteria

All tasks marked [x] and:
- All unit tests pass
- All integration tests pass
- E2E test with actual 109.mayu passes
- No UTF-8 parsing errors in engine log when loading valid Japanese keyboard layouts
- Both Japanese and English key names work for lookup
- Existing ASCII configurations still work without regression
- Code review completed and approved
