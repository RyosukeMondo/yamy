# Requirements Document: UTF-8 Multi-Byte Character Support in Configuration Parser

## Introduction

The YAMY configuration parser currently fails to correctly process multi-byte UTF-8 characters (Japanese, Korean, Chinese, and other non-ASCII text) in keyboard key definitions within `.mayu` configuration files. This limitation prevents international keyboard users from using their native language in configuration files and blocks proper support for IME-related keys (無変換, 変換, 英数, etc.) that are essential for Japanese keyboard layouts.

The parser's tokenizer incorrectly assumes UTF-8 characters are at most 2 bytes and misaligns character boundaries, causing key definitions with Japanese primary names (e.g., `def key 無変換 NonConvert = 0x7b`) to fail registration. Users are currently forced to manually edit 169+ key definitions in `109.mayu` to use ASCII-only names, breaking the original Japanese keyboard layout support.

This feature will fix the UTF-8 handling in the configuration parser to properly support international users and restore full Japanese keyboard layout functionality.

## Alignment with Product Vision

This feature directly supports **Tertiary Target Users: International Keyboard Users** identified in `product.md`:
- **Pain Point Addressed**: "Non-US keyboard layouts poorly supported" - Japanese keyboard key names fail to parse
- **Pain Point Addressed**: "Want to remap language-switching keys" - IME keys (無変換, 変換, 英数) cannot be used with Japanese names
- **Goal Enabled**: "Seamless multilingual input" - Allows configuration files in user's native language

Additionally supports **Product Goals > Short-Term (Q1 2025)**:
- **Linux Feature Parity**: Achieves parity with Windows version which handles UTF-8 correctly (Windows uses wide character APIs)
- **Documentation**: Enables Japanese documentation with native examples

Enables **User Outcome > For International Users**:
- **Before**: "I must manually edit 169 key definitions to English, breaking Japanese layout support"
- **After**: "My Japanese .mayu files work perfectly, I can use native key names"

## Requirements

### Requirement 1: Correct UTF-8 Character Tokenization

**User Story:** As an international keyboard user, I want the parser to correctly tokenize multi-byte UTF-8 characters in configuration files, so that I can use key names in my native language (Japanese, Korean, Chinese, etc.).

#### Acceptance Criteria

1. WHEN the tokenizer encounters a UTF-8 lead byte (0xC0-0xFF) THEN the parser SHALL correctly identify the character length (2, 3, or 4 bytes based on lead byte value)
2. WHEN the tokenizer processes a multi-byte UTF-8 character THEN the parser SHALL skip the correct number of continuation bytes (0x80-0xBF) to maintain proper character boundaries
3. WHEN a key is defined as `def key 無変換 NonConvert = 0x7b` THEN the parser SHALL tokenize "無変換" as a single complete token (6 bytes: E7 84 A1 E5 A4 89 E6 8F 9B)
4. WHEN the tokenizer encounters a UTF-8 continuation byte (0x80-0xBF) outside of a multi-byte sequence THEN the parser SHALL treat it as an error and report invalid UTF-8 encoding
5. WHEN parsing a configuration file with mixed ASCII and UTF-8 characters THEN the parser SHALL correctly tokenize both ASCII (single-byte) and UTF-8 (multi-byte) tokens without corruption

### Requirement 2: Proper UTF-8 Key Name Registration and Lookup

**User Story:** As a user with Japanese keyboard configuration files, I want key definitions with Japanese primary names to register correctly, so that both Japanese and English key name aliases work for remapping.

#### Acceptance Criteria

1. WHEN a key is defined as `def key 無変換 NonConvert = 0x7b` THEN the Key object SHALL store both "無変換" and "NonConvert" as valid names
2. WHEN looking up a key by name "NonConvert" THEN the parser SHALL find the key defined with Japanese primary name "無変換 NonConvert"
3. WHEN looking up a key by name "無変換" THEN the parser SHALL find the key using UTF-8-aware string comparison
4. WHEN comparing UTF-8 key names THEN the system SHALL use the existing `strcasecmp_utf8()` utility for case-insensitive comparison (already used in Token class)
5. IF a key name contains invalid UTF-8 byte sequences THEN the parser SHALL report a clear error message indicating the line number and corrupted character

### Requirement 3: Backward Compatibility with Existing ASCII Configurations

**User Story:** As an existing YAMY user with ASCII-only configuration files, I want the UTF-8 parser fix to maintain full backward compatibility, so that my existing `.mayu` files continue to work without modification.

#### Acceptance Criteria

