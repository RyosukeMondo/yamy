//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_context_keymap_test.cpp - Integration tests for window-context
// dependent keymaps on Linux
//
// Tests the window context functionality:
// 1. Window class matching with regex patterns
// 2. Window title matching with regex patterns
// 3. Keymap switching between different windows
// 4. AND/OR conditions for window matching
// 5. Focus change detection simulation
//
// These tests verify that the keymap system correctly activates based
// on window properties, which is essential for window-specific keybindings.
//
// IMPORTANT NOTE on Default Global Keymap:
// SettingLoader::initialize() creates a default "Global" keymap with
// Type_windowOr and ".*" patterns that matches ALL windows. This is the
// fallback keymap. When testing, searchWindow() will return this Global
// keymap PLUS any specific window keymaps that match. Tests must account
// for this by checking for the presence of specific keymaps rather than
// exact counts.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include "setting.h"
#include "setting_loader.h"
#include "keymap.h"
#include "keyboard.h"
#include "msgstream.h"
#include "multithread.h"
#include <vector>
#include <string>
#include <algorithm>

namespace yamy::test {

//=============================================================================
// Test Fixture for Window Context Keymap Tests
//=============================================================================

class WindowContextKeymapTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;

    void SetUp() override {
        m_loader.reset(new SettingLoader(&m_soLog, &m_logStream));
        m_loader->initialize(&m_setting);
    }

    void TearDown() override {
        m_loader.reset();
    }

    void LoadConfig(const std::string& config) {
        m_loader->loadFromData(config);
        std::string log_output = m_logStream.str();
        if (log_output.find(_T("error:")) != std::string::npos) {
            FAIL() << "Errors found during config loading: " << log_output;
        }
    }

    // Helper to define basic keyboard keys
    std::string getKeyDefinitions() {
        return
            _T("def key A = 0x1E\n")
            _T("def key B = 0x30\n")
            _T("def key C = 0x2E\n")
            _T("def key D = 0x20\n")
            _T("def key E = 0x12\n")
            _T("def key F = 0x21\n")
            _T("def key X = 0x2D\n")
            _T("def key Y = 0x15\n")
            _T("def key Z = 0x2C\n")
            _T("def key Escape = 0x01\n")
            _T("def key Enter = 0x1C\n")
            _T("def key Tab = 0x0F\n")
            _T("def key Space = 0x39\n")
            _T("def key LControl = 0x1D\n")
            _T("def key RControl = E0-0x1D\n")
            _T("def key LShift = 0x2A\n")
            _T("def key RShift = 0x36\n")
            _T("def key LAlt = 0x38\n")
            _T("def key RAlt = E0-0x38\n")
            _T("def mod Shift = LShift RShift\n")
            _T("def mod Control = LControl RControl\n")
            _T("def mod Alt = LAlt RAlt\n");
    }

    // Get keymaps matching a window
    std::vector<Keymap*> getMatchingKeymaps(const std::string& className,
                                             const std::string& titleName) {
        Keymaps::KeymapPtrList keymapList;
        m_setting.m_keymaps.searchWindow(&keymapList, className, titleName);
        return std::vector<Keymap*>(keymapList.begin(), keymapList.end());
    }

    // Check if a specific keymap is in the match list
    bool hasKeymap(const std::vector<Keymap*>& keymaps, const std::string& name) {
        return std::any_of(keymaps.begin(), keymaps.end(),
            [&name](const Keymap* km) { return km->getName() == name; });
    }

    // Get only non-Global keymaps (excluding the default fallback)
    std::vector<Keymap*> getNonGlobalKeymaps(const std::vector<Keymap*>& keymaps) {
        std::vector<Keymap*> result;
        for (auto* km : keymaps) {
            if (km->getName() != "Global") {
                result.push_back(km);
            }
        }
        return result;
    }

    // Count of specific (non-Global) keymaps
    size_t getSpecificKeymapCount(const std::vector<Keymap*>& keymaps) {
        return getNonGlobalKeymaps(keymaps).size();
    }
};

//=============================================================================
// Test 1: Window Class Matching - Simple Pattern
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowClassMatching_SimplePattern) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Terminal /gnome-terminal/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Verify Terminal keymap was created
    const Keymap* terminalMap = m_setting.m_keymaps.searchByName(_T("Terminal"));
    ASSERT_NE(terminalMap, nullptr) << "Terminal keymap not found";

    // Test window class matching - should include Terminal + Global (fallback)
    auto matches = getMatchingKeymaps("gnome-terminal", "Terminal");
    EXPECT_TRUE(hasKeymap(matches, "Terminal")) << "Should match gnome-terminal window";
    EXPECT_GE(matches.size(), 1u) << "Should have at least Terminal keymap";

    // Test non-matching window - only Global should match (not Terminal)
    auto noSpecificMatches = getMatchingKeymaps("firefox", "Mozilla Firefox");
    EXPECT_FALSE(hasKeymap(noSpecificMatches, "Terminal")) << "Terminal should not match firefox";
}

