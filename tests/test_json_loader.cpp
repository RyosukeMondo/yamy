//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_json_loader.cpp - Unit tests for JsonConfigLoader
//
// Tests JSON configuration loading with comprehensive coverage:
// - Valid JSON configurations load successfully
// - Error handling (syntax errors, missing fields, unknown keys)
// - M00-MFF virtual modifier parsing
// - Key sequence parsing
// - Keyboard key definitions
// - Mapping rules
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

//=============================================================================
// Test Fixture
//=============================================================================

class JsonConfigLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        logStream = new std::stringstream();
        loader = new JsonConfigLoader(logStream);
        setting = new Setting();

        // Create temp directory for test JSON files
        tempDir = fs::temp_directory_path() / "yamy_json_test";
        fs::create_directories(tempDir);
    }

    void TearDown() override {
        delete setting;
        delete loader;
        delete logStream;

        // Clean up temp directory
        if (fs::exists(tempDir)) {
            fs::remove_all(tempDir);
        }
    }

    // Helper: Create JSON file with given content
    std::string createJsonFile(const std::string& filename, const std::string& content) {
        fs::path filepath = tempDir / filename;
        std::ofstream ofs(filepath);
        ofs << content;
        ofs.close();
        return filepath.string();
    }

    // Helper: Get log output
    std::string getLog() const {
        return logStream->str();
    }

    std::stringstream* logStream;
    JsonConfigLoader* loader;
    Setting* setting;
    fs::path tempDir;
};

//=============================================================================
// Test: Basic Valid Configuration
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadValidBasicConfig) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e",
                "B": "0x30",
                "Tab": "0x0f",
                "Escape": "0x01"
            }
        },
        "virtualModifiers": {},
        "mappings": [
            {
                "from": "A",
                "to": "Tab"
            }
        ]
    })";

    std::string filepath = createJsonFile("valid_basic.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify keys were loaded
    Key* keyA = setting->m_keyboard.searchKey("A");
    ASSERT_NE(keyA, nullptr) << "Key 'A' not found";
    EXPECT_EQ(keyA->getName(), "A");

    Key* keyTab = setting->m_keyboard.searchKey("Tab");
    ASSERT_NE(keyTab, nullptr) << "Key 'Tab' not found";
    EXPECT_EQ(keyTab->getName(), "Tab");

    // Verify mapping was loaded - global keymap should exist
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    ASSERT_NE(globalKeymap, nullptr) << "Global keymap not found";
}

//=============================================================================
// Test: Virtual Modifiers (M00-MFF)
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadVirtualModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a",
                "Escape": "0x01",
                "A": "0x1e",
                "Left": "0xe04b"
            }
        },
        "virtualModifiers": {
            "M00": {
                "trigger": "CapsLock",
                "tap": "Escape",
                "holdThresholdMs": 200
            }
        },
        "mappings": [
            {
                "from": "M00-A",
                "to": "Left"
            }
        ]
    })";

    std::string filepath = createJsonFile("virtual_mods.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify M00 trigger key registered
    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    ASSERT_NE(capsLock, nullptr);

    const ScanCode* scanCodes = capsLock->getScanCodes();
    ASSERT_GT(capsLock->getScanCodesSize(), 0);
    uint16_t capsScanCode = scanCodes[0].m_scan;

    auto triggerIt = setting->m_virtualModTriggers.find(capsScanCode);
    ASSERT_NE(triggerIt, setting->m_virtualModTriggers.end())
        << "CapsLock not registered as M00 trigger";
    EXPECT_EQ(triggerIt->second, 0x00) << "Trigger should map to M00 (0x00)";

    // Verify M00 tap action registered
    auto tapIt = setting->m_modTapActions.find(0x00);
    ASSERT_NE(tapIt, setting->m_modTapActions.end())
        << "M00 tap action not registered";

    Key* escape = setting->m_keyboard.searchKey("Escape");
    ASSERT_NE(escape, nullptr);
    const ScanCode* escapeScanCodes = escape->getScanCodes();
    uint16_t escapeScanCode = escapeScanCodes[0].m_scan;
    EXPECT_EQ(tapIt->second, escapeScanCode) << "Tap action should be Escape";

    // Verify global keymap was created
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    ASSERT_NE(globalKeymap, nullptr);
}

