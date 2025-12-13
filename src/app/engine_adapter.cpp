#include "engine_adapter.h"
#include "../core/engine/engine.h"
#include "../core/settings/setting.h"
#include "../utils/metrics.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QDateTime>

EngineAdapter::EngineAdapter(Engine* engine)
    : m_engine(engine)
    , m_configPath("")
    , m_startTime(std::chrono::steady_clock::now())
    , m_configLoadedTime(std::chrono::system_clock::now())
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

    // Reset start time
    m_startTime = std::chrono::steady_clock::now();

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

    // Save config path and loaded time on success
    if (success) {
        m_configPath = path;
        m_configLoadedTime = std::chrono::system_clock::now();
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
    // Get key count from metrics
    auto& metrics = yamy::metrics::PerformanceMetrics::instance();
    auto keyProcStats = metrics.getStats(yamy::metrics::Operations::KEY_PROCESSING);
    return keyProcStats.count;
}

std::string EngineAdapter::getStatusJson() const
{
    QJsonObject obj;

    if (!m_engine) {
        obj["state"] = "error";
        obj["uptime"] = 0;
        obj["config"] = "";
        obj["key_count"] = 0;
        obj["current_keymap"] = "none";
        return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
    }

    // Get state from engine
    auto state = m_engine->getState();
    std::string stateStr;
    switch (state) {
        case yamy::EngineState::Running:
            stateStr = "running";
            break;
        case yamy::EngineState::Stopped:
            stateStr = "stopped";
            break;
        case yamy::EngineState::Loading:
            stateStr = "loading";
            break;
        case yamy::EngineState::Error:
            stateStr = "error";
            break;
    }
    obj["state"] = QString::fromStdString(stateStr);

    // Calculate uptime in seconds
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
    obj["uptime"] = static_cast<qint64>(uptime);

    // Config path
    obj["config"] = QString::fromStdString(m_configPath);

    // Key count from metrics
    obj["key_count"] = static_cast<qint64>(keyCount());

    // Current keymap - get from engine's current window
    std::string currentKeymap = "default";
    const Setting* setting = m_engine->getSetting();
    if (setting) {
        std::string className = m_engine->getCurrentWindowClassName();
        std::string titleName = m_engine->getCurrentWindowTitleName();

        // Search for matching keymap
        Keymaps::KeymapPtrList keymaps;
        const_cast<Keymaps&>(setting->m_keymaps).searchWindow(&keymaps, className, titleName);

        if (!keymaps.empty() && keymaps.front()) {
            currentKeymap = keymaps.front()->getName();
        }
    }
    obj["current_keymap"] = QString::fromStdString(currentKeymap);

    return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
}

std::string EngineAdapter::getConfigJson() const
{
    QJsonObject obj;

    obj["config_path"] = QString::fromStdString(m_configPath);

    // Extract config name from path
    std::string configName = m_configPath;
    size_t lastSlash = m_configPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        configName = m_configPath.substr(lastSlash + 1);
    }
    obj["config_name"] = QString::fromStdString(configName);

    // Convert loaded time to ISO8601 format
    auto time_t_val = std::chrono::system_clock::to_time_t(m_configLoadedTime);
    QDateTime qdt = QDateTime::fromSecsSinceEpoch(time_t_val);
    obj["loaded_time"] = qdt.toString(Qt::ISODate);

    return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
}

std::string EngineAdapter::getKeymapsJson() const
{
    QJsonObject obj;
    QJsonArray keymapsArray;

    if (m_engine) {
        const Setting* setting = m_engine->getSetting();
        if (setting) {
            // Iterate through all keymaps
            const auto& keymapList = setting->m_keymaps.getKeymapList();
            for (const auto& keymap : keymapList) {
                QJsonObject kmObj;
                kmObj["name"] = QString::fromStdString(keymap.getName());
                kmObj["window_class"] = QString::fromStdString(keymap.getWindowClassStr());
                kmObj["window_title"] = QString::fromStdString(keymap.getWindowTitleStr());
                keymapsArray.append(kmObj);
            }
        }
    }

    obj["keymaps"] = keymapsArray;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
}

std::string EngineAdapter::getMetricsJson() const
{
    QJsonObject obj;

    // Get metrics from the global PerformanceMetrics instance
    auto& metrics = yamy::metrics::PerformanceMetrics::instance();

    // Get key processing metrics (primary metric for yamy)
    auto keyProcStats = metrics.getStats(yamy::metrics::Operations::KEY_PROCESSING);

    // Populate JSON with metrics matching the expected format
    obj["latency_avg_ns"] = static_cast<qint64>(keyProcStats.averageNs);
    obj["latency_p99_ns"] = static_cast<qint64>(keyProcStats.p99Ns);
    obj["latency_max_ns"] = static_cast<qint64>(keyProcStats.maxNs);

    // CPU usage - not currently tracked, return 0.0 for now
    // This could be enhanced later with actual CPU monitoring
    obj["cpu_usage_percent"] = 0.0;

    // Keys per second calculation
    double keysPerSecond = 0.0;
    if (keyProcStats.periodEnd > keyProcStats.periodStart) {
        uint64_t periodMs = keyProcStats.periodEnd - keyProcStats.periodStart;
        if (periodMs > 0) {
            keysPerSecond = (keyProcStats.count * 1000.0) / periodMs;
        }
    }
    obj["keys_per_second"] = keysPerSecond;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
}
