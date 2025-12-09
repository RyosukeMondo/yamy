#include "cmd_plugin.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include <process.h>

namespace shu
{
class PlugIn
{
    enum Type {
        Type_A,
        Type_W
    };

private:
    WindowSystem* m_ws;
    void* m_dll;
    void* m_func;
    Type m_type;
    tstringq m_funcParam;

public:
    PlugIn(WindowSystem* ws) : m_ws(ws), m_dll(nullptr), m_func(nullptr) {
    }

    ~PlugIn() {
        if (m_dll)
            m_ws->freeLibrary(m_dll);
    }

    bool load(const tstringq &i_dllName, const tstringq &i_funcName,
              const tstringq &i_funcParam, tomsgstream &i_log) {
        m_dll = m_ws->loadLibrary((_T("Plugins\\") + i_dllName).c_str());
        if (!m_dll) {
            m_dll = m_ws->loadLibrary((_T("Plugin\\") + i_dllName).c_str());
            if (!m_dll) {
                m_dll = m_ws->loadLibrary(i_dllName.c_str());
                if (!m_dll) {
                    Acquire a(&i_log);
                    i_log << std::endl;
                    i_log << _T("error: &PlugIn() failed to load ") << i_dllName << std::endl;
                    return false;
                }
            }
        }

        // get function
#ifdef UNICODE
#  define to_wstring
#else
#  define to_string
#endif
        m_type = Type_W;
        m_func = m_ws->getProcAddress(m_dll, to_string(_T("mayu") + i_funcName + _T("W")));
        if (!m_func) {
            m_type = Type_A;
            m_func
            = m_ws->getProcAddress(m_dll, to_string(_T("mayu") + i_funcName + _T("A")));
            if (!m_func) {
                m_func = m_ws->getProcAddress(m_dll, to_string(_T("mayu") + i_funcName));
                if (!m_func) {
                    m_func = m_ws->getProcAddress(m_dll, to_string(i_funcName));
                    if (!m_func) {
                        Acquire a(&i_log);
                        i_log << std::endl;
                        i_log << _T("error: &PlugIn() failed to find function: ")
                        << i_funcName << std::endl;
                        return false;
                    }
                }
            }
        }

        m_funcParam = i_funcParam;
        return true;
    }

    void exec() {
        if (!m_dll || !m_func) return;

        typedef void (WINAPI * PLUGIN_FUNCTION_A)(const char *i_arg);
        typedef void (WINAPI * PLUGIN_FUNCTION_W)(const wchar_t *i_arg);
        switch (m_type) {
        case Type_A:
            reinterpret_cast<PLUGIN_FUNCTION_A>(m_func)(to_string(m_funcParam).c_str());
            break;
        case Type_W:
            reinterpret_cast<PLUGIN_FUNCTION_W>(m_func)(to_wstring(m_funcParam).c_str());
            break;
        }
    }
#undef to_string
#undef to_wstring
};

static void plugInThread(void *i_plugin)
{
    PlugIn *plugin = static_cast<PlugIn *>(i_plugin);
    plugin->exec();
    delete plugin;
}
}

Command_PlugIn::Command_PlugIn()
{
    m_funcName = StrExprArg();
    m_funcParam = StrExprArg();
    m_doesCreateThread = BooleanType_false;
}

void Command_PlugIn::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dllName);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_funcName);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_funcParam);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_doesCreateThread);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_PlugIn::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    shu::PlugIn *plugin = new shu::PlugIn(i_engine->m_windowSystem);
    if (!plugin->load(m_dllName.eval(), m_funcName.eval(), m_funcParam.eval(), i_engine->m_log)) {
        delete plugin;
        return;
    }
    if (m_doesCreateThread) {
        if (_beginthread(shu::plugInThread, 0, plugin) == static_cast<uintptr_t>(-1)) {
            delete plugin;
            Acquire a(&i_engine->m_log);
            i_engine->m_log << std::endl;
            i_engine->m_log << _T("error: &PlugIn() failed to create thread.");
        }
        return;
    } else
        plugin->exec();
}

tostream &Command_PlugIn::outputArgs(tostream &i_ost) const
{
    i_ost << m_dllName << _T(", ");
    i_ost << m_funcName << _T(", ");
    i_ost << m_funcParam << _T(", ");
    i_ost << m_doesCreateThread;
    return i_ost;
}
