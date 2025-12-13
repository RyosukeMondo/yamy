#include "engine_adapter.h"
#include "../core/engine/engine.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <filesystem>

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
    if (!m_engine) {
        std::cerr << "EngineAdapter::loadConfig() - No engine instance" << std::endl;
        return false;
    }

    // Validate file existence
    namespace fs = std::filesystem;
    if (!fs::exists(path)) {
        std::cerr << "EngineAdapter::loadConfig() - File not found: " << path << std::endl;
        return false;
    }

    if (!fs::is_regular_file(path)) {
        std::cerr << "EngineAdapter::loadConfig() - Not a regular file: " << path << std::endl;
        return false;
    }

    // Remember if engine was running before config load
    bool wasRunning = isRunning();

    // Stop engine if running (required for safe config reload)
    if (wasRunning) {
        stop();
    }

    // Use Engine's switchConfiguration method which handles parsing and applying
    bool success = false;
    try {
        success = m_engine->switchConfiguration(path);
    } catch (const std::exception& e) {
        std::cerr << "EngineAdapter::loadConfig() - Exception: " << e.what() << std::endl;
        success = false;
    } catch (...) {
        std::cerr << "EngineAdapter::loadConfig() - Unknown exception" << std::endl;
        success = false;
    }

    // Save config path on success
    if (success) {
        m_configPath = path;
    } else {
        std::cerr << "EngineAdapter::loadConfig() - Failed to load config: " << path << std::endl;
    }

    // Restart engine if it was running before
    if (wasRunning) {
        start();
    }

    return success;
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
