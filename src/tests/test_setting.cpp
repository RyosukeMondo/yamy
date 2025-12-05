#include <gtest/gtest.h>
#include "../core/setting.h"
#include "../utils/msgstream.h"
#include "../utils/multithread.h" // For CriticalSection

TEST(SettingLoaderTest, LoadFromDataSimpleConfig) {
    Setting setting;
    tstringstream log_stream;
    
    // Use CriticalSection which implements SyncObject
    CriticalSection so_log;
    
    // SettingLoader takes tostream* (which tstringstream inherits from)
    SettingLoader loader(&so_log, &log_stream);
    loader.initialize(&setting);
    
    // Simple configuration string
    // Define keys first because we don't load standard key definitions
    // Then define assignment in Global keymap
    tstring config = _T("def key A = 0x1E\n")
                     _T("def key B = 0x30\n")
                     _T("keymap Global\n")
                     _T("key A = B\n");
    
    // Load
    loader.loadFromData(config);
    
    // Verify
    // Check if keymap "Global" exists
    const Keymap *km = setting.m_keymaps.searchByName(_T("Global"));
    EXPECT_NE(km, nullptr);
    
    // Verify there were no errors
    tstring log_output = log_stream.str();
    EXPECT_EQ(log_output.find(_T("error:")), tstring::npos);
}