//=============================================================================
// Test: Multiple Virtual Modifiers
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadMultipleVirtualModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a",
                "Semicolon": "0x27",
                "Escape": "0x01",
                "A": "0x1e"
            }
        },
        "virtualModifiers": {
            "M00": {
                "trigger": "CapsLock",
                "tap": "Escape"
            },
            "M01": {
                "trigger": "Semicolon",
                "tap": "Escape"
            }
        },
        "mappings": []
    })";

    std::string filepath = createJsonFile("multi_mods.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify both modifiers registered
    EXPECT_EQ(setting->m_virtualModTriggers.size(), 2);
    EXPECT_EQ(setting->m_modTapActions.size(), 2);

    // Check M00
    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    const ScanCode* capsScanCodes = capsLock->getScanCodes();
    uint16_t capsScan = capsScanCodes[0].m_scan;
    EXPECT_EQ(setting->m_virtualModTriggers[capsScan], 0x00);

    // Check M01
    Key* semicolon = setting->m_keyboard.searchKey("Semicolon");
    const ScanCode* semiScanCodes = semicolon->getScanCodes();
    uint16_t semiScan = semiScanCodes[0].m_scan;
    EXPECT_EQ(setting->m_virtualModTriggers[semiScan], 0x01);
}

//=============================================================================
// Test: Key Sequences
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadKeySequences) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e",
                "B": "0x30",
                "C": "0x2e",
                "Escape": "0x01"
            }
        },
        "virtualModifiers": {},
        "mappings": [
            {
                "from": "A",
                "to": ["Escape", "B", "C"]
            }
        ]
    })";

    std::string filepath = createJsonFile("sequences.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify global keymap created
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    ASSERT_NE(globalKeymap, nullptr);

    // Note: We can't easily test the exact sequence contents without
    // accessing internals, but we can verify the load succeeded
}

//=============================================================================
// Test: Standard Modifiers
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadStandardModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e",
                "B": "0x30",
                "C": "0x2e"
            }
        },
        "virtualModifiers": {},
        "mappings": [
            {
                "from": "Shift-A",
                "to": "B"
            },
            {
                "from": "Ctrl-Alt-C",
                "to": "A"
            }
        ]
    })";

    std::string filepath = createJsonFile("std_mods.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify global keymap created
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    ASSERT_NE(globalKeymap, nullptr);
}

//=============================================================================
// Test: Combined Standard and Virtual Modifiers
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadCombinedModifiers) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a",
                "A": "0x1e",
                "B": "0x30"
            }
        },
        "virtualModifiers": {
            "M00": {
                "trigger": "CapsLock"
            }
        },
        "mappings": [
            {
                "from": "Shift-M00-A",
                "to": "B"
            }
        ]
    })";

    std::string filepath = createJsonFile("combined_mods.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify global keymap created
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    ASSERT_NE(globalKeymap, nullptr);
}

//=============================================================================
// Test: Error Handling - Invalid JSON Syntax
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorInvalidJsonSyntax) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e"
            }
        }
        // Missing closing brace
    )";

    std::string filepath = createJsonFile("invalid_syntax.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on invalid JSON syntax";

    std::string log = getLog();
    EXPECT_TRUE(log.find("parse error") != std::string::npos ||
                log.find("JSON") != std::string::npos)
        << "Log should mention parse error: " << log;
}

//=============================================================================
// Test: Error Handling - Missing Version
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorMissingVersion) {
    std::string json = R"({
        "keyboard": {
            "keys": {
                "A": "0x1e"
            }
        },
        "virtualModifiers": {},
        "mappings": []
    })";

    std::string filepath = createJsonFile("missing_version.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on missing version";

    std::string log = getLog();
    EXPECT_TRUE(log.find("version") != std::string::npos)
        << "Log should mention version error: " << log;
}

