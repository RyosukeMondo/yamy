#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// cmd_metrics.h - Display performance metrics command
//
// Outputs current performance metrics (avg, p50, p95, p99 latencies) to the log.
// Can be bound to a key to quickly check performance.
//

#ifndef _CMD_METRICS_H
#define _CMD_METRICS_H

#include "command_base.h"

class Engine;
class FunctionParam;

class Command_Metrics : public Command<Command_Metrics>
{
public:
    static constexpr const char* Name = "Metrics";

    void exec(Engine *i_engine, FunctionParam *i_param) const;
};

#endif // _CMD_METRICS_H
