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

class FunctionData_Prefix : public FunctionData
{
public:
  const Keymap * m_keymap;
  BooleanType m_doesIgnoreModifiers;

public:
  static FunctionData *create()
  {
    FunctionData_Prefix *fd
      = new FunctionData_Prefix;
    fd->m_doesIgnoreModifiers = BooleanType_true;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_Prefix::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_keymap);
    if (i_sl->getCloseParen(false, FunctionData_Prefix::getName()))
      return;
    i_sl->getComma(false, FunctionData_Prefix::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_doesIgnoreModifiers);
    i_sl->getCloseParen(true, FunctionData_Prefix::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcPrefix(i_param, m_keymap, m_doesIgnoreModifiers);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Prefix");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_keymap << _T(", ");
    i_ost << m_doesIgnoreModifiers;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Prefix(*this);
  }
};

class FunctionData_Keymap : public FunctionData
{
public:
  const Keymap * m_keymap;

public:
  static FunctionData *create()
  {
    FunctionData_Keymap *fd
      = new FunctionData_Keymap;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_Keymap::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_keymap);
    i_sl->getCloseParen(true, FunctionData_Keymap::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcKeymap(i_param, m_keymap);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Keymap");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_keymap;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Keymap(*this);
  }
};

class FunctionData_Sync : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_Sync *fd
      = new FunctionData_Sync;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_Sync::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_Sync::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcSync(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Sync");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Sync(*this);
  }
};

class FunctionData_Toggle : public FunctionData
{
public:
  ModifierLockType m_lock;
  ToggleType m_toggle;

public:
  static FunctionData *create()
  {
    FunctionData_Toggle *fd
      = new FunctionData_Toggle;
    fd->m_toggle = ToggleType_toggle;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_Toggle::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_lock);
    if (i_sl->getCloseParen(false, FunctionData_Toggle::getName()))
      return;
    i_sl->getComma(false, FunctionData_Toggle::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_toggle);
    i_sl->getCloseParen(true, FunctionData_Toggle::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcToggle(i_param, m_lock, m_toggle);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Toggle");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_lock << _T(", ");
    i_ost << m_toggle;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Toggle(*this);
  }
};

class FunctionData_EditNextModifier : public FunctionData
{
public:
  Modifier m_modifier;

public:
  static FunctionData *create()
  {
    FunctionData_EditNextModifier *fd
      = new FunctionData_EditNextModifier;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_EditNextModifier::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_modifier);
    i_sl->getCloseParen(true, FunctionData_EditNextModifier::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcEditNextModifier(i_param, m_modifier);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("EditNextModifier");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_modifier;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_EditNextModifier(*this);
  }
};

class FunctionData_Variable : public FunctionData
{
public:
  int m_mag;
  int m_inc;

public:
  static FunctionData *create()
  {
    FunctionData_Variable *fd
      = new FunctionData_Variable;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_Variable::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_mag);
    i_sl->getComma(false, FunctionData_Variable::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_inc);
    i_sl->getCloseParen(true, FunctionData_Variable::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcVariable(i_param, m_mag, m_inc);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Variable");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_mag << _T(", ");
    i_ost << m_inc;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Variable(*this);
  }
};

class FunctionData_Repeat : public FunctionData
{
public:
  const KeySeq * m_keySeq;
  int m_max;

public:
  static FunctionData *create()
  {
    FunctionData_Repeat *fd
      = new FunctionData_Repeat;
    fd->m_max = 10;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_Repeat::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_keySeq);
    if (i_sl->getCloseParen(false, FunctionData_Repeat::getName()))
      return;
    i_sl->getComma(false, FunctionData_Repeat::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_max);
    i_sl->getCloseParen(true, FunctionData_Repeat::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcRepeat(i_param, m_keySeq, m_max);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Repeat");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_keySeq << _T(", ");
    i_ost << m_max;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Repeat(*this);
  }
};

class FunctionData_Undefined : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_Undefined *fd
      = new FunctionData_Undefined;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_Undefined::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_Undefined::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcUndefined(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Undefined");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Undefined(*this);
  }
};

class FunctionData_Ignore : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_Ignore *fd
      = new FunctionData_Ignore;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_Ignore::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_Ignore::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcIgnore(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Ignore");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Ignore(*this);
  }
};

