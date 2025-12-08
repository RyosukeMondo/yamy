#include <gtest/gtest.h>
#include "setting.h"
#include "setting_loader.h"
#include "msgstream.h"
#include "multithread.h" // For CriticalSection

class SettingLoaderTest : public ::testing::Test {
protected:
    Setting m_setting;
    CriticalSection m_soLog;
    tstringstream m_logStream;
    std::unique_ptr<SettingLoader> m_loader;

    void SetUp() override {
        m_loader.reset(new SettingLoader(&m_soLog, &m_logStream));
        m_loader->initialize(&m_setting);
    }

    void LoadConfig(const tstring& config) {
        m_loader->loadFromData(config);
        // Check for errors in log
        tstring log_output = m_logStream.str();
        if (log_output.find(_T("error:")) != tstring::npos) {
            FAIL() << "Errors found in log: " << log_output;
        }
    }
    
    void LoadConfigExpectError(const tstring& config, const tstring& errorFragment) {
        m_loader->loadFromData(config);
        tstring log_output = m_logStream.str();
        EXPECT_NE(log_output.find(_T("error:")), tstring::npos) << "Expected error not found";
        EXPECT_NE(log_output.find(errorFragment), tstring::npos) 
            << "Expected error fragment '" << errorFragment << "' not found in: " << log_output;
    }
};

TEST_F(SettingLoaderTest, LoadSimpleKeyDef) {
    tstring config = _T("def key A = 0x1E\n");
    LoadConfig(config);
    
    Key* k = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(k, nullptr);
    EXPECT_EQ(k->getScanCodesSize(), 1ULL);
}

TEST_F(SettingLoaderTest, LoadKeymapDefinition) {
    tstring config = 
        _T("keymap MyMap\n")
        _T("keymap AnotherMap : MyMap\n"); // Inheritance
    
    LoadConfig(config);
    
    const Keymap* km1 = m_setting.m_keymaps.searchByName(_T("MyMap"));
    ASSERT_NE(km1, nullptr);
    
    const Keymap* km2 = m_setting.m_keymaps.searchByName(_T("AnotherMap"));
    ASSERT_NE(km2, nullptr);
    // We can't easily check inheritance hierarchy from public API of Keymap without casting/friends,
    // but successful load implies parsing worked.
}

TEST_F(SettingLoaderTest, ConditionalIf) {
    // We can test if/else by defining different keys based on condition.
    // However, SettingLoader evaluates conditions based on environment (windows, etc.)
    // which is hard to mock without Engine. 
    // But basic syntax check is possible.
    // 'if' requires a condition. '1' is not a valid condition symbol unless defined.
    // false is a valid symbol? No.
    // Let's try to define a variable? No, variables are for &Variable.
    
    // We can use 'if (1)' if we could evaluate expressions, but SettingLoader::load_IF uses symbols.
    // It checks m_setting->m_symbols.
    
    m_setting.m_symbols.insert(_T("TEST_SYMBOL"));
    
    tstring config = 
        _T("if ( TEST_SYMBOL )\n")
        _T("  def key A = 0x1E\n")
        _T("else\n")
        _T("  def key A = 0x1F\n")
        _T("endif\n");
        
    LoadConfig(config);
    
    Key* k = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(k, nullptr);
    // Should be 0x1E (30)
    ASSERT_EQ(k->getScanCodesSize(), 1);
    // ScanCode is distinct.
    // 0x1E is 30.
    EXPECT_EQ(k->getScanCodes()[0].m_scan, 0x1E);
}

TEST_F(SettingLoaderTest, ConditionalElse) {
    // NOT defining TEST_SYMBOL
    
    tstring config = 
        _T("if ( TEST_SYMBOL )\n")
        _T("  def key A = 0x1E\n")
        _T("else\n")
        _T("  def key A = 0x1F\n")
        _T("endif\n");
        
    LoadConfig(config);
    
    Key* k = m_setting.m_keyboard.searchKey(_T("A"));
    ASSERT_NE(k, nullptr);
    // Should be 0x1F (31)
    EXPECT_EQ(k->getScanCodes()[0].m_scan, 0x1F);
}

TEST_F(SettingLoaderTest, InvalidSyntax) {
    LoadConfigExpectError(_T("def mod shift = UnknownKey"), _T("invalid key name"));
}

TEST_F(SettingLoaderTest, ModifierDefinition) {
    // Need to define key first
    tstring config = 
        _T("def key LShift = 0x2A\n")
        _T("def mod shift = LShift\n");
        
    LoadConfig(config);
    
    // Check if modifier is registered.
    // Keyboard::m_mods is private.
    // We can check via Keyboard::getModifiers
    const Keyboard::Mods& mods = m_setting.m_keyboard.getModifiers(Modifier::Type_Shift);
    EXPECT_FALSE(mods.empty());
    EXPECT_EQ(mods.front()->getName(), _T("LShift"));
}

