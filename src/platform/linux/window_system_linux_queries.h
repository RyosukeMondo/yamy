#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_queries.h - Basic window query functions (Track 1)

#include "../../core/platform/types.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace yamy::platform {

/**
 * @brief Cache entry for window properties
 *
 * Stores cached window properties to reduce expensive X11 round-trips.
 * Each entry includes a timestamp for automatic expiration.
 */
struct WindowPropertyCacheEntry {
    std::string windowText;                          ///< Window title (_NET_WM_NAME or WM_NAME)
    std::string className;                           ///< Window class (WM_CLASS)
    uint32_t processId = 0;                         ///< Process ID (_NET_WM_PID)
    std::chrono::steady_clock::time_point timestamp; ///< Cache timestamp
    bool valid = false;                              ///< Cache validity flag
};

/**
 * @class WindowPropertyCache
 * @brief Property cache with timeout for reducing X11 round-trips
 *
 * Implements a thread-safe LRU cache for window properties with automatic
 * expiration. Reduces latency by avoiding redundant X11 queries for recently
 * accessed windows.
 *
 * Thread Safety: All methods are thread-safe via internal mutex.
 */
class WindowPropertyCache {
public:
    static constexpr auto kCacheTimeout = std::chrono::milliseconds(100); ///< Cache entry lifetime
    static constexpr size_t kMaxCacheEntries = 256;                       ///< Maximum cache size

    /**
     * @brief Get cached entry if valid
     *
     * @param hwnd Window handle to look up
     * @return Cached entry if valid and not expired, nullptr otherwise
     */
    const WindowPropertyCacheEntry* get(WindowHandle hwnd) const;

    /**
     * @brief Update cache entry
     *
     * Stores or updates the cached properties for a window. Automatically
     * evicts old entries if cache is full.
     *
     * @param hwnd Window handle
     * @param entry Property data to cache
     */
    void set(WindowHandle hwnd, const WindowPropertyCacheEntry& entry);

    /**
     * @brief Invalidate cache for a specific window
     *
     * Removes the cached entry for a window, forcing next access to re-query X11.
     *
     * @param hwnd Window handle to invalidate
     */
    void invalidate(WindowHandle hwnd);

    /**
     * @brief Clear all cache entries
     *
     * Removes all cached data. Useful when X11 connection is reset.
     */
    void clear();

    /**
     * @brief Clear expired entries
     *
     * Removes all entries older than kCacheTimeout. Called automatically
     * when cache is full.
     */
    void evictExpired();

private:
    mutable std::mutex mutex_;
    std::unordered_map<uintptr_t, WindowPropertyCacheEntry> cache_;
};

/**
 * @class WindowSystemLinuxQueries
 * @brief Basic window queries using X11 (Track 1)
 *
 * Provides window property queries using X11/Xlib API with caching
 * to reduce latency. All methods query the X11 server for window
 * information such as title, class, process ID, and geometry.
 *
 * Performance: Uses property cache to achieve <10ms query latency.
 *
 * Thread Safety: Safe to use from any thread (X11 connection is synchronized).
 */
class WindowSystemLinuxQueries {
public:
    WindowSystemLinuxQueries();
    ~WindowSystemLinuxQueries();

    /**
     * @brief Get the currently active/focused window
     *
     * Queries _NET_ACTIVE_WINDOW property from the root window to determine
     * which window currently has keyboard focus.
     *
     * @return Handle to focused window, or nullptr if none
     */
    WindowHandle getForegroundWindow();

    /**
     * @brief Get window at screen coordinates
     *
     * Uses XQueryPointer to find the window at the specified screen position.
     * Traverses window hierarchy to find the deepest child window.
     *
     * @param pt Screen coordinates (absolute)
     * @return Handle to window at position, or nullptr if none
     */
    WindowHandle windowFromPoint(const Point& pt);

    /**
     * @brief Get window title
     *
     * Queries window title using _NET_WM_NAME (UTF-8) with fallback to
     * legacy WM_NAME property. Result is cached for 100ms.
     *
     * @param hwnd Window handle
     * @return Window title string, or empty string if unavailable
     */
    std::string getWindowText(WindowHandle hwnd);

    /**
     * @brief Get window title (alias for getWindowText)
     *
     * @param hwnd Window handle
     * @return Window title string
     * @see getWindowText()
     */
    std::string getTitleName(WindowHandle hwnd);

    /**
     * @brief Get window class name
     *
     * Queries WM_CLASS property and returns the class part (not instance).
     * Example: For Firefox, returns "Navigator" not "firefox".
     * Result is cached for 100ms.
     *
     * @param hwnd Window handle
     * @return Window class name, or empty string if unavailable
     */
    std::string getClassName(WindowHandle hwnd);

    /**
     * @brief Get window's thread ID
     *
     * On Linux, this returns the same value as getWindowProcessId() since
     * X11 doesn't distinguish threads. Provided for API compatibility.
     *
     * @param hwnd Window handle
     * @return Thread/Process ID, or 0 if unavailable
     */
    uint32_t getWindowThreadId(WindowHandle hwnd);

    /**
     * @brief Get window's process ID
     *
     * Queries _NET_WM_PID property to get the process ID that owns the window.
     * Result is cached for 100ms.
     *
     * @param hwnd Window handle
     * @return Process ID, or 0 if unavailable
     */
    uint32_t getWindowProcessId(WindowHandle hwnd);

    /**
     * @brief Get window position and size
     *
     * Uses XGetGeometry() + XTranslateCoordinates() to get screen-absolute
     * position and size. Handles multi-monitor setups correctly.
     *
     * @param hwnd Window handle
     * @param rect Output parameter for window rectangle
     * @return true if successful, false if window invalid or error
     */
    bool getWindowRect(WindowHandle hwnd, Rect* rect);

    /**
     * @brief Invalidate cache for a window
     *
     * Forces next property query to re-fetch from X11. Call this when
     * you know a window's properties have changed (e.g., after rename).
     *
     * @param hwnd Window handle to invalidate
     */
    void invalidateWindowCache(WindowHandle hwnd);

    /**
     * @brief Clear entire window property cache
     *
     * Invalidates all cached window properties. Useful after X11 connection
     * reset or when debugging cache issues.
     */
    void clearCache();

private:
    WindowPropertyCache cache_;

    /// Fetch all properties at once and cache them
    void fetchAndCacheProperties(WindowHandle hwnd);
};

} // namespace yamy::platform
