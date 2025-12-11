//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// session_manager_test.cpp
// Unit tests for SessionManager

#include <gtest/gtest.h>
#include "session_manager.h"
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

using namespace yamy;

class SessionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temporary directory for testing
        m_originalHome = getenv("HOME") ? getenv("HOME") : "";
        m_originalXdgConfig = getenv("XDG_CONFIG_HOME") ? getenv("XDG_CONFIG_HOME") : "";

        // Create temp test dir
        char tmpDir[] = "/tmp/yamy_session_test_XXXXXX";
        m_testDir = mkdtemp(tmpDir);
        ASSERT_FALSE(m_testDir.empty());

        // Set XDG_CONFIG_HOME to test directory
        setenv("XDG_CONFIG_HOME", m_testDir.c_str(), 1);

        // Clear singleton state
        SessionManager::instance().clearSession();
        SessionManager::instance().data() = SessionData();
    }

    void TearDown() override {
        // Clean up test files
        std::string sessionPath = m_testDir + "/yamy/session.json";
        std::remove(sessionPath.c_str());

        std::string yamyDir = m_testDir + "/yamy";
        rmdir(yamyDir.c_str());
        rmdir(m_testDir.c_str());

        // Restore environment
        if (!m_originalXdgConfig.empty()) {
            setenv("XDG_CONFIG_HOME", m_originalXdgConfig.c_str(), 1);
        } else {
            unsetenv("XDG_CONFIG_HOME");
        }
    }

    std::string m_testDir;
    std::string m_originalHome;
    std::string m_originalXdgConfig;
};

TEST_F(SessionManagerTest, SingletonInstance) {
    SessionManager& sm1 = SessionManager::instance();
    SessionManager& sm2 = SessionManager::instance();
    EXPECT_EQ(&sm1, &sm2);
}

TEST_F(SessionManagerTest, ConfigDirPath) {
    std::string configDir = SessionManager::getConfigDir();
    EXPECT_EQ(configDir, m_testDir + "/yamy");
}

TEST_F(SessionManagerTest, SessionFilePath) {
    std::string sessionPath = SessionManager::getSessionPath();
    EXPECT_EQ(sessionPath, m_testDir + "/yamy/session.json");
}

TEST_F(SessionManagerTest, SaveAndRestoreBasicSession) {
    SessionManager& sm = SessionManager::instance();

    // Set session data
    sm.setActiveConfig("/home/user/.yamy/work.mayu");
    sm.setEngineRunning(true);

    // Save
    EXPECT_TRUE(sm.saveSession());
    EXPECT_TRUE(sm.hasSession());

    // Clear in-memory data
    sm.data() = SessionData();
    EXPECT_TRUE(sm.data().activeConfigPath.empty());
    EXPECT_FALSE(sm.data().engineWasRunning);

    // Restore
    EXPECT_TRUE(sm.restoreSession());
    EXPECT_EQ(sm.data().activeConfigPath, "/home/user/.yamy/work.mayu");
    EXPECT_TRUE(sm.data().engineWasRunning);
}

TEST_F(SessionManagerTest, SaveAndRestoreWindowPositions) {
    SessionManager& sm = SessionManager::instance();

    sm.saveWindowPosition("LogDialog", 100, 200, 800, 600);
    sm.saveWindowPosition("SettingsDialog", 50, 50, 400, 300);

    EXPECT_TRUE(sm.saveSession());

    // Clear and restore
    sm.data() = SessionData();
    EXPECT_TRUE(sm.restoreSession());

    WindowPosition logPos = sm.getWindowPosition("LogDialog");
    EXPECT_TRUE(logPos.valid);
    EXPECT_EQ(logPos.x, 100);
    EXPECT_EQ(logPos.y, 200);
    EXPECT_EQ(logPos.width, 800);
    EXPECT_EQ(logPos.height, 600);

    WindowPosition settingsPos = sm.getWindowPosition("SettingsDialog");
    EXPECT_TRUE(settingsPos.valid);
    EXPECT_EQ(settingsPos.x, 50);
    EXPECT_EQ(settingsPos.y, 50);
    EXPECT_EQ(settingsPos.width, 400);
    EXPECT_EQ(settingsPos.height, 300);
}

TEST_F(SessionManagerTest, GetNonExistentWindowPosition) {
    SessionManager& sm = SessionManager::instance();
    WindowPosition pos = sm.getWindowPosition("NonExistent");
    EXPECT_FALSE(pos.valid);
}

TEST_F(SessionManagerTest, RestoreNonExistentSession) {
    SessionManager& sm = SessionManager::instance();
    EXPECT_FALSE(sm.hasSession());
    EXPECT_FALSE(sm.restoreSession());
}

TEST_F(SessionManagerTest, ClearSession) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/test/config.mayu");
    EXPECT_TRUE(sm.saveSession());
    EXPECT_TRUE(sm.hasSession());

    EXPECT_TRUE(sm.clearSession());
    EXPECT_FALSE(sm.hasSession());
}

TEST_F(SessionManagerTest, TimestampSaved) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/test/config.mayu");
    EXPECT_TRUE(sm.saveSession());

    int64_t beforeRestore = sm.data().savedTimestamp;
    EXPECT_GT(beforeRestore, 0);

    sm.data() = SessionData();
    EXPECT_TRUE(sm.restoreSession());

    EXPECT_EQ(sm.data().savedTimestamp, beforeRestore);
}

