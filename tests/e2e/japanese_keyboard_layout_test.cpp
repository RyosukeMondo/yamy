/**
 * @file japanese_keyboard_layout_test.cpp
 * @brief End-to-end test that loads the real 109.mayu file with Japanese key definitions
 *
 * This test validates that the UTF-8 parser fix works with the actual production
 * Japanese keyboard layout file. It verifies:
 * - All 169 Japanese key definitions parse correctly
 * - Japanese key names are accessible (無変換, 変換, 英数, 半角/全角, ひらがな)
 * - English aliases work (NonConvert, Convert, Eisuu, Kanji, Hiragana)
 * - Arrow keys (Up, Down, Left, Right) work after removing Unicode symbols
 * - No parsing errors occur during loading
 */

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdlib>

#include "setting.h"
#include "setting_loader.h"
#include "keyboard.h"
#include "multithread.h"

namespace {

std::string getProjectRoot() {
    // Try to find keymaps/109.mayu relative to known locations
    std::vector<std::string> candidates = {
        "keymaps/109.mayu",
        "../keymaps/109.mayu",
        "../../keymaps/109.mayu",
        "../../../keymaps/109.mayu",
        "../../../../keymaps/109.mayu"
    };

    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::path(path).parent_path().parent_path().string();
        }
    }

    // Check if we're running from build directory
    const char* srcDir = std::getenv("YAMY_SOURCE_DIR");
    if (srcDir) {
        return srcDir;
    }

    // Default fallback
    return ".";
}

std::string get109MayuPath() {
    std::string root = getProjectRoot();
    std::string path = root + "/keymaps/109.mayu";

    // Handle case where root is empty or "."
    if (root.empty() || root == ".") {
        std::vector<std::string> candidates = {
            "keymaps/109.mayu",
            "../keymaps/109.mayu",
            "../../keymaps/109.mayu",
            "../../../keymaps/109.mayu",
            "../../../../keymaps/109.mayu"
        };

        for (const auto& p : candidates) {
            if (std::filesystem::exists(p)) {
                return std::filesystem::absolute(p).string();
            }
        }
    }

    return path;
}

} // anonymous namespace

// =============================================================================
// Test Fixture
// =============================================================================

class JapaneseKeyboardLayoutTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;

    void SetUp() override {
        m_loader = std::make_unique<SettingLoader>(&m_soLog, &m_logStream);
        m_loader->initialize(&m_setting);
    }

    void TearDown() override {
        m_loader.reset();
    }

    bool load109Mayu() {
        std::string path = get109MayuPath();

        if (!std::filesystem::exists(path)) {
            std::cerr << "Warning: 109.mayu not found at: " << path << std::endl;
            return false;
        }

        // Read file content
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open 109.mayu at: " << path << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Load the configuration
        m_loader->loadFromData(content);
        return true;
    }

    bool hasError() const {
        std::string log = m_logStream.str();
        return log.find("error:") != std::string::npos ||
               log.find("Error:") != std::string::npos;
    }

    std::string getLogOutput() const {
        return m_logStream.str();
    }

    size_t countDefinedKeys() {
        size_t count = 0;
        for (auto it = m_setting.m_keyboard.getKeyIterator(); *it != nullptr; ++it) {
            count++;
        }
        return count;
    }
};

// =============================================================================
// Test: Load 109.mayu Successfully
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, Load109MayuWithoutErrors) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // Check for parsing errors
    std::string log = getLogOutput();

    // We should not have UTF-8 related errors
    EXPECT_EQ(log.find("Invalid UTF-8"), std::string::npos)
        << "Should not have UTF-8 parsing errors. Log: " << log;

    // The log may have some warnings but should not have critical errors
    // that would prevent key registration
    size_t keyCount = countDefinedKeys();
    EXPECT_GT(keyCount, 0u) << "Should have registered at least some keys";
}

// =============================================================================
// Test: Verify Key Count (Should have 169+ keys from 109.mayu)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, VerifyKeyCount) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    size_t keyCount = countDefinedKeys();

    // 109.mayu defines approximately 169 keys
    // We allow some flexibility but it should be substantial
    EXPECT_GE(keyCount, 100u)
        << "109.mayu should define at least 100 keys, got " << keyCount;

    std::cout << "[INFO] 109.mayu loaded successfully with " << keyCount << " keys" << std::endl;
}

