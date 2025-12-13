#include "engine_adapter.h"
#include "../core/engine/engine.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>

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
    if (!m_engine) {
        return false;
    }
    return m_engine->getIsEnabled();
}

bool EngineAdapter::isRunning() const
{
    return m_engineThread.joinable();
}

void EngineAdapter::enable()
{
    if (m_engine) {
        m_engine->enable();
    }
}

void EngineAdapter::disable()
{
    if (m_engine) {
        m_engine->disable();
    }
}

void EngineAdapter::start()
{
    if (!m_engine) {
        std::cerr << "EngineAdapter::start() - No engine instance" << std::endl;
        return;
    }

    // Don't start if already running
    if (m_engineThread.joinable()) {
        std::cerr << "EngineAdapter::start() - Engine thread already running" << std::endl;
        return;
    }

    // Create thread to run the engine
    m_engineThread = std::thread([this]() {
        try {
            m_engine->start();
        } catch (const std::exception& e) {
            std::cerr << "Engine thread exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Engine thread unknown exception" << std::endl;
        }
    });

    // Give the engine thread a moment to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void EngineAdapter::stop()
{
    if (!m_engine) {
        return;
    }

    // Signal the engine to stop
    m_engine->stop();

    // Wait for the thread to complete
    if (m_engineThread.joinable()) {
        m_engineThread.join();
    }
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
