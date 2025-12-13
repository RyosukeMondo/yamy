//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_metadata.cpp
// Implementation of config metadata storage

#include "config_metadata.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#else
#include <unistd.h>
#include <pwd.h>
#endif
#include <cstring>

// Simple JSON parsing helpers (no external dependencies)
namespace {

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
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

// Extract string value from JSON key
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

// Extract integer value from JSON key
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

// Extract string array from JSON key
bool extractStringArray(const std::string& json, const std::string& key,
                        std::vector<std::string>& values) {
    values.clear();
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return false;

    size_t arrayStart = json.find('[', colonPos + 1);
    if (arrayStart == std::string::npos) return false;

    size_t arrayEnd = json.find(']', arrayStart + 1);
    if (arrayEnd == std::string::npos) return false;

    std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

    size_t pos = 0;
    while (pos < arrayContent.size()) {
        size_t startQuote = arrayContent.find('"', pos);
        if (startQuote == std::string::npos) break;

        size_t endQuote = startQuote + 1;
        while (endQuote < arrayContent.size()) {
            if (arrayContent[endQuote] == '"' && arrayContent[endQuote - 1] != '\\') break;
            ++endQuote;
        }
        if (endQuote >= arrayContent.size()) break;

        values.push_back(unescapeJson(
            arrayContent.substr(startQuote + 1, endQuote - startQuote - 1)));
        pos = endQuote + 1;
    }

    return true;
}

std::string getHomeDir() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    if (home && *home) return home;
    return "C:\\";
#else
    const char* home = getenv("HOME");
    if (home && *home) {
        return home;
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return pw->pw_dir;
    }
    return "/tmp";
#endif
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

std::string getBasename(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    std::string filename = (lastSlash != std::string::npos)
        ? path.substr(lastSlash + 1)
        : path;

    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        return filename.substr(0, lastDot);
    }
    return filename;
}

