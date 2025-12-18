#include "function.h"
#include "function_data.h"
#include "misc.h" // for NUMBER_OF
#include "../commands/cmd_default.h"
#include "../commands/cmd_default.h"
#include "../commands/cmd_keymap_prev_prefix.h"
#include "../commands/cmd_keymap_parent.h"
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
#include "../commands/cmd_mouse_move.h"
#include "../commands/cmd_mouse_wheel.h"
#include "../commands/cmd_log_clear.h"
#include "../commands/cmd_recenter.h"
#include "../commands/cmd_direct_sstp.h"
#include "../commands/cmd_plugin.h"
#include "../commands/cmd_set_ime_status.h"
#include "../commands/cmd_set_ime_string.h"
#include "../commands/cmd_mouse_hook.h"
#include "../commands/cmd_cancel_prefix.h"
#include "../commands/cmd_metrics.h"

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
  { "MouseMove", Command_MouseMove::create },
  { "MouseWheel", Command_MouseWheel::create },
  { "LogClear", Command_LogClear::create },
  { "Recenter", Command_Recenter::create },
  { "DirectSSTP", Command_DirectSSTP::create },
  { "PlugIn", Command_PlugIn::create },
  { "SetImeStatus", Command_SetImeStatus::create },
  { "SetImeString", Command_SetImeString::create },
  { "MouseHook", Command_MouseHook::create },
  { "CancelPrefix", Command_CancelPrefix::create },
  { "Metrics", Command_Metrics::create },
    };

    for (size_t i = 0; i != NUMBER_OF(functionCreators); ++ i)
        if (i_name == functionCreators[i].m_name)
            return functionCreators[i].m_creator();
    return nullptr;
}