TEST_F(SessionManagerTest, JsonFormatReadable) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/home/user/config.mayu");
    sm.setEngineRunning(true);
    sm.saveWindowPosition("TestWindow", 10, 20, 300, 200);

    EXPECT_TRUE(sm.saveSession());

    // Read the JSON file directly
    std::ifstream file(SessionManager::getSessionPath());
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // Verify it contains expected JSON structure
    EXPECT_NE(content.find("\"activeConfigPath\""), std::string::npos);
    EXPECT_NE(content.find("/home/user/config.mayu"), std::string::npos);
    EXPECT_NE(content.find("\"engineWasRunning\": true"), std::string::npos);
    EXPECT_NE(content.find("\"windowPositions\""), std::string::npos);
    EXPECT_NE(content.find("\"TestWindow\""), std::string::npos);
}

TEST_F(SessionManagerTest, EscapesSpecialCharactersInPath) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/home/user/My Configs/test\"config.mayu");
    EXPECT_TRUE(sm.saveSession());

    sm.data() = SessionData();
    EXPECT_TRUE(sm.restoreSession());

    EXPECT_EQ(sm.data().activeConfigPath, "/home/user/My Configs/test\"config.mayu");
}

TEST_F(SessionManagerTest, HandlesEmptySession) {
    SessionManager& sm = SessionManager::instance();

    // Save empty session
    EXPECT_TRUE(sm.saveSession());

    sm.data().activeConfigPath = "something";
    EXPECT_TRUE(sm.restoreSession());

    EXPECT_TRUE(sm.data().activeConfigPath.empty());
    EXPECT_FALSE(sm.data().engineWasRunning);
}

TEST_F(SessionManagerTest, ValidatesCorruptTimestamp) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/test/config.mayu");
    EXPECT_TRUE(sm.saveSession());

    // Corrupt the JSON file with a future timestamp
    std::string sessionPath = SessionManager::getSessionPath();
    std::ifstream inFile(sessionPath);
    std::string content((std::istreambuf_iterator<char>(inFile)),
                        std::istreambuf_iterator<char>());
    inFile.close();

    // Replace timestamp with a far future value
    size_t tsPos = content.find("\"savedTimestamp\":");
    ASSERT_NE(tsPos, std::string::npos);
    size_t colonPos = content.find(':', tsPos);
    size_t commaPos = content.find(',', colonPos);

    std::string newContent = content.substr(0, colonPos + 1);
    newContent += " 9999999999999";  // Far future
    newContent += content.substr(commaPos);

    std::ofstream outFile(sessionPath);
    outFile << newContent;
    outFile.close();

    sm.data() = SessionData();
    EXPECT_FALSE(sm.restoreSession());
}

TEST_F(SessionManagerTest, ValidatesUnreasonableWindowSize) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/test/config.mayu");
    sm.saveWindowPosition("BadWindow", 0, 0, 50000, 50000);  // Unreasonably large
    EXPECT_TRUE(sm.saveSession());

    sm.data() = SessionData();
    EXPECT_FALSE(sm.restoreSession());  // Should fail validation
}

TEST_F(SessionManagerTest, ValidatesNegativeWindowDimensions) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("/test/config.mayu");
    EXPECT_TRUE(sm.saveSession());

    // Corrupt the JSON with negative dimensions
    std::string sessionPath = SessionManager::getSessionPath();

    // Rewrite with bad window position
    std::ofstream outFile(sessionPath);
    outFile << "{\n"
            << "  \"activeConfigPath\": \"/test/config.mayu\",\n"
            << "  \"engineWasRunning\": false,\n"
            << "  \"savedTimestamp\": " << time(nullptr) << ",\n"
            << "  \"windowPositions\": {\n"
            << "    \"BadWindow\": {\n"
            << "      \"x\": 0,\n"
            << "      \"y\": 0,\n"
            << "      \"width\": -100,\n"
            << "      \"height\": 200\n"
            << "    }\n"
            << "  }\n"
            << "}\n";
    outFile.close();

    sm.data() = SessionData();
    EXPECT_FALSE(sm.restoreSession());  // Should fail validation
}

TEST_F(SessionManagerTest, RejectsInvalidConfigPath) {
    SessionManager& sm = SessionManager::instance();
    EXPECT_TRUE(sm.saveSession());

    // Write session with relative path (invalid)
    std::string sessionPath = SessionManager::getSessionPath();
    std::ofstream outFile(sessionPath);
    outFile << "{\n"
            << "  \"activeConfigPath\": \"relative/path.mayu\",\n"
            << "  \"engineWasRunning\": false,\n"
            << "  \"savedTimestamp\": " << time(nullptr) << ",\n"
            << "  \"windowPositions\": {}\n"
            << "}\n";
    outFile.close();

    sm.data() = SessionData();
    EXPECT_FALSE(sm.restoreSession());
}

TEST_F(SessionManagerTest, AcceptsTildeConfigPath) {
    SessionManager& sm = SessionManager::instance();

    sm.setActiveConfig("~/.yamy/config.mayu");
    EXPECT_TRUE(sm.saveSession());

    sm.data() = SessionData();
    EXPECT_TRUE(sm.restoreSession());
    EXPECT_EQ(sm.data().activeConfigPath, "~/.yamy/config.mayu");
}

TEST_F(SessionManagerTest, MultipleWindowPositionUpdates) {
    SessionManager& sm = SessionManager::instance();

    sm.saveWindowPosition("Window1", 0, 0, 100, 100);
    sm.saveWindowPosition("Window1", 50, 50, 200, 200);  // Update same window

    EXPECT_TRUE(sm.saveSession());

    sm.data() = SessionData();
    EXPECT_TRUE(sm.restoreSession());

    WindowPosition pos = sm.getWindowPosition("Window1");
    EXPECT_TRUE(pos.valid);
    EXPECT_EQ(pos.x, 50);
    EXPECT_EQ(pos.y, 50);
    EXPECT_EQ(pos.width, 200);
    EXPECT_EQ(pos.height, 200);
}