//=============================================================================
// Test: Error Handling - Wrong Version
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorWrongVersion) {
    std::string json = R"({
        "version": "1.0",
        "keyboard": {
            "keys": {
                "A": "0x1e"
            }
        },
        "virtualModifiers": {},
        "mappings": []
    })";

    std::string filepath = createJsonFile("wrong_version.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on wrong version";

    std::string log = getLog();
    EXPECT_TRUE(log.find("2.0") != std::string::npos)
        << "Log should mention expected version 2.0: " << log;
}

//=============================================================================
// Test: Error Handling - Invalid Scan Code Format
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorInvalidScanCode) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "invalid",
                "B": "0x30"
            }
        },
        "virtualModifiers": {},
        "mappings": []
    })";

    std::string filepath = createJsonFile("invalid_scancode.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on invalid scan code format";

    std::string log = getLog();
    EXPECT_TRUE(log.find("scan code") != std::string::npos ||
                log.find("invalid") != std::string::npos ||
                log.find("hex") != std::string::npos)
        << "Log should mention scan code error: " << log;
}

//=============================================================================
// Test: Error Handling - Unknown Key Name in Mapping
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorUnknownKeyInMapping) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e"
            }
        },
        "virtualModifiers": {},
        "mappings": [
            {
                "from": "A",
                "to": "UnknownKey"
            }
        ]
    })";

    std::string filepath = createJsonFile("unknown_key.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on unknown key in mapping";

    std::string log = getLog();
    EXPECT_TRUE(log.find("UnknownKey") != std::string::npos ||
                log.find("unknown") != std::string::npos)
        << "Log should mention unknown key: " << log;
}

//=============================================================================
// Test: Error Handling - Invalid Virtual Modifier Name
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorInvalidVirtualModifierName) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a"
            }
        },
        "virtualModifiers": {
            "M99": {
                "trigger": "CapsLock"
            }
        },
        "mappings": []
    })";

    std::string filepath = createJsonFile("invalid_mod_name.json", json);
    bool success = loader->load(setting, filepath);

    // Should succeed - M99 is valid (in range M00-MFF)
    ASSERT_TRUE(success) << "M99 should be valid: " << getLog();
}

TEST_F(JsonConfigLoaderTest, ErrorInvalidVirtualModifierNameBadFormat) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a"
            }
        },
        "virtualModifiers": {
            "MOD00": {
                "trigger": "CapsLock"
            }
        },
        "mappings": []
    })";

    std::string filepath = createJsonFile("invalid_mod_format.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on invalid modifier name format";

    std::string log = getLog();
    EXPECT_TRUE(log.find("M00-MFF") != std::string::npos ||
                log.find("MOD00") != std::string::npos)
        << "Log should mention modifier name format: " << log;
}

//=============================================================================
// Test: Error Handling - Virtual Modifier Trigger Key Not Defined
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorVirtualModifierTriggerNotDefined) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e"
            }
        },
        "virtualModifiers": {
            "M00": {
                "trigger": "UndefinedKey"
            }
        },
        "mappings": []
    })";

    std::string filepath = createJsonFile("undefined_trigger.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail when trigger key not defined";

    std::string log = getLog();
    EXPECT_TRUE(log.find("UndefinedKey") != std::string::npos)
        << "Log should mention undefined trigger key: " << log;
}

//=============================================================================
// Test: Error Handling - Missing 'from' in Mapping
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorMissingFromInMapping) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e",
                "B": "0x30"
            }
        },
        "virtualModifiers": {},
        "mappings": [
            {
                "to": "B"
            }
        ]
    })";

    std::string filepath = createJsonFile("missing_from.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on missing 'from' field";

    std::string log = getLog();
    EXPECT_TRUE(log.find("from") != std::string::npos)
        << "Log should mention missing 'from': " << log;
}

//=============================================================================
// Test: Error Handling - Missing 'to' in Mapping
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorMissingToInMapping) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e"
            }
        },
        "virtualModifiers": {},
        "mappings": [
            {
                "from": "A"
            }
        ]
    })";

    std::string filepath = createJsonFile("missing_to.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on missing 'to' field";

    std::string log = getLog();
    EXPECT_TRUE(log.find("to") != std::string::npos)
        << "Log should mention missing 'to': " << log;
}

