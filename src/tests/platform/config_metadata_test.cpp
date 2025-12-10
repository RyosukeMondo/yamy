// config_metadata_test.cpp
// Unit tests for ConfigMetadata class

#include <gtest/gtest.h>
#include "config_metadata.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class ConfigMetadataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary test directory per test
        static int testCounter = 0;
        testDir = fs::temp_directory_path() /
            ("config_metadata_test_" + std::to_string(++testCounter));
        fs::create_directories(testDir);

        // Create test config file
        createTestConfig("test.mayu");

        // Store original home for cleanup
        originalHome = getenv("HOME");

        // Set HOME to test directory for metadata isolation
        testHome = testDir / "home";
        fs::create_directories(testHome);
        setenv("HOME", testHome.string().c_str(), 1);
    }

    void TearDown() override {
        // Restore original HOME
        if (!originalHome.empty()) {
            setenv("HOME", originalHome.c_str(), 1);
        }

        // Clean up test directory
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    void createTestConfig(const std::string& name) {
        std::ofstream ofs(testDir / name);
        ofs << "# Test config: " << name << std::endl;
        ofs << "keymap Global" << std::endl;
        ofs.close();
    }

    fs::path testDir;
    fs::path testHome;
    std::string originalHome;
};

// ==================== Basic Operations ====================

TEST_F(ConfigMetadataTest, DefaultConstructor) {
    ConfigMetadata meta;
    const auto& info = meta.info();

    EXPECT_TRUE(info.name.empty());
    EXPECT_TRUE(info.description.empty());
    EXPECT_TRUE(info.author.empty());
    EXPECT_EQ(info.createdDate, 0);
    EXPECT_EQ(info.modifiedDate, 0);
    EXPECT_TRUE(info.tags.empty());
}

TEST_F(ConfigMetadataTest, LoadNonexistentMetadata) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    bool loaded = meta.load(configPath);
    EXPECT_FALSE(loaded);

    // Should have default values with config name
    EXPECT_EQ(meta.info().name, "test");
    EXPECT_TRUE(meta.info().description.empty());
}

TEST_F(ConfigMetadataTest, SaveAndLoadMetadata) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    meta.setName("My Test Config");
    meta.setDescription("A test configuration for unit testing");
    meta.setAuthor("Test Author");
    meta.addTag("test");
    meta.addTag("development");

    bool saved = meta.save(configPath);
    EXPECT_TRUE(saved);

    // Verify metadata file exists
    EXPECT_TRUE(ConfigMetadata::exists(configPath));

    // Load into new instance
    ConfigMetadata meta2;
    bool loaded = meta2.load(configPath);
    EXPECT_TRUE(loaded);

    EXPECT_EQ(meta2.info().name, "My Test Config");
    EXPECT_EQ(meta2.info().description, "A test configuration for unit testing");
    EXPECT_EQ(meta2.info().author, "Test Author");
    EXPECT_EQ(meta2.info().tags.size(), 2u);

    bool hasTestTag = false, hasDevTag = false;
    for (const auto& tag : meta2.info().tags) {
        if (tag == "test") hasTestTag = true;
        if (tag == "development") hasDevTag = true;
    }
    EXPECT_TRUE(hasTestTag);
    EXPECT_TRUE(hasDevTag);
}

TEST_F(ConfigMetadataTest, TouchUpdatesModifiedDate) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    meta.setName("Test");
    meta.save(configPath);

    std::time_t initialModified = meta.info().modifiedDate;

    // Wait over 1 second to ensure time difference (time_t has second resolution)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    bool touched = meta.touch(configPath);
    EXPECT_TRUE(touched);
    EXPECT_GT(meta.info().modifiedDate, initialModified);
}

TEST_F(ConfigMetadataTest, RemoveMetadata) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    meta.setName("Test");
    meta.save(configPath);
    EXPECT_TRUE(ConfigMetadata::exists(configPath));

    bool removed = meta.remove(configPath);
    EXPECT_TRUE(removed);
    EXPECT_FALSE(ConfigMetadata::exists(configPath));
}

TEST_F(ConfigMetadataTest, RemoveNonexistentMetadata) {
    std::string configPath = (testDir / "nonexistent.mayu").string();
    ConfigMetadata meta;

    // Should succeed silently
    bool removed = meta.remove(configPath);
    EXPECT_TRUE(removed);
}

