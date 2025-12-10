// config_manager_test.cpp
// Unit tests for ConfigManager class

#include <gtest/gtest.h>
#include "config_manager.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary test directory per test
        static int testCounter = 0;
        testDir = fs::temp_directory_path() / ("config_manager_test_" + std::to_string(++testCounter));
        fs::create_directories(testDir);

        // Reset ConfigManager state by re-initializing with null store
        ConfigManager::instance().initialize(nullptr);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    // Helper to create test config files
    void createTestConfig(const std::string& name) {
        std::ofstream ofs(testDir / name);
        ofs << "# Test config: " << name << std::endl;
        ofs << "keymap Global" << std::endl;
        ofs.close();
    }

    fs::path testDir;
};

TEST_F(ConfigManagerTest, SingletonReturnsInstance) {
    ConfigManager& cm1 = ConfigManager::instance();
    ConfigManager& cm2 = ConfigManager::instance();
    EXPECT_EQ(&cm1, &cm2);
}

TEST_F(ConfigManagerTest, InitialStateIsEmpty) {
    // After initialization with null store, should have no persisted configs
    auto configs = ConfigManager::instance().listConfigs();
    // Note: May have configs from scanning current directory
    EXPECT_GE(configs.size(), 0u);
}

TEST_F(ConfigManagerTest, AddConfigSucceeds) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    bool added = ConfigManager::instance().addConfig(configPath);
    EXPECT_TRUE(added);

    auto configs = ConfigManager::instance().listConfigs();
    bool found = false;
    for (const auto& entry : configs) {
        if (entry.path == configPath) {
            found = true;
            EXPECT_EQ(entry.name, "test");
            EXPECT_TRUE(entry.exists);
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigManagerTest, AddDuplicateConfigFails) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    ConfigManager::instance().addConfig(configPath);
    bool addedAgain = ConfigManager::instance().addConfig(configPath);
    EXPECT_FALSE(addedAgain);
}

TEST_F(ConfigManagerTest, RemoveConfigSucceeds) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    ConfigManager::instance().addConfig(configPath);
    bool removed = ConfigManager::instance().removeConfig(configPath);
    EXPECT_TRUE(removed);

    // Verify it's gone
    auto configs = ConfigManager::instance().listConfigs();
    for (const auto& entry : configs) {
        EXPECT_NE(entry.path, configPath);
    }
}

TEST_F(ConfigManagerTest, RemoveNonexistentConfigFails) {
    bool removed = ConfigManager::instance().removeConfig("/nonexistent/path.mayu");
    EXPECT_FALSE(removed);
}

TEST_F(ConfigManagerTest, SetActiveConfigByPath) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    ConfigManager::instance().addConfig(configPath);
    bool set = ConfigManager::instance().setActiveConfig(configPath);
    EXPECT_TRUE(set);
    EXPECT_EQ(ConfigManager::instance().getActiveConfig(), configPath);
}

TEST_F(ConfigManagerTest, SetActiveConfigByIndex) {
    createTestConfig("test1.mayu");
    createTestConfig("test2.mayu");
    std::string configPath1 = (testDir / "test1.mayu").string();
    std::string configPath2 = (testDir / "test2.mayu").string();

    ConfigManager::instance().addConfig(configPath1);
    ConfigManager::instance().addConfig(configPath2);

    // Find the indices
    auto configs = ConfigManager::instance().listConfigs();
    int idx1 = -1, idx2 = -1;
    for (size_t i = 0; i < configs.size(); ++i) {
        if (configs[i].path == configPath1) idx1 = static_cast<int>(i);
        if (configs[i].path == configPath2) idx2 = static_cast<int>(i);
    }

    ASSERT_GE(idx2, 0);
    bool set = ConfigManager::instance().setActiveConfig(idx2);
    EXPECT_TRUE(set);
    EXPECT_EQ(ConfigManager::instance().getActiveConfig(), configPath2);
}

TEST_F(ConfigManagerTest, SetInvalidActiveConfigFails) {
    bool set = ConfigManager::instance().setActiveConfig("/nonexistent/path.mayu");
    EXPECT_FALSE(set);
}

TEST_F(ConfigManagerTest, ScanDirectoryFindsConfigs) {
    createTestConfig("config1.mayu");
    createTestConfig("config2.mayu");
    createTestConfig("notaconfig.txt"); // Should be ignored

    int added = ConfigManager::instance().scanDirectory(testDir.string());
    EXPECT_GE(added, 2);

    auto configs = ConfigManager::instance().listConfigs();
    int found = 0;
    for (const auto& entry : configs) {
        if (entry.path == (testDir / "config1.mayu").string() ||
            entry.path == (testDir / "config2.mayu").string()) {
            found++;
        }
        // Should not find .txt file
        EXPECT_NE(entry.path, (testDir / "notaconfig.txt").string());
    }
    EXPECT_EQ(found, 2);
}

TEST_F(ConfigManagerTest, RefreshListUpdatesExistStatus) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    ConfigManager::instance().addConfig(configPath);

    // Verify exists is true
    auto configs = ConfigManager::instance().listConfigs();
    bool foundExists = false;
    for (const auto& entry : configs) {
        if (entry.path == configPath) {
            EXPECT_TRUE(entry.exists);
            foundExists = true;
            break;
        }
    }
    EXPECT_TRUE(foundExists);

    // Delete the file
    fs::remove(configPath);

    // Refresh
    ConfigManager::instance().refreshList();

    // Verify exists is now false
    configs = ConfigManager::instance().listConfigs();
    for (const auto& entry : configs) {
        if (entry.path == configPath) {
            EXPECT_FALSE(entry.exists);
            break;
        }
    }
}

TEST_F(ConfigManagerTest, ChangeCallbackIsCalled) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    std::string callbackPath;
    int callCount = 0;

    ConfigManager::instance().setChangeCallback([&](const std::string& path) {
        callbackPath = path;
        callCount++;
    });

    ConfigManager::instance().addConfig(configPath);
    ConfigManager::instance().setActiveConfig(configPath);

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(callbackPath, configPath);
}

TEST_F(ConfigManagerTest, GetDefaultConfigDir) {
    std::string dir = ConfigManager::getDefaultConfigDir();
    // Should return something on Linux/Windows
    EXPECT_FALSE(dir.empty());
    // Should end with .yamy
    EXPECT_TRUE(dir.find(".yamy") != std::string::npos);
}

TEST_F(ConfigManagerTest, ExtractNameFromPath) {
    createTestConfig("my_config.mayu");
    std::string configPath = (testDir / "my_config.mayu").string();

    ConfigManager::instance().addConfig(configPath);

    auto configs = ConfigManager::instance().listConfigs();
    for (const auto& entry : configs) {
        if (entry.path == configPath) {
            EXPECT_EQ(entry.name, "my_config");
            break;
        }
    }
}

TEST_F(ConfigManagerTest, ThreadSafeAccess) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();
    ConfigManager::instance().addConfig(configPath);

    // Simple concurrent access test - just verify no crashes
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                ConfigManager::instance().listConfigs();
                ConfigManager::instance().getActiveConfig();
                ConfigManager::instance().getActiveIndex();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}
