#include "cmd_sync.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../platform/hook_interface.h" // For platform hook data interface
#include "../platform/sync.h"
#include <iostream>

void Command_Sync::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (i_param->m_isPressed)
        i_engine->generateModifierEvents(i_param->m_af->m_modifier);
    if (!i_param->m_isPressed || i_engine->getWindowSystem()->isConsoleWindow(i_param->m_hwnd))
        return;

    Key *sync = i_engine->m_setting->m_keyboard.getSyncKey();
    if (sync->getScanCodesSize() == 0)
        return;
    const ScanCode *sc = sync->getScanCodes();

    // set variables exported from mayu.dll (hook data)
    auto* hookData = yamy::platform::getHookData();
    hookData->m_syncKey = sc->m_scan;
    hookData->m_syncKeyIsExtended = !!(sc->m_flags & ScanCode::E0E1);
    i_engine->m_isSynchronizing = true;
    i_engine->generateKeyEvent(sync, false, false);

    i_engine->m_cs.release();
    auto r = yamy::platform::waitForObject(i_engine->m_eSync, 5000);
    if (r == yamy::platform::WaitResult::Timeout) {
        Acquire a(&i_engine->m_log, 0);
        i_engine->m_log << " *FAILED*" << std::endl;
    }
    i_engine->m_cs.acquire();
    i_engine->m_isSynchronizing = false;
}
