//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_json_loader.cpp - Unit tests for JsonConfigLoader
//
// Tests JSON configuration loading with comprehensive coverage:
// - Valid JSON configurations load successfully
// - Error handling (syntax errors, missing fields, unknown keys)
// - M00-MFF virtual modifier parsing
// - Key sequence parsing
//
// Part of task 1.10 in json-refactoring spec
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "../../src/core/settings/json_config_loader.h"
#include "../../src/core/settings/setting.h"
#include "../../src/core/input/keyboard.h"
#include "../../src/core/input/keymap.h"

namespace fs = std::filesystem;

namespace yamy::settings::test {

class JsonConfigLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        logStream = new std::stringstream();
        loader = new JsonConfigLoader(logStream);
        setting = new Setting();
        tempDir = fs::temp_directory_path() / "yamy_json_test";
        fs::create_directories(tempDir);
    }

    void TearDown() override {
        delete setting;
        delete loader;
        delete logStream;
        if (fs::exists(tempDir)) {
            fs::remove_all(tempDir);
        }
    }

    std::string createJsonFile(const std::string& filename, const std::string& content) {
        fs::path filepath = tempDir / filename;
        std::ofstream ofs(filepath);
        ofs << content;
        ofs.close();
        return filepath.string();
    }

    std::string getLog() const {
        return logStream->str();
    }

    std::stringstream* logStream;
    JsonConfigLoader* loader;
    Setting* setting;
    fs::path tempDir;
};

TEST_F(JsonConfigLoaderTest, LoadValidBasicConfig) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e", "B": "0x30", "Tab": "0x0f"}},
        "virtualModifiers": {},
        "mappings": [{"from": "A", "to": "Tab"}]
    })";

    std::string filepath = createJsonFile("valid.json", json);
    ASSERT_TRUE(loader->load(setting, filepath)) << "Load failed: " << getLog();

    EXPECT_NE(setting->m_keyboard.searchKey("A"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("Tab"), nullptr);
    EXPECT_NE(setting->m_keymaps.searchByName("Global"), nullptr);
}

TEST_F(JsonConfigLoaderTest, LoadVirtualModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"CapsLock": "0x3a", "Escape": "0x01", "A": "0x1e", "Left": "0xe04b"}},
        "virtualModifiers": {"M00": {"trigger": "CapsLock", "tap": "Escape", "holdThresholdMs": 200}},
        "mappings": [{"from": "M00-A", "to": "Left"}]
    })";

    std::string filepath = createJsonFile("vmods.json", json);
    ASSERT_TRUE(loader->load(setting, filepath)) << "Load failed: " << getLog();

    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    ASSERT_NE(capsLock, nullptr);
    uint16_t capsScan = capsLock->getScanCodes()[0].m_scan;

    auto triggerIt = setting->m_virtualModTriggers.find(capsScan);
    ASSERT_NE(triggerIt, setting->m_virtualModTriggers.end());
    EXPECT_EQ(triggerIt->second, 0x00);

    auto tapIt = setting->m_modTapActions.find(0x00);
    ASSERT_NE(tapIt, setting->m_modTapActions.end());

    Key* escape = setting->m_keyboard.searchKey("Escape");
    uint16_t escapeScan = escape->getScanCodes()[0].m_scan;
    EXPECT_EQ(tapIt->second, escapeScan);
}

TEST_F(JsonConfigLoaderTest, LoadMultipleVirtualModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"CapsLock": "0x3a", "Semicolon": "0x27"}},
        "virtualModifiers": {
            "M00": {"trigger": "CapsLock"},
            "M01": {"trigger": "Semicolon"}
        },
        "mappings": []
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("multi.json", json)));
    EXPECT_EQ(setting->m_virtualModTriggers.size(), 2);
}

TEST_F(JsonConfigLoaderTest, LoadKeySequences) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e", "B": "0x30", "Escape": "0x01"}},
        "virtualModifiers": {},
        "mappings": [{"from": "A", "to": ["Escape", "B"]}]
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("seq.json", json)));
    EXPECT_NE(setting->m_keymaps.searchByName("Global"), nullptr);
}

TEST_F(JsonConfigLoaderTest, LoadStandardModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e", "B": "0x30"}},
        "virtualModifiers": {},
        "mappings": [{"from": "Shift-A", "to": "B"}]
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("stdmod.json", json)));
    EXPECT_NE(setting->m_keymaps.searchByName("Global"), nullptr);
}

TEST_F(JsonConfigLoaderTest, LoadCombinedModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"CapsLock": "0x3a", "A": "0x1e", "B": "0x30"}},
        "virtualModifiers": {"M00": {"trigger": "CapsLock"}},
        "mappings": [{"from": "Shift-M00-A", "to": "B"}]
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("combined.json", json)));
}

