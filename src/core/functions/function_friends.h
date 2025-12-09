// #pragma once is INTENTIONALLY OMITTED.
// This file is included multiple times (in SettingLoader and Engine) to inject friend declarations.

// Forward declarations
// Forward declarations removed to avoid nested class creation inside Engine
// friend class X; is sufficient for global class X if Engine is global

// Friend declarations for FunctionData classes
friend class Command_Default;
friend class Command_KeymapPrevPrefix;
friend class Command_KeymapParent;
friend class Command_KeymapWindow;
friend class Command_OtherWindowClass;
friend class Command_Prefix;
friend class Command_Keymap;
friend class Command_Sync;
friend class Command_Toggle;
friend class Command_EditNextModifier;
friend class Command_Variable;
friend class Command_Repeat;
friend class Command_Undefined;
friend class Command_Ignore;
friend class Command_PostMessage;
friend class Command_Wait;
friend class Command_VK;
friend class Command_LoadSetting;
friend class Command_ShellExecute;
friend class Command_SetForegroundWindow;

friend class Command_InvestigateCommand;
friend class Command_MayuDialog;
friend class Command_DescribeBindings;
friend class Command_HelpMessage;
friend class Command_HelpVariable;
friend class Command_WindowRaise;
friend class Command_WindowLower;
friend class Command_WindowMinimize;
friend class Command_WindowMaximize;
friend class Command_WindowHMaximize;
friend class Command_WindowVMaximize;
friend class Command_WindowHVMaximize;
friend class Command_WindowMove;
friend class Command_WindowMoveTo;
friend class Command_WindowMoveVisibly;
friend class Command_WindowMonitorTo;
friend class Command_WindowMonitor;
friend class Command_WindowClingToLeft;
friend class Command_WindowClingToRight;
friend class Command_WindowClingToTop;
friend class Command_WindowClingToBottom;
friend class Command_WindowClose;
friend class Command_WindowToggleTopMost;
friend class Command_WindowIdentify;
friend class Command_WindowSetAlpha;
friend class Command_WindowRedraw;
friend class Command_WindowResizeTo;
friend class FunctionData_MouseMove;
friend class FunctionData_MouseWheel;
friend class FunctionData_ClipboardChangeCase;
friend class FunctionData_ClipboardUpcaseWord;
friend class FunctionData_ClipboardDowncaseWord;
friend class FunctionData_ClipboardCopy;
friend class FunctionData_EmacsEditKillLinePred;
friend class FunctionData_EmacsEditKillLineFunc;
friend class FunctionData_LogClear;
friend class FunctionData_Recenter;
friend class FunctionData_DirectSSTP;
friend class FunctionData_PlugIn;
friend class FunctionData_SetImeStatus;
friend class FunctionData_SetImeString;
friend class FunctionData_MouseHook;
friend class FunctionData_CancelPrefix;