//=============================================================================
// Test 2: Window Class Matching - Case Insensitive
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowClassMatching_CaseInsensitive) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Should match regardless of case
    auto matchLower = getMatchingKeymaps("terminal", "bash");
    auto matchUpper = getMatchingKeymaps("TERMINAL", "BASH");
    auto matchMixed = getMatchingKeymaps("TerMiNaL", "Bash");

    EXPECT_TRUE(hasKeymap(matchLower, "Terminal")) << "Should match lowercase";
    EXPECT_TRUE(hasKeymap(matchUpper, "Terminal")) << "Should match uppercase";
    EXPECT_TRUE(hasKeymap(matchMixed, "Terminal")) << "Should match mixed case";
}

//=============================================================================
// Test 3: Window Class Matching - Partial Match
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowClassMatching_PartialMatch) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window XTerm /xterm/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Should match partial class names (regex is substring match by default)
    auto matchExact = getMatchingKeymaps("xterm", "XTerm");
    auto matchPartial = getMatchingKeymaps("uxterm", "UXTerm");
    auto matchSuffix = getMatchingKeymaps("gnome-xterm-wrapper", "Terminal");

    EXPECT_TRUE(hasKeymap(matchExact, "XTerm")) << "Should match exact";
    EXPECT_TRUE(hasKeymap(matchPartial, "XTerm")) << "Should match partial (contains)";
    EXPECT_TRUE(hasKeymap(matchSuffix, "XTerm")) << "Should match suffix";
}

//=============================================================================
// Test 4: Window Title Matching - AND Condition
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowTitleMatching_AndCondition) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Editor ( /code/ && /\\.cpp/ ) : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Both class AND title must match for the Editor keymap
    auto matchBoth = getMatchingKeymaps("code", "main.cpp - VS Code");
    auto matchClassOnly = getMatchingKeymaps("code", "Welcome");
    auto matchTitleOnly = getMatchingKeymaps("vim", "main.cpp");

    EXPECT_TRUE(hasKeymap(matchBoth, "Editor")) << "Should match when both match";
    EXPECT_FALSE(hasKeymap(matchClassOnly, "Editor")) << "Should not match class only";
    EXPECT_FALSE(hasKeymap(matchTitleOnly, "Editor")) << "Should not match title only";
}

//=============================================================================
// Test 5: Window Title Matching - OR Condition
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowTitleMatching_OrCondition) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Browser ( /firefox/ || /chrome/ ) : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Either class OR title can match for the Browser keymap
    auto matchClass = getMatchingKeymaps("firefox", "Mozilla Firefox");
    auto matchTitle = getMatchingKeymaps("something", "chrome");
    auto matchNeither = getMatchingKeymaps("vim", "editor");

    EXPECT_TRUE(hasKeymap(matchClass, "Browser")) << "Should match on class";
    EXPECT_TRUE(hasKeymap(matchTitle, "Browser")) << "Should match on title";
    EXPECT_FALSE(hasKeymap(matchNeither, "Browser")) << "Should not match when neither matches";
}

//=============================================================================
// Test 6: Multiple Window Keymaps
//=============================================================================

TEST_F(WindowContextKeymapTest, MultipleWindowKeymaps) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = C\n")
        _T("window Browser /firefox/ : Global\n")
        _T("key A = D\n")
        _T("window Editor /code/ : Global\n")
        _T("key A = E\n");

    LoadConfig(config);

    // Verify all keymaps created
    EXPECT_NE(m_setting.m_keymaps.searchByName(_T("Terminal")), nullptr);
    EXPECT_NE(m_setting.m_keymaps.searchByName(_T("Browser")), nullptr);
    EXPECT_NE(m_setting.m_keymaps.searchByName(_T("Editor")), nullptr);

    // Each window should match its specific keymap
    auto termMatches = getMatchingKeymaps("terminal", "bash");
    auto firefoxMatches = getMatchingKeymaps("firefox", "Google");
    auto codeMatches = getMatchingKeymaps("code", "project");

    EXPECT_TRUE(hasKeymap(termMatches, "Terminal"));
    EXPECT_FALSE(hasKeymap(termMatches, "Browser"));
    EXPECT_FALSE(hasKeymap(termMatches, "Editor"));

    EXPECT_TRUE(hasKeymap(firefoxMatches, "Browser"));
    EXPECT_FALSE(hasKeymap(firefoxMatches, "Terminal"));

    EXPECT_TRUE(hasKeymap(codeMatches, "Editor"));
    EXPECT_FALSE(hasKeymap(codeMatches, "Terminal"));
}

