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
    yamy::platform::IWindowSystem* m_ws;
    void* m_dll;
    void* m_func;
    Type m_type;
    std::string m_funcParam;

public:
    PlugIn(yamy::platform::IWindowSystem* ws) : m_ws(ws), m_dll(nullptr), m_func(nullptr) {
    }

    ~PlugIn() {
        if (m_dll)
            m_ws->freeLibrary(m_dll);
    }

    bool load(const std::string &i_dllName, const std::string &i_funcName,
              const std::string &i_funcParam, tomsgstream &i_log) {

        tstring tDllName = to_tstring(i_dllName);

        m_dll = m_ws->loadLibrary(to_UTF_8(_T("Plugins\\") + tDllName));
        if (!m_dll) {
            m_dll = m_ws->loadLibrary(to_UTF_8(_T("Plugin\\") + tDllName));
            if (!m_dll) {
                m_dll = m_ws->loadLibrary(i_dllName);
                if (!m_dll) {
                    Acquire a(&i_log);
                    i_log << std::endl;
                    i_log << _T("error: &PlugIn() failed to load ") << tDllName << std::endl;
                    return false;
                }
            }
        }

        // get function
        // i_funcName is UTF-8/ASCII.
        std::string baseName = i_funcName;

        m_type = Type_W;
        m_func = m_ws->getProcAddress(m_dll, "mayu" + baseName + "W");
        if (!m_func) {
            m_type = Type_A;
            m_func = m_ws->getProcAddress(m_dll, "mayu" + baseName + "A");
            if (!m_func) {
                m_func = m_ws->getProcAddress(m_dll, "mayu" + baseName);
                if (!m_func) {
                    m_func = m_ws->getProcAddress(m_dll, baseName);
                    if (!m_func) {
                        Acquire a(&i_log);
                        i_log << std::endl;
                        i_log << _T("error: &PlugIn() failed to find function: ")
                        << to_tstring(baseName) << std::endl;
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
            // UTF-8 -> MBCS
            reinterpret_cast<PLUGIN_FUNCTION_A>(m_func)(to_string(to_wstring(m_funcParam)).c_str());
            break;
        case Type_W:
            // UTF-8 -> Wide
            reinterpret_cast<PLUGIN_FUNCTION_W>(m_func)(to_wstring(m_funcParam).c_str());
            break;
        }
    }
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
    std::string sName = getName();
    const char* cName = sName.c_str();

    i_sl->getOpenParen(true, cName); // throw ...
    i_sl->load_ARGUMENT(&m_dllName);
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_funcName);
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_funcParam);
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_doesCreateThread);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_PlugIn::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    shu::PlugIn *plugin = new shu::PlugIn(i_engine->getWindowSystem());
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
