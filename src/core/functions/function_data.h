#pragma once
#ifndef _FUNCTION_DATA_H
#define _FUNCTION_DATA_H

#include "utils/stringtool.h"
#include "function.h"
#include "engine.h"
#include "setting_loader.h"
#include "strexpr.h"
#include "utils/stringtool.h"


// FunctionData_Default is now Command_Default in commands/cmd_default.h
// Replaced by generic Command template system.

// FunctionData_KeymapParent is now Command_KeymapParent in commands/cmd_keymap_parent.h
// Replaced by generic Command template system.

// FunctionData_KeymapWindow is now Command_KeymapWindow in commands/cmd_keymap_window.h
// Replaced by generic Command template system.

// FunctionData_KeymapPrevPrefix is now Command_KeymapPrevPrefix in commands/cmd_keymap_prev_prefix.h
// Replaced by generic Command template system.

// FunctionData_OtherWindowClass is now Command_OtherWindowClass in commands/cmd_other_window_class.h
// Replaced by generic Command template system.

// FunctionData_Prefix is now Command_Prefix in commands/cmd_prefix.h
// Replaced by generic Command template system.

// FunctionData_Keymap is now Command_Keymap in commands/cmd_keymap.h
// Replaced by generic Command template system.

// FunctionData_Sync is now Command_Sync in commands/cmd_sync.h
// Replaced by generic Command template system.

// FunctionData_Toggle is now Command_Toggle in commands/cmd_toggle.h
// Replaced by generic Command template system.

// FunctionData_EditNextModifier is now Command_EditNextModifier in commands/cmd_edit_next_modifier.h
// Replaced by generic Command template system.

// FunctionData_Variable is now Command_Variable in commands/cmd_variable.h
// Replaced by generic Command template system.

// FunctionData_Repeat is now Command_Repeat in commands/cmd_repeat.h
// Replaced by generic Command template system.

// FunctionData_Undefined is now Command_Undefined in commands/cmd_undefined.h
// Replaced by generic Command template system.

// FunctionData_Ignore is now Command_Ignore in commands/cmd_ignore.h
// Replaced by generic Command template system.

// FunctionData_PostMessage is now Command_PostMessage in commands/cmd_post_message.h
// Replaced by generic Command template system.

// FunctionData_ShellExecute is now Command_ShellExecute in commands/cmd_shell_execute.h
// Replaced by generic Command template system.

// FunctionData_SetForegroundWindow is now Command_SetForegroundWindow in commands/cmd_set_foreground_window.h
// Replaced by generic Command template system.

// FunctionData_LoadSetting is now Command_LoadSetting in commands/cmd_load_setting.h
// Replaced by generic Command template system.

// FunctionData_VK is now Command_VK in commands/cmd_vk.h
// Replaced by generic Command template system.

// FunctionData_Wait is now Command_Wait in commands/cmd_wait.h
// Replaced by generic Command template system.

// FunctionData_InvestigateCommand is now Command_InvestigateCommand in commands/cmd_investigate_command.h
// Replaced by generic Command template system.

// FunctionData_MayuDialog is now Command_MayuDialog in commands/cmd_mayu_dialog.h
// Replaced by generic Command template system.

// FunctionData_DescribeBindings is now Command_DescribeBindings in commands/cmd_describe_bindings.h
// Replaced by generic Command template system.

// FunctionData_HelpMessage is now Command_HelpMessage in commands/cmd_help_message.h
// Replaced by generic Command template system.

// FunctionData_HelpVariable is now Command_HelpVariable in commands/cmd_help_variable.h
// Replaced by generic Command template system.

class FunctionData_WindowRaise : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowRaise *fd
      = new FunctionData_WindowRaise;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowRaise::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowRaise::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowRaise::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowRaise(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowRaise");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowRaise(*this);
  }
};

class FunctionData_WindowLower : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowLower *fd
      = new FunctionData_WindowLower;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowLower::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowLower::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowLower::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowLower(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowLower");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowLower(*this);
  }
};

class FunctionData_WindowMinimize : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMinimize *fd
      = new FunctionData_WindowMinimize;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowMinimize::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowMinimize::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowMinimize::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMinimize(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMinimize");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMinimize(*this);
  }
};

