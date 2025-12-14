#include "notification_dispatcher.h"
#include "../utils/logger.h"
#include <algorithm>

namespace yamy {
namespace core {

NotificationDispatcher& NotificationDispatcher::instance()
{
    static NotificationDispatcher instance;
    return instance;
}

CallbackHandle NotificationDispatcher::registerCallback(NotificationCallback callback)
{
    return registerCallback({}, std::move(callback));
}

CallbackHandle NotificationDispatcher::registerCallback(
    std::unordered_set<MessageType> types,
    NotificationCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    CallbackEntry entry;
    entry.handle = m_nextHandle++;
    entry.types = std::move(types);
    entry.callback = std::move(callback);

    m_callbacks.push_back(std::move(entry));

    LOG_DEBUG("[dispatcher] Registered callback handle={} types={}",
              static_cast<unsigned long>(m_callbacks.back().handle),
              entry.types.empty() ? "all" : std::to_string(entry.types.size()));

    return m_callbacks.back().handle;
}

bool NotificationDispatcher::unregisterCallback(CallbackHandle handle)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
                           [handle](const CallbackEntry& e) { return e.handle == handle; });

    if (it == m_callbacks.end()) {
        LOG_WARN("[dispatcher] Attempted to unregister unknown handle={}",
                 static_cast<unsigned long>(handle));
        return false;
    }

    m_callbacks.erase(it);
    LOG_DEBUG("[dispatcher] Unregistered callback handle={}",
              static_cast<unsigned long>(handle));
    return true;
}

void NotificationDispatcher::dispatch(MessageType type, const std::string& data)
{
    // Take snapshot of callbacks while holding lock
    std::vector<NotificationCallback> toInvoke;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& entry : m_callbacks) {
            // Empty types = listen to all
            if (entry.types.empty() || entry.types.count(type) > 0) {
                toInvoke.push_back(entry.callback);
            }
        }
    }

    // Invoke callbacks outside lock to prevent deadlocks
    for (const auto& cb : toInvoke) {
        try {
            cb(type, data);
        } catch (const std::exception& e) {
            LOG_ERROR("[dispatcher] Callback threw exception: {}", e.what());
        } catch (...) {
            LOG_ERROR("[dispatcher] Callback threw unknown exception");
        }
    }
}

size_t NotificationDispatcher::callbackCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_callbacks.size();
}

void NotificationDispatcher::clearCallbacks()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.clear();
    LOG_DEBUG("[dispatcher] Cleared all callbacks");
}

} // namespace core
} // namespace yamy
