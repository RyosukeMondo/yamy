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
#include "../commands/cmd_window_monitor_to.h"
#include "../commands/cmd_window_monitor.h"
#include "../commands/cmd_window_cling_to_left.h"
#include "../commands/cmd_window_cling_to_right.h"
#include "../commands/cmd_window_cling_to_top.h"
#include "../commands/cmd_window_cling_to_bottom.h"
#include "../commands/cmd_window_close.h"
#include "../commands/cmd_window_toggle_top_most.h"
#include "../commands/cmd_window_identify.h"
#include "../commands/cmd_window_set_alpha.h"
#include "../commands/cmd_window_redraw.h"
#include "../commands/cmd_window_resize_to.h"
#include "../commands/cmd_mouse_move.h"
#include "../commands/cmd_mouse_wheel.h"
#include "../commands/cmd_clipboard_change_case.h"
#include "../commands/cmd_clipboard_upcase_word.h"
#include "../commands/cmd_clipboard_downcase_word.h"
#include "../commands/cmd_clipboard_copy.h"
#include "../commands/cmd_emacs_edit_kill_line_pred.h"
#include "../commands/cmd_emacs_edit_kill_line_func.h"
#include "../commands/cmd_log_clear.h"
#include "../commands/cmd_recenter.h"
#include "../commands/cmd_direct_sstp.h"
#include "../commands/cmd_plugin.h"
#include "../commands/cmd_set_ime_status.h"
#include "../commands/cmd_set_ime_string.h"
#include "../commands/cmd_mouse_hook.h"
#include "../commands/cmd_cancel_prefix.h"

class FunctionCreator
{
public:
    typedef FunctionData *(*Creator)();

public:
    const char *m_name;
    Creator m_creator;
};

FunctionData *createFunctionData(const std::string &i_name)
{
    static FunctionCreator functionCreators[] = {
  { "Default", Command_Default::create },
  { "KeymapParent", Command_KeymapParent::create },
  { "KeymapPrevPrefix", Command_KeymapPrevPrefix::create },
  { "KeymapWindow", Command_KeymapWindow::create },
  { "OtherWindowClass", Command_OtherWindowClass::create },
  { "Prefix", Command_Prefix::create },
  { "Keymap", Command_Keymap::create },
  { "Sync", Command_Sync::create },
  { "Toggle", Command_Toggle::create },
  { "EditNextModifier", Command_EditNextModifier::create },
  { "Variable", Command_Variable::create },
  { "Repeat", Command_Repeat::create },
  { "Undefined", Command_Undefined::create },
  { "Ignore", Command_Ignore::create },
  { "ShellExecute", Command_ShellExecute::create },
  { "SetForegroundWindow", Command_SetForegroundWindow::create },
  { "LoadSetting", Command_LoadSetting::create },
  { "VK", Command_VK::create },
  { "Wait", Command_Wait::create },
  { "PostMessage", Command_PostMessage::create },
  { "InvestigateCommand", Command_InvestigateCommand::create },
  { "MayuDialog", Command_MayuDialog::create },
  { "DescribeBindings", Command_DescribeBindings::create },
  { "HelpMessage", Command_HelpMessage::create },
  { "HelpVariable", Command_HelpVariable::create },
  { "WindowRaise", Command_WindowRaise::create },
  { "WindowLower", Command_WindowLower::create },
  { "WindowMinimize", Command_WindowMinimize::create },
  { "WindowMaximize", Command_WindowMaximize::create },
  { "WindowHMaximize", Command_WindowHMaximize::create },
  { "WindowVMaximize", Command_WindowVMaximize::create },
  { "WindowHVMaximize", Command_WindowHVMaximize::create },
  { "WindowMove", Command_WindowMove::create },
  { "WindowMoveTo", Command_WindowMoveTo::create },
  { "WindowMoveVisibly", Command_WindowMoveVisibly::create },
  { "WindowMonitorTo", Command_WindowMonitorTo::create },
  { "WindowMonitor", Command_WindowMonitor::create },
  { "WindowClingToLeft", Command_WindowClingToLeft::create },
  { "WindowClingToRight", Command_WindowClingToRight::create },
  { "WindowClingToTop", Command_WindowClingToTop::create },
  { "WindowClingToBottom", Command_WindowClingToBottom::create },
  { "WindowClose", Command_WindowClose::create },
  { "WindowToggleTopMost", Command_WindowToggleTopMost::create },
  { "WindowIdentify", Command_WindowIdentify::create },
  { "WindowSetAlpha", Command_WindowSetAlpha::create },
  { "WindowRedraw", Command_WindowRedraw::create },
  { "WindowResizeTo", Command_WindowResizeTo::create },
  { "MouseMove", Command_MouseMove::create },
  { "MouseWheel", Command_MouseWheel::create },
  { "ClipboardChangeCase", Command_ClipboardChangeCase::create },
  { "ClipboardUpcaseWord", Command_ClipboardUpcaseWord::create },
  { "ClipboardDowncaseWord", Command_ClipboardDowncaseWord::create },
  { "ClipboardCopy", Command_ClipboardCopy::create },
  { "EmacsEditKillLinePred", Command_EmacsEditKillLinePred::create },
  { "EmacsEditKillLineFunc", Command_EmacsEditKillLineFunc::create },
  { "LogClear", Command_LogClear::create },
  { "Recenter", Command_Recenter::create },
  { "DirectSSTP", Command_DirectSSTP::create },
  { "PlugIn", Command_PlugIn::create },
  { "SetImeStatus", Command_SetImeStatus::create },
  { "SetImeString", Command_SetImeString::create },
  { "MouseHook", Command_MouseHook::create },
  { "CancelPrefix", Command_CancelPrefix::create },
    };

    for (size_t i = 0; i != NUMBER_OF(functionCreators); ++ i)
        if (i_name == functionCreators[i].m_name)
            return functionCreators[i].m_creator();
    return nullptr;
}

