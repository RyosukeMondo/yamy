#include "cmd_load_setting.h"
#include "../engine/engine.h"
#include <regex>

#ifndef MAX_MAYU_REGISTRY_ENTRIES
#define MAX_MAYU_REGISTRY_ENTRIES 256
#endif

// load setting
void Command_LoadSetting::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    const StrExprArg &i_name = getArg<0>();
    if (!i_name.eval().empty()) {
        // set MAYU_REGISTRY_ROOT\.mayuIndex which name is same with i_name
        if (!i_engine->m_configStore) {
            // Should not happen if properly initialized
            return;
        }

        tregex split(_T("^([^;]*);([^;]*);(.*)$"));
        tstringi dot_mayu;
        for (size_t i = 0; i < MAX_MAYU_REGISTRY_ENTRIES; ++ i) {
            _TCHAR buf[100];
            _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), (int)i);
            if (!i_engine->m_configStore->read(buf, &dot_mayu))
                break;

            tsmatch what;
            if (std::regex_match(dot_mayu, what, split) &&
                    what.str(1) == to_tstring(i_name.eval())) {
                i_engine->m_configStore->write(_T(".mayuIndex"), (DWORD)i);
                goto success;
            }
        }

        {
            Acquire a(&i_engine->m_log, 0);
            i_engine->m_log << _T("unknown setting name: ") << i_name;
        }
        return;

success:
        ;
    }
    i_engine->getWindowSystem()->postMessage(i_engine->m_hwndAssocWindow,
                WM_APP_engineNotify, EngineNotify_loadSetting, 0);
}
