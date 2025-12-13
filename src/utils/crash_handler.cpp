//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// crash_handler.cpp - Crash reporting infrastructure implementation
//
// Uses only async-signal-safe functions inside signal handlers per POSIX.
// See: signal-safety(7) for list of async-signal-safe functions.

#include "crash_handler.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <dirent.h>
#include <pwd.h>
#include <execinfo.h>

namespace yamy {

namespace {

// Static storage for async-signal-safe access
// These must be set before signals can occur
constexpr size_t MAX_VERSION_LEN = 64;
constexpr size_t MAX_CONFIG_PATH_LEN = 512;
constexpr size_t MAX_PATH_LEN = 512;
constexpr size_t MAX_BACKTRACE_DEPTH = 64;

char g_version[MAX_VERSION_LEN] = "unknown";
char g_configPath[MAX_CONFIG_PATH_LEN] = "";
char g_crashDir[MAX_PATH_LEN] = "";
bool g_installed = false;

// Original signal handlers (to chain to after report generation)
struct sigaction g_oldSigsegv;
struct sigaction g_oldSigabrt;
struct sigaction g_oldSigfpe;
struct sigaction g_oldSigill;
struct sigaction g_oldSigbus;

// Async-signal-safe string length
size_t safeStrlen(const char* str) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len]) ++len;
    return len;
}

// Async-signal-safe string copy
void safeStrcpy(char* dest, const char* src, size_t maxLen) {
    if (!dest || !src || maxLen == 0) return;
    size_t i = 0;
    while (i < maxLen - 1 && src[i]) {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

// Async-signal-safe integer to string (for positive integers)
// Returns number of characters written (not including null terminator)
int safeItoa(int value, char* buf, size_t bufSize) {
    if (!buf || bufSize < 2) return 0;

    if (value < 0) {
        buf[0] = '-';
        int written = safeItoa(-value, buf + 1, bufSize - 1);
        return written > 0 ? written + 1 : 0;
    }

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }

    // Count digits
    int temp = value;
    int digits = 0;
    while (temp > 0) {
        ++digits;
        temp /= 10;
    }

    if (static_cast<size_t>(digits) >= bufSize) return 0;

    buf[digits] = '\0';
    int pos = digits - 1;
    while (value > 0) {
        buf[pos--] = '0' + (value % 10);
        value /= 10;
    }

    return digits;
}

// Async-signal-safe pointer to hex string
int safePtrToHex(void* ptr, char* buf, size_t bufSize) {
    if (!buf || bufSize < 19) return 0;  // "0x" + 16 hex digits + null

    static const char hexDigits[] = "0123456789abcdef";
    unsigned long value = reinterpret_cast<unsigned long>(ptr);

    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 15; i >= 0; --i) {
        buf[2 + (15 - i)] = hexDigits[(value >> (i * 4)) & 0xF];
    }
    buf[18] = '\0';

    return 18;
}

// Get signal name (async-signal-safe)
const char* getSignalName(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (Segmentation fault)";
        case SIGABRT: return "SIGABRT (Aborted)";
        case SIGFPE:  return "SIGFPE (Floating point exception)";
        case SIGILL:  return "SIGILL (Illegal instruction)";
        case SIGBUS:  return "SIGBUS (Bus error)";
        default:      return "Unknown signal";
    }
}

// Get SI_CODE description for SIGSEGV
const char* getSigsegvCodeDesc(int code) {
    switch (code) {
        case SEGV_MAPERR: return "Address not mapped to object";
        case SEGV_ACCERR: return "Invalid permissions for mapped object";
        default:          return "Unknown";
    }
}

// Get home directory (non-signal-safe, use only during init)
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

// Get data directory (non-signal-safe, use only during init)
std::string getDataDir() {
    const char* xdgData = getenv("XDG_DATA_HOME");
    if (xdgData && *xdgData) {
        return std::string(xdgData) + "/yamy";
    }
    return getHomeDir() + "/.local/share/yamy";
}