//=============================================================================
// Test 7: Window Keymap Inheritance
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowKeymapInheritance) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("key X = Y\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = C\n");
        // X = Y should be inherited from Global

    LoadConfig(config);

    const Keymap* terminalMap = m_setting.m_keymaps.searchByName(_T("Terminal"));
    ASSERT_NE(terminalMap, nullptr);

    // Check inheritance
    EXPECT_NE(terminalMap->getParentKeymap(), nullptr) << "Should have parent";
    EXPECT_EQ(terminalMap->getParentKeymap()->getName(), _T("Global"));

    // Check Terminal has its own A mapping
    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);
    ModifiedKey mkA(keyA);
    const Keymap::KeyAssignment* terminalKA = terminalMap->searchAssignment(mkA);
    EXPECT_NE(terminalKA, nullptr) << "Terminal should have A mapping";
}

//=============================================================================
// Test 8: Window Context Switch Simulation
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowContextSwitchSimulation) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = X\n")
        _T("window Terminal /gnome-terminal/ : Global\n")
        _T("key A = Y\n")
        _T("window Browser /firefox/ : Global\n")
        _T("key A = Z\n");

    LoadConfig(config);

    // Simulate switching between windows
    struct WindowContext {
        std::string className;
        std::string title;
        std::string expectedKeymap;  // specific keymap (empty = none)
        bool shouldHaveSpecific;
    };

    std::vector<WindowContext> contexts = {
        {"gnome-terminal", "user@host: ~", "Terminal", true},
        {"firefox", "Google Search - Mozilla Firefox", "Browser", true},
        {"gnome-terminal", "vim project", "Terminal", true},
        {"code", "main.cpp - Visual Studio Code", "", false}, // No specific keymap
        {"firefox", "GitHub - Mozilla Firefox", "Browser", true},
    };

    for (const auto& ctx : contexts) {
        auto matches = getMatchingKeymaps(ctx.className, ctx.title);

        if (ctx.shouldHaveSpecific) {
            EXPECT_TRUE(hasKeymap(matches, ctx.expectedKeymap))
                << "Window (" << ctx.className << ", " << ctx.title
                << ") should match keymap " << ctx.expectedKeymap;
        } else {
            // For windows with no specific keymap, verify none of the specific ones match
            EXPECT_FALSE(hasKeymap(matches, "Terminal"))
                << "Window (" << ctx.className << ", " << ctx.title
                << ") should not match Terminal";
            EXPECT_FALSE(hasKeymap(matches, "Browser"))
                << "Window (" << ctx.className << ", " << ctx.title
                << ") should not match Browser";
        }
    }
}

//=============================================================================
// Test 9: Regex Special Characters in Window Pattern
//=============================================================================

TEST_F(WindowContextKeymapTest, RegexSpecialCharacters) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // Match windows with dots in class name (e.g., org.gnome.Terminal)
        _T("window GnomeApp /org\\.gnome\\./ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Should match org.gnome.* patterns
    auto matchGnome = getMatchingKeymaps("org.gnome.Terminal", "Terminal");
    auto matchNonGnome = getMatchingKeymaps("org-gnome-Terminal", "Terminal");

    EXPECT_TRUE(hasKeymap(matchGnome, "GnomeApp")) << "Should match org.gnome. pattern";
    // The non-dot version should not match because we escaped the dots
    EXPECT_FALSE(hasKeymap(matchNonGnome, "GnomeApp")) << "Should not match org-gnome- (dots escaped)";
}

//=============================================================================
// Test 10: Window Pattern with Wildcard (Any Class)
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowPatternWildcard) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // Match any window with ".cpp" in title
        _T("window CppFile ( /.*/ && /\\.cpp/ ) : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Any window class with .cpp in title should match
    auto matchVim = getMatchingKeymaps("vim", "main.cpp");
    auto matchCode = getMatchingKeymaps("code", "test.cpp - Editor");
    auto matchNoExtension = getMatchingKeymaps("vim", "main.py");

    EXPECT_TRUE(hasKeymap(matchVim, "CppFile")) << "Should match any class with .cpp title";
    EXPECT_TRUE(hasKeymap(matchCode, "CppFile")) << "Should match any class with .cpp title";
    EXPECT_FALSE(hasKeymap(matchNoExtension, "CppFile")) << "Should not match .py title";
}

