#include "config_backup.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <vector>

namespace yamy::settings {

namespace fs = std::filesystem;

static fs::path getBackupDir(const fs::path& configPath) {
    return configPath.parent_path() / ".backups";
}

bool ConfigBackup::createBackup(const std::string& configPath) {
    try {
        const auto p = fs::path(configPath);
        if (!fs::exists(p)) {
            return false;
        }

        const auto backupDir = getBackupDir(p);
        fs::create_directories(backupDir);

        const auto now = std::chrono::system_clock::now();
        const auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        #ifdef _WIN32
        localtime_s(&tm, &time_t);
        #else
        localtime_r(&time_t, &tm);
        #endif

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y%m%d%H%M%S");
        
        const auto backupFileName = p.filename().string() + ".bak." + ss.str();
        const auto backupFilePath = backupDir / backupFileName;

        fs::copy_file(p, backupFilePath, fs::copy_options::overwrite_existing);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error in createBackup: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> ConfigBackup::listBackups(const std::string& configPath) {
    std::vector<std::string> backups;
    try {
        const auto p = fs::path(configPath);
        const auto backupDir = getBackupDir(p);

        if (!fs::exists(backupDir) || !fs::is_directory(backupDir)) {
            return backups;
        }

        const auto baseName = p.filename().string();
        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (entry.is_regular_file() && entry.path().filename().string().rfind(baseName + ".bak.", 0) == 0) {
                backups.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error in listBackups: " << e.what() << std::endl;
    }
    return backups;
}

bool ConfigBackup::restoreBackup(const std::string& backupPath) {
    try {
        const auto p = fs::path(backupPath);
        if (!fs::exists(p)) {
            return false;
        }

        const auto backupDir = p.parent_path();
        const auto filename = p.filename().string();
        const auto originalFilename = filename.substr(0, filename.find(".bak."));
        const auto originalConfigPath = backupDir.parent_path() / originalFilename;

        fs::copy_file(p, originalConfigPath, fs::copy_options::overwrite_existing);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error in restoreBackup: " << e.what() << std::endl;
        return false;
    }
}

void ConfigBackup::pruneBackups(const std::string& configPath, size_t maxCount) {
    try {
        auto backups = listBackups(configPath);
        if (backups.size() <= maxCount) {
            return;
        }

        std::sort(backups.begin(), backups.end(), std::greater<std::string>());

        for (size_t i = maxCount; i < backups.size(); ++i) {
            fs::remove(backups[i]);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error in pruneBackups: " << e.what() << std::endl;
    }
}

} // namespace yamy::settings