class FunctionData_WindowMaximize : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMaximize *fd
      = new FunctionData_WindowMaximize;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowMaximize::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowMaximize::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowMaximize::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMaximize(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMaximize");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMaximize(*this);
  }
};

class FunctionData_WindowHMaximize : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowHMaximize *fd
      = new FunctionData_WindowHMaximize;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowHMaximize::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowHMaximize::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowHMaximize::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowHMaximize(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowHMaximize");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowHMaximize(*this);
  }
};

class FunctionData_WindowVMaximize : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowVMaximize *fd
      = new FunctionData_WindowVMaximize;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowVMaximize::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowVMaximize::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowVMaximize::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowVMaximize(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowVMaximize");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowVMaximize(*this);
  }
};

class FunctionData_WindowHVMaximize : public FunctionData
{
public:
  BooleanType m_isHorizontal;
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowHVMaximize *fd
      = new FunctionData_WindowHVMaximize;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowHVMaximize::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_isHorizontal);
    if (i_sl->getCloseParen(false, FunctionData_WindowHVMaximize::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowHVMaximize::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowHVMaximize::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowHVMaximize(i_param, m_isHorizontal, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowHVMaximize");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_isHorizontal << _T(", ");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowHVMaximize(*this);
  }
};

class FunctionData_WindowMove : public FunctionData
{
public:
  int m_dx;
  int m_dy;
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMove *fd
      = new FunctionData_WindowMove;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowMove::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, FunctionData_WindowMove::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    if (i_sl->getCloseParen(false, FunctionData_WindowMove::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowMove::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowMove::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMove(i_param, m_dx, m_dy, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMove");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMove(*this);
  }
};

class FunctionData_WindowMoveTo : public FunctionData
{
public:
  GravityType m_gravityType;
  int m_dx;
  int m_dy;
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMoveTo *fd
      = new FunctionData_WindowMoveTo;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowMoveTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_gravityType);
    i_sl->getComma(false, FunctionData_WindowMoveTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, FunctionData_WindowMoveTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    if (i_sl->getCloseParen(false, FunctionData_WindowMoveTo::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowMoveTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowMoveTo::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMoveTo(i_param, m_gravityType, m_dx, m_dy, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMoveTo");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_gravityType << _T(", ");
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMoveTo(*this);
  }
};

class FunctionData_WindowMoveVisibly : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMoveVisibly *fd
      = new FunctionData_WindowMoveVisibly;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowMoveVisibly::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowMoveVisibly::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowMoveVisibly::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMoveVisibly(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMoveVisibly");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMoveVisibly(*this);
  }
};

class FunctionData_WindowMonitorTo : public FunctionData
{
public:
  WindowMonitorFromType m_fromType;
  int m_monitor;
  BooleanType m_adjustPos;
  BooleanType m_adjustSize;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMonitorTo *fd
      = new FunctionData_WindowMonitorTo;
    fd->m_adjustPos = BooleanType_true;
    fd->m_adjustSize = BooleanType_false;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowMonitorTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_fromType);
    i_sl->getComma(false, FunctionData_WindowMonitorTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_monitor);
    if (i_sl->getCloseParen(false, FunctionData_WindowMonitorTo::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowMonitorTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_adjustPos);
    if (i_sl->getCloseParen(false, FunctionData_WindowMonitorTo::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowMonitorTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_adjustSize);
    i_sl->getCloseParen(true, FunctionData_WindowMonitorTo::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMonitorTo(i_param, m_fromType, m_monitor, m_adjustPos, m_adjustSize);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMonitorTo");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_fromType << _T(", ");
    i_ost << m_monitor << _T(", ");
    i_ost << m_adjustPos << _T(", ");
    i_ost << m_adjustSize;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMonitorTo(*this);
  }
};

class FunctionData_WindowMonitor : public FunctionData
{
public:
  int m_monitor;
  BooleanType m_adjustPos;
  BooleanType m_adjustSize;

public:
  static FunctionData *create()
  {
    FunctionData_WindowMonitor *fd
      = new FunctionData_WindowMonitor;
    fd->m_adjustPos = BooleanType_true;
    fd->m_adjustSize = BooleanType_false;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowMonitor::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_monitor);
    if (i_sl->getCloseParen(false, FunctionData_WindowMonitor::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowMonitor::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_adjustPos);
    if (i_sl->getCloseParen(false, FunctionData_WindowMonitor::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowMonitor::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_adjustSize);
    i_sl->getCloseParen(true, FunctionData_WindowMonitor::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowMonitor(i_param, m_monitor, m_adjustPos, m_adjustSize);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowMonitor");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_monitor << _T(", ");
    i_ost << m_adjustPos << _T(", ");
    i_ost << m_adjustSize;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowMonitor(*this);
  }
};

class FunctionData_WindowClingToLeft : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowClingToLeft *fd
      = new FunctionData_WindowClingToLeft;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowClingToLeft::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowClingToLeft::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowClingToLeft::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowClingToLeft(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowClingToLeft");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowClingToLeft(*this);
  }
};

class FunctionData_WindowClingToRight : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowClingToRight *fd
      = new FunctionData_WindowClingToRight;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowClingToRight::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowClingToRight::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowClingToRight::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowClingToRight(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowClingToRight");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowClingToRight(*this);
  }
};

class FunctionData_WindowClingToTop : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowClingToTop *fd
      = new FunctionData_WindowClingToTop;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowClingToTop::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowClingToTop::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowClingToTop::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowClingToTop(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowClingToTop");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowClingToTop(*this);
  }
};

class FunctionData_WindowClingToBottom : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowClingToBottom *fd
      = new FunctionData_WindowClingToBottom;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowClingToBottom::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowClingToBottom::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowClingToBottom::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowClingToBottom(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowClingToBottom");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowClingToBottom(*this);
  }
};

class FunctionData_WindowClose : public FunctionData
{
public:
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowClose *fd
      = new FunctionData_WindowClose;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowClose::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_WindowClose::getName()))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowClose::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowClose(i_param, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowClose");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowClose(*this);
  }
};

class FunctionData_WindowToggleTopMost : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_WindowToggleTopMost *fd
      = new FunctionData_WindowToggleTopMost;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowToggleTopMost::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_WindowToggleTopMost::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowToggleTopMost(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowToggleTopMost");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowToggleTopMost(*this);
  }
};

