#include <gtest/gtest.h>
#include "parser.h"

TEST(ParserTest, BasicTokenization) {
    std::string input = _T("key A = B");
    Parser p(input.c_str(), input.length());
    
    // Parser requires symbols like '=' to be registered as prefixes
    std::vector<tstringi> prefixes;
    prefixes.push_back(_T("="));
    p.setPrefixes(&prefixes);
    
    std::vector<Token> tokens;
    bool result = p.getLine(&tokens);
    
    EXPECT_TRUE(result);
    ASSERT_EQ(tokens.size(), 4);
    
    EXPECT_EQ(tokens[0].getType(), Token::Type_string);
    EXPECT_EQ(tokens[0].getString(), _T("key"));
    
    EXPECT_EQ(tokens[1].getType(), Token::Type_string);
    EXPECT_EQ(tokens[1].getString(), _T("A"));
    
    EXPECT_EQ(tokens[2].getType(), Token::Type_string);
    EXPECT_EQ(tokens[2].getString(), _T("="));
    
    EXPECT_EQ(tokens[3].getType(), Token::Type_string);
    EXPECT_EQ(tokens[3].getString(), _T("B"));
    
    EXPECT_FALSE(p.getLine(&tokens)); // No more lines
}

TEST(ParserTest, QuotedStrings) {
    std::string input = _T("key \"Space Key\"");
    Parser p(input.c_str(), input.length());
    
    std::vector<Token> tokens;
    p.getLine(&tokens);
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].getString(), _T("key"));
    EXPECT_EQ(tokens[1].getString(), _T("Space Key"));
    EXPECT_TRUE(tokens[1].isQuoted());
}

TEST(ParserTest, Comments) {
    std::string input = _T("key A # This is a comment");
    Parser p(input.c_str(), input.length());
    
    std::vector<Token> tokens;
    p.getLine(&tokens);
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].getString(), _T("key"));
    EXPECT_EQ(tokens[1].getString(), _T("A"));
    // Comment should be ignored
}

TEST(ParserTest, Prefixes) {
    std::string input = _T("M0-A");
    Parser p(input.c_str(), input.length());
    
    std::vector<tstringi> prefixes;
    prefixes.push_back(_T("M0-"));
    p.setPrefixes(&prefixes);
    
    std::vector<Token> tokens;
    p.getLine(&tokens);
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].getString(), _T("M0-"));
    EXPECT_EQ(tokens[1].getString(), _T("A"));
}