class FunctionData_PostMessage : public FunctionData
{
public:
  ToWindowType m_window;
  UINT m_message;
  WPARAM m_wParam;
  LPARAM m_lParam;

public:
  static FunctionData *create()
  {
    FunctionData_PostMessage *fd
      = new FunctionData_PostMessage;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_PostMessage::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_window);
    i_sl->getComma(false, FunctionData_PostMessage::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_message);
    i_sl->getComma(false, FunctionData_PostMessage::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_wParam);
    i_sl->getComma(false, FunctionData_PostMessage::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_lParam);
    i_sl->getCloseParen(true, FunctionData_PostMessage::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcPostMessage(i_param, m_window, m_message, m_wParam, m_lParam);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("PostMessage");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_window << _T(", ");
    i_ost << m_message << _T(", ");
    i_ost << m_wParam << _T(", ");
    i_ost << m_lParam;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_PostMessage(*this);
  }
};

class FunctionData_ShellExecute : public FunctionData
{
public:
  StrExprArg m_operation;
  StrExprArg m_file;
  StrExprArg m_parameters;
  StrExprArg m_directory;
  ShowCommandType m_showCommand;

public:
  static FunctionData *create()
  {
    FunctionData_ShellExecute *fd
      = new FunctionData_ShellExecute;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_ShellExecute::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_operation);
    i_sl->getComma(false, FunctionData_ShellExecute::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_file);
    i_sl->getComma(false, FunctionData_ShellExecute::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_parameters);
    i_sl->getComma(false, FunctionData_ShellExecute::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_directory);
    i_sl->getComma(false, FunctionData_ShellExecute::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_showCommand);
    i_sl->getCloseParen(true, FunctionData_ShellExecute::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcShellExecute(i_param, m_operation, m_file, m_parameters, m_directory, m_showCommand);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("ShellExecute");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_operation << _T(", ");
    i_ost << m_file << _T(", ");
    i_ost << m_parameters << _T(", ");
    i_ost << m_directory << _T(", ");
    i_ost << m_showCommand;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_ShellExecute(*this);
  }
};

class FunctionData_SetForegroundWindow : public FunctionData
{
public:
  tregex m_windowClassName;
  LogicalOperatorType m_logicalOp;
  tregex m_windowTitleName;

public:
  static FunctionData *create()
  {
    FunctionData_SetForegroundWindow *fd
      = new FunctionData_SetForegroundWindow;
    fd->m_logicalOp = LogicalOperatorType_and;
    fd->m_windowTitleName = tregex(_T(".*"));
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_SetForegroundWindow::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_windowClassName);
    if (i_sl->getCloseParen(false, FunctionData_SetForegroundWindow::getName()))
      return;
    i_sl->getComma(false, FunctionData_SetForegroundWindow::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_logicalOp);
    if (i_sl->getCloseParen(false, FunctionData_SetForegroundWindow::getName()))
      return;
    i_sl->getComma(false, FunctionData_SetForegroundWindow::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_windowTitleName);
    i_sl->getCloseParen(true, FunctionData_SetForegroundWindow::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcSetForegroundWindow(i_param, m_windowClassName, m_logicalOp, m_windowTitleName);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("SetForegroundWindow");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_windowClassName << _T(", ");
    i_ost << m_logicalOp << _T(", ");
    i_ost << m_windowTitleName;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_SetForegroundWindow(*this);
  }
};

class FunctionData_LoadSetting : public FunctionData
{
public:
  StrExprArg m_name;

public:
  static FunctionData *create()
  {
    FunctionData_LoadSetting *fd
      = new FunctionData_LoadSetting;
    fd->m_name = StrExprArg();
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_LoadSetting::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_LoadSetting::getName()))
      return;
    i_sl->load_ARGUMENT(&m_name);
    i_sl->getCloseParen(true, FunctionData_LoadSetting::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcLoadSetting(i_param, m_name);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("LoadSetting");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_name;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_LoadSetting(*this);
  }
};