class FunctionData_WindowIdentify : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_WindowIdentify *fd
      = new FunctionData_WindowIdentify;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowIdentify::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_WindowIdentify::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowIdentify(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowIdentify");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowIdentify(*this);
  }
};

class FunctionData_WindowSetAlpha : public FunctionData
{
public:
  int m_alpha;

public:
  static FunctionData *create()
  {
    FunctionData_WindowSetAlpha *fd
      = new FunctionData_WindowSetAlpha;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowSetAlpha::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_alpha);
    i_sl->getCloseParen(true, FunctionData_WindowSetAlpha::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowSetAlpha(i_param, m_alpha);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowSetAlpha");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_alpha;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowSetAlpha(*this);
  }
};

class FunctionData_WindowRedraw : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_WindowRedraw *fd
      = new FunctionData_WindowRedraw;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_WindowRedraw::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_WindowRedraw::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowRedraw(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowRedraw");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowRedraw(*this);
  }
};

class FunctionData_WindowResizeTo : public FunctionData
{
public:
  int m_width;
  int m_height;
  TargetWindowType m_twt;

public:
  static FunctionData *create()
  {
    FunctionData_WindowResizeTo *fd
      = new FunctionData_WindowResizeTo;
    fd->m_twt = TargetWindowType_overlapped;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_WindowResizeTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_width);
    i_sl->getComma(false, FunctionData_WindowResizeTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_height);
    if (i_sl->getCloseParen(false, FunctionData_WindowResizeTo::getName()))
      return;
    i_sl->getComma(false, FunctionData_WindowResizeTo::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, FunctionData_WindowResizeTo::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWindowResizeTo(i_param, m_width, m_height, m_twt);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("WindowResizeTo");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_width << _T(", ");
    i_ost << m_height << _T(", ");
    i_ost << m_twt;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_WindowResizeTo(*this);
  }
};