1. WHEN parsing an ASCII-only configuration file (all characters 0x00-0x7F) THEN the parser SHALL produce identical results as before the UTF-8 fix
2. WHEN a key is defined with only ASCII names (e.g., `def key Esc Escape = 0x01`) THEN the key SHALL register and lookup exactly as it did before
3. WHEN the tokenizer encounters single-byte ASCII characters THEN the parser SHALL process them with no performance degradation compared to the current implementation
4. WHEN parsing large ASCII configuration files (>10KB) THEN the parsing time SHALL not increase by more than 5%

### Requirement 4: UTF-8 Validation and Error Reporting

**User Story:** As a configuration file author, I want clear error messages when my file contains invalid UTF-8 sequences, so that I can quickly identify and fix encoding issues.

#### Acceptance Criteria

1. WHEN the parser encounters an invalid UTF-8 lead byte (0x80-0xBF as first byte, or 0xF8-0xFF) THEN the parser SHALL report error: "Invalid UTF-8 encoding at line X: invalid lead byte 0xYY"
2. WHEN a multi-byte sequence is incomplete (e.g., 2-byte character with only 1 byte before whitespace) THEN the parser SHALL report error: "Invalid UTF-8 encoding at line X: incomplete multi-byte sequence"
3. WHEN a continuation byte is expected but a non-continuation byte (0x00-0x7F or 0xC0-0xFF) appears THEN the parser SHALL report error: "Invalid UTF-8 encoding at line X: expected continuation byte, got 0xYY"
4. WHEN reporting UTF-8 errors THEN the parser SHALL include the line number, column number (if available), and the hexadecimal value of the problematic byte

### Requirement 5: Cross-Platform UTF-8 Consistency

**User Story:** As a cross-platform user, I want the same `.mayu` configuration file with Japanese key names to work identically on both Windows and Linux, so that I can share configurations across platforms.

#### Acceptance Criteria

1. WHEN a configuration file is saved with UTF-8 encoding (with or without BOM) on Windows THEN the Linux parser SHALL correctly process it
2. WHEN a configuration file is saved with UTF-8 encoding (no BOM) on Linux THEN the Windows parser SHALL correctly process it
3. WHEN a key name contains Japanese characters THEN the key SHALL be accessible by the same name on both Windows and Linux using UTF-8-aware comparison
4. IF a configuration file contains a UTF-8 BOM (0xEF 0xBB 0xBF) THEN the parser SHALL automatically skip it and parse the file correctly (this is already implemented)

## Non-Functional Requirements

### Code Architecture and Modularity

- **Single Responsibility Principle**: The tokenizer fix shall be contained in `parser.cpp` with a dedicated `utf8_char_length()` helper function for UTF-8 byte sequence length calculation
- **Modular Design**: UTF-8 utilities shall use the existing `stringtool.h` infrastructure (which already has `strcasecmp_utf8()`)
- **Dependency Management**: The fix shall not introduce new third-party dependencies - use standard C++17 and existing utilities only
- **Clear Interfaces**: The `Token` class interface shall remain unchanged; only internal tokenization logic is modified

### Performance

- **Input Latency**: UTF-8 character processing SHALL NOT add measurable latency to configuration file parsing (target: <5ms for typical 10KB `.mayu` file)
- **Memory Overhead**: Token storage SHALL NOT increase memory usage beyond the actual UTF-8 character bytes (no wide character conversion or duplication)
- **Parsing Speed**: Tokenization of UTF-8 characters SHALL be implemented with simple byte checks (no regex, no complex state machines)

### Security

- **Input Validation**: The parser SHALL validate all UTF-8 byte sequences against the UTF-8 specification (RFC 3629) to prevent buffer overruns from malformed sequences
- **Bounds Checking**: When advancing pointers for multi-byte sequences, the parser SHALL verify `t + N` does not exceed buffer bounds before dereferencing
- **Denial of Service**: The parser SHALL reject files with excessive invalid UTF-8 sequences (>100 errors) to prevent error message flooding

### Reliability

- **Graceful Degradation**: If a single key definition contains invalid UTF-8, the parser SHALL skip only that definition and continue parsing the rest of the file
- **Error Recovery**: After encountering an invalid UTF-8 sequence, the parser SHALL resynchronize at the next whitespace or newline character
- **Test Coverage**: UTF-8 tokenization logic SHALL have unit tests covering all valid UTF-8 character ranges (1-byte, 2-byte, 3-byte, 4-byte) and common error cases (incomplete sequences, invalid lead bytes, unexpected continuation bytes)

### Usability

- **Transparent Operation**: Users SHALL NOT need to modify existing configuration files or change any settings to benefit from UTF-8 support
- **Clear Documentation**: The parser error messages for UTF-8 issues SHALL be understandable to non-technical users (no hexadecimal dumps without explanation)
- **Logging**: UTF-8 parsing errors SHALL be written to the engine log (`/tmp/yamy-engine.log` on Linux) with sufficient context for debugging
