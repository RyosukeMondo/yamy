//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// x11_connection.cpp - Centralized X11 display connection management

#include "x11_connection.h"
#include "../../core/platform/platform_exception.h"
#include "../../utils/platform_logger.h"
#include <cstdlib>
#include <cstring>

namespace yamy::platform {

// Thread-local storage for error information
thread_local int X11Connection::s_lastErrorCode = 0;
thread_local char X11Connection::s_lastErrorText[256] = {0};
thread_local X11ErrorGuard* X11ErrorGuard::s_currentGuard = nullptr;

X11Connection& X11Connection::instance() {
    static X11Connection instance;
    return instance;
}

X11Connection::X11Connection()
    : m_display(nullptr)
    , m_connected(false)
{
    initialize();
}

X11Connection::~X11Connection() {
    close();
}

bool X11Connection::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_display) {
        return true;  // Already initialized
    }

    // Get display name from environment
    const char* displayEnv = std::getenv("DISPLAY");
    m_displayName = displayEnv ? displayEnv : "";

    // Set up error handler before opening display
    setupErrorHandler();

    // Attempt to open display
    m_display = XOpenDisplay(displayEnv);

    if (!m_display) {
        m_lastError = "XOpenDisplay failed";
        if (m_displayName.empty()) {
            m_lastError += " (DISPLAY environment variable not set)";
        } else {
            m_lastError += " for display: " + m_displayName;
        }
        PLATFORM_LOG_ERROR("x11", "%s", m_lastError.c_str());
        m_connected = false;
        return false;
    }

    m_connected = true;
    PLATFORM_LOG_INFO("x11", "Connected to display: %s",
                      m_displayName.empty() ? "(default)" : m_displayName.c_str());

    return true;
}

void X11Connection::setupErrorHandler() {
    // Install custom error handlers
    XSetErrorHandler(handleX11Error);
    XSetIOErrorHandler(handleX11IOError);
}

int X11Connection::handleX11Error(Display* display, XErrorEvent* event) {
    s_lastErrorCode = event->error_code;

    // Get error text
    XGetErrorText(display, event->error_code, s_lastErrorText, sizeof(s_lastErrorText));

    // Log the error
    PLATFORM_LOG_WARN("x11", "Protocol error: %s (code: %d, request: %d, minor: %d)",
                      s_lastErrorText, event->error_code,
                      static_cast<int>(event->request_code),
                      static_cast<int>(event->minor_code));

    // If an error guard is active, notify it
    if (X11ErrorGuard::s_currentGuard) {
        X11ErrorGuard::s_currentGuard->m_hadError = true;
        X11ErrorGuard::s_currentGuard->m_errorCode = event->error_code;
    }

    return 0;  // Return 0 to continue (non-fatal)
}

int X11Connection::handleX11IOError(Display* display) {
    (void)display;
    PLATFORM_LOG_ERROR("x11", "Fatal I/O error - connection to X server lost");
    X11Connection::instance().m_connected = false;
    X11Connection::instance().m_lastError = "X server connection lost";

    // This is typically fatal - the default handler calls exit()
    // We log and allow the default behavior
    return 0;
}

Display* X11Connection::getDisplayOrThrow() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_display) {
        throw DisplayConnectionException(m_displayName);
    }

    if (!m_connected) {
        throw DisplayConnectionException(m_displayName);
    }

    return m_display;
}

Display* X11Connection::getDisplayOrNull() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_connected) {
        return nullptr;
    }

    return m_display;
}

bool X11Connection::isConnected() const {
    return m_connected;
}

std::string X11Connection::getLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

void X11Connection::close() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
        m_connected = false;
        PLATFORM_LOG_INFO("x11", "Display connection closed");
    }
}

Atom X11Connection::getAtom(const char* name) {
    Display* display = getDisplayOrNull();
    if (!display) {
        return None;
    }
    return XInternAtom(display, name, False);
}

Window X11Connection::getDefaultRootWindow() {
    Display* display = getDisplayOrNull();
    if (!display) {
        return None;
    }
    return DefaultRootWindow(display);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// X11ErrorGuard Implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

X11ErrorGuard::X11ErrorGuard()
    : m_hadError(false)
    , m_errorCode(0)
    , m_previousHandler(nullptr)
{
    // Store current guard and set ourselves as current
    m_previousHandler = XSetErrorHandler(errorHandler);
    s_currentGuard = this;

    // Sync to ensure any pending errors are processed
    Display* display = X11Connection::instance().getDisplayOrNull();
    if (display) {
        XSync(display, False);
    }
}

X11ErrorGuard::~X11ErrorGuard() {
    // Sync to ensure errors from our scope are caught
    Display* display = X11Connection::instance().getDisplayOrNull();
    if (display) {
        XSync(display, False);
    }

    // Restore previous handler and guard
    XSetErrorHandler(m_previousHandler);
    s_currentGuard = nullptr;
}

int X11ErrorGuard::errorHandler(Display* display, XErrorEvent* event) {
    if (s_currentGuard) {
        s_currentGuard->m_hadError = true;
        s_currentGuard->m_errorCode = event->error_code;
    }

    // Get error text for logging
    char errorText[256];
    XGetErrorText(display, event->error_code, errorText, sizeof(errorText));
    PLATFORM_LOG_WARN("x11", "Error (guarded): %s (code: %d)", errorText, event->error_code);

    return 0;
}

} // namespace yamy::platform
