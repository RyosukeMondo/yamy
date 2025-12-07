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
  { _T("LoadSetting"), FunctionData_LoadSetting::create },
  { _T("VK"), Command_VK::create },
  { _T("Wait"), Command_Wait::create },
  { _T("PostMessage"), Command_PostMessage::create },
  { _T("InvestigateCommand"), FunctionData_InvestigateCommand::create },
  { _T("MayuDialog"), FunctionData_MayuDialog::create },
  { _T("DescribeBindings"), FunctionData_DescribeBindings::create },
  { _T("HelpMessage"), FunctionData_HelpMessage::create },
  { _T("HelpVariable"), FunctionData_HelpVariable::create },
  { _T("WindowRaise"), FunctionData_WindowRaise::create },
  { _T("WindowLower"), FunctionData_WindowLower::create },
  { _T("WindowMinimize"), FunctionData_WindowMinimize::create },
  { _T("WindowMaximize"), FunctionData_WindowMaximize::create },
  { _T("WindowHMaximize"), FunctionData_WindowHMaximize::create },
  { _T("WindowVMaximize"), FunctionData_WindowVMaximize::create },
  { _T("WindowHVMaximize"), FunctionData_WindowHVMaximize::create },
  { _T("WindowMove"), FunctionData_WindowMove::create },
  { _T("WindowMoveTo"), FunctionData_WindowMoveTo::create },
  { _T("WindowMoveVisibly"), FunctionData_WindowMoveVisibly::create },
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
	return NULL;
}