//=============================================================================
// Test 11: Linux-Specific Window Classes
//=============================================================================

TEST_F(WindowContextKeymapTest, LinuxSpecificWindowClasses) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // Common Linux desktop application classes
        _T("window Nautilus /nautilus|thunar|dolphin/ : Global\n")
        _T("key A = C\n")
        _T("window Terminal /gnome-terminal|konsole|xfce4-terminal|alacritty|kitty/ : Global\n")
        _T("key A = D\n");

    LoadConfig(config);

    // Test various Linux terminal emulators
    std::vector<std::string> terminalApps = {
        "gnome-terminal-server",
        "konsole",
        "alacritty",
        "kitty",
        "xfce4-terminal",
    };

    for (const auto& app : terminalApps) {
        auto matches = getMatchingKeymaps(app, "Terminal Window");
        EXPECT_TRUE(hasKeymap(matches, "Terminal")) << app << " should match Terminal keymap";
    }

    // Test Linux file managers
    std::vector<std::string> fileManagers = {"nautilus", "thunar", "dolphin"};
    for (const auto& fm : fileManagers) {
        auto matches = getMatchingKeymaps(fm, "Home");
        EXPECT_TRUE(hasKeymap(matches, "Nautilus")) << fm << " should match Nautilus keymap";
    }
}

//=============================================================================
// Test 12: Overlapping Window Patterns
//=============================================================================

TEST_F(WindowContextKeymapTest, OverlappingWindowPatterns) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // More specific pattern
        _T("window VSCode /code/ : Global\n")
        _T("key A = C\n")
        // Broader pattern that also matches 'code'
        _T("window AnyEditor /code|vim|emacs/ : Global\n")
        _T("key A = D\n");

    LoadConfig(config);

    // Both keymaps should match for "code"
    auto matches = getMatchingKeymaps("code", "project.cpp");
    EXPECT_TRUE(hasKeymap(matches, "VSCode")) << "VSCode keymap should match 'code'";
    EXPECT_TRUE(hasKeymap(matches, "AnyEditor")) << "AnyEditor keymap should match 'code'";

    // Only AnyEditor should match for "vim"
    auto vimMatches = getMatchingKeymaps("vim", "file.cpp");
    EXPECT_FALSE(hasKeymap(vimMatches, "VSCode")) << "VSCode should not match vim";
    EXPECT_TRUE(hasKeymap(vimMatches, "AnyEditor")) << "AnyEditor should match vim";
}

//=============================================================================
// Test 13: Empty Window Class/Title Handling
//=============================================================================

TEST_F(WindowContextKeymapTest, EmptyWindowHandling) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Empty class name - Terminal shouldn't match
    auto emptyClass = getMatchingKeymaps("", "Terminal Window");
    EXPECT_FALSE(hasKeymap(emptyClass, "Terminal")) << "Empty class should not match Terminal";

    // Empty title (some X11 windows don't have titles) - should still match class
    auto emptyTitle = getMatchingKeymaps("terminal", "");
    EXPECT_TRUE(hasKeymap(emptyTitle, "Terminal")) << "Empty title should still match class";

    // Both empty - Terminal shouldn't match
    auto bothEmpty = getMatchingKeymaps("", "");
    EXPECT_FALSE(hasKeymap(bothEmpty, "Terminal")) << "Both empty should not match Terminal";
}

//=============================================================================
// Test 14: Unicode Window Titles (Linux/X11 uses UTF-8)
//=============================================================================

TEST_F(WindowContextKeymapTest, UnicodeWindowTitles) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // Match firefox with any title (including unicode)
        _T("window Firefox /firefox/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Test with UTF-8 window titles (common in Linux)
    auto matchJapanese = getMatchingKeymaps("firefox", "日本語 - Mozilla Firefox");
    auto matchChinese = getMatchingKeymaps("firefox", "中文网站 - Firefox");
    auto matchEmoji = getMatchingKeymaps("firefox", "GitHub 🚀 - Firefox");

    // All should match Firefox keymap because class matches
    EXPECT_TRUE(hasKeymap(matchJapanese, "Firefox")) << "Should handle Japanese UTF-8";
    EXPECT_TRUE(hasKeymap(matchChinese, "Firefox")) << "Should handle Chinese UTF-8";
    EXPECT_TRUE(hasKeymap(matchEmoji, "Firefox")) << "Should handle emoji UTF-8";
}

