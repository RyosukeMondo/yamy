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

// FunctionData_MouseMove is now Command_MouseMove in commands/cmd_mouse_move.h
// Replaced by generic Command template system.

// FunctionData_MouseWheel is now Command_MouseWheel in commands/cmd_mouse_wheel.h
// Replaced by generic Command template system.

// FunctionData_ClipboardChangeCase is now Command_ClipboardChangeCase in commands/cmd_clipboard_change_case.h
// Replaced by generic Command template system.

// FunctionData_ClipboardUpcaseWord is now Command_ClipboardUpcaseWord in commands/cmd_clipboard_upcase_word.h
// Replaced by generic Command template system.

// FunctionData_ClipboardDowncaseWord is now Command_ClipboardDowncaseWord in commands/cmd_clipboard_downcase_word.h
// Replaced by generic Command template system.

// FunctionData_ClipboardCopy is now Command_ClipboardCopy in commands/cmd_clipboard_copy.h
// Replaced by generic Command template system.

// FunctionData_EmacsEditKillLinePred is now Command_EmacsEditKillLinePred in commands/cmd_emacs_edit_kill_line_pred.h
// Replaced by generic Command template system.

// FunctionData_EmacsEditKillLineFunc is now Command_EmacsEditKillLineFunc in commands/cmd_emacs_edit_kill_line_func.h
// Replaced by generic Command template system.

// FunctionData_LogClear is now Command_LogClear in commands/cmd_log_clear.h
// Replaced by generic Command template system.

// FunctionData_Recenter is now Command_Recenter in commands/cmd_recenter.h
// Replaced by generic Command template system.

// FunctionData_DirectSSTP is now Command_DirectSSTP in commands/cmd_direct_sstp.h
// Replaced by generic Command template system.

// FunctionData_PlugIn is now Command_PlugIn in commands/cmd_plugin.h
// Replaced by generic Command template system.

// FunctionData_SetImeStatus is now Command_SetImeStatus in commands/cmd_set_ime_status.h
// Replaced by generic Command template system.

// FunctionData_SetImeString is now Command_SetImeString in commands/cmd_set_ime_string.h
// Replaced by generic Command template system.

// FunctionData_MouseHook is now Command_MouseHook in commands/cmd_mouse_hook.h
// Replaced by generic Command template system.

// FunctionData_CancelPrefix is now Command_CancelPrefix in commands/cmd_cancel_prefix.h
// Replaced by generic Command template system.

#endif // _FUNCTION_DATA_H
