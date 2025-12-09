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

// FunctionData_WindowRaise is now Command_WindowRaise in commands/cmd_window_raise.h
// Replaced by generic Command template system.

// FunctionData_WindowLower is now Command_WindowLower in commands/cmd_window_lower.h
// Replaced by generic Command template system.

// FunctionData_WindowMinimize is now Command_WindowMinimize in commands/cmd_window_minimize.h
// Replaced by generic Command template system.

// FunctionData_WindowMaximize is now Command_WindowMaximize in commands/cmd_window_maximize.h
// Replaced by generic Command template system.

// FunctionData_WindowHMaximize is now Command_WindowHMaximize in commands/cmd_window_h_maximize.h
// Replaced by generic Command template system.

// FunctionData_WindowVMaximize is now Command_WindowVMaximize in commands/cmd_window_v_maximize.h
// Replaced by generic Command template system.

// FunctionData_WindowHVMaximize is now Command_WindowHVMaximize in commands/cmd_window_hv_maximize.h
// Replaced by generic Command template system.

// FunctionData_WindowMove is now Command_WindowMove in commands/cmd_window_move.h
// Replaced by generic Command template system.

// FunctionData_WindowMoveTo is now Command_WindowMoveTo in commands/cmd_window_move_to.h
// Replaced by generic Command template system.

// FunctionData_WindowMoveVisibly is now Command_WindowMoveVisibly in commands/cmd_window_move_visibly.h
// Replaced by generic Command template system.

// FunctionData_WindowMonitorTo is now Command_WindowMonitorTo in commands/cmd_window_monitor_to.h
// Replaced by generic Command template system.

// FunctionData_WindowMonitor is now Command_WindowMonitor in commands/cmd_window_monitor.h
// Replaced by generic Command template system.

// FunctionData_WindowClingToLeft is now Command_WindowClingToLeft in commands/cmd_window_cling_to_left.h
// Replaced by generic Command template system.

// FunctionData_WindowClingToRight is now Command_WindowClingToRight in commands/cmd_window_cling_to_right.h
// Replaced by generic Command template system.

// FunctionData_WindowClingToTop is now Command_WindowClingToTop in commands/cmd_window_cling_to_top.h
// Replaced by generic Command template system.

// FunctionData_WindowClingToBottom is now Command_WindowClingToBottom in commands/cmd_window_cling_to_bottom.h
// Replaced by generic Command template system.

// FunctionData_WindowClose is now Command_WindowClose in commands/cmd_window_close.h
// Replaced by generic Command template system.

// FunctionData_WindowToggleTopMost is now Command_WindowToggleTopMost in commands/cmd_window_toggle_top_most.h
// Replaced by generic Command template system.

// FunctionData_WindowIdentify is now Command_WindowIdentify in commands/cmd_window_identify.h
// Replaced by generic Command template system.

// FunctionData_WindowSetAlpha is now Command_WindowSetAlpha in commands/cmd_window_set_alpha.h
// Replaced by generic Command template system.

// FunctionData_WindowRedraw is now Command_WindowRedraw in commands/cmd_window_redraw.h
// Replaced by generic Command template system.

// FunctionData_WindowResizeTo is now Command_WindowResizeTo in commands/cmd_window_resize_to.h
// Replaced by generic Command template system.

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
