//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy_ctl.cpp - Command-line tool for controlling running YAMY instance
//
// Usage:
//   yamy-ctl reload [--config NAME]  - Reload configuration
//   yamy-ctl stop                    - Stop the engine
//   yamy-ctl start                   - Start the engine
//   yamy-ctl status [--json]         - Get engine status
//   yamy-ctl config [--json]         - Get configuration details
//   yamy-ctl keymaps [--json]        - List loaded keymaps
//   yamy-ctl metrics [--json]        - Get performance metrics
//   yamy-ctl --help                  - Show help
//

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cctype>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <getopt.h>

namespace {

/// Default socket path for engine control
constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/yamy-engine.sock";

/// Default timeout for waiting for response (milliseconds)
constexpr int DEFAULT_TIMEOUT_MS = 5000;

/// Exit codes
enum ExitCode {
    SUCCESS = 0,
    ENGINE_NOT_RUNNING = 1,
    COMMAND_FAILED = 2,
    INVALID_ARGS = 3
};

/// IPC Message Types (must match ipc_messages.h)
enum MessageType : uint32_t {
    CmdReload = 0x2001,
    CmdStop = 0x2002,
    CmdStart = 0x2003,
    CmdGetStatus = 0x2004,
    CmdGetConfig = 0x2005,
    CmdGetKeymaps = 0x2006,
    CmdGetMetrics = 0x2007,
    RspOk = 0x2100,
    RspError = 0x2101,
    RspStatus = 0x2102,
    RspConfig = 0x2103,
    RspKeymaps = 0x2104,
    RspMetrics = 0x2105
};

/// Wire protocol message header
struct MessageHeader {
    uint32_t type;
    uint32_t dataSize;
};

/// Extract a string value from a JSON object (simple parser)
/// Assumes valid JSON format from engine
std::string jsonGetString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) {
        return "";
    }
    pos += searchKey.length();

    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }

    if (pos >= json.length()) {
        return "";
    }

    // Check if it's a string value
    if (json[pos] == '"') {
        ++pos;
        size_t end = json.find('"', pos);
        if (end != std::string::npos) {
            return json.substr(pos, end - pos);
        }
    }
    return "";
}

/// Extract a numeric value from a JSON object (simple parser)
int64_t jsonGetInt(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) {
        return 0;
    }
    pos += searchKey.length();

    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }

    if (pos >= json.length()) {
        return 0;
    }

    // Parse number (handles negatives)
    std::string numStr;
    if (json[pos] == '-') {
        numStr += json[pos++];
    }
    while (pos < json.length() && std::isdigit(json[pos])) {
        numStr += json[pos++];
    }

    if (numStr.empty() || numStr == "-") {
        return 0;
    }
    return std::strtoll(numStr.c_str(), nullptr, 10);
}

/// Extract a double value from a JSON object (simple parser)
double jsonGetDouble(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) {
        return 0.0;
    }
    pos += searchKey.length();

    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }

    if (pos >= json.length()) {
        return 0.0;
    }

    // Parse number (handles negatives and decimals)
    std::string numStr;
    if (json[pos] == '-') {
        numStr += json[pos++];
    }
    while (pos < json.length() && (std::isdigit(json[pos]) || json[pos] == '.')) {
        numStr += json[pos++];
    }

    if (numStr.empty() || numStr == "-") {
        return 0.0;
    }
    return std::strtod(numStr.c_str(), nullptr);
}

/// Format uptime seconds into human readable string
std::string formatUptime(int64_t seconds) {
    if (seconds < 60) {
        return std::to_string(seconds) + "s";
    }

    int64_t hours = seconds / 3600;
    int64_t mins = (seconds % 3600) / 60;

    if (hours > 0) {
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
    return std::to_string(mins) + "m";
}

/// Format nanoseconds into human readable string
std::string formatLatency(int64_t ns) {
    if (ns >= 1000000) {
        return std::to_string(ns / 1000000) + "ms";
    } else if (ns >= 1000) {
        return std::to_string(ns / 1000) + "us";
    }
    return std::to_string(ns) + "ns";
}

/// Extract JSON array element from keymaps JSON
/// Returns next object in array or empty string if done
std::string jsonGetArrayObject(const std::string& json, const std::string& arrayKey, size_t& offset) {
    if (offset == 0) {
        // First call - find the array
        std::string searchKey = "\"" + arrayKey + "\":";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) {
            return "";
        }
        pos += searchKey.length();

        // Find opening bracket
        pos = json.find('[', pos);
        if (pos == std::string::npos) {
            return "";
        }
        offset = pos + 1;
    }

    // Skip whitespace
    while (offset < json.length() && (json[offset] == ' ' || json[offset] == '\t' ||
           json[offset] == '\n' || json[offset] == '\r' || json[offset] == ',')) {
        ++offset;
    }

    // Check for end of array
    if (offset >= json.length() || json[offset] == ']') {
        return "";
    }

    // Find object start
    if (json[offset] != '{') {
        return "";
    }

    // Find matching closing brace
    size_t start = offset;
    int braceCount = 1;
    ++offset;
    while (offset < json.length() && braceCount > 0) {
        if (json[offset] == '{') {
            ++braceCount;
        } else if (json[offset] == '}') {
            --braceCount;
        }
        ++offset;
    }

    return json.substr(start, offset - start);
}

