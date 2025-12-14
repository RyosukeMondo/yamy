// mock_ipc_server.cpp - Lightweight IPC mock server for GUI testing
//
// This utility spins up a QLocalServer using IPCChannelQt and simulates
// daemon responses so the Qt GUI can be exercised without a running engine.
// Responses can be customized via command-line overrides or a JSON fixture.

#include "core/ipc_messages.h"
#include "core/platform/ipc_defs.h"
#include "core/platform/linux/ipc_channel_qt.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using yamy::ipc::MessageType;
using GuiMessageType = yamy::MessageType;

constexpr MessageType kGuiCmdGetStatus =
    static_cast<MessageType>(static_cast<uint32_t>(GuiMessageType::CmdGetStatus));
constexpr MessageType kGuiCmdSetEnabled =
    static_cast<MessageType>(static_cast<uint32_t>(GuiMessageType::CmdSetEnabled));
constexpr MessageType kGuiCmdSwitchConfig =
    static_cast<MessageType>(static_cast<uint32_t>(GuiMessageType::CmdSwitchConfig));
constexpr MessageType kGuiCmdReloadConfig =
    static_cast<MessageType>(static_cast<uint32_t>(GuiMessageType::CmdReloadConfig));

constexpr MessageType kGuiRspStatus =
    static_cast<MessageType>(static_cast<uint32_t>(GuiMessageType::RspStatus));
constexpr MessageType kGuiRspConfigList =
    static_cast<MessageType>(static_cast<uint32_t>(GuiMessageType::RspConfigList));

template <size_t N>
std::string toString(const std::array<char, N>& buffer) {
    auto end = std::find(buffer.begin(), buffer.end(), '\0');
    return std::string(buffer.begin(), end);
}

template <size_t N>
void copyString(const std::string& value, std::array<char, N>& buffer) {
    std::fill(buffer.begin(), buffer.end(), '\0');
    const auto copyLen = std::min(value.size(), buffer.size() - 1);
    std::copy_n(value.c_str(), copyLen, buffer.data());
}

struct ResponseConfig {
    std::string statusJson =
        R"({"engine_running":true,"enabled":true,"active_config":"mock.mayu","uptime":42})";
    std::string configJson =
        R"({"active_config":"mock.mayu","configs":["mock.mayu","layered.mayu"]})";
    std::string keymapsJson = R"({"keymaps":["mock.mayu","layered.mayu"]})";
    std::string metricsJson =
        R"({"latency_ns":8000,"cpu_pct":2.5,"event_count":128})";
    std::string okMessage = "OK";
    std::string errorMessage = "Mock server error";
    std::unordered_set<MessageType> forcedErrors;
    bool guiEngineRunning = true;
    bool guiEnabled = true;
    std::string guiActiveConfig = "mock.mayu";
    std::vector<std::string> guiConfigs{"mock.mayu", "layered.mayu"};
    std::string guiLastError{};
};

