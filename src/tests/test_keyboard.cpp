#include <gtest/gtest.h>
#include "../core/keyboard.h"

TEST(ScanCodeTest, Equality) {
    ScanCode sc1(10, 0);
    ScanCode sc2(10, 0);
    ScanCode sc3(11, 0);
    
    EXPECT_EQ(sc1, sc2);
    EXPECT_NE(sc1, sc3);
}

TEST(KeyTest, BasicProperties) {
    Key k(_T("A"));
    EXPECT_EQ(k.getName(), _T("A"));
    // Constructor adds a default scan code
    EXPECT_EQ(k.getScanCodesSize(), 1);
    
    ScanCode sc(0x1E, 0);
    k.addScanCode(sc);
    EXPECT_EQ(k.getScanCodesSize(), 2);
}

TEST(KeyboardTest, AddAndSearchKey) {
    Keyboard kb;
    
    Key k2;
    k2.addName(_T("B"));
    k2.addScanCode(ScanCode(0x30, 0));
    
    kb.addKey(k2);
    
    Key* found = kb.searchKey(_T("B"));
    ASSERT_TRUE(found != nullptr);
    EXPECT_EQ(found->getName(), _T("B"));
    
    // Search by object (scan code match)
    Key searchK;
    searchK.addScanCode(ScanCode(0x30, 0));
    Key* found2 = kb.searchKey(searchK);
    ASSERT_TRUE(found2 != nullptr);
    EXPECT_EQ(found2->getName(), _T("B"));
}

TEST(KeyboardTest, Aliases) {
    Keyboard kb;
    Key k;
    k.addName(_T("Original"));
    k.addScanCode(ScanCode(1, 0));
    kb.addKey(k);
    
    Key* ptr = kb.searchKey(_T("Original"));
    ASSERT_TRUE(ptr != nullptr);
    
    kb.addAlias(_T("Alias"), ptr);
    
    Key* found = kb.searchKey(_T("Alias"));
    EXPECT_EQ(found, ptr);
    EXPECT_EQ(found->getName(), _T("Original"));
}
