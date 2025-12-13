/**
 * @file parser_utf8_test.cpp
 * @brief Comprehensive unit tests for UTF-8 character length detection
 *
 * Tests cover:
 * - Valid 1-byte ASCII (0x00-0x7F)
 * - Valid 2-byte sequences (0xC0-0xDF + continuation)
 * - Valid 3-byte sequences (0xE0-0xEF + 2 continuations) - Japanese characters
 * - Valid 4-byte sequences (0xF0-0xF7 + 3 continuations)
 * - Invalid: continuation byte as first byte (0x80-0xBF)
 * - Invalid: incomplete sequences (buffer too short)
 * - Invalid: bad continuation bytes (not in 0x80-0xBF range)
 * - Invalid: reserved lead bytes (0xF8-0xFF)
 * - Boundary conditions: buffer end handling
 */

#include <gtest/gtest.h>
#include "core/settings/parser.h"

// =============================================================================
// Test Fixture
// =============================================================================

class Utf8CharLengthTest : public ::testing::Test {
protected:
    bool is_valid = false;

    void SetUp() override {
        is_valid = false;
    }
};

// =============================================================================
// Valid 1-byte ASCII Tests (0x00-0x7F)
// =============================================================================

TEST_F(Utf8CharLengthTest, ValidAsciiNull) {
    const char str[] = "\x00";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, ValidAsciiLetter) {
    const char str[] = "A";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, ValidAsciiDigit) {
    const char str[] = "9";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, ValidAsciiSpace) {
    const char str[] = " ";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, ValidAsciiMaxValue) {
    // 0x7F is DEL, the highest 1-byte ASCII value
    const char str[] = "\x7F";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, ValidAsciiPunctuation) {
    const char str[] = "!";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Valid 2-byte UTF-8 Tests (0xC0-0xDF + continuation)
// =============================================================================

TEST_F(Utf8CharLengthTest, Valid2ByteMinimum) {
    // Minimum 2-byte sequence: 0xC0 0x80 (U+0000 in overlong form, but syntactically valid)
    const char str[] = "\xC0\x80";
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 2);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid2ByteMaximum) {
    // Maximum 2-byte sequence: 0xDF 0xBF (U+07FF)
    const char str[] = "\xDF\xBF";
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 2);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid2ByteLatinExtended) {
    // Latin letter with diacritic (e.g., e with accent)
    const char str[] = "\xC3\xA9";  // U+00E9 (e with acute accent)
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 2);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid2ByteWithExtraBuffer) {
    // Ensure function only consumes 2 bytes even with larger buffer
    const char str[] = "\xC3\xA9XYZ";
    int len = utf8_char_length(str, 5, is_valid);
    EXPECT_EQ(len, 2);
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Valid 3-byte UTF-8 Tests (0xE0-0xEF + 2 continuations) - Japanese chars
// =============================================================================

TEST_F(Utf8CharLengthTest, Valid3ByteJapaneseHiragana) {
    // Hiragana "a" - U+3042
    const char str[] = "\xE3\x81\x82";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid3ByteJapaneseKatakana) {
    // Katakana "a" - U+30A2
    const char str[] = "\xE3\x82\xA2";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid3ByteJapaneseKanji) {
    // Kanji for "sun/day" - U+65E5 (common character)
    const char str[] = "\xE6\x97\xA5";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid3ByteMinimum) {
    // Minimum 3-byte sequence: 0xE0 0x80 0x80 (U+0000 overlong, but syntactically valid)
    const char str[] = "\xE0\x80\x80";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid3ByteMaximum) {
    // Maximum 3-byte sequence: 0xEF 0xBF 0xBF (U+FFFF)
    const char str[] = "\xEF\xBF\xBF";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid3ByteKeyName_Muhenkan) {
    // Japanese key name: "Muhenkan" (No Conversion key)
    const char str[] = "\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B";  // Full string for context
    int len = utf8_char_length(str, 9, is_valid);
    EXPECT_EQ(len, 3);  // First character only
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid3ByteWithExtraBuffer) {
    const char str[] = "\xE3\x81\x82XYZ";
    int len = utf8_char_length(str, 6, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Valid 4-byte UTF-8 Tests (0xF0-0xF7 + 3 continuations)
// =============================================================================

TEST_F(Utf8CharLengthTest, Valid4ByteEmoji) {
    // Grinning face emoji - U+1F600
    const char str[] = "\xF0\x9F\x98\x80";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 4);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid4ByteMinimum) {
    // Minimum 4-byte sequence: 0xF0 0x80 0x80 0x80 (U+0000 overlong)
    const char str[] = "\xF0\x80\x80\x80";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 4);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid4ByteMaximum) {
    // Maximum valid 4-byte sequence: 0xF7 0xBF 0xBF 0xBF
    const char str[] = "\xF7\xBF\xBF\xBF";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 4);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid4ByteSupplementary) {
    // Mathematical bold capital A - U+1D400
    const char str[] = "\xF0\x9D\x90\x80";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 4);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, Valid4ByteWithExtraBuffer) {
    const char str[] = "\xF0\x9F\x98\x80XYZ";
    int len = utf8_char_length(str, 7, is_valid);
    EXPECT_EQ(len, 4);
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Invalid: Continuation byte as first byte (0x80-0xBF)
// =============================================================================

TEST_F(Utf8CharLengthTest, InvalidContinuationAsLeadMin) {
    // 0x80 is a continuation byte, not a valid lead byte
    const char str[] = "\x80";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidContinuationAsLeadMax) {
    // 0xBF is the maximum continuation byte value
    const char str[] = "\xBF";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidContinuationAsLeadMiddle) {
    // 0xA0 is in the middle of continuation range
    const char str[] = "\xA0";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidContinuationWithMoreBytes) {
    // Continuation byte followed by valid ASCII
    const char str[] = "\x80\x41\x42\x43";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

// =============================================================================
// Invalid: Incomplete sequences (buffer too short)
// =============================================================================

TEST_F(Utf8CharLengthTest, InvalidIncomplete2ByteNoBuffer) {
    // 2-byte lead byte but buffer is only 1 byte
    const char str[] = "\xC3";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidIncomplete3ByteBuffer1) {
    // 3-byte lead byte but buffer is only 1 byte
    const char str[] = "\xE3";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidIncomplete3ByteBuffer2) {
    // 3-byte lead byte but buffer is only 2 bytes
    const char str[] = "\xE3\x81";
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidIncomplete4ByteBuffer1) {
    // 4-byte lead byte but buffer is only 1 byte
    const char str[] = "\xF0";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidIncomplete4ByteBuffer2) {
    // 4-byte lead byte but buffer is only 2 bytes
    const char str[] = "\xF0\x9F";
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidIncomplete4ByteBuffer3) {
    // 4-byte lead byte but buffer is only 3 bytes
    const char str[] = "\xF0\x9F\x98";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

// =============================================================================
// Invalid: Bad continuation bytes (not in 0x80-0xBF range)
// =============================================================================

TEST_F(Utf8CharLengthTest, Invalid2ByteBadContinuation_Below) {
    // 2-byte sequence with continuation byte below valid range
    const char str[] = "\xC3\x7F";  // 0x7F is below 0x80
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, Invalid2ByteBadContinuation_Above) {
    // 2-byte sequence with continuation byte above valid range
    const char str[] = "\xC3\xC0";  // 0xC0 is above 0xBF
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, Invalid3ByteBadFirstContinuation) {
    // 3-byte sequence with first continuation byte invalid
    const char str[] = "\xE3\x00\x82";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, Invalid3ByteBadSecondContinuation) {
    // 3-byte sequence with second continuation byte invalid
    const char str[] = "\xE3\x81\xFF";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, Invalid4ByteBadFirstContinuation) {
    // 4-byte sequence with first continuation byte invalid
    const char str[] = "\xF0\x7F\x98\x80";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, Invalid4ByteBadSecondContinuation) {
    // 4-byte sequence with second continuation byte invalid
    const char str[] = "\xF0\x9F\x00\x80";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, Invalid4ByteBadThirdContinuation) {
    // 4-byte sequence with third continuation byte invalid
    const char str[] = "\xF0\x9F\x98\xFF";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

// =============================================================================
// Invalid: Reserved lead bytes (0xF8-0xFF)
// =============================================================================

TEST_F(Utf8CharLengthTest, InvalidReservedLeadF8) {
    const char str[] = "\xF8\x80\x80\x80\x80";
    int len = utf8_char_length(str, 5, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidReservedLeadFC) {
    const char str[] = "\xFC\x80\x80\x80\x80\x80";
    int len = utf8_char_length(str, 6, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidReservedLeadFE) {
    const char str[] = "\xFE";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, InvalidReservedLeadFF) {
    const char str[] = "\xFF";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

// =============================================================================
// Boundary Conditions: Buffer end handling
// =============================================================================

TEST_F(Utf8CharLengthTest, BoundaryZeroLengthBuffer) {
    const char str[] = "A";
    int len = utf8_char_length(str, 0, is_valid);
    EXPECT_EQ(len, 0);
    EXPECT_FALSE(is_valid);
}

TEST_F(Utf8CharLengthTest, BoundaryExactBufferFor1Byte) {
    const char str[] = "X";
    int len = utf8_char_length(str, 1, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, BoundaryExactBufferFor2Byte) {
    const char str[] = "\xC3\xA9";
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 2);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, BoundaryExactBufferFor3Byte) {
    const char str[] = "\xE3\x81\x82";
    int len = utf8_char_length(str, 3, is_valid);
    EXPECT_EQ(len, 3);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, BoundaryExactBufferFor4Byte) {
    const char str[] = "\xF0\x9F\x98\x80";
    int len = utf8_char_length(str, 4, is_valid);
    EXPECT_EQ(len, 4);
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, BoundaryLargeBuffer) {
    // Large buffer should not affect result
    const char str[] = "A";
    int len = utf8_char_length(str, 1000, is_valid);
    EXPECT_EQ(len, 1);
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Realistic test cases from 109.mayu keyboard layout
// =============================================================================

TEST_F(Utf8CharLengthTest, RealisticJapaneseKeyName_Henkan) {
    // Japanese key name sequence
    const char str[] = "\xE5\xA4\x89\xE6\x8F\x9B";  // Full string for context
    int len = utf8_char_length(str, 6, is_valid);
    EXPECT_EQ(len, 3);  // First character only
    EXPECT_TRUE(is_valid);
}

TEST_F(Utf8CharLengthTest, RealisticArrowSymbolRemoved) {
    // Arrow symbols are no longer in the keymap, but test still valid
    // This is ASCII now
    const char str[] = "Up";
    int len = utf8_char_length(str, 2, is_valid);
    EXPECT_EQ(len, 1);  // Just 'U'
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Sequential parsing simulation
// =============================================================================

TEST_F(Utf8CharLengthTest, SequentialParsingMixedString) {
    // Simulate parsing a string with mixed ASCII and UTF-8
    const char str[] = "A\xE3\x81\x82\x42";  // "A" + hiragana + "B"
    size_t pos = 0;
    size_t total_len = 5;

    // First character: 'A' (ASCII)
    int len1 = utf8_char_length(str + pos, total_len - pos, is_valid);
    EXPECT_EQ(len1, 1);
    EXPECT_TRUE(is_valid);
    pos += len1;

    // Second character: hiragana (3 bytes)
    int len2 = utf8_char_length(str + pos, total_len - pos, is_valid);
    EXPECT_EQ(len2, 3);
    EXPECT_TRUE(is_valid);
    pos += len2;

    // Third character: 'B' (ASCII)
    int len3 = utf8_char_length(str + pos, total_len - pos, is_valid);
    EXPECT_EQ(len3, 1);
    EXPECT_TRUE(is_valid);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