TEST_F(JsonConfigLoaderTest, ErrorInvalidJsonSyntax) {
    std::string json = R"({"version": "2.0", "keyboard": {"keys": {})";
    EXPECT_FALSE(loader->load(setting, createJsonFile("bad.json", json)));
    EXPECT_TRUE(getLog().find("parse") != std::string::npos ||
                getLog().find("JSON") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorMissingVersion) {
    std::string json = R"({"keyboard": {"keys": {}}, "virtualModifiers": {}, "mappings": []})";
    EXPECT_FALSE(loader->load(setting, createJsonFile("nover.json", json)));
    EXPECT_TRUE(getLog().find("version") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorWrongVersion) {
    std::string json = R"({"version": "1.0", "keyboard": {"keys": {}}, "virtualModifiers": {}, "mappings": []})";
    EXPECT_FALSE(loader->load(setting, createJsonFile("wrongver.json", json)));
    EXPECT_TRUE(getLog().find("2.0") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorInvalidScanCode) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "invalid"}},
        "virtualModifiers": {},
        "mappings": []
    })";

    EXPECT_FALSE(loader->load(setting, createJsonFile("badscan.json", json)));
    std::string log = getLog();
    EXPECT_TRUE(log.find("scan") != std::string::npos ||
                log.find("invalid") != std::string::npos ||
                log.find("hex") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorUnknownKeyInMapping) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e"}},
        "virtualModifiers": {},
        "mappings": [{"from": "A", "to": "UnknownKey"}]
    })";

    EXPECT_FALSE(loader->load(setting, createJsonFile("unkn.json", json)));
    EXPECT_TRUE(getLog().find("UnknownKey") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorInvalidVirtualModifierFormat) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"CapsLock": "0x3a"}},
        "virtualModifiers": {"MOD00": {"trigger": "CapsLock"}},
        "mappings": []
    })";

    EXPECT_FALSE(loader->load(setting, createJsonFile("badmod.json", json)));
    EXPECT_TRUE(getLog().find("M00-MFF") != std::string::npos ||
                getLog().find("MOD00") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorUndefinedTriggerKey) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e"}},
        "virtualModifiers": {"M00": {"trigger": "UndefinedKey"}},
        "mappings": []
    })";

    EXPECT_FALSE(loader->load(setting, createJsonFile("notrig.json", json)));
    EXPECT_TRUE(getLog().find("UndefinedKey") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorMissingFromField) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e", "B": "0x30"}},
        "virtualModifiers": {},
        "mappings": [{"to": "B"}]
    })";

    EXPECT_FALSE(loader->load(setting, createJsonFile("nofrom.json", json)));
    EXPECT_TRUE(getLog().find("from") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, ErrorMissingToField) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e"}},
        "virtualModifiers": {},
        "mappings": [{"from": "A"}]
    })";

    EXPECT_FALSE(loader->load(setting, createJsonFile("noto.json", json)));
    EXPECT_TRUE(getLog().find("to") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, LoadHexScanCodeVariants) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"A": "0x1e", "B": "0x30", "C": "0x2E", "Left": "0xe04b", "Right": "0xE04D"}},
        "virtualModifiers": {},
        "mappings": []
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("hexvar.json", json)));
    EXPECT_NE(setting->m_keyboard.searchKey("A"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("C"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("Left"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("Right"), nullptr);
}

TEST_F(JsonConfigLoaderTest, LoadEmptySections) {
    std::string json = R"({"version": "2.0", "keyboard": {"keys": {}}, "virtualModifiers": {}, "mappings": []})";
    EXPECT_TRUE(loader->load(setting, createJsonFile("empty.json", json)));
}

TEST_F(JsonConfigLoaderTest, LoadVirtualModifierWithoutTap) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"CapsLock": "0x3a", "A": "0x1e", "Left": "0xe04b"}},
        "virtualModifiers": {"M00": {"trigger": "CapsLock"}},
        "mappings": [{"from": "M00-A", "to": "Left"}]
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("notap.json", json)));

    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    uint16_t capsScan = capsLock->getScanCodes()[0].m_scan;
    EXPECT_NE(setting->m_virtualModTriggers.find(capsScan),
              setting->m_virtualModTriggers.end());
}

TEST_F(JsonConfigLoaderTest, ErrorFileNotFound) {
    std::string filepath = (tempDir / "nonexistent.json").string();
    EXPECT_FALSE(loader->load(setting, filepath));
    std::string log = getLog();
    EXPECT_TRUE(log.find("open") != std::string::npos ||
                log.find("file") != std::string::npos ||
                log.find("Failed") != std::string::npos);
}

TEST_F(JsonConfigLoaderTest, LoadLargeModifierNumber) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {"keys": {"CapsLock": "0x3a", "A": "0x1e"}},
        "virtualModifiers": {"MFF": {"trigger": "CapsLock"}},
        "mappings": [{"from": "MFF-A", "to": "CapsLock"}]
    })";

    ASSERT_TRUE(loader->load(setting, createJsonFile("mff.json", json)));

    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    uint16_t capsScan = capsLock->getScanCodes()[0].m_scan;

    auto triggerIt = setting->m_virtualModTriggers.find(capsScan);
    ASSERT_NE(triggerIt, setting->m_virtualModTriggers.end());
    EXPECT_EQ(triggerIt->second, 0xFF);
}

} // namespace yamy::settings::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