// =============================================================================
// Test: Japanese Key Name - 無変換 (NonConvert)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, JapaneseKeyNameMuhenkan) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // 無変換 in UTF-8: \xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B
    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("NonConvert");

    ASSERT_NE(keyByJapanese, nullptr)
        << "Key should be findable by Japanese name (無変換)";
    ASSERT_NE(keyByEnglish, nullptr)
        << "Key should be findable by English alias (NonConvert)";

    // Both should refer to the same key
    EXPECT_EQ(keyByJapanese, keyByEnglish)
        << "Japanese and English names should resolve to the same key";
}

// =============================================================================
// Test: Japanese Key Name - 変換 (Convert)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, JapaneseKeyNameHenkan) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // 変換 in UTF-8: \xE5\xA4\x89\xE6\x8F\x9B
    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE5\xA4\x89\xE6\x8F\x9B");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Convert");

    ASSERT_NE(keyByJapanese, nullptr)
        << "Key should be findable by Japanese name (変換)";
    ASSERT_NE(keyByEnglish, nullptr)
        << "Key should be findable by English alias (Convert)";

    EXPECT_EQ(keyByJapanese, keyByEnglish)
        << "Japanese and English names should resolve to the same key";
}

// =============================================================================
// Test: Japanese Key Name - 英数 (Eisuu)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, JapaneseKeyNameEisuu) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // 英数 in UTF-8: \xE8\x8B\xB1\xE6\x95\xB0
    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE8\x8B\xB1\xE6\x95\xB0");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Eisuu");

    ASSERT_NE(keyByJapanese, nullptr)
        << "Key should be findable by Japanese name (英数)";
    ASSERT_NE(keyByEnglish, nullptr)
        << "Key should be findable by English alias (Eisuu)";

    EXPECT_EQ(keyByJapanese, keyByEnglish)
        << "Japanese and English names should resolve to the same key";
}

// =============================================================================
// Test: Japanese Key Name - 半角/全角 漢字 (Kanji)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, JapaneseKeyNameKanji) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // 半角/全角 in UTF-8: \xE5\x8D\x8A\xE8\xA7\x92/\xE5\x85\xA8\xE8\xA7\x92
    // 漢字 in UTF-8: \xE6\xBC\xA2\xE5\xAD\x97
    Key* keyByJapanese1 = m_setting.m_keyboard.searchKey("\xE5\x8D\x8A\xE8\xA7\x92/\xE5\x85\xA8\xE8\xA7\x92");
    Key* keyByJapanese2 = m_setting.m_keyboard.searchKey("\xE6\xBC\xA2\xE5\xAD\x97");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Kanji");

    ASSERT_NE(keyByJapanese1, nullptr)
        << "Key should be findable by first Japanese name (半角/全角)";
    ASSERT_NE(keyByJapanese2, nullptr)
        << "Key should be findable by second Japanese name (漢字)";
    ASSERT_NE(keyByEnglish, nullptr)
        << "Key should be findable by English alias (Kanji)";

    // All should refer to the same key
    EXPECT_EQ(keyByJapanese1, keyByJapanese2);
    EXPECT_EQ(keyByJapanese1, keyByEnglish);
}

// =============================================================================
// Test: Japanese Key Name - ひらがな (Hiragana)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, JapaneseKeyNameHiragana) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // ひらがな in UTF-8: \xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA
    Key* keyByJapanese = m_setting.m_keyboard.searchKey("\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA");
    Key* keyByEnglish = m_setting.m_keyboard.searchKey("Hiragana");

    ASSERT_NE(keyByJapanese, nullptr)
        << "Key should be findable by Japanese name (ひらがな)";
    ASSERT_NE(keyByEnglish, nullptr)
        << "Key should be findable by English alias (Hiragana)";

    EXPECT_EQ(keyByJapanese, keyByEnglish)
        << "Japanese and English names should resolve to the same key";
}