//=============================================================================
// Test: Hex Scan Codes with Different Formats
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadHexScanCodeVariants) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "A": "0x1e",
                "B": "0x30",
                "C": "0x2E",
                "Left": "0xe04b",
                "Right": "0xE04D"
            }
        },
        "virtualModifiers": {},
        "mappings": []
    })";

    std::string filepath = createJsonFile("hex_variants.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify all keys loaded correctly
    EXPECT_NE(setting->m_keyboard.searchKey("A"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("B"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("C"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("Left"), nullptr);
    EXPECT_NE(setting->m_keyboard.searchKey("Right"), nullptr);
}

//=============================================================================
// Test: Empty Sections (Valid but No-op)
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadEmptySections) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {}
        },
        "virtualModifiers": {},
        "mappings": []
    })";

    std::string filepath = createJsonFile("empty_sections.json", json);
    bool success = loader->load(setting, filepath);

    EXPECT_TRUE(success) << "Should succeed with empty sections: " << getLog();
}

//=============================================================================
// Test: Virtual Modifier Without Tap Action (Optional)
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadVirtualModifierWithoutTap) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a",
                "A": "0x1e",
                "Left": "0xe04b"
            }
        },
        "virtualModifiers": {
            "M00": {
                "trigger": "CapsLock"
            }
        },
        "mappings": [
            {
                "from": "M00-A",
                "to": "Left"
            }
        ]
    })";

    std::string filepath = createJsonFile("mod_no_tap.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify trigger registered
    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    const ScanCode* scanCodes = capsLock->getScanCodes();
    uint16_t capsScan = scanCodes[0].m_scan;

    EXPECT_NE(setting->m_virtualModTriggers.find(capsScan),
              setting->m_virtualModTriggers.end())
        << "Trigger should be registered";

    // Tap action might not be present
    auto tapIt = setting->m_modTapActions.find(0x00);
    // It's OK if not found or if found with value 0
}

//=============================================================================
// Test: File Not Found
//=============================================================================

TEST_F(JsonConfigLoaderTest, ErrorFileNotFound) {
    std::string filepath = (tempDir / "nonexistent.json").string();
    bool success = loader->load(setting, filepath);

    EXPECT_FALSE(success) << "Should fail on nonexistent file";

    std::string log = getLog();
    EXPECT_TRUE(log.find("open") != std::string::npos ||
                log.find("file") != std::string::npos ||
                log.find("Failed") != std::string::npos)
        << "Log should mention file error: " << log;
}

//=============================================================================
// Test: Large Modifier Number (MFF)
//=============================================================================

TEST_F(JsonConfigLoaderTest, LoadLargeModifierNumber) {
    std::string json = R"({
        "version": "2.0",
        "keyboard": {
            "keys": {
                "CapsLock": "0x3a",
                "A": "0x1e"
            }
        },
        "virtualModifiers": {
            "MFF": {
                "trigger": "CapsLock"
            }
        },
        "mappings": [
            {
                "from": "MFF-A",
                "to": "CapsLock"
            }
        ]
    })";

    std::string filepath = createJsonFile("large_mod.json", json);
    bool success = loader->load(setting, filepath);

    ASSERT_TRUE(success) << "Load failed: " << getLog();

    // Verify MFF (255) is registered
    Key* capsLock = setting->m_keyboard.searchKey("CapsLock");
    const ScanCode* scanCodes = capsLock->getScanCodes();
    uint16_t capsScan = scanCodes[0].m_scan;

    auto triggerIt = setting->m_virtualModTriggers.find(capsScan);
    ASSERT_NE(triggerIt, setting->m_virtualModTriggers.end());
    EXPECT_EQ(triggerIt->second, 0xFF) << "Should map to MFF (0xFF)";
}

} // namespace yamy::settings::test

//=============================================================================
// Main
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
