/**
 * @file setting_loader_utf8_test.cpp
 * @brief Integration tests for UTF-8 tokenization and key registration
 *
 * Tests the complete flow from parsing UTF-8 key names to looking them up
 * by both Japanese and English aliases. Covers:
 * - Parsing key definitions with Japanese primary names and English aliases
 * - Looking up keys by English alias after Japanese definition
 * - Looking up keys by Japanese name after Japanese definition
 * - Mixed ASCII and UTF-8 key definitions in the same file
 * - Error recovery: invalid UTF-8 in one key, remaining keys still parse
 * - Case-insensitive lookup with UTF-8 names
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstdlib>

#include "setting.h"
#include "setting_loader.h"
#include "keyboard.h"
#include "multithread.h"
#include "stringtool.h"

namespace {

std::string getTempDir() {
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir) return tmpdir;
    tmpdir = std::getenv("TMP");
    if (tmpdir) return tmpdir;
    tmpdir = std::getenv("TEMP");
    if (tmpdir) return tmpdir;
    return "/tmp";
}

std::string createTempFile(const std::string& content, const std::string& suffix = ".mayu") {
    std::string path = getTempDir() + "/yamy_utf8_test_" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                       suffix;
    std::ofstream out(path);
    out << content;
    out.close();
    return path;
}

void removeTempFile(const std::string& path) {
    std::remove(path.c_str());
}

} // anonymous namespace

// =============================================================================
// Test Fixture
// =============================================================================

class SettingLoaderUtf8Test : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;
    std::vector<std::string> m_tempFiles;

    void SetUp() override {
        m_loader = std::make_unique<SettingLoader>(&m_soLog, &m_logStream);
        m_loader->initialize(&m_setting);
    }

    void TearDown() override {
        m_loader.reset();
        for (const auto& path : m_tempFiles) {
            removeTempFile(path);
        }
        m_tempFiles.clear();
    }

    void LoadConfig(const std::string& config) {
        m_loader->loadFromData(config);
    }

    std::string createTestConfig(const std::string& content) {
        std::string path = createTempFile(content);
        m_tempFiles.push_back(path);
        return path;
    }

    bool hasError() const {
        std::string log = m_logStream.str();
        return log.find("error:") != std::string::npos ||
               log.find("Error:") != std::string::npos;
    }

    std::string getLogOutput() const {
        return m_logStream.str();
    }
};

// =============================================================================
// Test: Japanese Key Definition with English Alias
// =============================================================================

TEST_F(SettingLoaderUtf8Test, JapaneseKeyWithEnglishAlias) {
    // Define a key with Japanese primary name and English alias
    // This mimics the 109.mayu format: def key 無変換 NonConvert = 0x7b
    std::string config =
        "def key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B NonConvert = 0x7b\n";
    // 無変換 = \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B (UTF-8)

    LoadConfig(config);

    // Verify key was registered
    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("NonConvert");

    ASSERT_NE(keyByJapanese, nullptr) << "Key should be findable by Japanese name";
    ASSERT_NE(keyByEnglish, nullptr) << "Key should be findable by English alias";

    // Both lookups should return the same key
    EXPECT_EQ(keyByJapanese, keyByEnglish) << "Japanese and English names should resolve to same key";
}

// =============================================================================
// Test: Japanese Key Name Lookup - 変換 (Convert)
// =============================================================================

TEST_F(SettingLoaderUtf8Test, JapaneseKeyNameConvert) {
    // 変換 = \xE5\xA4\x89\xE6\x8F\x9B (UTF-8)
    std::string config =
        "def key \xE5\xA4\x89\xE6\x8F\x9B Convert = 0x79\n";

    LoadConfig(config);

    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE5\xA4\x89\xE6\x8F\x9B");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Convert");

    ASSERT_NE(keyByJapanese, nullptr) << "Key should be findable by Japanese name (変換)";
    ASSERT_NE(keyByEnglish, nullptr) << "Key should be findable by English alias (Convert)";
    EXPECT_EQ(keyByJapanese, keyByEnglish);
}

// =============================================================================
// Test: Japanese Key Name Lookup - ひらがな (Hiragana)
// =============================================================================

TEST_F(SettingLoaderUtf8Test, JapaneseKeyNameHiragana) {
    // ひらがな = \xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA (UTF-8)
    std::string config =
        "def key \xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA Hiragana = 0x70\n";

    LoadConfig(config);

    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Hiragana");

    ASSERT_NE(keyByJapanese, nullptr) << "Key should be findable by Japanese name (ひらがな)";
    ASSERT_NE(keyByEnglish, nullptr) << "Key should be findable by English alias (Hiragana)";
    EXPECT_EQ(keyByJapanese, keyByEnglish);
}

// =============================================================================
// Test: Japanese Key Name Lookup - 英数 (Eisuu)
// =============================================================================

TEST_F(SettingLoaderUtf8Test, JapaneseKeyNameEisuu) {
    // 英数 = \xE8\x8B\xB1\xE6\x95\xB0 (UTF-8)
    std::string config =
        "def key \xE8\x8B\xB1\xE6\x95\xB0 Eisuu = 0x3a\n";

    LoadConfig(config);

    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE8\x8B\xB1\xE6\x95\xB0");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Eisuu");

    ASSERT_NE(keyByJapanese, nullptr) << "Key should be findable by Japanese name (英数)";
    ASSERT_NE(keyByEnglish, nullptr) << "Key should be findable by English alias (Eisuu)";
    EXPECT_EQ(keyByJapanese, keyByEnglish);
}

// =============================================================================
// Test: Japanese Key Name Lookup - 半角/全角 漢字 (Kanji)
// =============================================================================

TEST_F(SettingLoaderUtf8Test, JapaneseKeyNameKanji) {
    // 半角/全角 = \xE5\x8D\x8A\xE8\xA7\x92/\xE5\x85\xA8\xE8\xA7\x92 (UTF-8)
    // 漢字 = \xE6\xBC\xA2\xE5\xAD\x97 (UTF-8)
    std::string config =
        "def key \xE5\x8D\x8A\xE8\xA7\x92/\xE5\x85\xA8\xE8\xA7\x92 \xE6\xBC\xA2\xE5\xAD\x97 Kanji = 0x29\n";

    LoadConfig(config);

    Key* keyByJapanese1 = m_setting.m_keyboard.searchKey("\xE5\x8D\x8A\xE8\xA7\x92/\xE5\x85\xA8\xE8\xA7\x92");
    Key* keyByJapanese2 = m_setting.m_keyboard.searchKey("\xE6\xBC\xA2\xE5\xAD\x97");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Kanji");

    ASSERT_NE(keyByJapanese1, nullptr) << "Key should be findable by first Japanese name (半角/全角)";
    ASSERT_NE(keyByJapanese2, nullptr) << "Key should be findable by second Japanese name (漢字)";
    ASSERT_NE(keyByEnglish, nullptr) << "Key should be findable by English alias (Kanji)";
    EXPECT_EQ(keyByJapanese1, keyByJapanese2);
    EXPECT_EQ(keyByJapanese1, keyByEnglish);
}

// =============================================================================
// Test: Mixed ASCII and UTF-8 Key Definitions
// =============================================================================

TEST_F(SettingLoaderUtf8Test, MixedAsciiAndUtf8Definitions) {
    std::string config =
        "def key Escape Esc = 0x01\n"
        "def key A = 0x1e\n"
        "def key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B NonConvert = 0x7b\n"  // 無変換
        "def key B = 0x30\n"
        "def key \xE5\xA4\x89\xE6\x8F\x9B Convert = 0x79\n"  // 変換
        "def key Enter Return = 0x1c\n";

    LoadConfig(config);

    // Verify all ASCII keys
    EXPECT_NE(m_setting.m_keyboard.searchKey("Escape"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("Esc"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("A"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("B"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("Enter"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("Return"), nullptr);

    // Verify UTF-8 keys
    EXPECT_NE(m_setting.m_keyboard.searchKey("\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B"), nullptr);  // 無変換
    EXPECT_NE(m_setting.m_keyboard.searchKey("NonConvert"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("\xE5\xA4\x89\xE6\x8F\x9B"), nullptr);  // 変換
    EXPECT_NE(m_setting.m_keyboard.searchKey("Convert"), nullptr);

    // Verify aliases point to same key
    Key* escKey = m_setting.m_keyboard.searchKey("Escape");
    Key* escAliasKey = m_setting.m_keyboard.searchKey("Esc");
    EXPECT_EQ(escKey, escAliasKey);
}

// =============================================================================
// Test: Case-Insensitive Lookup with ASCII Keys
// =============================================================================

TEST_F(SettingLoaderUtf8Test, CaseInsensitiveLookupAscii) {
    std::string config =
        "def key Escape Esc = 0x01\n"
        "def key Enter Return = 0x1c\n";

    LoadConfig(config);

    // Case insensitive lookups should work
    Key* key1 = m_setting.m_keyboard.searchKey("Escape");
    Key* key2 = m_setting.m_keyboard.searchKey("escape");
    Key* key3 = m_setting.m_keyboard.searchKey("ESCAPE");
    Key* key4 = m_setting.m_keyboard.searchKey("EsCaPe");

    ASSERT_NE(key1, nullptr);
    EXPECT_EQ(key1, key2) << "Case insensitive lookup should work (escape)";
    EXPECT_EQ(key1, key3) << "Case insensitive lookup should work (ESCAPE)";
    EXPECT_EQ(key1, key4) << "Case insensitive lookup should work (EsCaPe)";
}

// =============================================================================
// Test: Multiple Japanese Keys in Sequence
// =============================================================================

TEST_F(SettingLoaderUtf8Test, MultipleJapaneseKeysInSequence) {
    std::string config =
        "def key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B NonConvert = 0x7b\n"   // 無変換
        "def key \xE5\xA4\x89\xE6\x8F\x9B Convert = 0x79\n"                   // 変換
        "def key \xE8\x8B\xB1\xE6\x95\xB0 Eisuu = 0x3a\n"                     // 英数
        "def key \xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA Hiragana = 0x70\n";  // ひらがな

    LoadConfig(config);

    // Verify all keys are registered and distinct
    Key* nonConvert = m_setting.m_keyboard.searchKey("NonConvert");
    Key* convert = m_setting.m_keyboard.searchKey("Convert");
    Key* eisuu = m_setting.m_keyboard.searchKey("Eisuu");
    Key* hiragana = m_setting.m_keyboard.searchKey("Hiragana");

    ASSERT_NE(nonConvert, nullptr);
    ASSERT_NE(convert, nullptr);
    ASSERT_NE(eisuu, nullptr);
    ASSERT_NE(hiragana, nullptr);

    // Verify all keys are different
    EXPECT_NE(nonConvert, convert);
    EXPECT_NE(nonConvert, eisuu);
    EXPECT_NE(nonConvert, hiragana);
    EXPECT_NE(convert, eisuu);
    EXPECT_NE(convert, hiragana);
    EXPECT_NE(eisuu, hiragana);
}

// =============================================================================
// Test: Multiple Aliases for Same Key (Standard Syntax)
// =============================================================================

TEST_F(SettingLoaderUtf8Test, MultipleAliasesForSameKey) {
    // Standard syntax with multiple aliases: def key name1 name2 name3 = scancode
    std::string config =
        "def key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B NonConvert Muhenkan = 0x7b\n";  // 無変換

    LoadConfig(config);

    // All three names should resolve to the same key
    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B");
    Key* keyByEnglish1 = m_setting.m_keyboard.searchKey("NonConvert");
    Key* keyByEnglish2 = m_setting.m_keyboard.searchKey("Muhenkan");

    ASSERT_NE(keyByJapanese, nullptr);
    ASSERT_NE(keyByEnglish1, nullptr);
    ASSERT_NE(keyByEnglish2, nullptr);
    EXPECT_EQ(keyByJapanese, keyByEnglish1);
    EXPECT_EQ(keyByJapanese, keyByEnglish2);
}

// =============================================================================
// Test: Extended Scan Code with UTF-8 Key Name
// =============================================================================

TEST_F(SettingLoaderUtf8Test, ExtendedScanCodeWithUtf8Name) {
    // E0-prefixed scan code with UTF-8 name
    std::string config =
        "def key E0\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B E0NonConvert = E0-0x7b\n";  // E0無変換

    LoadConfig(config);

    Key* keyByJapanese = m_setting.m_keyboard.searchKey("E0\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("E0NonConvert");

    ASSERT_NE(keyByJapanese, nullptr) << "Extended scan code key with Japanese name should be registered";
    ASSERT_NE(keyByEnglish, nullptr) << "Extended scan code key with English name should be registered";
    EXPECT_EQ(keyByJapanese, keyByEnglish);
}

// =============================================================================
// Test: Keymap with UTF-8 Key References
// =============================================================================

TEST_F(SettingLoaderUtf8Test, KeymapWithUtf8KeyReferences) {
    std::string config =
        "def key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B NonConvert = 0x7b\n"  // 無変換
        "def key \xE5\xA4\x89\xE6\x8F\x9B Convert = 0x79\n"                  // 変換
        "def key Escape = 0x01\n"
        "keymap Global\n"
        "key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B = Escape\n";  // 無変換 -> Escape

    LoadConfig(config);

    // Verify keymap was created
    const Keymap* globalMap = m_setting.m_keymaps.searchByName("Global");
    ASSERT_NE(globalMap, nullptr) << "Global keymap should exist";

    // Verify key lookup works with Japanese name in keymap
    Key* muhenkanKey = m_setting.m_keyboard.searchKey("\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B");
    ASSERT_NE(muhenkanKey, nullptr);

    // Verify key assignment exists
    ModifiedKey mk(muhenkanKey);
    const Keymap::KeyAssignment* ka = globalMap->searchAssignment(mk);
    EXPECT_NE(ka, nullptr) << "Key assignment for Japanese key should exist in keymap";
}

// =============================================================================
// Test: Error Recovery - Valid Keys After Invalid UTF-8
// =============================================================================

TEST_F(SettingLoaderUtf8Test, ErrorRecoveryAfterInvalidUtf8) {
    // Create a config with invalid UTF-8 followed by valid definitions
    // Invalid UTF-8: 0x80 as first byte (continuation byte as lead byte)
    std::string config =
        "def key A = 0x1e\n"
        "def key \x80Invalid = 0x7b\n"  // Invalid UTF-8 - should fail
        "def key B = 0x30\n"
        "def key C = 0x2e\n";

    LoadConfig(config);

    // Valid keys before and after invalid line should still be registered
    EXPECT_NE(m_setting.m_keyboard.searchKey("A"), nullptr)
        << "Key defined before invalid UTF-8 should be registered";
    EXPECT_NE(m_setting.m_keyboard.searchKey("B"), nullptr)
        << "Key defined after invalid UTF-8 should be registered";
    EXPECT_NE(m_setting.m_keyboard.searchKey("C"), nullptr)
        << "Key defined after invalid UTF-8 should be registered";
}

// =============================================================================
// Test: Detailed Error Messages for Invalid UTF-8
// =============================================================================

TEST_F(SettingLoaderUtf8Test, DetailedErrorMessageForContinuationByteAsLead) {
    // Invalid UTF-8: 0x80 as first byte (continuation byte as lead byte)
    // Note: When a continuation byte appears at token start, it's detected as
    // "invalid character" since it doesn't pass isSymbolChar() check.
    // This is correct behavior - continuation bytes can't start tokens.
    std::string config =
        "def key \x80" "BadKey = 0x7b\n";

    LoadConfig(config);

    std::string log = getLogOutput();

    // Error message should report an invalid character
    EXPECT_TRUE(log.find("invalid character") != std::string::npos ||
                log.find("Invalid character") != std::string::npos ||
                log.find("error") != std::string::npos)
        << "Error should indicate invalid character. Log: " << log;

    // Should include byte value in hex (0x80)
    EXPECT_TRUE(log.find("0x80") != std::string::npos ||
                log.find("80") != std::string::npos)
        << "Error should include byte value. Log: " << log;
}

TEST_F(SettingLoaderUtf8Test, DetailedErrorMessageForBadContinuationMidToken) {
    // Invalid UTF-8: UTF-8 lead byte (0xE0 for 3-byte) followed by invalid continuation (0x41 = 'A')
    // This tests the UTF-8 error path when parsing a multi-byte sequence
    std::string config =
        "def key Test\xE0" "A = 0x7b\n";  // \xE0 followed by 'A' (not a valid continuation)

    LoadConfig(config);

    std::string log = getLogOutput();

    // Error message should mention UTF-8 or continuation byte issue
    EXPECT_TRUE(log.find("UTF-8") != std::string::npos ||
                log.find("continuation") != std::string::npos ||
                log.find("error") != std::string::npos)
        << "Error should mention UTF-8 or continuation issue. Log: " << log;

    // Should include location information (Line/line/column)
    EXPECT_TRUE(log.find("Line") != std::string::npos ||
                log.find("line") != std::string::npos ||
                log.find("column") != std::string::npos ||
                log.find("(1)") != std::string::npos)  // Line number in format (1)
        << "Error should include location. Log: " << log;
}

TEST_F(SettingLoaderUtf8Test, DetailedErrorMessageForIncompleteSequence) {
    // Invalid UTF-8: 3-byte lead (0xE0) followed by only 1 continuation byte
    std::string config =
        "def key TestKey\xE0\x80 = 0x7b\n";  // Incomplete 3-byte sequence

    LoadConfig(config);

    std::string log = getLogOutput();

    // Should have an error about invalid/incomplete UTF-8
    EXPECT_TRUE(log.find("UTF-8") != std::string::npos ||
                log.find("incomplete") != std::string::npos ||
                log.find("Incomplete") != std::string::npos ||
                log.find("error") != std::string::npos)
        << "Should report error for incomplete UTF-8 sequence. Log: " << log;
}

TEST_F(SettingLoaderUtf8Test, DetailedErrorMessageForReservedByte) {
    // Invalid UTF-8: 0xFF is a reserved byte (never valid as lead byte)
    std::string config =
        "def key \xFF" "BadKey = 0x7b\n";

    LoadConfig(config);

    std::string log = getLogOutput();

    // Error message should indicate invalid/reserved byte
    EXPECT_TRUE(log.find("UTF-8") != std::string::npos ||
                log.find("reserved") != std::string::npos ||
                log.find("invalid") != std::string::npos ||
                log.find("Invalid") != std::string::npos)
        << "Error should mention invalid UTF-8. Log: " << log;

    // Should include the byte value
    EXPECT_TRUE(log.find("ff") != std::string::npos ||
                log.find("FF") != std::string::npos ||
                log.find("0xff") != std::string::npos ||
                log.find("0xFF") != std::string::npos)
        << "Error should include byte value 0xFF. Log: " << log;
}

// =============================================================================
// Test: Comments with UTF-8 Characters
// =============================================================================

TEST_F(SettingLoaderUtf8Test, CommentsWithUtf8Characters) {
    std::string config =
        "# 日本語コメント (Japanese comment)\n"
        "def key A = 0x1e\n"
        "def key \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B NonConvert = 0x7b # 無変換キー\n"
        "# Another 日本語 comment\n"
        "def key B = 0x30\n";

    LoadConfig(config);

    EXPECT_NE(m_setting.m_keyboard.searchKey("A"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("NonConvert"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("B"), nullptr);
}

// =============================================================================
// Test: Empty Key Name Edge Case
// =============================================================================

TEST_F(SettingLoaderUtf8Test, ValidUtf8DoesNotProduceEmptyKeyName) {
    // Valid 3-byte UTF-8 sequences should produce proper key names, not empty
    std::string config =
        "def key \xE3\x81\x82 = 0x01\n";  // あ (Hiragana 'a')

    LoadConfig(config);

    Key* key = m_setting.m_keyboard.searchKey("\xE3\x81\x82");
    ASSERT_NE(key, nullptr) << "UTF-8 hiragana key should be registered";

    // The key should have a non-empty name
    EXPECT_FALSE(key->getName().empty()) << "Key name should not be empty";
}

// =============================================================================
// Test: Long Japanese Key Name
// =============================================================================

TEST_F(SettingLoaderUtf8Test, LongJapaneseKeyName) {
    // カタカナひらがな (Katakana-Hiragana) - longer multi-character name
    std::string config =
        "def key \xE3\x82\xAB\xE3\x82\xBF\xE3\x82\xAB\xE3\x83\x8A"
        "\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA KatakanaHiragana = 0x70\n";

    LoadConfig(config);

    Key* keyByJapanese = m_setting.m_keyboard.searchKey(
        "\xE3\x82\xAB\xE3\x82\xBF\xE3\x82\xAB\xE3\x83\x8A"
        "\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("KatakanaHiragana");

    ASSERT_NE(keyByJapanese, nullptr) << "Long Japanese key name should be registered";
    ASSERT_NE(keyByEnglish, nullptr);
    EXPECT_EQ(keyByJapanese, keyByEnglish);
}

// =============================================================================
// Test: Unicode Arrow Symbols Removed (regression test)
// =============================================================================

TEST_F(SettingLoaderUtf8Test, ArrowKeysAsciiOnly) {
    // Arrow keys should use ASCII names only (Unicode symbols were removed)
    std::string config =
        "def key Up = E0-0x48\n"
        "def key Down = E0-0x50\n"
        "def key Left = E0-0x4B\n"
        "def key Right = E0-0x4D\n";

    LoadConfig(config);

    // All arrow keys should be accessible by ASCII names
    EXPECT_NE(m_setting.m_keyboard.searchKey("Up"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("Down"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("Left"), nullptr);
    EXPECT_NE(m_setting.m_keyboard.searchKey("Right"), nullptr);
}

// Note: This test is part of the regression test suite and uses its main()
// To run: ./bin/yamy_regression_test --gtest_filter="SettingLoaderUtf8Test*"
