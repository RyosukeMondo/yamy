#include "cmd_shell_execute.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#ifdef _WIN32
#include <shellapi.h> // For ShellExecute and error constants
#else
// Shell error constants for Linux compatibility
#define ERROR_FILE_NOT_FOUND    2
#define ERROR_PATH_NOT_FOUND    3
#define ERROR_BAD_FORMAT        11
#define SE_ERR_ACCESSDENIED     5
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDEBUSY          30
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DLLNOTFOUND      32
#define SE_ERR_FNF              2
#define SE_ERR_NOASSOC          31
#define SE_ERR_OOM              8
#define SE_ERR_PNF              3
#define SE_ERR_SHARE            26
#endif
#ifndef NUMBER_OF
#define NUMBER_OF(array) (sizeof(array) / sizeof((array)[0]))
#endif

void Command_ShellExecute::load(SettingLoader *i_sl)
{
    const char* tName = Name;

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

std::ostream &Command_ShellExecute::output(std::ostream &i_ost) const
{
    i_ost << "&" << getName() << "(";
    i_ost << m_operation << ", ";
    i_ost << m_file << ", ";
    i_ost << m_parameters << ", ";
    i_ost << m_directory << ", ";
    i_ost << m_showCommand;
    i_ost << ") ";
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
                fd->m_operation.eval().empty() ? "open" : fd->m_operation.eval(),
                fd->m_file.eval(),
                fd->m_parameters.eval(),
                fd->m_directory.eval(),
                static_cast<int>(fd->m_showCommand));
    if (32 < r)
        return; // success

    struct ErrorEntry {
        int m_type;
        const char *m_name;
    };
    static const ErrorEntry errorTable[] = {
        { 0, "The operating system is out of memory or resources." },
        { ERROR_FILE_NOT_FOUND, "The specified file was not found." },
        { ERROR_PATH_NOT_FOUND, "The specified path was not found." },
        { ERROR_BAD_FORMAT, "The .exe file is invalid "
          "(non-Win32R .exe or error in .exe image)." },
        { SE_ERR_ACCESSDENIED,
          "The operating system denied access to the specified file." },
        { SE_ERR_ASSOCINCOMPLETE,
          "The file name association is incomplete or invalid." },
        { SE_ERR_DDEBUSY,
          "The DDE transaction could not be completed "
          "because other DDE transactions were being processed. " },
        { SE_ERR_DDEFAIL, "The DDE transaction failed." },
        { SE_ERR_DDETIMEOUT, "The DDE transaction could not be completed "
          "because the request timed out." },
        { SE_ERR_DLLNOTFOUND,
          "The specified dynamic-link library was not found." },
        { SE_ERR_FNF, "The specified file was not found." },
        { SE_ERR_NOASSOC, "There is no application associated "
          "with the given file name extension." },
        { SE_ERR_OOM,
          "There was not enough memory to complete the operation." },
        { SE_ERR_PNF, "The specified path was not found." },
        { SE_ERR_SHARE, "A sharing violation occurred." },
    };

    std::string errorMessage("Unknown error.");

    for (size_t i = 0; i < NUMBER_OF(errorTable); ++ i)
        if (errorTable[i].m_type == r) {
            errorMessage = errorTable[i].m_name;
            break;
        }

    Acquire b(&i_engine->m_log, 0);
    i_engine->m_log << "error: " << fd << ": " << errorMessage << std::endl;
}