//=============================================================================
// Test 15: Key Assignment Lookup in Context
//=============================================================================

TEST_F(WindowContextKeymapTest, KeyAssignmentLookupInContext) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = X\n")
        _T("window Terminal /terminal/ : Global\n")
        _T("key A = Y\n");

    LoadConfig(config);

    Key* keyA = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(keyA, nullptr);
    ModifiedKey mkA(keyA);

    // Get keymaps for terminal windows
    auto termMatches = getMatchingKeymaps("terminal", "bash");
    EXPECT_TRUE(hasKeymap(termMatches, "Terminal"));

    // Verify Terminal keymap has the A -> Y mapping
    const Keymap* terminalMap = m_setting.m_keymaps.searchByName(_T("Terminal"));
    ASSERT_NE(terminalMap, nullptr);
    const Keymap::KeyAssignment* termKA = terminalMap->searchAssignment(mkA);
    EXPECT_NE(termKA, nullptr) << "Terminal should have A mapping";

    // Verify Global keymap still has A -> X mapping
    const Keymap* globalMap = m_setting.m_keymaps.searchByName(_T("Global"));
    ASSERT_NE(globalMap, nullptr);
    const Keymap::KeyAssignment* globalKA = globalMap->searchAssignment(mkA);
    EXPECT_NE(globalKA, nullptr) << "Global should have A mapping";
}

//=============================================================================
// Test 16: Keymap Type Detection (AND vs OR logic)
//=============================================================================

TEST_F(WindowContextKeymapTest, KeymapTypeDetection) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // Type_windowAnd (class && title)
        _T("window AndTest ( /class/ && /title/ ) : Global\n")
        _T("key A = C\n")
        // Type_windowOr (class || title)
        _T("window OrTest ( /class/ || /title/ ) : Global\n")
        _T("key A = D\n");

    LoadConfig(config);

    // Verify keymaps exist
    EXPECT_NE(m_setting.m_keymaps.searchByName(_T("Global")), nullptr);
    EXPECT_NE(m_setting.m_keymaps.searchByName(_T("AndTest")), nullptr);
    EXPECT_NE(m_setting.m_keymaps.searchByName(_T("OrTest")), nullptr);

    // Test AND logic - both must match
    auto andBothMatch = getMatchingKeymaps("class", "title");
    auto andClassOnly = getMatchingKeymaps("class", "other");
    auto andTitleOnly = getMatchingKeymaps("other", "title");

    EXPECT_TRUE(hasKeymap(andBothMatch, "AndTest")) << "AndTest should match when both match";
    EXPECT_FALSE(hasKeymap(andClassOnly, "AndTest")) << "AndTest should not match class only";
    EXPECT_FALSE(hasKeymap(andTitleOnly, "AndTest")) << "AndTest should not match title only";

    // Test OR logic - either can match
    EXPECT_TRUE(hasKeymap(andBothMatch, "OrTest")) << "OrTest should match when both match";
    EXPECT_TRUE(hasKeymap(andClassOnly, "OrTest")) << "OrTest should match when class matches";
    EXPECT_TRUE(hasKeymap(andTitleOnly, "OrTest")) << "OrTest should match when title matches";
}

//=============================================================================
// Test 17: Does Same Window Method Direct Test
// REMOVED: doesSameWindow method no longer exists (FR-3: global keymap only)
//=============================================================================

//=============================================================================
// Test 18: Window Pattern with Anchor (Exact Match)
//=============================================================================

TEST_F(WindowContextKeymapTest, WindowPatternExactMatch) {
    std::string config = getKeyDefinitions() +
        _T("keymap Global\n")
        _T("key A = B\n")
        // Use anchors for exact match
        _T("window Exact /^firefox$/ : Global\n")
        _T("key A = C\n");

    LoadConfig(config);

    // Only exact "firefox" should match
    auto matchExact = getMatchingKeymaps("firefox", "Mozilla Firefox");
    auto matchPrefix = getMatchingKeymaps("firefox-esr", "Mozilla Firefox");
    auto matchSuffix = getMatchingKeymaps("my-firefox", "Mozilla Firefox");

    EXPECT_TRUE(hasKeymap(matchExact, "Exact")) << "Should match exact 'firefox'";
    EXPECT_FALSE(hasKeymap(matchPrefix, "Exact")) << "Should not match 'firefox-esr'";
    EXPECT_FALSE(hasKeymap(matchSuffix, "Exact")) << "Should not match 'my-firefox'";
}

} // namespace yamy::test
