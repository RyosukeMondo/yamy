// mock_ipc_server.cpp - Lightweight IPC mock server for GUI testing
//
// This utility spins up a QLocalServer using IPCChannelQt and simulates
// daemon responses so the Qt GUI can be exercised without a running engine.
// Responses can be customized via command-line overrides or a JSON fixture.

#include "core/ipc_messages.h"
#include "core/platform/linux/ipc_channel_qt.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <iostream>
#include <unistd.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

using yamy::ipc::MessageType;

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
};

std::optional<MessageType> parseCommandName(const QString& name) {
    static const std::unordered_map<std::string, MessageType> kMap = {
        {"CmdReload", MessageType::CmdReload},       {"CmdStop", MessageType::CmdStop},
        {"CmdStart", MessageType::CmdStart},         {"CmdGetStatus", MessageType::CmdGetStatus},
        {"CmdGetConfig", MessageType::CmdGetConfig}, {"CmdGetKeymaps", MessageType::CmdGetKeymaps},
        {"CmdGetMetrics", MessageType::CmdGetMetrics},
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
            default:
                send(MessageType::RspError, "Unsupported command");
                break;
        }
    }

private:
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
