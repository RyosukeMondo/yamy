#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace yamy::settings {

class ConfigBackup {
public:
    bool createBackup(const std::string& configPath);
    std::vector<std::string> listBackups(const std::string& configPath);
    bool restoreBackup(const std::string& backupPath);
    void pruneBackups(const std::string& configPath, size_t maxCount);
};

} // namespace yamy::settings
