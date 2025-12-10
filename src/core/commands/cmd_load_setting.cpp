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

        std::regex split("^([^;]*);([^;]*);(.*)$");
        std::string dot_mayu;
        for (size_t i = 0; i < MAX_MAYU_REGISTRY_ENTRIES; ++ i) {
            char buf[100];
            snprintf(buf, NUMBER_OF(buf), ".mayu%d", (int)i);
            if (!i_engine->m_configStore->read(buf, &dot_mayu))
                break;

            std::smatch what;
            if (std::regex_match(dot_mayu, what, split) &&
                    what.str(1) == i_name.eval()) {
                i_engine->m_configStore->write(".mayuIndex", (uint32_t)i);
                goto success;
            }
        }

        {
            Acquire a(&i_engine->m_log, 0);
            i_engine->m_log << "unknown setting name: " << i_name;
        }
        return;

success:
        ;
    }
    i_engine->getWindowSystem()->postMessage(i_engine->getAssociatedWndow(),
                WM_APP_engineNotify, EngineNotify_loadSetting, 0);
}
