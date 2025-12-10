#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_queries.h - Basic window query functions (Track 1)

#include "../../core/platform/types.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace yamy::platform {

/// Cache entry for window properties
struct WindowPropertyCacheEntry {
    std::string windowText;
    std::string className;
    uint32_t processId = 0;
    std::chrono::steady_clock::time_point timestamp;
    bool valid = false;
};

/// Property cache with timeout for reducing X11 round-trips
class WindowPropertyCache {
public:
    static constexpr auto kCacheTimeout = std::chrono::milliseconds(100);
    static constexpr size_t kMaxCacheEntries = 256;

    /// Get cached entry if valid
    const WindowPropertyCacheEntry* get(WindowHandle hwnd) const;

    /// Update cache entry
    void set(WindowHandle hwnd, const WindowPropertyCacheEntry& entry);

    /// Invalidate cache for a specific window
    void invalidate(WindowHandle hwnd);

    /// Clear all cache entries
    void clear();

    /// Clear expired entries
    void evictExpired();

private:
    mutable std::mutex mutex_;
    std::unordered_map<uintptr_t, WindowPropertyCacheEntry> cache_;
};

/// Track 1: Basic window queries using X11
class WindowSystemLinuxQueries {
public:
    WindowSystemLinuxQueries();
    ~WindowSystemLinuxQueries();

    /// Get the currently active/focused window
    WindowHandle getForegroundWindow();

    /// Get window at screen coordinates
    WindowHandle windowFromPoint(const Point& pt);

    /// Get window title
    std::string getWindowText(WindowHandle hwnd);

    /// Get window title (same as getWindowText)
    std::string getTitleName(WindowHandle hwnd);

    /// Get window class name
    std::string getClassName(WindowHandle hwnd);

    /// Get window's thread ID
    uint32_t getWindowThreadId(WindowHandle hwnd);

    /// Get window's process ID
    uint32_t getWindowProcessId(WindowHandle hwnd);

    /// Get window position and size
    bool getWindowRect(WindowHandle hwnd, Rect* rect);

    /// Invalidate cache for a window (called on property change events)
    void invalidateWindowCache(WindowHandle hwnd);

    /// Clear entire window property cache
    void clearCache();

private:
    WindowPropertyCache cache_;

    /// Fetch all properties at once and cache them
    void fetchAndCacheProperties(WindowHandle hwnd);
};

} // namespace yamy::platform