class FunctionData_VK : public FunctionData
{
public:
  VKey m_vkey;

public:
  static FunctionData *create()
  {
    FunctionData_VK *fd
      = new FunctionData_VK;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_VK::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_vkey);
    i_sl->getCloseParen(true, FunctionData_VK::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcVK(i_param, m_vkey);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("VK");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_vkey;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_VK(*this);
  }
};

class FunctionData_Wait : public FunctionData
{
public:
  int m_milliSecond;

public:
  static FunctionData *create()
  {
    FunctionData_Wait *fd
      = new FunctionData_Wait;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_Wait::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_milliSecond);
    i_sl->getCloseParen(true, FunctionData_Wait::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcWait(i_param, m_milliSecond);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("Wait");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_milliSecond;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_Wait(*this);
  }
};

class FunctionData_InvestigateCommand : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_InvestigateCommand *fd
      = new FunctionData_InvestigateCommand;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_InvestigateCommand::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_InvestigateCommand::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcInvestigateCommand(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("InvestigateCommand");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_InvestigateCommand(*this);
  }
};

class FunctionData_MayuDialog : public FunctionData
{
public:
  MayuDialogType m_dialog;
  ShowCommandType m_showCommand;

public:
  static FunctionData *create()
  {
    FunctionData_MayuDialog *fd
      = new FunctionData_MayuDialog;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_MayuDialog::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_dialog);
    i_sl->getComma(false, FunctionData_MayuDialog::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_showCommand);
    i_sl->getCloseParen(true, FunctionData_MayuDialog::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcMayuDialog(i_param, m_dialog, m_showCommand);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("MayuDialog");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_dialog << _T(", ");
    i_ost << m_showCommand;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_MayuDialog(*this);
  }
};

class FunctionData_DescribeBindings : public FunctionData
{
public:

public:
  static FunctionData *create()
  {
    FunctionData_DescribeBindings *fd
      = new FunctionData_DescribeBindings;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_DescribeBindings::getName()))
      return;
    i_sl->getCloseParen(true, FunctionData_DescribeBindings::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcDescribeBindings(i_param);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("DescribeBindings");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_DescribeBindings(*this);
  }
};

class FunctionData_HelpMessage : public FunctionData
{
public:
  StrExprArg m_title;
  StrExprArg m_message;

public:
  static FunctionData *create()
  {
    FunctionData_HelpMessage *fd
      = new FunctionData_HelpMessage;
    fd->m_title = StrExprArg();
    fd->m_message = StrExprArg();
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    if (!i_sl->getOpenParen(false, FunctionData_HelpMessage::getName()))
      return;
    if (i_sl->getCloseParen(false, FunctionData_HelpMessage::getName()))
      return;
    i_sl->load_ARGUMENT(&m_title);
    if (i_sl->getCloseParen(false, FunctionData_HelpMessage::getName()))
      return;
    i_sl->getComma(false, FunctionData_HelpMessage::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_message);
    i_sl->getCloseParen(true, FunctionData_HelpMessage::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcHelpMessage(i_param, m_title, m_message);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("HelpMessage");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_title << _T(", ");
    i_ost << m_message;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_HelpMessage(*this);
  }
};

class FunctionData_HelpVariable : public FunctionData
{
public:
  StrExprArg m_title;

public:
  static FunctionData *create()
  {
    FunctionData_HelpVariable *fd
      = new FunctionData_HelpVariable;
    return fd;
  }
  
  virtual void load(SettingLoader *i_sl)
  {
    i_sl->getOpenParen(true, FunctionData_HelpVariable::getName()); // throw ...
    i_sl->load_ARGUMENT(&m_title);
    i_sl->getCloseParen(true, FunctionData_HelpVariable::getName()); // throw ...
  }

  virtual void exec(Engine *i_engine, FunctionParam *i_param) const
  {
    i_engine->funcHelpVariable(i_param, m_title);
  }

  inline virtual const _TCHAR *getName() const
  {
    return _T("HelpVariable");
  }

  virtual tostream &output(tostream &i_ost) const
  {
    i_ost << _T("&") << getName();
    i_ost << _T("(");
    i_ost << m_title;
    i_ost << _T(") ");
    return i_ost;
  }

  virtual FunctionData *clone() const
  {
    return new FunctionData_HelpVariable(*this);
  }
};

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
