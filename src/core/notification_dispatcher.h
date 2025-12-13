#pragma once

#include "platform/ipc_defs.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace yamy {
namespace core {

/// Callback signature for notification handlers
/// @param type The notification message type
/// @param data Associated data string (may be empty)
using NotificationCallback = std::function<void(MessageType, const std::string&)>;

/// Registration handle for unregistration
using CallbackHandle = uint64_t;

/// Thread-safe notification dispatcher for plugin/extension support
/// Implements singleton pattern for global access
class NotificationDispatcher {
public:
    /// Get singleton instance
    static NotificationDispatcher& instance();

    /// Register callback for all notification types
    /// @param callback Function to invoke on notifications
    /// @return Handle for unregistration
    CallbackHandle registerCallback(NotificationCallback callback);

    /// Register callback for specific notification types only
    /// @param types Set of message types to listen for
    /// @param callback Function to invoke on matching notifications
    /// @return Handle for unregistration
    CallbackHandle registerCallback(
        std::unordered_set<MessageType> types,
        NotificationCallback callback);

    /// Unregister a previously registered callback
    /// @param handle Handle returned from registerCallback
    /// @return true if successfully unregistered, false if handle not found
    bool unregisterCallback(CallbackHandle handle);

    /// Dispatch notification to all registered callbacks
    /// Thread-safe, catches and logs callback exceptions
    /// @param type Notification type
    /// @param data Associated data string
    void dispatch(MessageType type, const std::string& data = "");

    /// Get number of registered callbacks (for testing)
    size_t callbackCount() const;

    /// Clear all registered callbacks (primarily for testing)
    void clearCallbacks();

    // Non-copyable
    NotificationDispatcher(const NotificationDispatcher&) = delete;
    NotificationDispatcher& operator=(const NotificationDispatcher&) = delete;

private:
    NotificationDispatcher() = default;
    ~NotificationDispatcher() = default;

    struct CallbackEntry {
        CallbackHandle handle;
        std::unordered_set<MessageType> types; // empty = all types
        NotificationCallback callback;
    };

    mutable std::mutex m_mutex;
    std::vector<CallbackEntry> m_callbacks;
    CallbackHandle m_nextHandle{1};
};

} // namespace core
} // namespace yamy
