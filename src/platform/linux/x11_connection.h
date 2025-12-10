#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// x11_connection.h - Centralized X11 display connection management
//
// Provides thread-safe access to a shared X11 Display connection with proper
// error handling and diagnostics.

#include <X11/Xlib.h>
#include <string>
#include <atomic>
#include <mutex>

namespace yamy::platform {

/// Manages a shared X11 display connection with error handling
class X11Connection {
public:
    /// Get the singleton instance
    static X11Connection& instance();

    /// Get the X11 Display, throwing DisplayConnectionException if unavailable.
    /// This is the preferred method for code that cannot handle null displays.
    Display* getDisplayOrThrow();

    /// Get the X11 Display, returning nullptr if unavailable.
    /// Use this for code that can gracefully handle missing display.
    Display* getDisplayOrNull();

    /// Check if display is connected
    bool isConnected() const;

    /// Get the last error message (if any)
    std::string getLastError() const;

    /// Close the display connection (called during shutdown)
    void close();

    /// Get an X11 atom by name (cached for efficiency)
    Atom getAtom(const char* name);

    /// Get the default root window
    Window getDefaultRootWindow();

private:
    X11Connection();
    ~X11Connection();

    // Non-copyable
    X11Connection(const X11Connection&) = delete;
    X11Connection& operator=(const X11Connection&) = delete;

    bool initialize();
    void setupErrorHandler();

    static int handleX11Error(Display* display, XErrorEvent* event);
    static int handleX11IOError(Display* display);

    Display* m_display;
    std::string m_displayName;
    std::string m_lastError;
    std::atomic<bool> m_connected;
    mutable std::mutex m_mutex;

    // Track last X11 error for better diagnostics
    static thread_local int s_lastErrorCode;
    static thread_local char s_lastErrorText[256];
};

/// RAII helper to temporarily ignore X11 errors
class X11ErrorGuard {
    friend class X11Connection;  // Allow X11Connection to access members

public:
    X11ErrorGuard();
    ~X11ErrorGuard();

    /// Check if an error occurred while guard was active
    bool hadError() const { return m_hadError; }

    /// Get the error code (if any)
    int errorCode() const { return m_errorCode; }

private:
    static int errorHandler(Display* display, XErrorEvent* event);

    bool m_hadError;
    int m_errorCode;
    int (*m_previousHandler)(Display*, XErrorEvent*);

    static thread_local X11ErrorGuard* s_currentGuard;
};

} // namespace yamy::platform
