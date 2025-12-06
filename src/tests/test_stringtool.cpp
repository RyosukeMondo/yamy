#include <gtest/gtest.h>
#include "stringtool.h"

TEST(StringToolTest, ToWStringConversion) {
    std::string src = "Hello";
    std::wstring expected = L"Hello";
    EXPECT_EQ(to_wstring(src), expected);
}

TEST(StringToolTest, ToStringConversion) {
    std::wstring src = L"Hello";
    std::string expected = "Hello";
    EXPECT_EQ(to_string(src), expected);
}

TEST(StringToolTest, ToUTF8Conversion) {
    // Test ASCII
    std::wstring src = L"Hello";
    std::string expected = "Hello";
    EXPECT_EQ(to_UTF_8(src), expected);

    // Test non-ASCII (e.g. Japanese "Hiragana A" U+3042 -> E3 81 82)
    std::wstring src_jp = L"\u3042";
    std::string expected_jp = "\xE3\x81\x82";
    EXPECT_EQ(to_UTF_8(src_jp), expected_jp);
}
