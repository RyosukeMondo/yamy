#include "function.h"
#include "function_data.h"
#include "misc.h" // for NUMBER_OF
#include "../commands/cmd_default.h"
#include "../commands/cmd_default.h"
#include "../commands/cmd_keymap_prev_prefix.h"
#include "../commands/cmd_keymap_parent.h"
#include "../commands/cmd_keymap_window.h"
#include "../commands/cmd_other_window_class.h"
#include "../commands/cmd_prefix.h"
#include "../commands/cmd_keymap.h"
#include "../commands/cmd_sync.h"
#include "../commands/cmd_toggle.h"
#include "../commands/cmd_edit_next_modifier.h"
#include "../commands/cmd_variable.h"
#include "../commands/cmd_repeat.h"
#include "../commands/cmd_undefined.h"
#include "../commands/cmd_ignore.h"
#include "../commands/cmd_post_message.h"
#include "../commands/cmd_wait.h"
#include "../commands/cmd_vk.h"
#include "../commands/cmd_load_setting.h"
#include "../commands/cmd_shell_execute.h"
#include "../commands/cmd_set_foreground_window.h"
#include "../commands/cmd_investigate_command.h"
#include "../commands/cmd_mayu_dialog.h"
#include "../commands/cmd_describe_bindings.h"
#include "../commands/cmd_help_message.h"
#include "../commands/cmd_help_variable.h"
#include "../commands/cmd_window_raise.h"
#include "../commands/cmd_window_lower.h"
#include "../commands/cmd_window_minimize.h"
#include "../commands/cmd_window_maximize.h"
#include "../commands/cmd_window_h_maximize.h"
#include "../commands/cmd_window_v_maximize.h"
#include "../commands/cmd_window_hv_maximize.h"
#include "../commands/cmd_window_move.h"
#include "../commands/cmd_window_move_to.h"
#include "../commands/cmd_window_move_visibly.h"

class FunctionCreator
{
public:
    typedef FunctionData *(*Creator)();

public:
    const _TCHAR *m_name;
    Creator m_creator;
};

FunctionData *createFunctionData(const tstring &i_name)
{
    static FunctionCreator functionCreators[] = {
  { _T("Default"), Command_Default::create },
  { _T("KeymapParent"), Command_KeymapParent::create },
  { _T("KeymapPrevPrefix"), Command_KeymapPrevPrefix::create },
  { _T("KeymapWindow"), Command_KeymapWindow::create },
  { _T("OtherWindowClass"), Command_OtherWindowClass::create },
  { _T("Prefix"), Command_Prefix::create },
  { _T("Keymap"), Command_Keymap::create },
  { _T("Sync"), Command_Sync::create },
  { _T("Toggle"), Command_Toggle::create },
  { _T("EditNextModifier"), Command_EditNextModifier::create },
  { _T("Variable"), Command_Variable::create },
  { _T("Repeat"), Command_Repeat::create },
  { _T("Undefined"), Command_Undefined::create },
  { _T("Ignore"), Command_Ignore::create },
  { _T("ShellExecute"), Command_ShellExecute::create },
  { _T("SetForegroundWindow"), Command_SetForegroundWindow::create },
  { _T("LoadSetting"), Command_LoadSetting::create },
  { _T("VK"), Command_VK::create },
  { _T("Wait"), Command_Wait::create },
  { _T("PostMessage"), Command_PostMessage::create },
  { _T("InvestigateCommand"), Command_InvestigateCommand::create },
  { _T("MayuDialog"), Command_MayuDialog::create },
  { _T("DescribeBindings"), Command_DescribeBindings::create },
  { _T("HelpMessage"), Command_HelpMessage::create },
  { _T("HelpVariable"), Command_HelpVariable::create },
  { _T("WindowRaise"), Command_WindowRaise::create },
  { _T("WindowLower"), Command_WindowLower::create },
  { _T("WindowMinimize"), Command_WindowMinimize::create },
  { _T("WindowMaximize"), Command_WindowMaximize::create },
  { _T("WindowHMaximize"), Command_WindowHMaximize::create },
  { _T("WindowVMaximize"), Command_WindowVMaximize::create },
  { _T("WindowHVMaximize"), Command_WindowHVMaximize::create },
  { _T("WindowMove"), Command_WindowMove::create },
  { _T("WindowMoveTo"), Command_WindowMoveTo::create },
  { _T("WindowMoveVisibly"), Command_WindowMoveVisibly::create },
  { _T("WindowMonitorTo"), FunctionData_WindowMonitorTo::create },
  { _T("WindowMonitor"), FunctionData_WindowMonitor::create },
  { _T("WindowClingToLeft"), FunctionData_WindowClingToLeft::create },
  { _T("WindowClingToRight"), FunctionData_WindowClingToRight::create },
  { _T("WindowClingToTop"), FunctionData_WindowClingToTop::create },
  { _T("WindowClingToBottom"), FunctionData_WindowClingToBottom::create },
  { _T("WindowClose"), FunctionData_WindowClose::create },
  { _T("WindowToggleTopMost"), FunctionData_WindowToggleTopMost::create },
  { _T("WindowIdentify"), FunctionData_WindowIdentify::create },
  { _T("WindowSetAlpha"), FunctionData_WindowSetAlpha::create },
  { _T("WindowRedraw"), FunctionData_WindowRedraw::create },
  { _T("WindowResizeTo"), FunctionData_WindowResizeTo::create },
  { _T("MouseMove"), FunctionData_MouseMove::create },
  { _T("MouseWheel"), FunctionData_MouseWheel::create },
  { _T("ClipboardChangeCase"), FunctionData_ClipboardChangeCase::create },
  { _T("ClipboardUpcaseWord"), FunctionData_ClipboardUpcaseWord::create },
  { _T("ClipboardDowncaseWord"), FunctionData_ClipboardDowncaseWord::create },
  { _T("ClipboardCopy"), FunctionData_ClipboardCopy::create },
  { _T("EmacsEditKillLinePred"), FunctionData_EmacsEditKillLinePred::create },
  { _T("EmacsEditKillLineFunc"), FunctionData_EmacsEditKillLineFunc::create },
  { _T("LogClear"), FunctionData_LogClear::create },
  { _T("Recenter"), FunctionData_Recenter::create },
  { _T("DirectSSTP"), FunctionData_DirectSSTP::create },
  { _T("PlugIn"), FunctionData_PlugIn::create },
  { _T("SetImeStatus"), FunctionData_SetImeStatus::create },
  { _T("SetImeString"), FunctionData_SetImeString::create },
  { _T("MouseHook"), FunctionData_MouseHook::create },
  { _T("CancelPrefix"), FunctionData_CancelPrefix::create },
    };

    for (size_t i = 0; i != NUMBER_OF(functionCreators); ++ i)
        if (i_name == functionCreators[i].m_name)
            return functionCreators[i].m_creator();
    return nullptr;
}
