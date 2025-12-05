#include <gtest/gtest.h>

// Test Suites
// Ideally these should be compiled separately and linked,
// but for now we include them to avoid modifying vcxproj for every new test file.
#include "test_stringtool.cpp"
#include "test_setting.cpp"

TEST(SanityCheck, BasicAssertion) {
    EXPECT_EQ(1 + 1, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
