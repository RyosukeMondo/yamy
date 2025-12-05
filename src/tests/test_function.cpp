#include <gtest/gtest.h>
#include "../core/engine.h"
#include "../core/function.h"
#include "../core/function_data.h"

TEST(FunctionDataTest, DefaultFunction) {
    std::unique_ptr<FunctionData> fd(FunctionData_Default::create());
    EXPECT_STREQ(fd->getName(), _T("Default"));
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&Default"));
    
    std::unique_ptr<FunctionData> clone(fd->clone());
    EXPECT_STREQ(clone->getName(), _T("Default"));
}

TEST(FunctionDataTest, VariableFunction) {
    std::unique_ptr<FunctionData> fd(FunctionData_Variable::create());
    FunctionData_Variable* vfd = static_cast<FunctionData_Variable*>(fd.get());
    // Initial values
    vfd->m_mag = 1;
    vfd->m_inc = 0;
    
    // Set some values
    vfd->m_mag = 10;
    vfd->m_inc = 5;
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&Variable(10, 5) "));
    
    std::unique_ptr<FunctionData> clone(fd->clone());
    FunctionData_Variable* vclone = static_cast<FunctionData_Variable*>(clone.get());
    EXPECT_EQ(vclone->m_mag, 10);
    EXPECT_EQ(vclone->m_inc, 5);
}

TEST(FunctionDataTest, KeymapFunction) {
    // Keymap function takes a Keymap pointer.
    // We can pass nullptr for testing output format if it handles it,
    // but let's pass a dummy keymap to be safe.
    Keymap km(Keymap::Type_keymap, _T("TestMap"), _T(""), _T(""), nullptr, nullptr);
    
    std::unique_ptr<FunctionData> fd(FunctionData_Keymap::create());
    FunctionData_Keymap* kfd = static_cast<FunctionData_Keymap*>(fd.get());
    kfd->m_keymap = &km;
    
    tstringstream ss;
    fd->output(ss);
    // output expects valid keymap pointer?
    // Keymap output operator prints name.
    EXPECT_EQ(ss.str(), _T("&Keymap(TestMap) "));
}

TEST(FunctionDataTest, WaitFunction) {
    std::unique_ptr<FunctionData> fd(FunctionData_Wait::create());
    FunctionData_Wait* wfd = static_cast<FunctionData_Wait*>(fd.get());
    wfd->m_milliSecond = 100;
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&Wait(100) "));
}
