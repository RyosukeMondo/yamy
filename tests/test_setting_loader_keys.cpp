
#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "../../src/core/settings/setting_loader.h"
#include "../../src/core/settings/setting.h"
#include "../../src/utils/msgstream.h" // For tomsgstream, SyncObject
#include "../../src/core/input/keyboard.h" // For Key, ScanCode

// Mock SyncObject for SettingLoader
class DummySyncObject : public SyncObject {
public:
    void acquire() override {}
    void acquire(int) override {}
    void release() override {}
};

class SettingLoaderKeysTest : public ::testing::Test {
protected:
    void SetUp() override {
        logStream = new std::stringstream();
        dummySyncObject = new DummySyncObject();
        setting = new Setting();
    }

    void TearDown() override {
        delete setting;
        delete dummySyncObject;
        delete logStream;
    }

    std::stringstream* logStream;
    DummySyncObject* dummySyncObject;
    Setting* setting;
};

TEST_F(SettingLoaderKeysTest, LoadsKeysCorrectly) {
    // Path to the test configuration file
    std::string configFilePath = "keymaps/config_clean.mayu";

    // Create SettingLoader
    SettingLoader loader(dummySyncObject, logStream, nullptr);

    // Load the configuration
    bool loadSuccess = loader.load(setting, configFilePath);

    // Assert that loading was successful
    ASSERT_TRUE(loadSuccess) << "Failed to load configuration: " << logStream->str();
    
    // Assert that the 'A' key is found
    Key searchKeyA; searchKeyA.addScanCode(ScanCode(0x1E, 0)); // A
    Key* foundKeyA = setting->m_keyboard.searchKey(searchKeyA);
    ASSERT_NE(foundKeyA, nullptr) << "Key 'A' (0x1E) not found in loaded settings.";
    EXPECT_EQ(foundKeyA->getName(), "A");

    // Assert that the 'Tab' key is found
    Key searchKeyTab; searchKeyTab.addScanCode(ScanCode(0x0F, 0)); // Tab
    Key* foundKeyTab = setting->m_keyboard.searchKey(searchKeyTab);
    ASSERT_NE(foundKeyTab, nullptr) << "Key 'Tab' (0x0F) not found in loaded settings.";
    EXPECT_EQ(foundKeyTab->getName(), "Tab");
    
    // Assert that the '_0' key is found
    Key searchKey_0; searchKey_0.addScanCode(ScanCode(0x0B, 0)); // _0
    Key* foundKey_0 = setting->m_keyboard.searchKey(searchKey_0);
    ASSERT_NE(foundKey_0, nullptr) << "Key '_0' (0x0B) not found in loaded settings.";
    EXPECT_EQ(foundKey_0->getName(), "_0");

    // Assert that a substitution rule exists for 'A'
    ModifiedKey mkeyFromA(setting->m_keyboard.searchKey("A"));
    ModifiedKey mkeyToA = setting->m_keyboard.searchSubstitute(mkeyFromA);
    ASSERT_NE(mkeyToA.m_key, nullptr) << "Substitution for 'A' not found.";
    EXPECT_EQ(mkeyToA.m_key->getName(), "Tab");

    // Assert that the keymap "Global" is found and is a window type
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    ASSERT_NE(globalKeymap, nullptr) << "Keymap 'Global' not found.";
    EXPECT_EQ(globalKeymap->getType(), Keymap::Type_keymap) << "Keymap 'Global' should be of type Type_keymap.";

    // Assert that a window keymap for 'Default' is found
    Keymaps::KeymapPtrList windowKeymaps;
    setting->m_keymaps.searchWindow(&windowKeymaps, "", ""); // Search for any window (empty class/title)
    ASSERT_FALSE(windowKeymaps.empty()) << "No window keymaps found for Default focus.";
    EXPECT_EQ(windowKeymaps.front()->getName(), "Default") << "Expected 'Default' window keymap.";
    EXPECT_TRUE(windowKeymaps.front()->doesSameWindow("", "")) << "Default window keymap should match empty strings.";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
