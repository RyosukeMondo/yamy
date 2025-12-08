#include <gtest/gtest.h>

// Test Suites
// Ideally these should be compiled separately and linked,
// but for now we include them to avoid modifying vcxproj for every new test file.
#include "test_stringtool.cpp"
#include "test_setting.cpp"
#include "test_misc.cpp"
#include "test_keymap.cpp"
#include "test_parser.cpp"
#include "test_keyboard.cpp"
#include "test_function.cpp"
#include "test_layoutmanager.cpp"
#include "test_regex.cpp"

TEST(SanityCheck, BasicAssertion) {
    EXPECT_EQ(1 + 1, 2);
}

int wmain(int argc, wchar_t **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