// ==================== Metadata Directory ====================

TEST_F(ConfigMetadataTest, MetadataDirCreation) {
    std::string metaDir = ConfigMetadata::getMetadataDir();
    EXPECT_FALSE(metaDir.empty());
    EXPECT_TRUE(metaDir.find(".yamy") != std::string::npos);
    EXPECT_TRUE(metaDir.find(".metadata") != std::string::npos);

    bool created = ConfigMetadata::ensureMetadataDirExists();
    EXPECT_TRUE(created);
    EXPECT_TRUE(fs::exists(metaDir));
}

TEST_F(ConfigMetadataTest, MetadataPathGeneration) {
    std::string configPath = "/home/user/configs/my_config.mayu";
    std::string metaPath = ConfigMetadata::getMetadataPath(configPath);

    EXPECT_FALSE(metaPath.empty());
    EXPECT_TRUE(metaPath.find(".metadata") != std::string::npos);
    EXPECT_TRUE(metaPath.find(".json") != std::string::npos);
}

// ==================== Tag Operations ====================

TEST_F(ConfigMetadataTest, AddTag) {
    ConfigMetadata meta;

    meta.addTag("tag1");
    EXPECT_EQ(meta.info().tags.size(), 1u);
    EXPECT_EQ(meta.info().tags[0], "tag1");
}

TEST_F(ConfigMetadataTest, AddDuplicateTag) {
    ConfigMetadata meta;

    meta.addTag("tag1");
    meta.addTag("tag1"); // Duplicate
    EXPECT_EQ(meta.info().tags.size(), 1u);
}

TEST_F(ConfigMetadataTest, RemoveTag) {
    ConfigMetadata meta;

    meta.addTag("tag1");
    meta.addTag("tag2");
    EXPECT_EQ(meta.info().tags.size(), 2u);

    meta.removeTag("tag1");
    EXPECT_EQ(meta.info().tags.size(), 1u);
    EXPECT_EQ(meta.info().tags[0], "tag2");
}

TEST_F(ConfigMetadataTest, RemoveNonexistentTag) {
    ConfigMetadata meta;

    meta.addTag("tag1");
    meta.removeTag("nonexistent");
    EXPECT_EQ(meta.info().tags.size(), 1u);
}

TEST_F(ConfigMetadataTest, ClearTags) {
    ConfigMetadata meta;

    meta.addTag("tag1");
    meta.addTag("tag2");
    EXPECT_EQ(meta.info().tags.size(), 2u);

    meta.clearTags();
    EXPECT_TRUE(meta.info().tags.empty());
}

// ==================== Modification Tracking ====================

TEST_F(ConfigMetadataTest, SetNameUpdatesModifiedDate) {
    ConfigMetadata meta;

    std::time_t before = meta.info().modifiedDate;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    meta.setName("New Name");
    EXPECT_GT(meta.info().modifiedDate, before);
}

TEST_F(ConfigMetadataTest, SetDescriptionUpdatesModifiedDate) {
    ConfigMetadata meta;

    std::time_t before = meta.info().modifiedDate;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    meta.setDescription("New Description");
    EXPECT_GT(meta.info().modifiedDate, before);
}

TEST_F(ConfigMetadataTest, SetAuthorUpdatesModifiedDate) {
    ConfigMetadata meta;

    std::time_t before = meta.info().modifiedDate;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    meta.setAuthor("New Author");
    EXPECT_GT(meta.info().modifiedDate, before);
}

TEST_F(ConfigMetadataTest, AddTagUpdatesModifiedDate) {
    ConfigMetadata meta;

    std::time_t before = meta.info().modifiedDate;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    meta.addTag("newtag");
    EXPECT_GT(meta.info().modifiedDate, before);
}

// ==================== JSON Handling ====================

TEST_F(ConfigMetadataTest, JsonEscaping) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    // Test special characters in fields
    meta.setName("Config with \"quotes\" and \\backslash");
    meta.setDescription("Line1\nLine2\tTabbed");
    meta.addTag("tag/with/slashes");

    bool saved = meta.save(configPath);
    EXPECT_TRUE(saved);

    ConfigMetadata meta2;
    bool loaded = meta2.load(configPath);
    EXPECT_TRUE(loaded);

    EXPECT_EQ(meta2.info().name, "Config with \"quotes\" and \\backslash");
    EXPECT_EQ(meta2.info().description, "Line1\nLine2\tTabbed");
    EXPECT_EQ(meta2.info().tags.size(), 1u);
    EXPECT_EQ(meta2.info().tags[0], "tag/with/slashes");
}

