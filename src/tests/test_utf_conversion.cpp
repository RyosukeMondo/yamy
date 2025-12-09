#include <gtest/gtest.h>
#include "../platform/windows/utf_conversion.h"

using namespace yamy::platform;

TEST(UtfConversionTest, RoundTrip_ASCII) {
    std::string original = "Hello, World!";
    std::wstring wide = utf8_to_wstring(original);
    std::string back = wstring_to_utf8(wide);

    EXPECT_EQ(original, back);
    EXPECT_EQ(wide, L"Hello, World!");
}

TEST(UtfConversionTest, RoundTrip_Japanese) {
    std::string original = "日本語テスト"; // "Japanese Test"
    std::wstring wide = utf8_to_wstring(original);
    std::string back = wstring_to_utf8(wide);

    EXPECT_EQ(original, back);
    // Hard to check wide literal equality portably in source code without wide execution charset awareness,
    // but round trip confirms data integrity.
}

TEST(UtfConversionTest, EmptyString) {
    EXPECT_EQ(utf8_to_wstring(""), L"");
    EXPECT_EQ(utf8_to_wstring(std::string("")), L"");
    EXPECT_EQ(wstring_to_utf8(L""), "");
    EXPECT_EQ(wstring_to_utf8(std::wstring(L"")), "");
}

TEST(UtfConversionTest, NullPointer) {
    EXPECT_EQ(utf8_to_wstring(nullptr), L"");
    EXPECT_EQ(wstring_to_utf8(nullptr), "");
}

TEST(UtfConversionTest, Emoji) {
    std::string original = "😀"; // U+1F600 Grinning Face
    std::wstring wide = utf8_to_wstring(original);
    std::string back = wstring_to_utf8(wide);

    EXPECT_EQ(original, back);
    EXPECT_FALSE(wide.empty());
}

TEST(UtfConversionTest, LongString) {
    std::string original;
    for (int i = 0; i < 1000; ++i) {
        original += "Test";
    }
    std::wstring wide = utf8_to_wstring(original);
    std::string back = wstring_to_utf8(wide);

    EXPECT_EQ(original, back);
    EXPECT_EQ(wide.length(), 4000);
}

TEST(UtfConversionTest, CStringInterface) {
    const char* utf8 = "Test";
    std::wstring wide = utf8_to_wstring(utf8);
    EXPECT_EQ(wide, L"Test");

    const wchar_t* wstr = L"Test";
    std::string back = wstring_to_utf8(wstr);
    EXPECT_EQ(back, "Test");
}
