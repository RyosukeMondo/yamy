//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// session_manager.cpp
// Implementation of session state persistence

#include "session_manager.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <cerrno>
#include <cstring>

namespace yamy {

namespace {

std::string getHomeDir() {
    const char* home = getenv("HOME");
    if (home && *home) {
        return home;
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return pw->pw_dir;
    }
    return "/tmp";
}

bool directoryExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string escapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 2);
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string unescapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[i + 1]) {
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

bool extractString(const std::string& json, const std::string& key,
                   std::string& value) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return false;

    size_t startQuote = json.find('"', colonPos + 1);
    if (startQuote == std::string::npos) return false;

    size_t endQuote = startQuote + 1;
    while (endQuote < json.size()) {
        if (json[endQuote] == '"' && json[endQuote - 1] != '\\') break;
        ++endQuote;
    }
    if (endQuote >= json.size()) return false;

    value = unescapeJson(json.substr(startQuote + 1, endQuote - startQuote - 1));
    return true;
}

bool extractInt(const std::string& json, const std::string& key, int64_t& value) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return false;

    size_t start = colonPos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) {
        ++start;
    }
    if (start >= json.size()) return false;

    size_t end = start;
    if (json[end] == '-') ++end;
    while (end < json.size() && std::isdigit(json[end])) ++end;

    if (end == start) return false;

    try {
        value = std::stoll(json.substr(start, end - start));
        return true;
    } catch (...) {
        return false;
    }
}

bool extractBool(const std::string& json, const std::string& key, bool& value) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return false;

    size_t start = colonPos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) {
        ++start;
    }

    if (json.compare(start, 4, "true") == 0) {
        value = true;
        return true;
    }
    if (json.compare(start, 5, "false") == 0) {
        value = false;
        return true;
    }
    return false;
}

// Extract a JSON object block starting after the key
std::string extractObject(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return "";

    size_t braceStart = json.find('{', colonPos + 1);
    if (braceStart == std::string::npos) return "";

    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < json.size() && depth > 0) {
        if (json[pos] == '{') ++depth;
        else if (json[pos] == '}') --depth;
        ++pos;
    }

    if (depth != 0) return "";
    return json.substr(braceStart, pos - braceStart);
}

// Parse window positions object
void parseWindowPositions(const std::string& json,
                          std::map<std::string, WindowPosition>& positions) {
    positions.clear();

    std::string objStr = extractObject(json, "windowPositions");
    if (objStr.empty()) return;

    // Find each window entry (simple parser - expects format:
    // "windowName": { "x": N, "y": N, "width": N, "height": N }
    size_t pos = 0;
    while (pos < objStr.size()) {
        size_t nameStart = objStr.find('"', pos);
        if (nameStart == std::string::npos) break;

        size_t nameEnd = objStr.find('"', nameStart + 1);
        if (nameEnd == std::string::npos) break;

        std::string windowName = objStr.substr(nameStart + 1, nameEnd - nameStart - 1);

        // Find the object for this window
        size_t objStart = objStr.find('{', nameEnd);
        if (objStart == std::string::npos) break;

        size_t objEnd = objStr.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string windowObj = objStr.substr(objStart, objEnd - objStart + 1);

        WindowPosition wp;
        int64_t x = 0, y = 0, w = 0, h = 0;
        if (extractInt(windowObj, "x", x) &&
            extractInt(windowObj, "y", y) &&
            extractInt(windowObj, "width", w) &&
            extractInt(windowObj, "height", h)) {
            wp.x = static_cast<int>(x);
            wp.y = static_cast<int>(y);
            wp.width = static_cast<int>(w);
            wp.height = static_cast<int>(h);
            wp.valid = true;
            positions[windowName] = wp;
        }

        pos = objEnd + 1;
    }
}

}  // anonymous namespace

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SessionManager
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager() {
}

SessionManager::~SessionManager() {
}

std::string SessionManager::getConfigDir() {
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    if (xdgConfig && *xdgConfig) {
        return std::string(xdgConfig) + "/yamy";
    }
    return getHomeDir() + "/.config/yamy";
}