/// Print usage information
void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <command> [options]\n"
              << "\n"
              << "Commands:\n"
              << "  reload [--config NAME]  Reload configuration (optionally switch to NAME)\n"
              << "  stop                    Stop the key remapping engine\n"
              << "  start                   Start the key remapping engine\n"
              << "  status                  Show engine status\n"
              << "  config                  Show configuration details\n"
              << "  keymaps                 List loaded keymaps\n"
              << "  metrics                 Show performance metrics\n"
              << "\n"
              << "Options:\n"
              << "  -c, --config NAME       Specify configuration name for reload\n"
              << "  -j, --json              Output raw JSON (for status, config, keymaps, metrics)\n"
              << "  -s, --socket PATH       Use custom socket path (default: " << DEFAULT_SOCKET_PATH << ")\n"
              << "  -t, --timeout MS        Response timeout in milliseconds (default: " << DEFAULT_TIMEOUT_MS << ")\n"
              << "  -h, --help              Show this help message\n"
              << "\n"
              << "Exit codes:\n"
              << "  0  Success\n"
              << "  1  YAMY engine is not running\n"
              << "  2  Command failed\n"
              << "  3  Invalid arguments\n"
              << "\n"
              << "Examples:\n"
              << "  " << progName << " status\n"
              << "  " << progName << " status --json\n"
              << "  " << progName << " config\n"
              << "  " << progName << " keymaps\n"
              << "  " << progName << " metrics\n"
              << "  " << progName << " reload\n"
              << "  " << progName << " reload --config work\n"
              << "  " << progName << " stop\n";
}

/// Connect to YAMY engine socket
/// @return Socket file descriptor, or -1 on failure
int connectToEngine(const char* socketPath) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Error: Failed to create socket: " << std::strerror(errno) << "\n";
        return -1;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        if (errno == ECONNREFUSED || errno == ENOENT) {
            std::cerr << "Error: YAMY engine is not running\n";
        } else {
            std::cerr << "Error: Failed to connect to engine: " << std::strerror(errno) << "\n";
        }
        close(sock);
        return -1;
    }

    return sock;
}

/// Send message to engine
/// @return true on success
bool sendMessage(int sock, MessageType type, const std::string& data = "") {
    MessageHeader header;
    header.type = static_cast<uint32_t>(type);
    header.dataSize = static_cast<uint32_t>(data.size());

    // Send header
    ssize_t sent = send(sock, &header, sizeof(header), MSG_NOSIGNAL);
    if (sent != sizeof(header)) {
        std::cerr << "Error: Failed to send message header: " << std::strerror(errno) << "\n";
        return false;
    }

    // Send data if present
    if (!data.empty()) {
        sent = send(sock, data.c_str(), data.size(), MSG_NOSIGNAL);
        if (sent != static_cast<ssize_t>(data.size())) {
            std::cerr << "Error: Failed to send message data: " << std::strerror(errno) << "\n";
            return false;
        }
    }

    return true;
}

/// Receive response from engine
/// @return true on success, response stored in type and data
bool receiveResponse(int sock, int timeoutMs, MessageType& type, std::string& data) {
    // Wait for data with timeout
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;

    int pollResult = poll(&pfd, 1, timeoutMs);
    if (pollResult == 0) {
        std::cerr << "Error: Timeout waiting for response from engine\n";
        return false;
    }
    if (pollResult < 0) {
        std::cerr << "Error: poll() failed: " << std::strerror(errno) << "\n";
        return false;
    }

    // Read header
    MessageHeader header;
    ssize_t received = recv(sock, &header, sizeof(header), MSG_WAITALL);
    if (received != sizeof(header)) {
        if (received == 0) {
            std::cerr << "Error: Connection closed by engine\n";
        } else {
            std::cerr << "Error: Failed to receive response header: " << std::strerror(errno) << "\n";
        }
        return false;
    }

    type = static_cast<MessageType>(header.type);

    // Read data if present
    data.clear();
    if (header.dataSize > 0) {
        // Sanity check - don't allow huge messages
        if (header.dataSize > 1024 * 1024) {
            std::cerr << "Error: Response data too large (" << header.dataSize << " bytes)\n";
            return false;
        }

        data.resize(header.dataSize);
        received = recv(sock, &data[0], header.dataSize, MSG_WAITALL);
        if (received != static_cast<ssize_t>(header.dataSize)) {
            std::cerr << "Error: Failed to receive response data: " << std::strerror(errno) << "\n";
            return false;
        }
    }

    return true;
}