class FunctionData_MouseMove : public FunctionData
{
public:
  int m_dx;
  int m_dy;

public:
  static FunctionData *create()
  {
    FunctionData_MouseMove *fd
      = new FunctionData_MouseMove;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_MouseMove::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, FunctionData_MouseMove::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    i_sl->getCloseParen(true, FunctionData_MouseMove::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcMouseMove(i_param, m_dx, m_dy);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("MouseMove");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_dx << _T(", ");
    i_ost << m_dy;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_MouseMove(*this);
  }
};

class FunctionData_MouseWheel : public FunctionData
{
public:
  int m_delta;

public:
  static FunctionData *create()
  {
    FunctionData_MouseWheel *fd
      = new FunctionData_MouseWheel;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_MouseWheel::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_delta);
    i_sl->getCloseParen(true, FunctionData_MouseWheel::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcMouseWheel(i_param, m_delta);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("MouseWheel");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_delta;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_MouseWheel(*this);
  }
};

class FunctionData_ClipboardChangeCase : public FunctionData
{
public:
  BooleanType m_doesConvertToUpperCase;

public:
  static FunctionData *create()
  {
    FunctionData_ClipboardChangeCase *fd
      = new FunctionData_ClipboardChangeCase;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_ClipboardChangeCase::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_doesConvertToUpperCase);
    i_sl->getCloseParen(true, FunctionData_ClipboardChangeCase::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcClipboardChangeCase(i_param, m_doesConvertToUpperCase);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("ClipboardChangeCase");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_doesConvertToUpperCase;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_ClipboardChangeCase(*this);
  }
};

class FunctionData_ClipboardUpcaseWord : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_ClipboardUpcaseWord *fd
      = new FunctionData_ClipboardUpcaseWord;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_ClipboardUpcaseWord::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_ClipboardUpcaseWord::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcClipboardUpcaseWord(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("ClipboardUpcaseWord");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_ClipboardUpcaseWord(*this);
  }
};

class FunctionData_ClipboardDowncaseWord : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_ClipboardDowncaseWord *fd
      = new FunctionData_ClipboardDowncaseWord;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_ClipboardDowncaseWord::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_ClipboardDowncaseWord::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcClipboardDowncaseWord(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("ClipboardDowncaseWord");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_ClipboardDowncaseWord(*this);
  }
};

class FunctionData_ClipboardCopy : public FunctionData
{
public:
  StrExprArg m_text;

public:
  static FunctionData *create()
  {
    FunctionData_ClipboardCopy *fd
      = new FunctionData_ClipboardCopy;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_ClipboardCopy::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_text);
    i_sl->getCloseParen(true, FunctionData_ClipboardCopy::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcClipboardCopy(i_param, m_text);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("ClipboardCopy");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_text;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_ClipboardCopy(*this);
  }
};

class FunctionData_EmacsEditKillLinePred : public FunctionData
{
public:
  const KeySeq * m_keySeq1;
  const KeySeq * m_keySeq2;

public:
  static FunctionData *create()
  {
    FunctionData_EmacsEditKillLinePred *fd
      = new FunctionData_EmacsEditKillLinePred;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_EmacsEditKillLinePred::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_keySeq1);
    i_sl->getComma(false, FunctionData_EmacsEditKillLinePred::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_keySeq2);
    i_sl->getCloseParen(true, FunctionData_EmacsEditKillLinePred::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcEmacsEditKillLinePred(i_param, m_keySeq1, m_keySeq2);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("EmacsEditKillLinePred");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_keySeq1 << _T(", ");
    i_ost << m_keySeq2;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_EmacsEditKillLinePred(*this);
  }
};

class FunctionData_EmacsEditKillLineFunc : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_EmacsEditKillLineFunc *fd
      = new FunctionData_EmacsEditKillLineFunc;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_EmacsEditKillLineFunc::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_EmacsEditKillLineFunc::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcEmacsEditKillLineFunc(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("EmacsEditKillLineFunc");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_EmacsEditKillLineFunc(*this);
  }
};

class FunctionData_LogClear : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_LogClear *fd
      = new FunctionData_LogClear;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_LogClear::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_LogClear::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcLogClear(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("LogClear");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_LogClear(*this);
  }
};

class FunctionData_Recenter : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_Recenter *fd
      = new FunctionData_Recenter;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_Recenter::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_Recenter::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcRecenter(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Recenter");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Recenter(*this);
  }
};

