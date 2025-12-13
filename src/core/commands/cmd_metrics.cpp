//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// cmd_metrics.cpp - Display performance metrics command implementation

#include "cmd_metrics.h"
#include "../engine/engine.h"
#include "../../utils/metrics.h"
#include "../utils/stringtool.h"

void Command_Metrics::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    auto& metrics = yamy::metrics::PerformanceMetrics::instance();
    std::string statsStr = metrics.getStatsString();

    Acquire a(&i_engine->m_log, 0);
    i_engine->m_log << to_tstring(statsStr) << std::endl;
}