/// Execute reload command
int cmdReload(int sock, int timeoutMs, const std::string& configName) {
    if (!sendMessage(sock, MessageType::CmdReload, configName)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspOk) {
        if (configName.empty()) {
            std::cout << "Configuration reloaded successfully\n";
        } else {
            std::cout << "Switched to configuration: " << configName << "\n";
        }
        if (!respData.empty()) {
            std::cout << respData << "\n";
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to reload configuration" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute stop command
int cmdStop(int sock, int timeoutMs) {
    if (!sendMessage(sock, MessageType::CmdStop)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspOk) {
        std::cout << "Engine stopped\n";
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to stop engine" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute start command
int cmdStart(int sock, int timeoutMs) {
    if (!sendMessage(sock, MessageType::CmdStart)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspOk) {
        std::cout << "Engine started\n";
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to start engine" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute status command
/// Output format (human-readable): "Engine: running | Config: work.mayu | Uptime: 2h 15m | Keys: 12,453"
int cmdStatus(int sock, int timeoutMs, bool rawJson) {
    if (!sendMessage(sock, MessageType::CmdGetStatus)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspStatus || respType == MessageType::RspOk) {
        if (respData.empty()) {
            std::cout << "Engine is running\n";
            return SUCCESS;
        }

        if (rawJson) {
            std::cout << respData << "\n";
        } else {
            // Parse JSON and format nicely
            std::string state = jsonGetString(respData, "state");
            std::string config = jsonGetString(respData, "config");
            int64_t uptime = jsonGetInt(respData, "uptime");
            int64_t keyCount = jsonGetInt(respData, "key_count");
            std::string keymap = jsonGetString(respData, "current_keymap");

            // Extract just the config filename
            size_t lastSlash = config.find_last_of('/');
            if (lastSlash != std::string::npos) {
                config = config.substr(lastSlash + 1);
            }
            if (config.empty()) {
                config = "(none)";
            }

            // Format state with capitalized first letter
            if (!state.empty()) {
                state[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(state[0])));
            }

            std::cout << "Engine: " << state
                      << " | Config: " << config
                      << " | Uptime: " << formatUptime(uptime)
                      << " | Keys: " << keyCount;

            if (!keymap.empty()) {
                std::cout << " | Keymap: " << keymap;
            }
            std::cout << "\n";
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to get status" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute config command
/// Shows configuration details: path, name, and when it was loaded
int cmdConfig(int sock, int timeoutMs, bool rawJson) {
    if (!sendMessage(sock, MessageType::CmdGetConfig)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspConfig || respType == MessageType::RspOk) {
        if (respData.empty()) {
            std::cout << "No configuration loaded\n";
            return SUCCESS;
        }

        if (rawJson) {
            std::cout << respData << "\n";
        } else {
            // Parse JSON and format nicely
            std::string configPath = jsonGetString(respData, "config_path");
            std::string configName = jsonGetString(respData, "config_name");
            std::string loadedTime = jsonGetString(respData, "loaded_time");

            std::cout << "Configuration Details:\n";
            std::cout << "  Name:   " << (configName.empty() ? "(none)" : configName) << "\n";
            std::cout << "  Path:   " << (configPath.empty() ? "(none)" : configPath) << "\n";
            std::cout << "  Loaded: " << (loadedTime.empty() ? "(unknown)" : loadedTime) << "\n";
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to get config" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute keymaps command
/// Lists all loaded keymaps with window matching conditions
int cmdKeymaps(int sock, int timeoutMs, bool rawJson) {
    if (!sendMessage(sock, MessageType::CmdGetKeymaps)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspKeymaps || respType == MessageType::RspOk) {
        if (respData.empty()) {
            std::cout << "No keymaps loaded\n";
            return SUCCESS;
        }

        if (rawJson) {
            std::cout << respData << "\n";
        } else {
            std::cout << "Loaded Keymaps:\n";

            size_t offset = 0;
            int count = 0;
            std::string obj;
            while (!(obj = jsonGetArrayObject(respData, "keymaps", offset)).empty()) {
                ++count;
                std::string name = jsonGetString(obj, "name");
                std::string windowClass = jsonGetString(obj, "window_class");
                std::string windowTitle = jsonGetString(obj, "window_title");

                std::cout << "  " << count << ". " << (name.empty() ? "(unnamed)" : name);

                bool hasCondition = false;
                if (!windowClass.empty()) {
                    std::cout << " [class: " << windowClass;
                    hasCondition = true;
                }
                if (!windowTitle.empty()) {
                    if (hasCondition) {
                        std::cout << ", title: " << windowTitle;
                    } else {
                        std::cout << " [title: " << windowTitle;
                        hasCondition = true;
                    }
                }
                if (hasCondition) {
                    std::cout << "]";
                }
                std::cout << "\n";
            }

            if (count == 0) {
                std::cout << "  (no keymaps defined)\n";
            }
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to get keymaps" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute metrics command
/// Shows performance metrics: latency stats and CPU usage
int cmdMetrics(int sock, int timeoutMs, bool rawJson) {
    if (!sendMessage(sock, MessageType::CmdGetMetrics)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspMetrics || respType == MessageType::RspOk) {
        if (respData.empty()) {
            std::cout << "No metrics available\n";
            return SUCCESS;
        }

        if (rawJson) {
            std::cout << respData << "\n";
        } else {
            // Parse JSON and format nicely
            int64_t latencyAvg = jsonGetInt(respData, "latency_avg_ns");
            int64_t latencyP99 = jsonGetInt(respData, "latency_p99_ns");
            int64_t latencyMax = jsonGetInt(respData, "latency_max_ns");
            double cpuPercent = jsonGetDouble(respData, "cpu_usage_percent");
            double keysPerSec = jsonGetDouble(respData, "keys_per_second");

            std::cout << "Performance Metrics:\n";
            std::cout << "  Latency (avg):   " << formatLatency(latencyAvg) << "\n";
            std::cout << "  Latency (p99):   " << formatLatency(latencyP99) << "\n";
            std::cout << "  Latency (max):   " << formatLatency(latencyMax) << "\n";

            // Format CPU with fixed precision
            std::cout << "  CPU usage:       ";
            std::cout << std::fixed;
            std::cout.precision(1);
            std::cout << cpuPercent << "%\n";

            std::cout << "  Keys/second:     ";
            std::cout.precision(1);
            std::cout << keysPerSec << "\n";
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to get metrics" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    const char* socketPath = DEFAULT_SOCKET_PATH;
    int timeoutMs = DEFAULT_TIMEOUT_MS;
    std::string configName;
    bool rawJson = false;

    // Long options
    static struct option longOpts[] = {
        {"config",  required_argument, nullptr, 'c'},
        {"json",    no_argument,       nullptr, 'j'},
        {"socket",  required_argument, nullptr, 's'},
        {"timeout", required_argument, nullptr, 't'},
        {"help",    no_argument,       nullptr, 'h'},
        {nullptr,   0,                 nullptr, 0}
    };

    // Parse options
    int opt;
    while ((opt = getopt_long(argc, argv, "c:js:t:h", longOpts, nullptr)) != -1) {
        switch (opt) {
            case 'c':
                configName = optarg;
                break;
            case 'j':
                rawJson = true;
                break;
            case 's':
                socketPath = optarg;
                break;
            case 't':
                timeoutMs = std::atoi(optarg);
                if (timeoutMs <= 0) {
                    std::cerr << "Error: Invalid timeout value\n";
                    return INVALID_ARGS;
                }
                break;
            case 'h':
                printUsage(argv[0]);
                return SUCCESS;
            default:
                printUsage(argv[0]);
                return INVALID_ARGS;
        }
    }

    // Check for command
    if (optind >= argc) {
        std::cerr << "Error: No command specified\n\n";
        printUsage(argv[0]);
        return INVALID_ARGS;
    }

    std::string command = argv[optind];

    // Validate command
    if (command != "reload" && command != "stop" && command != "start" &&
        command != "status" && command != "config" && command != "keymaps" &&
        command != "metrics") {
        std::cerr << "Error: Unknown command: " << command << "\n\n";
        printUsage(argv[0]);
        return INVALID_ARGS;
    }

    // Connect to engine
    int sock = connectToEngine(socketPath);
    if (sock < 0) {
        return ENGINE_NOT_RUNNING;
    }

    // Execute command
    int result;
    if (command == "reload") {
        result = cmdReload(sock, timeoutMs, configName);
    } else if (command == "stop") {
        result = cmdStop(sock, timeoutMs);
    } else if (command == "start") {
        result = cmdStart(sock, timeoutMs);
    } else if (command == "status") {
        result = cmdStatus(sock, timeoutMs, rawJson);
    } else if (command == "config") {
        result = cmdConfig(sock, timeoutMs, rawJson);
    } else if (command == "keymaps") {
        result = cmdKeymaps(sock, timeoutMs, rawJson);
    } else if (command == "metrics") {
        result = cmdMetrics(sock, timeoutMs, rawJson);
    } else {
        result = INVALID_ARGS;
    }

    close(sock);
    return result;
}
