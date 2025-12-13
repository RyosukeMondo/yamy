#pragma once

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

namespace yamy {
namespace debug {

class DebugConsole {
private:
    static bool s_consoleAllocated;
    static std::ofstream s_logFile;
    static bool s_fileLoggingEnabled;
    static std::string s_logPath;

public:
    // Enable debug console window
    static void AllocateConsole() {
        if (s_consoleAllocated) return;

        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();

        s_consoleAllocated = true;
        Log("Debug console allocated");
    }

    // Enable file logging
    static void EnableFileLogging(const std::string& logPath = "") {
        if (s_fileLoggingEnabled) return;

        if (logPath.empty()) {
            // Use AppData/Local/YAMY/yamy.log
            char appData[MAX_PATH];
            if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData) == S_OK) {
                s_logPath = std::string(appData) + "\\YAMY";
                CreateDirectoryA(s_logPath.c_str(), NULL);
                s_logPath += "\\yamy.log";
            } else {
                s_logPath = "yamy.log";
            }
        } else {
            s_logPath = logPath;
        }

        s_logFile.open(s_logPath, std::ios::out | std::ios::app);
        if (s_logFile.is_open()) {
            s_fileLoggingEnabled = true;
            Log("=== YAMY Started ===");
            Log("Log file: " + s_logPath);
        }
    }

    // Log message to console and/or file
    static void Log(const std::string& message) {
        std::string timestamp = GetTimestamp();
        std::string logMsg = "[" + timestamp + "] " + message;

        if (s_consoleAllocated) {
            std::cout << logMsg << std::endl;
            std::cout.flush(); // Force immediate flush
        }

        if (s_fileLoggingEnabled && s_logFile.is_open()) {
            s_logFile << logMsg << std::endl;
            s_logFile.flush();
        }

        OutputDebugStringA((logMsg + "\n").c_str());
    }

    // Log error with ERROR prefix
    static void LogError(const std::string& message) {
        Log("ERROR: " + message);
    }

    // Log warning
    static void LogWarning(const std::string& message) {
        Log("WARNING: " + message);
    }

    // Log info
    static void LogInfo(const std::string& message) {
        Log("INFO: " + message);
    }

    // Show critical error dialog and log
    static void CriticalError(const std::string& message) {
        LogError("CRITICAL: " + message);
        MessageBoxA(NULL, message.c_str(), "YAMY Critical Error", MB_OK | MB_ICONERROR);
    }

    // Close console and log file
    static void Shutdown() {
        Log("=== YAMY Shutting Down ===");

        if (s_fileLoggingEnabled && s_logFile.is_open()) {
            s_logFile.close();
            s_fileLoggingEnabled = false;
        }

        if (s_consoleAllocated) {
            FreeConsole();
            s_consoleAllocated = false;
        }
    }

    static std::string GetLogPath() {
        return s_logPath;
    }

private:
    static std::string GetTimestamp() {
        time_t now = time(nullptr);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }
};

// Static member initialization (inline to avoid multiple definition errors)
inline bool DebugConsole::s_consoleAllocated = false;
inline std::ofstream DebugConsole::s_logFile;
inline bool DebugConsole::s_fileLoggingEnabled = false;
inline std::string DebugConsole::s_logPath;

} // namespace debug
} // namespace yamy

#endif // _WIN32
