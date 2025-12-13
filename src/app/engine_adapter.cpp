#include "engine_adapter.h"
#include "../core/engine/engine.h"

EngineAdapter::EngineAdapter(Engine* engine)
    : m_engine(engine)
    , m_configPath("")
{
    // Engine pointer ownership transferred to adapter
}

EngineAdapter::~EngineAdapter()
{
    // Ensure thread is stopped before destroying
    stop();

    // Clean up engine
    delete m_engine;
    m_engine = nullptr;
}

bool EngineAdapter::getIsEnabled() const
{
    // Stub: Return default value
    return false;
}

bool EngineAdapter::isRunning() const
{
    // Stub: Return default value
    return false;
}

void EngineAdapter::enable()
{
    // Stub: No operation yet
}

void EngineAdapter::disable()
{
    // Stub: No operation yet
}

void EngineAdapter::start()
{
    // Stub: No operation yet
}

void EngineAdapter::stop()
{
    // Stub: No operation yet
}

bool EngineAdapter::loadConfig(const std::string& path)
{
    // Stub: Return false (not implemented)
    return false;
}

const std::string& EngineAdapter::getConfigPath() const
{
    return m_configPath;
}

uint64_t EngineAdapter::keyCount() const
{
    // Stub: Return zero
    return 0;
}

std::string EngineAdapter::getStatusJson() const
{
    // Stub: Return empty JSON object
    return "{}";
}

std::string EngineAdapter::getConfigJson() const
{
    // Stub: Return empty JSON object
    return "{}";
}

std::string EngineAdapter::getKeymapsJson() const
{
    // Stub: Return empty JSON object
    return "{}";
}

std::string EngineAdapter::getMetricsJson() const
{
    // Stub: Return empty JSON object
    return "{}";
}