// Check if directory exists (non-signal-safe)
bool directoryExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// Create directory recursively (non-signal-safe)
bool createDirectoryRecursive(const std::string& path) {
    if (path.empty()) return false;

    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        current += path[i];
        if (path[i] == '/' || i == path.size() - 1) {
            if (!current.empty() && current != "/") {
                if (!directoryExists(current)) {
                    if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

// Get original handler for signal
struct sigaction* getOriginalHandler(int sig) {
    switch (sig) {
        case SIGSEGV: return &g_oldSigsegv;
        case SIGABRT: return &g_oldSigabrt;
        case SIGFPE:  return &g_oldSigfpe;
        case SIGILL:  return &g_oldSigill;
        case SIGBUS:  return &g_oldSigbus;
        default:      return nullptr;
    }
}

} // anonymous namespace

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CrashHandler implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void CrashHandler::install() {
    if (g_installed) return;

    // Initialize crash directory path
    std::string crashDir = getCrashDir();
    safeStrcpy(g_crashDir, crashDir.c_str(), MAX_PATH_LEN);

    // Ensure crash directory exists
    ensureCrashDirExists();

    // Set up signal handler structure
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;  // One-shot handler with siginfo
    sigemptyset(&sa.sa_mask);

    // Install handlers for crash signals
    sigaction(SIGSEGV, &sa, &g_oldSigsegv);
    sigaction(SIGABRT, &sa, &g_oldSigabrt);
    sigaction(SIGFPE, &sa, &g_oldSigfpe);
    sigaction(SIGILL, &sa, &g_oldSigill);
    sigaction(SIGBUS, &sa, &g_oldSigbus);

    g_installed = true;
}

void CrashHandler::uninstall() {
    if (!g_installed) return;

    // Restore original handlers
    sigaction(SIGSEGV, &g_oldSigsegv, nullptr);
    sigaction(SIGABRT, &g_oldSigabrt, nullptr);
    sigaction(SIGFPE, &g_oldSigfpe, nullptr);
    sigaction(SIGILL, &g_oldSigill, nullptr);
    sigaction(SIGBUS, &g_oldSigbus, nullptr);

    g_installed = false;
}

void CrashHandler::setVersion(const std::string& version) {
    safeStrcpy(g_version, version.c_str(), MAX_VERSION_LEN);
}

void CrashHandler::setConfigPath(const std::string& configPath) {
    safeStrcpy(g_configPath, configPath.c_str(), MAX_CONFIG_PATH_LEN);
}

std::string CrashHandler::getCrashDir() {
    return getDataDir() + "/crashes";
}

bool CrashHandler::ensureCrashDirExists() {
    return createDirectoryRecursive(getCrashDir());
}

bool CrashHandler::hasCrashReports() {
    return !getCrashReports().empty();
}

std::vector<std::string> CrashHandler::getCrashReports() {
    std::vector<std::string> reports;
    std::string crashDir = getCrashDir();

    DIR* dir = opendir(crashDir.c_str());
    if (!dir) {
        return reports;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        // Look for crash_*.txt files
        if (filename.length() > 10 &&
            filename.substr(0, 6) == "crash_" &&
            filename.substr(filename.length() - 4) == ".txt") {
            reports.push_back(crashDir + "/" + filename);
        }
    }

    closedir(dir);
    return reports;
}

bool CrashHandler::deleteCrashReport(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

int CrashHandler::deleteAllCrashReports() {
    std::vector<std::string> reports = getCrashReports();
    int deleted = 0;
    for (const auto& path : reports) {
        if (deleteCrashReport(path)) {
            ++deleted;
        }
    }
    return deleted;
}

void CrashHandler::signalHandler(int sig, siginfo_t* info, void* context) {
    // Write crash report using async-signal-safe operations
    writeCrashReport(sig, info, context);

    // Re-raise signal with default handler to generate core dump
    struct sigaction* old = getOriginalHandler(sig);
    if (old && old->sa_handler != SIG_DFL && old->sa_handler != SIG_IGN) {
        // Chain to original handler
        if (old->sa_flags & SA_SIGINFO) {
            old->sa_sigaction(sig, info, context);
        } else {
            old->sa_handler(sig);
        }
    } else {
        // Restore default handler and re-raise
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigaction(sig, &sa, nullptr);
        raise(sig);
    }
}

void CrashHandler::writeCrashReport(int sig, siginfo_t* info, void* context) {
    // Build crash report filename with timestamp
    // Using time() which is async-signal-safe on Linux
    time_t now = time(nullptr);

    char filename[MAX_PATH_LEN];
    char timestamp[32];
    safeItoa(static_cast<int>(now), timestamp, sizeof(timestamp));

    safeStrcpy(filename, g_crashDir, MAX_PATH_LEN);
    size_t len = safeStrlen(filename);
    if (len > 0 && filename[len - 1] != '/') {
        filename[len++] = '/';
        filename[len] = '\0';
    }
    safeStrcpy(filename + len, "crash_", MAX_PATH_LEN - len);
    len = safeStrlen(filename);
    safeStrcpy(filename + len, timestamp, MAX_PATH_LEN - len);
    len = safeStrlen(filename);
    safeStrcpy(filename + len, ".txt", MAX_PATH_LEN - len);

    // Open file for writing
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        // Try to write to stderr instead
        fd = STDERR_FILENO;
    }

    // Use writev for efficient async-signal-safe writing
    // Build iovec array for all parts of the report

    // Static buffers for async-signal-safe operations
    char addrBuf[32];
    char pidBuf[16];
    char sigNumBuf[16];

    safeItoa(sig, sigNumBuf, sizeof(sigNumBuf));
    safeItoa(getpid(), pidBuf, sizeof(pidBuf));
    if (info) {
        safePtrToHex(info->si_addr, addrBuf, sizeof(addrBuf));
    } else {
        safeStrcpy(addrBuf, "(unknown)", sizeof(addrBuf));
    }

    // Write header
    struct iovec iov[40];
    int iovcnt = 0;

    #define ADD_IOV_STR(s) do { \
        iov[iovcnt].iov_base = const_cast<char*>(s); \
        iov[iovcnt].iov_len = safeStrlen(s); \
        ++iovcnt; \
    } while(0)

    ADD_IOV_STR("=== YAMY Crash Report ===\n\n");
    ADD_IOV_STR("Version: ");
    ADD_IOV_STR(g_version);
    ADD_IOV_STR("\n");

    ADD_IOV_STR("Signal: ");
    ADD_IOV_STR(sigNumBuf);
    ADD_IOV_STR(" (");
    ADD_IOV_STR(getSignalName(sig));
    ADD_IOV_STR(")\n");

    ADD_IOV_STR("PID: ");
    ADD_IOV_STR(pidBuf);
    ADD_IOV_STR("\n");

    if (info) {
        ADD_IOV_STR("Fault address: ");
        ADD_IOV_STR(addrBuf);
        ADD_IOV_STR("\n");

        if (sig == SIGSEGV) {
            ADD_IOV_STR("Fault reason: ");
            ADD_IOV_STR(getSigsegvCodeDesc(info->si_code));
            ADD_IOV_STR("\n");
        }
    }

    if (g_configPath[0]) {
        ADD_IOV_STR("Config: ");
        ADD_IOV_STR(g_configPath);
        ADD_IOV_STR("\n");
    }

    ADD_IOV_STR("\n");

    // Write what we have so far
    // Note: return values ignored in signal handler (async-signal-safe context)
    (void)writev(fd, iov, iovcnt);

    // Get backtrace
    void* btBuffer[MAX_BACKTRACE_DEPTH];
    int btSize = backtrace(btBuffer, MAX_BACKTRACE_DEPTH);

    // Write backtrace header
    const char* btHeader = "=== Backtrace ===\n";
    (void)write(fd, btHeader, safeStrlen(btHeader));

    // Get backtrace symbols (note: backtrace_symbols_fd is safer for signal handlers)
    // It writes directly to the file descriptor
    backtrace_symbols_fd(btBuffer, btSize, fd);

    // Write footer
    const char* footer = "\n=== End of Crash Report ===\n";
    (void)write(fd, footer, safeStrlen(footer));

    #undef ADD_IOV_STR

    // Close file (if not stderr)
    if (fd != STDERR_FILENO) {
        close(fd);
    }

    // Also write a brief message to stderr
    const char* stderrMsg = "\n*** YAMY crashed. Report saved to: ";
    (void)write(STDERR_FILENO, stderrMsg, safeStrlen(stderrMsg));
    (void)write(STDERR_FILENO, filename, safeStrlen(filename));
    (void)write(STDERR_FILENO, " ***\n", 5);
}

} // namespace yamy
