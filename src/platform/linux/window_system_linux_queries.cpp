#include "window_system_linux_queries.h"
#include "x11_connection.h"
#include "../../utils/platform_logger.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace yamy::platform {

// ============================================================================
// WindowPropertyCache implementation
// ============================================================================

const WindowPropertyCacheEntry* WindowPropertyCache::get(WindowHandle hwnd) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto key = reinterpret_cast<uintptr_t>(hwnd);
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return nullptr;
    }

    // Check if entry has expired
    auto now = std::chrono::steady_clock::now();
    if (now - it->second.timestamp > kCacheTimeout) {
        return nullptr;  // Expired
    }

    return &it->second;
}

void WindowPropertyCache::set(WindowHandle hwnd, const WindowPropertyCacheEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Evict if cache is too large
    if (cache_.size() >= kMaxCacheEntries) {
        evictExpired();
        // If still too large, remove oldest entries
        if (cache_.size() >= kMaxCacheEntries) {
            auto oldest = cache_.begin();
            for (auto it = cache_.begin(); it != cache_.end(); ++it) {
                if (it->second.timestamp < oldest->second.timestamp) {
                    oldest = it;
                }
            }
            cache_.erase(oldest);
        }
    }

    auto key = reinterpret_cast<uintptr_t>(hwnd);
    cache_[key] = entry;
    cache_[key].timestamp = std::chrono::steady_clock::now();
    cache_[key].valid = true;
}

void WindowPropertyCache::invalidate(WindowHandle hwnd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto key = reinterpret_cast<uintptr_t>(hwnd);
    cache_.erase(key);
}

void WindowPropertyCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
}

void WindowPropertyCache::evictExpired() {
    // Note: caller must hold mutex_
    auto now = std::chrono::steady_clock::now();
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (now - it->second.timestamp > kCacheTimeout) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Helper functions
// ============================================================================

// Helper: Get X11 display via centralized connection manager
static Display* getDisplay() {
    return X11Connection::instance().getDisplayOrNull();
}

// Helper: Get atom by name
static Atom getAtom(const char* name) {
    return X11Connection::instance().getAtom(name);
}

// Helper: Get window property
static bool getWindowProperty(WindowHandle hwnd, Atom property,
                             Atom type, unsigned char** data,
                             unsigned long* items) {
    Display* display = getDisplay();
    if (!display) return false;

    Window window = reinterpret_cast<Window>(hwnd);

    Atom actual_type;
    int actual_format;
    unsigned long bytes_after;

    int status = XGetWindowProperty(display, window, property, 0, (~0L),
                                    False, type, &actual_type, &actual_format,
                                    items, &bytes_after, data);

    return (status == Success && *items > 0);
}

// Helper: Fetch window text (UTF-8 or fallback)
static std::string fetchWindowText(WindowHandle hwnd) {
    Display* display = getDisplay();
    if (!display || !hwnd) return "";

    Window window = reinterpret_cast<Window>(hwnd);

    // Try UTF-8 name first (_NET_WM_NAME)
    Atom netWmName = getAtom("_NET_WM_NAME");
    Atom utf8String = getAtom("UTF8_STRING");
    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(hwnd, netWmName, utf8String, &data, &items)) {
        std::string result(reinterpret_cast<char*>(data));
        XFree(data);
        return result;
    }

    // Fallback to WM_NAME
    char* name = nullptr;
    if (XFetchName(display, window, &name)) {
        std::string result(name);
        XFree(name);
        return result;
    }

    return "";
}

// Helper: Fetch window class name
static std::string fetchClassName(WindowHandle hwnd) {
    Display* display = getDisplay();
    if (!display || !hwnd) return "";

    Window window = reinterpret_cast<Window>(hwnd);

    XClassHint class_hint;
    if (XGetClassHint(display, window, &class_hint)) {
        std::string result;
        if (class_hint.res_class) {
            result = class_hint.res_class;
        } else if (class_hint.res_name) {
            result = class_hint.res_name;
        }

        if (class_hint.res_name) XFree(class_hint.res_name);
        if (class_hint.res_class) XFree(class_hint.res_class);

        return result;
    }

    return "";
}