std::string SessionManager::getSessionPath() {
    return getConfigDir() + "/session.json";
}

bool SessionManager::ensureConfigDirExists() const {
    std::string configDir = getConfigDir();

    // Create parent .config if needed
    std::string parentDir = configDir.substr(0, configDir.rfind('/'));
    if (!directoryExists(parentDir)) {
        if (!createDirectory(parentDir)) {
            return false;
        }
    }

    // Create yamy config dir
    if (!directoryExists(configDir)) {
        if (!createDirectory(configDir)) {
            return false;
        }
    }

    return true;
}

bool SessionManager::saveSession() {
    if (!ensureConfigDirExists()) {
        return false;
    }

    m_data.savedTimestamp = static_cast<int64_t>(std::time(nullptr));

    std::string sessionPath = getSessionPath();
    std::ofstream file(sessionPath);
    if (!file.is_open()) {
        return false;
    }

    file << toJson();
    file.close();
    return file.good();
}

bool SessionManager::restoreSession() {
    std::string sessionPath = getSessionPath();
    if (!fileExists(sessionPath)) {
        return false;
    }

    std::ifstream file(sessionPath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    if (!parseJson(buffer.str())) {
        return false;
    }

    return validateSession();
}

bool SessionManager::hasSession() const {
    return fileExists(getSessionPath());
}

bool SessionManager::clearSession() {
    std::string sessionPath = getSessionPath();
    if (!fileExists(sessionPath)) {
        return true;  // Nothing to clear
    }

    if (std::remove(sessionPath.c_str()) != 0) {
        return false;
    }

    m_data = SessionData();
    return true;
}

void SessionManager::setActiveConfig(const std::string& configPath) {
    m_data.activeConfigPath = configPath;
}

void SessionManager::setEngineRunning(bool running) {
    m_data.engineWasRunning = running;
}

void SessionManager::saveWindowPosition(const std::string& windowName,
                                         int x, int y, int width, int height) {
    WindowPosition wp;
    wp.x = x;
    wp.y = y;
    wp.width = width;
    wp.height = height;
    wp.valid = true;
    m_data.windowPositions[windowName] = wp;
}

WindowPosition SessionManager::getWindowPosition(const std::string& windowName) const {
    auto it = m_data.windowPositions.find(windowName);
    if (it != m_data.windowPositions.end()) {
        return it->second;
    }
    return WindowPosition();
}

bool SessionManager::parseJson(const std::string& jsonContent) {
    m_data = SessionData();

    extractString(jsonContent, "activeConfigPath", m_data.activeConfigPath);
    extractBool(jsonContent, "engineWasRunning", m_data.engineWasRunning);

    int64_t timestamp = 0;
    if (extractInt(jsonContent, "savedTimestamp", timestamp)) {
        m_data.savedTimestamp = timestamp;
    }

    parseWindowPositions(jsonContent, m_data.windowPositions);

    return true;
}

std::string SessionManager::toJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"activeConfigPath\": \"" << escapeJson(m_data.activeConfigPath) << "\",\n";
    oss << "  \"engineWasRunning\": " << (m_data.engineWasRunning ? "true" : "false") << ",\n";
    oss << "  \"savedTimestamp\": " << m_data.savedTimestamp << ",\n";
    oss << "  \"windowPositions\": {";

    bool first = true;
    for (const auto& entry : m_data.windowPositions) {
        if (!entry.second.valid) continue;

        if (!first) oss << ",";
        first = false;

        oss << "\n    \"" << escapeJson(entry.first) << "\": {"
            << "\n      \"x\": " << entry.second.x << ","
            << "\n      \"y\": " << entry.second.y << ","
            << "\n      \"width\": " << entry.second.width << ","
            << "\n      \"height\": " << entry.second.height
            << "\n    }";
    }

    if (!m_data.windowPositions.empty()) {
        oss << "\n  ";
    }
    oss << "}\n";
    oss << "}\n";

    return oss.str();
}

std::string SessionManager::getAutoStartPath() {
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    if (xdgConfig && *xdgConfig) {
        return std::string(xdgConfig) + "/autostart";
    }
    return getHomeDir() + "/.config/autostart";
}