class FunctionData_DirectSSTP : public FunctionData
{
public:
  tregex m_name;
  StrExprArg m_protocol;
  std::list<tstringq> m_headers;

public:
  static FunctionData *create()
  {
    FunctionData_DirectSSTP *fd
      = new FunctionData_DirectSSTP;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_DirectSSTP::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_name);
    i_sl->getComma(false, FunctionData_DirectSSTP::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_protocol);
    i_sl->getComma(false, FunctionData_DirectSSTP::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_headers);
    i_sl->getCloseParen(true, FunctionData_DirectSSTP::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcDirectSSTP(i_param, m_name, m_protocol, m_headers);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("DirectSSTP");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_name << _T(", ");
    i_ost << m_protocol << _T(", ");
    i_ost << m_headers;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_DirectSSTP(*this);
  }
};

class FunctionData_PlugIn : public FunctionData
{
public:
  StrExprArg m_dllName;
  StrExprArg m_funcName;
  StrExprArg m_funcParam;
  BooleanType m_doesCreateThread;

public:
  static FunctionData *create()
  {
    FunctionData_PlugIn *fd
      = new FunctionData_PlugIn;
    fd->m_funcName = StrExprArg();
    fd->m_funcParam = StrExprArg();
    fd->m_doesCreateThread = BooleanType_false;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_PlugIn::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dllName);
    if (i_sl->getCloseParen(false, FunctionData_PlugIn::getName()))
      return;
    i_sl->getComma(false, FunctionData_PlugIn::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_funcName);
    if (i_sl->getCloseParen(false, FunctionData_PlugIn::getName()))
      return;
    i_sl->getComma(false, FunctionData_PlugIn::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_funcParam);
    if (i_sl->getCloseParen(false, FunctionData_PlugIn::getName()))
      return;
    i_sl->getComma(false, FunctionData_PlugIn::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_doesCreateThread);
    i_sl->getCloseParen(true, FunctionData_PlugIn::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcPlugIn(i_param, m_dllName, m_funcName, m_funcParam, m_doesCreateThread);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("PlugIn");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_dllName << _T(", ");
    i_ost << m_funcName << _T(", ");
    i_ost << m_funcParam << _T(", ");
    i_ost << m_doesCreateThread;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_PlugIn(*this);
  }
};

class FunctionData_SetImeStatus : public FunctionData
{
public:
  ToggleType m_toggle;

public:
  static FunctionData *create()
  {
    FunctionData_SetImeStatus *fd
      = new FunctionData_SetImeStatus;
    fd->m_toggle = ToggleType_toggle;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_SetImeStatus::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_SetImeStatus::getName()))
      return;
    i_sl->load_ARGUMENT(&m_toggle);
    i_sl->getCloseParen(true, FunctionData_SetImeStatus::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcSetImeStatus(i_param, m_toggle);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("SetImeStatus");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_toggle;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_SetImeStatus(*this);
  }
};

class FunctionData_SetImeString : public FunctionData
{
public:
  StrExprArg m_data;

public:
  static FunctionData *create()
  {
    FunctionData_SetImeString *fd
      = new FunctionData_SetImeString;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_SetImeString::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_data);
    i_sl->getCloseParen(true, FunctionData_SetImeString::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcSetImeString(i_param, m_data);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("SetImeString");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_data;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_SetImeString(*this);
  }
};

class FunctionData_MouseHook : public FunctionData
{
public:
  MouseHookType m_hookType;
  int m_hookParam;

public:
  static FunctionData *create()
  {
    FunctionData_MouseHook *fd
      = new FunctionData_MouseHook;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_MouseHook::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_hookType);
    i_sl->getComma(false, FunctionData_MouseHook::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_hookParam);
    i_sl->getCloseParen(true, FunctionData_MouseHook::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcMouseHook(i_param, m_hookType, m_hookParam);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("MouseHook");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_hookType << _T(", ");
    i_ost << m_hookParam;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_MouseHook(*this);
  }
};

class FunctionData_CancelPrefix : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_CancelPrefix *fd
      = new FunctionData_CancelPrefix;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_CancelPrefix::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_CancelPrefix::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcCancelPrefix(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("CancelPrefix");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_CancelPrefix(*this);
  }
};

#endif // _FUNCTION_DATA_H