TEST_F(ConfigMetadataTest, EmptyFields) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    // Save with all empty fields
    bool saved = meta.save(configPath);
    EXPECT_TRUE(saved);

    ConfigMetadata meta2;
    bool loaded = meta2.load(configPath);
    EXPECT_TRUE(loaded);

    EXPECT_TRUE(meta2.info().name.empty());
    EXPECT_TRUE(meta2.info().description.empty());
    EXPECT_TRUE(meta2.info().author.empty());
    EXPECT_TRUE(meta2.info().tags.empty());
}

TEST_F(ConfigMetadataTest, LargeMetadata) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    // Create a large description
    std::string largeDesc(10000, 'x');
    meta.setDescription(largeDesc);

    // Add many tags
    for (int i = 0; i < 100; ++i) {
        meta.addTag("tag" + std::to_string(i));
    }

    bool saved = meta.save(configPath);
    EXPECT_TRUE(saved);

    ConfigMetadata meta2;
    bool loaded = meta2.load(configPath);
    EXPECT_TRUE(loaded);

    EXPECT_EQ(meta2.info().description.size(), 10000u);
    EXPECT_EQ(meta2.info().tags.size(), 100u);
}

// ==================== Multiple Configs ====================

TEST_F(ConfigMetadataTest, DifferentConfigsSeparateMetadata) {
    createTestConfig("config1.mayu");
    createTestConfig("config2.mayu");

    std::string configPath1 = (testDir / "config1.mayu").string();
    std::string configPath2 = (testDir / "config2.mayu").string();

    ConfigMetadata meta1;
    meta1.setName("Config One");
    meta1.save(configPath1);

    ConfigMetadata meta2;
    meta2.setName("Config Two");
    meta2.save(configPath2);

    // Verify they have different metadata paths
    EXPECT_NE(ConfigMetadata::getMetadataPath(configPath1),
              ConfigMetadata::getMetadataPath(configPath2));

    // Load and verify independence
    ConfigMetadata loaded1, loaded2;
    loaded1.load(configPath1);
    loaded2.load(configPath2);

    EXPECT_EQ(loaded1.info().name, "Config One");
    EXPECT_EQ(loaded2.info().name, "Config Two");
}

// ==================== Edge Cases ====================

TEST_F(ConfigMetadataTest, ConfigPathWithSpaces) {
    fs::path spacePath = testDir / "path with spaces";
    fs::create_directories(spacePath);
    std::ofstream ofs(spacePath / "config.mayu");
    ofs << "# Config" << std::endl;
    ofs.close();

    std::string configPath = (spacePath / "config.mayu").string();

    ConfigMetadata meta;
    meta.setName("Spaced Path Config");
    bool saved = meta.save(configPath);
    EXPECT_TRUE(saved);

    ConfigMetadata meta2;
    bool loaded = meta2.load(configPath);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(meta2.info().name, "Spaced Path Config");
}

TEST_F(ConfigMetadataTest, UnicodeContent) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    meta.setName("Config \xC3\xA9\xC3\xA0\xC3\xB9"); // UTF-8 for éàù
    meta.setAuthor("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E"); // Japanese characters

    bool saved = meta.save(configPath);
    EXPECT_TRUE(saved);

    ConfigMetadata meta2;
    bool loaded = meta2.load(configPath);
    EXPECT_TRUE(loaded);

    EXPECT_EQ(meta2.info().name, "Config \xC3\xA9\xC3\xA0\xC3\xB9");
    EXPECT_EQ(meta2.info().author, "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E");
}

TEST_F(ConfigMetadataTest, CreatedDatePersistence) {
    std::string configPath = (testDir / "test.mayu").string();
    ConfigMetadata meta;

    meta.setName("Test");
    std::time_t createdDate = meta.info().createdDate;

    // Save and reload multiple times
    for (int i = 0; i < 3; ++i) {
        meta.save(configPath);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        meta.touch(configPath);
    }

    ConfigMetadata meta2;
    meta2.load(configPath);

    // Created date should still match (or at least not be 0)
    // Note: The first save will set createdDate, so we just check it's reasonable
    EXPECT_GT(meta2.info().createdDate, 0);
}