std::string SessionManager::getAutoStartFilePath() {
    return getAutoStartPath() + "/yamy.desktop";
}

namespace {

std::string getExecutablePath() {
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return path;
    }
    // Fallback: try to find yamy in PATH or use current working directory
    return "yamy";
}

bool isValidDesktopEntry(const std::string& content) {
    // Verify it's a valid .desktop file for yamy
    return content.find("[Desktop Entry]") != std::string::npos &&
           content.find("Type=Application") != std::string::npos &&
           content.find("Name=YAMY") != std::string::npos &&
           content.find("Exec=") != std::string::npos;
}

}  // anonymous namespace

bool SessionManager::enableAutoStart() {
    std::string autostartDir = getAutoStartPath();
    std::string desktopFilePath = getAutoStartFilePath();

    // Create autostart directory if it doesn't exist
    std::string parentDir = autostartDir.substr(0, autostartDir.rfind('/'));
    if (!directoryExists(parentDir)) {
        if (!createDirectory(parentDir)) {
            return false;
        }
    }

    if (!directoryExists(autostartDir)) {
        if (!createDirectory(autostartDir)) {
            return false;
        }
    }

    // Get the absolute path to the yamy executable
    std::string execPath = getExecutablePath();

    // Create the .desktop file
    std::ofstream desktopFile(desktopFilePath);
    if (!desktopFile.is_open()) {
        return false;
    }

    desktopFile << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Name=YAMY\n"
                << "GenericName=Keyboard Remapper\n"
                << "Comment=Keyboard remapping utility\n"
                << "Exec=" << execPath << "\n"
                << "Icon=yamy\n"
                << "Terminal=false\n"
                << "Categories=Utility;System;\n"
                << "X-GNOME-Autostart-enabled=true\n";

    desktopFile.close();
    return desktopFile.good();
}

bool SessionManager::disableAutoStart() {
    std::string desktopFilePath = getAutoStartFilePath();

    if (!fileExists(desktopFilePath)) {
        return true;  // Already disabled
    }

    if (std::remove(desktopFilePath.c_str()) != 0) {
        return false;
    }

    return true;
}

bool SessionManager::isAutoStartEnabled() const {
    std::string desktopFilePath = getAutoStartFilePath();

    if (!fileExists(desktopFilePath)) {
        return false;
    }

    // Read and validate the desktop file
    std::ifstream file(desktopFilePath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string content = buffer.str();

    // Check if it's a valid yamy desktop entry
    if (!isValidDesktopEntry(content)) {
        return false;
    }

    // Check if X-GNOME-Autostart-enabled is false
    if (content.find("X-GNOME-Autostart-enabled=false") != std::string::npos) {
        return false;
    }

    return true;
}

bool SessionManager::validateSession() const {
    // Validate timestamp is reasonable (not in the future, not too old)
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    if (m_data.savedTimestamp > now) {
        return false;  // Future timestamp is suspicious
    }

    // Session older than 1 year is considered stale
    constexpr int64_t ONE_YEAR_SECONDS = 365 * 24 * 60 * 60;
    if (now - m_data.savedTimestamp > ONE_YEAR_SECONDS) {
        return false;
    }

    // Validate window positions have reasonable values
    for (const auto& entry : m_data.windowPositions) {
        const auto& wp = entry.second;
        if (!wp.valid) continue;

        // Check for unreasonable values
        if (wp.width < 0 || wp.height < 0) {
            return false;
        }
        if (wp.width > 10000 || wp.height > 10000) {
            return false;  // Unreasonably large
        }
        // x,y can be negative (multi-monitor), but not excessively
        if (wp.x < -10000 || wp.x > 10000 ||
            wp.y < -10000 || wp.y > 10000) {
            return false;
        }
    }

    // Config path validation - if specified, should look like a path
    if (!m_data.activeConfigPath.empty()) {
        // Should start with / or ~
        if (m_data.activeConfigPath[0] != '/' &&
            m_data.activeConfigPath[0] != '~') {
            return false;
        }
    }

    return true;
}

}  // namespace yamy