// Helper: Fetch process ID
static uint32_t fetchProcessId(WindowHandle hwnd) {
    if (!hwnd) return 0;

    Atom netWmPid = getAtom("_NET_WM_PID");
    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(hwnd, netWmPid, XA_CARDINAL, &data, &items)) {
        uint32_t pid = *reinterpret_cast<uint32_t*>(data);
        XFree(data);
        return pid;
    }

    return 0;
}

WindowSystemLinuxQueries::WindowSystemLinuxQueries() {
    // X11 connection is managed by X11Connection singleton
    // Check connection at construction time to fail early
    if (!X11Connection::instance().isConnected()) {
        PLATFORM_LOG_WARN("window", "X11 connection not available during WindowSystemLinuxQueries init");
    } else {
        PLATFORM_LOG_DEBUG("window", "WindowSystemLinuxQueries initialized");
    }
}

WindowSystemLinuxQueries::~WindowSystemLinuxQueries() {
    // Don't close display (it's static)
}

WindowHandle WindowSystemLinuxQueries::getForegroundWindow() {
    Display* display = getDisplay();
    if (!display) {
        PLATFORM_LOG_DEBUG("window", "getForegroundWindow: no display");
        return nullptr;
    }

    // Method 1: Try _NET_ACTIVE_WINDOW
    Atom netActiveWindow = getAtom("_NET_ACTIVE_WINDOW");
    Window root = DefaultRootWindow(display);

    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(reinterpret_cast<WindowHandle>(root),
                         netActiveWindow, XA_WINDOW,
                         &data, &items)) {
        Window* active = reinterpret_cast<Window*>(data);
        Window result = *active;
        XFree(data);
        PLATFORM_LOG_DEBUG("window", "getForegroundWindow: 0x%lx (via _NET_ACTIVE_WINDOW)", result);
        return reinterpret_cast<WindowHandle>(result);
    }

    // Method 2: Fallback to XGetInputFocus
    Window focus;
    int revert_to;
    XGetInputFocus(display, &focus, &revert_to);

    PLATFORM_LOG_DEBUG("window", "getForegroundWindow: 0x%lx (via XGetInputFocus fallback)", focus);
    return reinterpret_cast<WindowHandle>(focus);
}

WindowHandle WindowSystemLinuxQueries::windowFromPoint(const Point& pt) {
    Display* display = getDisplay();
    if (!display) {
        PLATFORM_LOG_DEBUG("window", "windowFromPoint(%d,%d): no display", pt.x, pt.y);
        return nullptr;
    }

    Window root = DefaultRootWindow(display);
    Window child = root;
    int root_x = pt.x, root_y = pt.y;
    int win_x, win_y;
    unsigned int mask;

    // Query pointer to find window at position
    if (XQueryPointer(display, root, &root, &child,
                     &root_x, &root_y, &win_x, &win_y, &mask)) {
        // Traverse to find deepest window at position
        while (child != None) {
            Window temp_child;
            XTranslateCoordinates(display, root, child,
                                 pt.x, pt.y, &win_x, &win_y, &temp_child);
            if (temp_child == None) break;
            child = temp_child;
        }

        PLATFORM_LOG_DEBUG("window", "windowFromPoint(%d,%d): 0x%lx", pt.x, pt.y, child);
        return reinterpret_cast<WindowHandle>(child);
    }

    PLATFORM_LOG_DEBUG("window", "windowFromPoint(%d,%d): not found", pt.x, pt.y);
    return nullptr;
}

void WindowSystemLinuxQueries::fetchAndCacheProperties(WindowHandle hwnd) {
    WindowPropertyCacheEntry entry;
    entry.windowText = fetchWindowText(hwnd);
    entry.className = fetchClassName(hwnd);
    entry.processId = fetchProcessId(hwnd);
    cache_.set(hwnd, entry);

    Window window = reinterpret_cast<Window>(hwnd);
    PLATFORM_LOG_DEBUG("window", "fetchAndCacheProperties(0x%lx): text='%s', class='%s', pid=%u",
                       window, entry.windowText.c_str(), entry.className.c_str(), entry.processId);
}