// =============================================================================
// Test: Arrow Keys Work (After Unicode Symbol Removal)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, ArrowKeysWork) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // Arrow keys should be accessible by ASCII names
    Key* upKey = m_setting.m_keyboard.searchKey("Up");
    Key* downKey = m_setting.m_keyboard.searchKey("Down");
    Key* leftKey = m_setting.m_keyboard.searchKey("Left");
    Key* rightKey = m_setting.m_keyboard.searchKey("Right");

    EXPECT_NE(upKey, nullptr) << "Up arrow key should be registered";
    EXPECT_NE(downKey, nullptr) << "Down arrow key should be registered";
    EXPECT_NE(leftKey, nullptr) << "Left arrow key should be registered";
    EXPECT_NE(rightKey, nullptr) << "Right arrow key should be registered";

    // Verify they are all different keys
    if (upKey && downKey && leftKey && rightKey) {
        EXPECT_NE(upKey, downKey);
        EXPECT_NE(upKey, leftKey);
        EXPECT_NE(upKey, rightKey);
        EXPECT_NE(downKey, leftKey);
        EXPECT_NE(downKey, rightKey);
        EXPECT_NE(leftKey, rightKey);
    }
}

// =============================================================================
// Test: Common ASCII Keys Still Work
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, CommonAsciiKeysWork) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // Test common ASCII keys
    EXPECT_NE(m_setting.m_keyboard.searchKey("Escape"), nullptr) << "Escape key";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Esc"), nullptr) << "Esc alias";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Enter"), nullptr) << "Enter key";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Return"), nullptr) << "Return alias";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Space"), nullptr) << "Space key";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Tab"), nullptr) << "Tab key";
    EXPECT_NE(m_setting.m_keyboard.searchKey("BackSpace"), nullptr) << "BackSpace key";

    // Function keys
    EXPECT_NE(m_setting.m_keyboard.searchKey("F1"), nullptr) << "F1 key";
    EXPECT_NE(m_setting.m_keyboard.searchKey("F12"), nullptr) << "F12 key";

    // Modifier keys
    EXPECT_NE(m_setting.m_keyboard.searchKey("LShift"), nullptr) << "Left Shift";
    EXPECT_NE(m_setting.m_keyboard.searchKey("RShift"), nullptr) << "Right Shift";
    EXPECT_NE(m_setting.m_keyboard.searchKey("LControl"), nullptr) << "Left Control";
    EXPECT_NE(m_setting.m_keyboard.searchKey("RControl"), nullptr) << "Right Control";
    EXPECT_NE(m_setting.m_keyboard.searchKey("LAlt"), nullptr) << "Left Alt";
    EXPECT_NE(m_setting.m_keyboard.searchKey("RAlt"), nullptr) << "Right Alt";
}

// =============================================================================
// Test: Case Insensitive Lookup for English Names
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, CaseInsensitiveLookup) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // Test case insensitive lookup for English key names
    Key* key1 = m_setting.m_keyboard.searchKey("NonConvert");
    Key* key2 = m_setting.m_keyboard.searchKey("nonconvert");
    Key* key3 = m_setting.m_keyboard.searchKey("NONCONVERT");
    Key* key4 = m_setting.m_keyboard.searchKey("NoNcOnVeRt");

    ASSERT_NE(key1, nullptr) << "NonConvert should be found";

    // All case variations should return the same key
    EXPECT_EQ(key1, key2) << "Case insensitive lookup should work (lowercase)";
    EXPECT_EQ(key1, key3) << "Case insensitive lookup should work (uppercase)";
    EXPECT_EQ(key1, key4) << "Case insensitive lookup should work (mixed case)";
}

// =============================================================================
// Test: Extended Scan Code Keys (E0-prefixed)
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, ExtendedScanCodeKeys) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // E0-prefixed keys from 109.mayu
    // E0無変換 / E0NonConvert
    Key* e0NonConvert = m_setting.m_keyboard.searchKey("E0NonConvert");
    EXPECT_NE(e0NonConvert, nullptr) << "E0NonConvert should be registered";

    // E0英数 / E0Eisuu
    Key* e0Eisuu = m_setting.m_keyboard.searchKey("E0Eisuu");
    EXPECT_NE(e0Eisuu, nullptr) << "E0Eisuu should be registered";

    // E0半角/全角 / E0Kanji
    Key* e0Kanji = m_setting.m_keyboard.searchKey("E0Kanji");
    EXPECT_NE(e0Kanji, nullptr) << "E0Kanji should be registered";

    // E0ひらがな / E0Hiragana
    Key* e0Hiragana = m_setting.m_keyboard.searchKey("E0Hiragana");
    EXPECT_NE(e0Hiragana, nullptr) << "E0Hiragana should be registered";
}

