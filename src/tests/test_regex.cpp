#include <gtest/gtest.h>
#include "stringtool.h"

TEST(RegexTest, BasicMatching) {
    tregex re(_T("abc"));
    EXPECT_TRUE(std::regex_match(_T("abc"), re));
    EXPECT_FALSE(std::regex_match(_T("abcd"), re));
    EXPECT_FALSE(std::regex_match(_T("ab"), re));
}

TEST(RegexTest, Wildcards) {
    tregex re(_T(".*"));
    EXPECT_TRUE(std::regex_match(_T("anything"), re));
    EXPECT_TRUE(std::regex_match(_T(""), re));
}

TEST(RegexTest, SubExpressions) {
    // Pattern from setting.cpp: getFilename
    tregex re(_T("^([^;]*);([^;]*);(.*)$"));
    tstring input = _T("Name;Filename;Symbols");
    
    tsmatch match;
    EXPECT_TRUE(std::regex_match(input, match, re));
    ASSERT_EQ(match.size(), 4); // 0: full, 1: Name, 2: Filename, 3: Symbols
    
    EXPECT_EQ(match.str(1), _T("Name"));
    EXPECT_EQ(match.str(2), _T("Filename"));
    EXPECT_EQ(match.str(3), _T("Symbols"));
}

TEST(RegexTest, Search) {
    // Pattern from setting.cpp: symbol parsing
    // The pattern -D([^;]*)(.*)$ consumes until semicolon.
    // So input must be separated by semicolons if multiple symbols exist.
    tregex re(_T("-D([^;]*)(.*)$"));
    tstring input = _T("-DSYM1;-DSYM2");
    
    tsmatch match;
    EXPECT_TRUE(std::regex_search(input, match, re));
    EXPECT_EQ(match.str(1), _T("SYM1"));
    EXPECT_EQ(match.str(2), _T(";-DSYM2"));
}

TEST(RegexTest, WindowClassMatching) {
    // Typical window class regex
    tregex re(_T("Notepad"));
    // regex_match matches WHOLE string by default in Boost/Std? 
    // Boost default is match_default.
    // regex_match requires full match.
    EXPECT_TRUE(std::regex_match(_T("Notepad"), re));
    EXPECT_FALSE(std::regex_match(_T("Notepad2"), re));
    
    tregex re2(_T(".*Notepad.*"));
    EXPECT_TRUE(std::regex_match(_T("MyNotepadApp"), re2));
}
