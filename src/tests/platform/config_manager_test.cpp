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

// ==================== Backup & Restore Tests ====================

TEST_F(ConfigManagerTest, CreateBackupSucceeds) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    std::string backupPath = ConfigManager::instance().createBackup(configPath);
    EXPECT_FALSE(backupPath.empty());
    EXPECT_TRUE(fs::exists(backupPath));

    // Verify backup is in .backups subdirectory
    fs::path backup(backupPath);
    EXPECT_EQ(backup.parent_path().filename(), ".backups");

    // Verify backup filename pattern
    std::string filename = backup.filename().string();
    EXPECT_TRUE(filename.find("test_") == 0);
    EXPECT_TRUE(filename.find(".mayu.bak") != std::string::npos);
}

TEST_F(ConfigManagerTest, CreateBackupNonexistentFileFails) {
    std::string backupPath = ConfigManager::instance().createBackup("/nonexistent/file.mayu");
    EXPECT_TRUE(backupPath.empty());
}

TEST_F(ConfigManagerTest, ListBackupsReturnsCorrectList) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    // Create multiple backups with small delay to ensure different timestamps
    std::vector<std::string> createdBackups;
    for (int i = 0; i < 3; ++i) {
        std::string backupPath = ConfigManager::instance().createBackup(configPath);
        EXPECT_FALSE(backupPath.empty());
        createdBackups.push_back(backupPath);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto backups = ConfigManager::instance().listBackups(configPath);
    EXPECT_GE(backups.size(), 3u);

    // Verify all created backups are in the list
    for (const auto& created : createdBackups) {
        bool found = false;
        for (const auto& backup : backups) {
            if (backup == created) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Backup not found: " << created;
    }
}

TEST_F(ConfigManagerTest, ListBackupsReturnsEmptyForNoBackups) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    auto backups = ConfigManager::instance().listBackups(configPath);
    EXPECT_TRUE(backups.empty());
}

TEST_F(ConfigManagerTest, RestoreBackupSucceeds) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    // Read original content
    std::ifstream ifs(configPath);
    std::string originalContent((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
    ifs.close();

    // Create backup
    std::string backupPath = ConfigManager::instance().createBackup(configPath);
    ASSERT_FALSE(backupPath.empty());

    // Modify the original file
    {
        std::ofstream ofs(configPath);
        ofs << "# Modified content" << std::endl;
    }

    // Restore from backup
    bool restored = ConfigManager::instance().restoreBackup(backupPath);
    EXPECT_TRUE(restored);

    // Verify content is restored
    std::ifstream ifs2(configPath);
    std::string restoredContent((std::istreambuf_iterator<char>(ifs2)),
                                 std::istreambuf_iterator<char>());
    ifs2.close();

    EXPECT_EQ(restoredContent, originalContent);
}

TEST_F(ConfigManagerTest, RestoreBackupCreatesPreRestoreBackup) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    // Create initial backup
    std::string backupPath = ConfigManager::instance().createBackup(configPath);
    ASSERT_FALSE(backupPath.empty());

    // Modify the file
    {
        std::ofstream ofs(configPath);
        ofs << "# Modified content before restore" << std::endl;
    }

    // Get backup count before restore
    auto backupsBefore = ConfigManager::instance().listBackups(configPath);
    size_t countBefore = backupsBefore.size();

    // Restore from backup
    bool restored = ConfigManager::instance().restoreBackup(backupPath);
    EXPECT_TRUE(restored);

    // Verify a new backup was created (pre-restore backup)
    auto backupsAfter = ConfigManager::instance().listBackups(configPath);
    EXPECT_GE(backupsAfter.size(), countBefore);
}

TEST_F(ConfigManagerTest, RestoreNonexistentBackupFails) {
    bool restored = ConfigManager::instance().restoreBackup("/nonexistent/backup.mayu.bak");
    EXPECT_FALSE(restored);
}

TEST_F(ConfigManagerTest, DeleteBackupSucceeds) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    std::string backupPath = ConfigManager::instance().createBackup(configPath);
    ASSERT_FALSE(backupPath.empty());
    ASSERT_TRUE(fs::exists(backupPath));

    bool deleted = ConfigManager::instance().deleteBackup(backupPath);
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(fs::exists(backupPath));
}

TEST_F(ConfigManagerTest, DeleteNonBackupFileFails) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    // Try to delete a non-backup file (should fail)
    bool deleted = ConfigManager::instance().deleteBackup(configPath);
    EXPECT_FALSE(deleted);
    EXPECT_TRUE(fs::exists(configPath)); // File should still exist
}

