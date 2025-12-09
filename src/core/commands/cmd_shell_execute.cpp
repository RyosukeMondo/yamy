#include "cmd_shell_execute.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include <shellapi.h> // For ShellExecute and error constants
#ifndef NUMBER_OF
#define NUMBER_OF(array) (sizeof(array) / sizeof((array)[0]))
#endif

void Command_ShellExecute::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_operation);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_file);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_parameters);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_directory);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_showCommand);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_ShellExecute::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    i_engine->m_afShellExecute = i_param->m_af;
    i_engine->getWindowSystem()->postMessage(i_engine->m_hwndAssocWindow,
                WM_APP_engineNotify, EngineNotify_shellExecute, 0);
}

tostream &Command_ShellExecute::output(tostream &i_ost) const
{
    i_ost << _T("&") << to_tstring(getName()) << _T("(");
    i_ost << m_operation << _T(", ");
    i_ost << m_file << _T(", ");
    i_ost << m_parameters << _T(", ");
    i_ost << m_directory << _T(", ");
    i_ost << m_showCommand;
    i_ost << _T(") ");
    return i_ost;
}

void Command_ShellExecute::executeOnMainThread(Engine *i_engine)
{
    // Need to access Engine's private members, which is allowed via friend declaration
    Acquire a(&i_engine->m_cs);

    Command_ShellExecute *fd =
        reinterpret_cast<Command_ShellExecute *>(
            i_engine->m_afShellExecute->m_functionData);

    int r = i_engine->getWindowSystem()->shellExecute(
                fd->m_operation.eval().empty() ? _T("open") : fd->m_operation.eval(),
                fd->m_file.eval(),
                fd->m_parameters.eval(),
                fd->m_directory.eval(),
                static_cast<int>(fd->m_showCommand));
    if (32 < r)
        return; // success

    struct ErrorEntry {
        int m_type;
        const _TCHAR *m_name;
    };
    static const ErrorEntry errorTable[] = {
        { 0, _T("The operating system is out of memory or resources.") },
        { ERROR_FILE_NOT_FOUND, _T("The specified file was not found.") },
        { ERROR_PATH_NOT_FOUND, _T("The specified path was not found.") },
        { ERROR_BAD_FORMAT, _T("The .exe file is invalid ")
          _T("(non-Win32R .exe or error in .exe image).") },
        { SE_ERR_ACCESSDENIED,
          _T("The operating system denied access to the specified file.") },
        { SE_ERR_ASSOCINCOMPLETE,
          _T("The file name association is incomplete or invalid.") },
        { SE_ERR_DDEBUSY,
          _T("The DDE transaction could not be completed ")
          _T("because other DDE transactions were being processed. ") },
        { SE_ERR_DDEFAIL, _T("The DDE transaction failed.") },
        { SE_ERR_DDETIMEOUT, _T("The DDE transaction could not be completed ")
          _T("because the request timed out.") },
        { SE_ERR_DLLNOTFOUND,
          _T("The specified dynamic-link library was not found.") },
        { SE_ERR_FNF, _T("The specified file was not found.") },
        { SE_ERR_NOASSOC, _T("There is no application associated ")
          _T("with the given file name extension.") },
        { SE_ERR_OOM,
          _T("There was not enough memory to complete the operation.") },
        { SE_ERR_PNF, _T("The specified path was not found.") },
        { SE_ERR_SHARE, _T("A sharing violation occurred.") },
    };

    tstring errorMessage(_T("Unknown error."));

    for (size_t i = 0; i < NUMBER_OF(errorTable); ++ i)
        if (errorTable[i].m_type == r) {
            errorMessage = errorTable[i].m_name;
            break;
        }

    Acquire b(&i_engine->m_log, 0);
    i_engine->m_log << _T("error: ") << fd << _T(": ") << errorMessage << std::endl;
}