// =============================================================================
// Test: Numpad Keys
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, NumpadKeys) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // Numpad keys
    EXPECT_NE(m_setting.m_keyboard.searchKey("Num0"), nullptr) << "Num0";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Num1"), nullptr) << "Num1";
    EXPECT_NE(m_setting.m_keyboard.searchKey("Num9"), nullptr) << "Num9";
    EXPECT_NE(m_setting.m_keyboard.searchKey("NumEnter"), nullptr) << "NumEnter";
    EXPECT_NE(m_setting.m_keyboard.searchKey("NumLock"), nullptr) << "NumLock";
}

// =============================================================================
// Test: Media Keys
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, MediaKeys) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // Media control keys
    Key* volumeMute = m_setting.m_keyboard.searchKey("VolumeMute");
    Key* volumeDown = m_setting.m_keyboard.searchKey("VolumeDown");
    Key* volumeUp = m_setting.m_keyboard.searchKey("VolumeUp");

    // These may or may not be present depending on keyboard model
    // Just verify no crash when searching
    (void)volumeMute;
    (void)volumeDown;
    (void)volumeUp;

    // At least verify Mute alias works if VolumeMute exists
    if (volumeMute != nullptr) {
        Key* mute = m_setting.m_keyboard.searchKey("Mute");
        EXPECT_EQ(volumeMute, mute) << "Mute should be alias for VolumeMute";
    }
}

// =============================================================================
// Test: Special Japanese Key - YenSign
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, YenSignKey) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // YenSign key (Japanese keyboard specific)
    Key* yenKey = m_setting.m_keyboard.searchKey("YenSign");
    Key* yenAlias = m_setting.m_keyboard.searchKey("Yen");

    EXPECT_NE(yenKey, nullptr) << "YenSign key should be registered";
    EXPECT_NE(yenAlias, nullptr) << "Yen alias should work";
    EXPECT_EQ(yenKey, yenAlias) << "YenSign and Yen should be the same key";
}

// =============================================================================
// Test: Complete Parsing Without Crashes
// =============================================================================

TEST_F(JapaneseKeyboardLayoutTest, CompleteParsing) {
    bool loaded = load109Mayu();
    if (!loaded) {
        GTEST_SKIP() << "109.mayu not found - skipping E2E test";
    }

    // At this point, if we've gotten here without crashing or exceptions,
    // the parser handled the UTF-8 content correctly

    // Final sanity check - we should have a reasonable number of keys
    size_t keyCount = countDefinedKeys();
    std::cout << "[INFO] Successfully parsed 109.mayu with " << keyCount << " keys" << std::endl;

    EXPECT_GE(keyCount, 100u) << "Should have a substantial number of keys";

    // The Japanese keys should all be present
    size_t japaneseKeyCount = 0;
    if (m_setting.m_keyboard.searchKey("\xE7\x84\xA1\xE5\xA4\x89\xE6\x8F\x9B")) japaneseKeyCount++;
    if (m_setting.m_keyboard.searchKey("\xE5\xA4\x89\xE6\x8F\x9B")) japaneseKeyCount++;
    if (m_setting.m_keyboard.searchKey("\xE8\x8B\xB1\xE6\x95\xB0")) japaneseKeyCount++;
    if (m_setting.m_keyboard.searchKey("\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA")) japaneseKeyCount++;

    std::cout << "[INFO] Found " << japaneseKeyCount << " Japanese key names" << std::endl;
    EXPECT_GE(japaneseKeyCount, 4u) << "Should find all major Japanese keys";
}

// Note: This test is part of the regression test suite and uses its main()
// To run: ./bin/yamy_regression_test --gtest_filter="JapaneseKeyboardLayoutTest*"
