#pragma once

#include <QObject>
#include <QSettings>
#include "../../core/platform/ipc_defs.h"

namespace yamy::ui {

/**
 * @brief Manages desktop notification display preferences
 *
 * Controls which notification types are shown as desktop notifications.
 * Preferences are stored via QSettings.
 */
class NotificationPrefs : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     */
    static NotificationPrefs& instance();

    /**
     * @brief Check if desktop notifications should be shown for a message type
     * @param type The notification message type
     * @return true if desktop notification should be displayed
     */
    bool shouldShowDesktopNotification(yamy::MessageType type) const;

    /**
     * @brief Check if desktop notifications are globally enabled
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable all desktop notifications
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if notifications are enabled for error events
     */
    bool isErrorNotificationEnabled() const;

    /**
     * @brief Enable or disable error notifications
     */
    void setErrorNotificationEnabled(bool enabled);

    /**
     * @brief Check if notifications are enabled for config loaded events
     */
    bool isConfigLoadedNotificationEnabled() const;

    /**
     * @brief Enable or disable config loaded notifications
     */
    void setConfigLoadedNotificationEnabled(bool enabled);

    /**
     * @brief Check if notifications are enabled for engine state changes
     */
    bool isStateChangeNotificationEnabled() const;

    /**
     * @brief Enable or disable engine state change notifications
     */
    void setStateChangeNotificationEnabled(bool enabled);

    /**
     * @brief Check if notifications are enabled for keymap switches
     */
    bool isKeymapSwitchNotificationEnabled() const;

    /**
     * @brief Enable or disable keymap switch notifications
     */
    void setKeymapSwitchNotificationEnabled(bool enabled);

    /**
     * @brief Get timeout for error notifications in milliseconds
     */
    int errorTimeout() const;

    /**
     * @brief Set timeout for error notifications
     * @param ms Timeout in milliseconds
     */
    void setErrorTimeout(int ms);

    /**
     * @brief Get timeout for info notifications in milliseconds
     */
    int infoTimeout() const;

    /**
     * @brief Set timeout for info notifications
     * @param ms Timeout in milliseconds
     */
    void setInfoTimeout(int ms);

    /**
     * @brief Load preferences from settings
     */
    void loadSettings();

    /**
     * @brief Save preferences to settings
     */
    void saveSettings();

signals:
    /**
     * @brief Emitted when any preference changes
     */
    void preferencesChanged();

private:
    NotificationPrefs();
    ~NotificationPrefs() = default;

    // Prevent copying
    NotificationPrefs(const NotificationPrefs&) = delete;
    NotificationPrefs& operator=(const NotificationPrefs&) = delete;

    // Settings
    bool m_enabled;
    bool m_errorNotificationEnabled;
    bool m_configLoadedNotificationEnabled;
    bool m_stateChangeNotificationEnabled;
    bool m_keymapSwitchNotificationEnabled;
    int m_errorTimeout;
    int m_infoTimeout;
};

} // namespace yamy::ui
