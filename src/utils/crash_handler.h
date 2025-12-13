#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// crash_handler.h - Crash reporting infrastructure for YAMY
//
// Provides crash signal handling and report generation for post-mortem debugging.
// Catches fatal signals (SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS) and generates
// crash reports containing backtraces, version info, and system details.
//
// Reports are saved to ~/.local/share/yamy/crashes/crash_TIMESTAMP.txt
//
// Usage:
//   yamy::CrashHandler::install();  // Call early in main()
//   yamy::CrashHandler::setVersion("0.04");  // Optional: Set version string
//
// Important: Signal handlers use only async-signal-safe functions.

#ifndef _CRASH_HANDLER_H
#define _CRASH_HANDLER_H

#include <string>
#include <vector>
#include <csignal>

#ifdef _WIN32
// Stub for Windows to allow compilation of header
struct siginfo_t; 
#endif

namespace yamy {

/// Crash handler for generating crash reports on fatal signals
class CrashHandler {
public:
    /// Install signal handlers for crash signals
    /// Should be called early in main(), before any other initialization
    static void install();

    /// Uninstall crash signal handlers (restore default handlers)
    static void uninstall();

    /// Set application version string (shown in crash reports)
    /// @param version Version string (e.g., "0.04")
    static void setVersion(const std::string& version);

    /// Set the current config path (shown in crash reports)
    /// @param configPath Path to the active configuration file
    static void setConfigPath(const std::string& configPath);

    /// Get the crash reports directory path
    /// @return Path to ~/.local/share/yamy/crashes/
    static std::string getCrashDir();

    /// Check if any crash reports exist
    /// @return true if one or more crash reports exist
    static bool hasCrashReports();

    /// Get list of crash report file paths
    /// @return Vector of full paths to crash report files
    static std::vector<std::string> getCrashReports();

    /// Delete a crash report file
    /// @param path Full path to crash report file
    /// @return true if deleted successfully
    static bool deleteCrashReport(const std::string& path);

    /// Delete all crash report files
    /// @return Number of files deleted
    static int deleteAllCrashReports();

private:
    CrashHandler() = delete;
    ~CrashHandler() = delete;

    /// Signal handler function (async-signal-safe)
    static void signalHandler(int sig, siginfo_t* info, void* context);

    /// Write crash report to file (async-signal-safe)
    static void writeCrashReport(int sig, siginfo_t* info, void* context);

    /// Ensure crash directory exists
    static bool ensureCrashDirExists();
};

} // namespace yamy

#endif // _CRASH_HANDLER_H
