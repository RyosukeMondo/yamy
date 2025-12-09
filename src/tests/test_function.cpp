#include <gtest/gtest.h>
#include "engine.h"
#include "function.h"
#include "function_data.h"
#include "../core/commands/cmd_default.h"
#include "../core/commands/cmd_keymap_prev_prefix.h"
#include "../core/commands/cmd_keymap.h"
#include "../core/commands/cmd_variable.h"
#include "../core/commands/cmd_wait.h"

TEST(FunctionDataTest, DefaultFunction) {
    std::unique_ptr<FunctionData> fd(Command_Default::create());
    EXPECT_EQ(fd->getName(), "Default");
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&Default"));
    
    std::unique_ptr<FunctionData> clone(fd->clone());
    EXPECT_EQ(clone->getName(), "Default");
}

TEST(FunctionDataTest, KeymapPrevPrefixFunction) {
    std::unique_ptr<FunctionData> fd(Command_KeymapPrevPrefix::create());
    // Modify args via casting to Command_KeymapPrevPrefix
    Command_KeymapPrevPrefix* cmd = static_cast<Command_KeymapPrevPrefix*>(fd.get());
    cmd->getArg<0>() = 5;

    EXPECT_EQ(fd->getName(), "KeymapPrevPrefix");
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&KeymapPrevPrefix(5) "));

    std::unique_ptr<FunctionData> clone(fd->clone());
    tstringstream ss2;
    clone->output(ss2);
    EXPECT_EQ(ss2.str(), _T("&KeymapPrevPrefix(5) "));
}

TEST(FunctionDataTest, VariableFunction) {
    std::unique_ptr<FunctionData> fd(Command_Variable::create());
    Command_Variable* vfd = static_cast<Command_Variable*>(fd.get());
    // Initial values (default is typically 0 or similar, but let's see constructor)
    // Command_Variable args: int m_mag, int m_inc
    
    // Set some values
    vfd->getArg<0>() = 10;
    vfd->getArg<1>() = 5;
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&Variable(10, 5) "));
    
    std::unique_ptr<FunctionData> clone(fd->clone());
    Command_Variable* vclone = static_cast<Command_Variable*>(clone.get());
    EXPECT_EQ(vclone->getArg<0>(), 10);
    EXPECT_EQ(vclone->getArg<1>(), 5);
}

TEST(FunctionDataTest, KeymapFunction) {
    // Keymap function takes a Keymap pointer.
    // We can pass nullptr for testing output format if it handles it,
    // but let's pass a dummy keymap to be safe.
    Keymap km(Keymap::Type_keymap, _T("TestMap"), _T(""), _T(""), nullptr, nullptr);
    
    std::unique_ptr<FunctionData> fd(Command_Keymap::create());
    Command_Keymap* kfd = static_cast<Command_Keymap*>(fd.get());
    kfd->getArg<0>() = &km;
    
    tstringstream ss;
    fd->output(ss);
    // output expects valid keymap pointer?
    // Keymap output operator prints name.
    EXPECT_EQ(ss.str(), _T("&Keymap(TestMap) "));
}

TEST(FunctionDataTest, WaitFunction) {
    std::unique_ptr<FunctionData> fd(Command_Wait::create());
    Command_Wait* wfd = static_cast<Command_Wait*>(fd.get());
    wfd->getArg<0>() = 100;
    
    tstringstream ss;
    fd->output(ss);
    EXPECT_EQ(ss.str(), _T("&Wait(100) "));
}