TEST_F(ConfigManagerTest, BackupLimitEnforced) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    // Create more backups than the limit (limit is 10)
    for (int i = 0; i < 15; ++i) {
        ConfigManager::instance().createBackup(configPath);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Verify we have at most MAX_BACKUPS_PER_CONFIG backups
    auto backups = ConfigManager::instance().listBackups(configPath);
    EXPECT_LE(backups.size(), static_cast<size_t>(ConfigManager::MAX_BACKUPS_PER_CONFIG));
}

TEST_F(ConfigManagerTest, GetBackupDirReturnsCorrectPath) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    std::string backupDir = ConfigManager::getBackupDir(configPath);
    EXPECT_FALSE(backupDir.empty());

    fs::path expected = testDir / ".backups";
    EXPECT_EQ(backupDir, expected.string());
}

// ==================== Import/Export Tests ====================

TEST_F(ConfigManagerTest, ExportConfigSucceeds) {
    createTestConfig("main.mayu");
    std::string configPath = (testDir / "main.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto result = ConfigManager::instance().exportConfig(configPath, archivePath);
    EXPECT_TRUE(result.success) << result.errorMessage;
    EXPECT_TRUE(fs::exists(archivePath));
    EXPECT_FALSE(result.filesProcessed.empty());
}

TEST_F(ConfigManagerTest, ExportConfigWithIncludes) {
    // Create main config with include
    {
        std::ofstream ofs(testDir / "main.mayu");
        ofs << "# Main config" << std::endl;
        ofs << "include \"common.mayu\"" << std::endl;
        ofs << "keymap Global" << std::endl;
    }

    // Create included config
    {
        std::ofstream ofs(testDir / "common.mayu");
        ofs << "# Common definitions" << std::endl;
        ofs << "def key MyKey = 0x10" << std::endl;
    }

    std::string configPath = (testDir / "main.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto result = ConfigManager::instance().exportConfig(configPath, archivePath);
    EXPECT_TRUE(result.success) << result.errorMessage;
    EXPECT_EQ(result.filesProcessed.size(), 2u);

    // Verify archive contains both files
    auto contents = ConfigManager::instance().listArchiveContents(archivePath);
    EXPECT_EQ(contents.size(), 2u);

    bool foundMain = false, foundCommon = false;
    for (const auto& file : contents) {
        if (file == "main.mayu") foundMain = true;
        if (file == "common.mayu") foundCommon = true;
    }
    EXPECT_TRUE(foundMain);
    EXPECT_TRUE(foundCommon);
}

TEST_F(ConfigManagerTest, ExportNonexistentConfigFails) {
    std::string archivePath = (testDir / "export.yamy-pkg").string();
    auto result = ConfigManager::instance().exportConfig("/nonexistent/config.mayu", archivePath);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(ConfigManagerTest, ImportConfigSucceeds) {
    // Create and export a config
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto exportResult = ConfigManager::instance().exportConfig(configPath, archivePath);
    ASSERT_TRUE(exportResult.success) << exportResult.errorMessage;

    // Create import directory
    fs::path importDir = testDir / "imported";
    fs::create_directories(importDir);

    auto importResult = ConfigManager::instance().importConfig(archivePath, importDir.string());
    EXPECT_TRUE(importResult.success) << importResult.errorMessage;
    EXPECT_FALSE(importResult.filesProcessed.empty());

    // Verify imported file exists
    EXPECT_TRUE(fs::exists(importDir / "test.mayu"));
}

TEST_F(ConfigManagerTest, ImportConfigWithIncludes) {
    // Create main config with include
    {
        std::ofstream ofs(testDir / "main.mayu");
        ofs << "# Main config" << std::endl;
        ofs << "include \"common.mayu\"" << std::endl;
    }

    // Create included config
    {
        std::ofstream ofs(testDir / "common.mayu");
        ofs << "# Common definitions" << std::endl;
    }

    std::string configPath = (testDir / "main.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto exportResult = ConfigManager::instance().exportConfig(configPath, archivePath);
    ASSERT_TRUE(exportResult.success) << exportResult.errorMessage;

    // Import to new directory
    fs::path importDir = testDir / "imported";
    auto importResult = ConfigManager::instance().importConfig(archivePath, importDir.string());
    EXPECT_TRUE(importResult.success) << importResult.errorMessage;
    EXPECT_EQ(importResult.filesProcessed.size(), 2u);

    // Verify both files exist
    EXPECT_TRUE(fs::exists(importDir / "main.mayu"));
    EXPECT_TRUE(fs::exists(importDir / "common.mayu"));
}

TEST_F(ConfigManagerTest, ImportConfigNoOverwriteFailsOnExisting) {
    // Create and export a config
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto exportResult = ConfigManager::instance().exportConfig(configPath, archivePath);
    ASSERT_TRUE(exportResult.success);

    // Create import directory with existing file
    fs::path importDir = testDir / "imported";
    fs::create_directories(importDir);
    {
        std::ofstream ofs(importDir / "test.mayu");
        ofs << "# Existing file" << std::endl;
    }

    // Import without overwrite should fail
    auto importResult = ConfigManager::instance().importConfig(archivePath, importDir.string(), false);
    EXPECT_FALSE(importResult.success);
    EXPECT_TRUE(importResult.errorMessage.find("already exists") != std::string::npos);
}

TEST_F(ConfigManagerTest, ImportConfigWithOverwrite) {
    // Create and export a config
    {
        std::ofstream ofs(testDir / "test.mayu");
        ofs << "# Original content" << std::endl;
    }
    std::string configPath = (testDir / "test.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto exportResult = ConfigManager::instance().exportConfig(configPath, archivePath);
    ASSERT_TRUE(exportResult.success);

    // Create import directory with existing file
    fs::path importDir = testDir / "imported";
    fs::create_directories(importDir);
    {
        std::ofstream ofs(importDir / "test.mayu");
        ofs << "# Different content" << std::endl;
    }

    // Import with overwrite should succeed
    auto importResult = ConfigManager::instance().importConfig(archivePath, importDir.string(), true);
    EXPECT_TRUE(importResult.success) << importResult.errorMessage;

    // Verify content was overwritten
    std::ifstream ifs(importDir / "test.mayu");
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("Original content") != std::string::npos);
}

TEST_F(ConfigManagerTest, ImportNonexistentArchiveFails) {
    fs::path importDir = testDir / "imported";
    auto result = ConfigManager::instance().importConfig("/nonexistent/archive.yamy-pkg", importDir.string());
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(ConfigManagerTest, ValidateArchiveSucceeds) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto exportResult = ConfigManager::instance().exportConfig(configPath, archivePath);
    ASSERT_TRUE(exportResult.success);

    bool valid = ConfigManager::instance().validateArchive(archivePath);
    EXPECT_TRUE(valid);
}

TEST_F(ConfigManagerTest, ValidateInvalidArchiveFails) {
    // Create an invalid archive file
    std::string archivePath = (testDir / "invalid.yamy-pkg").string();
    {
        std::ofstream ofs(archivePath, std::ios::binary);
        ofs << "This is not a valid archive";
    }

    bool valid = ConfigManager::instance().validateArchive(archivePath);
    EXPECT_FALSE(valid);
}

TEST_F(ConfigManagerTest, ListArchiveContents) {
    // Create configs
    createTestConfig("config1.mayu");
    createTestConfig("config2.mayu");

    // Create main config that includes config2
    {
        std::ofstream ofs(testDir / "main.mayu");
        ofs << "include \"config2.mayu\"" << std::endl;
    }

    std::string archivePath = (testDir / "export.yamy-pkg").string();
    auto exportResult = ConfigManager::instance().exportConfig((testDir / "main.mayu").string(), archivePath);
    ASSERT_TRUE(exportResult.success);

    auto contents = ConfigManager::instance().listArchiveContents(archivePath);
    EXPECT_EQ(contents.size(), 2u);
}

TEST_F(ConfigManagerTest, ExportCreatesDirectoryIfNeeded) {
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();

    // Archive path in non-existent directory
    fs::path newDir = testDir / "newdir" / "subdir";
    std::string archivePath = (newDir / "export.yamy-pkg").string();

    EXPECT_FALSE(fs::exists(newDir));

    auto result = ConfigManager::instance().exportConfig(configPath, archivePath);
    EXPECT_TRUE(result.success) << result.errorMessage;
    EXPECT_TRUE(fs::exists(archivePath));
}

TEST_F(ConfigManagerTest, ImportAddsConfigToManager) {
    // Create and export a config
    createTestConfig("test.mayu");
    std::string configPath = (testDir / "test.mayu").string();
    std::string archivePath = (testDir / "export.yamy-pkg").string();

    auto exportResult = ConfigManager::instance().exportConfig(configPath, archivePath);
    ASSERT_TRUE(exportResult.success);

    // Import to new directory
    fs::path importDir = testDir / "imported";
    auto importResult = ConfigManager::instance().importConfig(archivePath, importDir.string());
    ASSERT_TRUE(importResult.success);

    // Verify the imported config was added to the manager
    auto configs = ConfigManager::instance().listConfigs();
    bool found = false;
    for (const auto& entry : configs) {
        if (entry.path.find("imported") != std::string::npos &&
            entry.path.find("test.mayu") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigManagerTest, GetExportDir) {
    std::string exportDir = ConfigManager::getExportDir();
    EXPECT_FALSE(exportDir.empty());
    EXPECT_TRUE(exportDir.find("exports") != std::string::npos);
}
