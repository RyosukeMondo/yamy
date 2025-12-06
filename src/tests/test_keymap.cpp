#include <gtest/gtest.h>
#include "keymap.h"
#include "keyboard.h"

TEST(KeySeqTest, BasicConstruction) {
    KeySeq ks(_T("MySeq"));
    EXPECT_EQ(ks.getName(), _T("MySeq"));
    EXPECT_EQ(ks.getMode(), Modifier::Type_KEYSEQ);
    EXPECT_EQ(ks.getActions().size(), 0);
}

TEST(KeySeqTest, AddAction) {
    KeySeq ks(_T("Seq1"));
    Key keyA(_T("A"));
    ModifiedKey mkA(&keyA);
    ActionKey ak(mkA);
    
    ks.add(ak);
    
    EXPECT_EQ(ks.getActions().size(), 1);
    // verify content
    const Action* act = ks.getActions()[0].get();
    EXPECT_EQ(act->getType(), Action::Type_key);
}

TEST(KeySeqTest, CopyConstruction) {
    KeySeq ks1(_T("Seq1"));
    Key keyA(_T("A"));
    ModifiedKey mkA(&keyA);
    ActionKey ak(mkA);
    ks1.add(ak);
    
    KeySeq ks2(ks1); // Copy constructor
    EXPECT_EQ(ks2.getName(), _T("Seq1"));
    EXPECT_EQ(ks2.getActions().size(), 1);
    
    // Should be deep copy (different pointers)
    EXPECT_NE(ks2.getActions()[0].get(), ks1.getActions()[0].get()); 
    EXPECT_EQ(ks2.getActions()[0]->getType(), Action::Type_key);
}

TEST(KeymapTest, BasicConstruction) {
    Keymap km(Keymap::Type_keymap, _T("Global"), _T(""), _T(""), nullptr, nullptr);
    EXPECT_EQ(km.getName(), _T("Global"));
    EXPECT_EQ(km.getDefaultKeySeq(), nullptr);
    EXPECT_EQ(km.getParentKeymap(), nullptr);
}

TEST(KeymapTest, WindowMatching) {
    // WindowAnd: match both class and title
    // Use wide string literals matching the TCHAR definition in this project (likely unicode)
    Keymap km(Keymap::Type_windowAnd, _T("Notepad"), _T("Notepad"), _T(".*Untitled.*"), nullptr, nullptr);
    
    EXPECT_TRUE(km.doesSameWindow(_T("Notepad"), _T("Untitled - Notepad")));
    EXPECT_FALSE(km.doesSameWindow(_T("Explorer"), _T("Untitled - Notepad")));
    EXPECT_FALSE(km.doesSameWindow(_T("Notepad"), _T("Document.txt")));
}

TEST(KeymapTest, AddAssignment) {
    Keymap km(Keymap::Type_keymap, _T("Global"), _T(""), _T(""), nullptr, nullptr);
    
    // We need a Key to create ModifiedKey
    // Key constructor automatically adds a default scan code, so it's safe to use.
    Key keyA(_T("A")); 
    ModifiedKey mkA(&keyA);
    
    KeySeq *ks = new KeySeq(_T("ActionA"));
    
    km.addAssignment(mkA, ks);
    
    const Keymap::KeyAssignment *ka = km.searchAssignment(mkA);
    ASSERT_TRUE(ka != nullptr);
    EXPECT_EQ(ka->m_keySeq, ks);
    
    // Cleanup
    delete ks;
}