std::optional<MessageType> parseCommandName(const QString& name) {
    static const std::unordered_map<std::string, MessageType> kMap = {
        {"CmdReload", MessageType::CmdReload},       {"CmdStop", MessageType::CmdStop},
        {"CmdStart", MessageType::CmdStart},         {"CmdGetStatus", MessageType::CmdGetStatus},
        {"CmdGetConfig", MessageType::CmdGetConfig}, {"CmdGetKeymaps", MessageType::CmdGetKeymaps},
        {"CmdGetMetrics", MessageType::CmdGetMetrics},
        {"CmdSetEnabled", kGuiCmdSetEnabled},
        {"CmdSwitchConfig", kGuiCmdSwitchConfig},
        {"CmdReloadConfig", kGuiCmdReloadConfig},
        {"CmdGetStatusGui", kGuiCmdGetStatus},
    };

    auto it = kMap.find(name.toStdString());
    if (it == kMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

QString commandName(MessageType type) {
    switch (type) {
        case MessageType::CmdReload: return "CmdReload";
        case MessageType::CmdStop: return "CmdStop";
        case MessageType::CmdStart: return "CmdStart";
        case MessageType::CmdGetStatus: return "CmdGetStatus";
        case MessageType::CmdGetConfig: return "CmdGetConfig";
        case MessageType::CmdGetKeymaps: return "CmdGetKeymaps";
        case MessageType::CmdGetMetrics: return "CmdGetMetrics";
        case kGuiCmdGetStatus: return "CmdGetStatusGui";
        case kGuiCmdSetEnabled: return "CmdSetEnabled";
        case kGuiCmdSwitchConfig: return "CmdSwitchConfig";
        case kGuiCmdReloadConfig: return "CmdReloadConfig";
        case kGuiRspStatus: return "RspStatus";
        case kGuiRspConfigList: return "RspConfigList";
        default: return "Unknown";
    }
}

void applyFixture(const QJsonObject& obj, ResponseConfig& config) {
    if (obj.contains("status")) {
        config.statusJson = obj.value("status").toString().toStdString();
    }
    if (obj.contains("config")) {
        config.configJson = obj.value("config").toString().toStdString();
    }
    if (obj.contains("keymaps")) {
        config.keymapsJson = obj.value("keymaps").toString().toStdString();
    }
    if (obj.contains("metrics")) {
        config.metricsJson = obj.value("metrics").toString().toStdString();
    }
    if (obj.contains("okMessage")) {
        config.okMessage = obj.value("okMessage").toString().toStdString();
    }
    if (obj.contains("errorMessage")) {
        config.errorMessage = obj.value("errorMessage").toString().toStdString();
    }
    if (obj.contains("forceError")) {
        auto commands = obj.value("forceError").toArray();
        for (const auto& entry : commands) {
            auto maybeType = parseCommandName(entry.toString());
            if (maybeType) {
                config.forcedErrors.insert(*maybeType);
            }
        }
    }
}

class MockIpcServer : public QObject {
    Q_OBJECT

public:
    MockIpcServer(const std::string& socketName, ResponseConfig config, QObject* parent = nullptr)
        : QObject(parent)
        , m_channel(socketName)
        , m_config(std::move(config))
        , m_channelNameCache(socketName) {
        QObject::connect(&m_channel, &yamy::platform::IPCChannelQt::messageReceived,
                         this, &MockIpcServer::handleMessage);
    }

    void start() {
        m_channel.listen();
        std::cout << "[mock-ipc-server] Listening on /tmp/yamy-" << m_channelName()
                  << "-" << getuid() << std::endl;
    }

    void stop() {
        m_channel.disconnect();
        std::cout << "[mock-ipc-server] Stopped" << std::endl;
    }

private slots:
    void handleMessage(const yamy::ipc::Message& message) {
        const std::string payload =
            (message.data && message.size > 0)
                ? std::string(static_cast<const char*>(message.data), message.size)
                : std::string();

        std::cout << "[mock-ipc-server] Received " << commandName(message.type).toStdString()
                  << " (" << payload << ")" << std::endl;

        if (m_config.forcedErrors.count(message.type) > 0) {
            send(MessageType::RspError, m_config.errorMessage);
            return;
        }

        switch (message.type) {
            case MessageType::CmdReload:
            case MessageType::CmdStop:
            case MessageType::CmdStart:
                send(MessageType::RspOk, m_config.okMessage);
                break;
            case MessageType::CmdGetStatus:
                send(MessageType::RspStatus, m_config.statusJson);
                break;
            case MessageType::CmdGetConfig:
                send(MessageType::RspConfig, m_config.configJson);
                break;
            case MessageType::CmdGetKeymaps:
                send(MessageType::RspKeymaps, m_config.keymapsJson);
                break;
            case MessageType::CmdGetMetrics:
                send(MessageType::RspMetrics, m_config.metricsJson);
                break;
            case kGuiCmdGetStatus: {
                sendGuiStatus();
                sendGuiConfigList();
                break;
            }
            case kGuiCmdSetEnabled: {
                if (message.size == sizeof(yamy::CmdSetEnabledRequest)) {
                    auto* request = static_cast<const yamy::CmdSetEnabledRequest*>(message.data);
                    m_config.guiEnabled = request->enabled;
                }
                sendGuiStatus();
                sendGuiConfigList();
                break;
            }
            case kGuiCmdSwitchConfig: {
                if (message.size == sizeof(yamy::CmdSwitchConfigRequest)) {
                    auto* request = static_cast<const yamy::CmdSwitchConfigRequest*>(message.data);
                    m_config.guiActiveConfig = toString(request->configName);
                }
                sendGuiStatus();
                sendGuiConfigList();
                break;
            }
            case kGuiCmdReloadConfig: {
                if (message.size == sizeof(yamy::CmdReloadConfigRequest)) {
                    auto* request = static_cast<const yamy::CmdReloadConfigRequest*>(message.data);
                    m_config.guiActiveConfig = toString(request->configName);
                }
                sendGuiStatus();
                sendGuiConfigList();
                break;
            }
            default:
                send(MessageType::RspError, "Unsupported command");
                break;
        }
    }

private:
    void sendGuiStatus() {
        yamy::RspStatusPayload status{};
        status.engineRunning = m_config.guiEngineRunning;
        status.enabled = m_config.guiEnabled;
        copyString(m_config.guiActiveConfig, status.activeConfig);
        copyString(m_config.guiLastError, status.lastError);

        yamy::ipc::Message response{kGuiRspStatus, &status, sizeof(status)};
        m_channel.send(response);
    }

    void sendGuiConfigList() {
        yamy::RspConfigListPayload configs{};
        configs.count = static_cast<uint32_t>(std::min(m_config.guiConfigs.size(),
                                                       configs.configs.size()));
        for (size_t i = 0; i < configs.count; ++i) {
            copyString(m_config.guiConfigs[i], configs.configs[i]);
        }
        yamy::ipc::Message response{kGuiRspConfigList, &configs, sizeof(configs)};
        m_channel.send(response);
    }

    void send(MessageType type, const std::string& data) {
        yamy::ipc::Message response{type, data.data(), data.size()};
        m_channel.send(response);
    }

    std::string m_channelName() const {
        return m_channelNameCache.empty() ? "yamy-engine" : m_channelNameCache;
    }

    yamy::platform::IPCChannelQt m_channel;
    ResponseConfig m_config;
    std::string m_channelNameCache{"yamy-engine"};
};

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Mock IPC server for Yamy GUI testing");
    parser.addHelpOption();

    QCommandLineOption socketNameOpt({"s", "socket-name"},
                                     "Logical socket name (default: yamy-engine)",
                                     "name", "yamy-engine");
    QCommandLineOption fixtureOpt({"f", "fixture"},
                                  "JSON file with response overrides",
                                  "file");
    QCommandLineOption statusOpt({"", "status-json"}, "Status response JSON", "json");
    QCommandLineOption configOpt({"", "config-json"}, "Config response JSON", "json");
    QCommandLineOption keymapsOpt({"", "keymaps-json"}, "Keymaps response JSON", "json");
    QCommandLineOption metricsOpt({"", "metrics-json"}, "Metrics response JSON", "json");
    QCommandLineOption okOpt({"", "ok-message"}, "Text payload for RspOk", "text", "OK");
    QCommandLineOption errorOpt({"", "error-message"}, "Text payload for RspError", "text",
                                "Mock server error");
    QCommandLineOption failOpt({"", "fail-cmd"},
                               "Command name to force error (repeatable)",
                               "cmd");

    parser.addOption(socketNameOpt);
    parser.addOption(fixtureOpt);
    parser.addOption(statusOpt);
    parser.addOption(configOpt);
    parser.addOption(keymapsOpt);
    parser.addOption(metricsOpt);
    parser.addOption(okOpt);
    parser.addOption(errorOpt);
    parser.addOption(failOpt);

    parser.process(app);

    ResponseConfig config;

    if (parser.isSet(fixtureOpt)) {
        QFile file(parser.value(fixtureOpt));
        if (file.open(QIODevice::ReadOnly)) {
            const auto doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                applyFixture(doc.object(), config);
                std::cout << "[mock-ipc-server] Loaded fixture from "
                          << parser.value(fixtureOpt).toStdString() << std::endl;
            } else {
                std::cerr << "[mock-ipc-server] Fixture is not a JSON object" << std::endl;
            }
        } else {
            std::cerr << "[mock-ipc-server] Failed to open fixture file: "
                      << parser.value(fixtureOpt).toStdString() << std::endl;
        }
    }

    if (parser.isSet(statusOpt)) {
        config.statusJson = parser.value(statusOpt).toStdString();
    }
    if (parser.isSet(configOpt)) {
        config.configJson = parser.value(configOpt).toStdString();
    }
    if (parser.isSet(keymapsOpt)) {
        config.keymapsJson = parser.value(keymapsOpt).toStdString();
    }
    if (parser.isSet(metricsOpt)) {
        config.metricsJson = parser.value(metricsOpt).toStdString();
    }
    if (parser.isSet(okOpt)) {
        config.okMessage = parser.value(okOpt).toStdString();
    }
    if (parser.isSet(errorOpt)) {
        config.errorMessage = parser.value(errorOpt).toStdString();
    }

    for (const auto& value : parser.values(failOpt)) {
        auto maybe = parseCommandName(value);
        if (maybe) {
            config.forcedErrors.insert(*maybe);
        } else {
            std::cerr << "[mock-ipc-server] Unknown command in --fail-cmd: "
                      << value.toStdString() << std::endl;
        }
    }

    const std::string socketName = parser.value(socketNameOpt).toStdString();
    MockIpcServer server(socketName, config);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&server]() { server.stop(); });

    server.start();
    return app.exec();
}

#include "mock_ipc_server.moc"