std::string WindowSystemLinuxQueries::getWindowText(WindowHandle hwnd) {
    if (!hwnd) {
        PLATFORM_LOG_DEBUG("window", "getWindowText: null handle");
        return "";
    }

    Window window = reinterpret_cast<Window>(hwnd);

    // Check cache first
    const WindowPropertyCacheEntry* cached = cache_.get(hwnd);
    if (cached) {
        PLATFORM_LOG_DEBUG("window", "getWindowText(0x%lx): '%s' (cached)", window, cached->windowText.c_str());
        return cached->windowText;
    }

    // Cache miss - fetch all properties at once to reduce X11 round-trips
    fetchAndCacheProperties(hwnd);

    cached = cache_.get(hwnd);
    if (cached) {
        return cached->windowText;
    }

    return "";
}

std::string WindowSystemLinuxQueries::getTitleName(WindowHandle hwnd) {
    return getWindowText(hwnd);  // Same as window text
}

std::string WindowSystemLinuxQueries::getClassName(WindowHandle hwnd) {
    if (!hwnd) {
        PLATFORM_LOG_DEBUG("window", "getClassName: null handle");
        return "";
    }

    Window window = reinterpret_cast<Window>(hwnd);

    // Check cache first
    const WindowPropertyCacheEntry* cached = cache_.get(hwnd);
    if (cached) {
        PLATFORM_LOG_DEBUG("window", "getClassName(0x%lx): '%s' (cached)", window, cached->className.c_str());
        return cached->className;
    }

    // Cache miss - fetch all properties at once
    fetchAndCacheProperties(hwnd);

    cached = cache_.get(hwnd);
    if (cached) {
        return cached->className;
    }

    return "";
}

uint32_t WindowSystemLinuxQueries::getWindowThreadId(WindowHandle hwnd) {
    // Linux doesn't have per-window thread IDs
    // Return process ID instead (same as getWindowProcessId)
    return getWindowProcessId(hwnd);
}

uint32_t WindowSystemLinuxQueries::getWindowProcessId(WindowHandle hwnd) {
    if (!hwnd) return 0;

    // Check cache first
    const WindowPropertyCacheEntry* cached = cache_.get(hwnd);
    if (cached) {
        return cached->processId;
    }

    // Cache miss - fetch all properties at once
    fetchAndCacheProperties(hwnd);

    cached = cache_.get(hwnd);
    if (cached) {
        return cached->processId;
    }

    return 0;
}

void WindowSystemLinuxQueries::invalidateWindowCache(WindowHandle hwnd) {
    cache_.invalidate(hwnd);
    Window window = reinterpret_cast<Window>(hwnd);
    PLATFORM_LOG_DEBUG("window", "invalidateWindowCache(0x%lx)", window);
}

void WindowSystemLinuxQueries::clearCache() {
    cache_.clear();
    PLATFORM_LOG_DEBUG("window", "clearCache: all entries cleared");
}

bool WindowSystemLinuxQueries::getWindowRect(WindowHandle hwnd, Rect* rect) {
    if (!hwnd || !rect) {
        PLATFORM_LOG_DEBUG("window", "getWindowRect: invalid params");
        return false;
    }

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display, window, &attrs)) {
        PLATFORM_LOG_DEBUG("window", "getWindowRect(0x%lx): XGetWindowAttributes failed", window);
        return false;
    }

    // Translate to screen coordinates
    Window child;
    int x, y;
    XTranslateCoordinates(display, window, attrs.root,
                         0, 0, &x, &y, &child);

    rect->left = x;
    rect->top = y;
    rect->right = x + attrs.width;
    rect->bottom = y + attrs.height;

    PLATFORM_LOG_DEBUG("window", "getWindowRect(0x%lx): (%d,%d,%d,%d)", window,
                       rect->left, rect->top, rect->right, rect->bottom);
    return true;
}

} // namespace yamy::platform