// Generate a safe filename from config path (handles paths with slashes)
std::string pathToMetadataFilename(const std::string& configPath) {
    std::string result;
    result.reserve(configPath.size());
    for (char c : configPath) {
        if (c == '/') {
            result += '_';
        } else if (c == ' ') {
            result += '-';
        } else if (std::isalnum(c) || c == '.' || c == '-' || c == '_') {
            result += c;
        }
    }
    return result + ".json";
}

} // anonymous namespace

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ConfigMetadataInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ConfigMetadataInfo::ConfigMetadataInfo()
    : createdDate(0)
    , modifiedDate(0)
{
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ConfigMetadata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ConfigMetadata::ConfigMetadata()
{
}

ConfigMetadata::~ConfigMetadata()
{
}

bool ConfigMetadata::load(const std::string& configPath)
{
    std::string metaPath = getMetadataPath(configPath);
    if (!fileExists(metaPath)) {
        // No metadata file - initialize with defaults
        m_info = ConfigMetadataInfo();
        m_info.name = getBasename(configPath);
        m_info.createdDate = std::time(nullptr);
        m_info.modifiedDate = m_info.createdDate;
        return false;
    }

    std::ifstream file(metaPath);
    if (!file.is_open()) {
        m_info = ConfigMetadataInfo();
        m_info.name = getBasename(configPath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return parseJson(buffer.str());
}

bool ConfigMetadata::save(const std::string& configPath)
{
    if (!ensureMetadataDirExists()) {
        return false;
    }

    // Auto-set createdDate if not already set
    if (m_info.createdDate == 0) {
        m_info.createdDate = std::time(nullptr);
    }
    // Also ensure modifiedDate is set
    if (m_info.modifiedDate == 0) {
        m_info.modifiedDate = m_info.createdDate;
    }

    std::string metaPath = getMetadataPath(configPath);
    std::ofstream file(metaPath);
    if (!file.is_open()) {
        return false;
    }

    file << toJson();
    file.close();
    return file.good();
}

bool ConfigMetadata::touch(const std::string& configPath)
{
    m_info.modifiedDate = std::time(nullptr);
    return save(configPath);
}

bool ConfigMetadata::remove(const std::string& configPath)
{
    std::string metaPath = getMetadataPath(configPath);
    if (!fileExists(metaPath)) {
        return true;  // Nothing to remove
    }
    return std::remove(metaPath.c_str()) == 0;
}

bool ConfigMetadata::exists(const std::string& configPath)
{
    return fileExists(getMetadataPath(configPath));
}

std::string ConfigMetadata::getMetadataPath(const std::string& configPath)
{
    return getMetadataDir() + "/" + pathToMetadataFilename(configPath);
}

std::string ConfigMetadata::getMetadataDir()
{
    return getHomeDir() + "/.yamy/.metadata";
}

bool ConfigMetadata::ensureMetadataDirExists()
{
    std::string yamyDir = getHomeDir() + "/.yamy";
    if (!directoryExists(yamyDir)) {
        if (!createDirectory(yamyDir)) {
            return false;
        }
    }

    std::string metaDir = getMetadataDir();
    if (!directoryExists(metaDir)) {
        if (!createDirectory(metaDir)) {
            return false;
        }
    }

    return true;
}

void ConfigMetadata::setName(const std::string& name)
{
    m_info.name = name;
    m_info.modifiedDate = std::time(nullptr);
}

void ConfigMetadata::setDescription(const std::string& description)
{
    m_info.description = description;
    m_info.modifiedDate = std::time(nullptr);
}

void ConfigMetadata::setAuthor(const std::string& author)
{
    m_info.author = author;
    m_info.modifiedDate = std::time(nullptr);
}

void ConfigMetadata::addTag(const std::string& tag)
{
    if (std::find(m_info.tags.begin(), m_info.tags.end(), tag) == m_info.tags.end()) {
        m_info.tags.push_back(tag);
        m_info.modifiedDate = std::time(nullptr);
    }
}

void ConfigMetadata::removeTag(const std::string& tag)
{
    auto it = std::find(m_info.tags.begin(), m_info.tags.end(), tag);
    if (it != m_info.tags.end()) {
        m_info.tags.erase(it);
        m_info.modifiedDate = std::time(nullptr);
    }
}

void ConfigMetadata::clearTags()
{
    if (!m_info.tags.empty()) {
        m_info.tags.clear();
        m_info.modifiedDate = std::time(nullptr);
    }
}

bool ConfigMetadata::parseJson(const std::string& jsonContent)
{
    m_info = ConfigMetadataInfo();

    extractString(jsonContent, "name", m_info.name);
    extractString(jsonContent, "description", m_info.description);
    extractString(jsonContent, "author", m_info.author);

    int64_t created = 0, modified = 0;
    if (extractInt(jsonContent, "createdDate", created)) {
        m_info.createdDate = static_cast<std::time_t>(created);
    }
    if (extractInt(jsonContent, "modifiedDate", modified)) {
        m_info.modifiedDate = static_cast<std::time_t>(modified);
    }

    extractStringArray(jsonContent, "tags", m_info.tags);

    return true;
}

std::string ConfigMetadata::toJson() const
{
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"name\": \"" << escapeJson(m_info.name) << "\",\n";
    oss << "  \"description\": \"" << escapeJson(m_info.description) << "\",\n";
    oss << "  \"author\": \"" << escapeJson(m_info.author) << "\",\n";
    oss << "  \"createdDate\": " << static_cast<int64_t>(m_info.createdDate) << ",\n";
    oss << "  \"modifiedDate\": " << static_cast<int64_t>(m_info.modifiedDate) << ",\n";
    oss << "  \"tags\": [";

    for (size_t i = 0; i < m_info.tags.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << escapeJson(m_info.tags[i]) << "\"";
    }

    oss << "]\n";
    oss << "}\n";

    return oss.str();
}
