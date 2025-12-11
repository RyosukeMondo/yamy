#include "notification_prefs.h"
#include <QDebug>

namespace yamy::ui {

NotificationPrefs& NotificationPrefs::instance() {
    static NotificationPrefs instance;
    return instance;
}

NotificationPrefs::NotificationPrefs()
    : QObject(nullptr)
    , m_enabled(true)
    , m_errorNotificationEnabled(true)
    , m_configLoadedNotificationEnabled(true)
    , m_stateChangeNotificationEnabled(true)
    , m_keymapSwitchNotificationEnabled(false)
    , m_focusChangeNotificationEnabled(false)
    , m_performanceNotificationEnabled(false)
    , m_errorTimeout(10000)
    , m_infoTimeout(3000) {
    loadSettings();
}

bool NotificationPrefs::shouldShowDesktopNotification(yamy::MessageType type) const {
    if (!m_enabled) {
        return false;
    }

    switch (type) {
        case yamy::MessageType::EngineError:
        case yamy::MessageType::ConfigError:
            // Errors are always shown when notifications are enabled
            return m_errorNotificationEnabled;

        case yamy::MessageType::ConfigLoaded:
            return m_configLoadedNotificationEnabled;

        case yamy::MessageType::EngineStarted:
        case yamy::MessageType::EngineStopped:
        case yamy::MessageType::EngineStarting:
        case yamy::MessageType::EngineStopping:
            return m_stateChangeNotificationEnabled;

        case yamy::MessageType::KeymapSwitched:
            return m_keymapSwitchNotificationEnabled;

        case yamy::MessageType::FocusChanged:
            return m_focusChangeNotificationEnabled;

        case yamy::MessageType::LatencyReport:
        case yamy::MessageType::CpuUsageReport:
            return m_performanceNotificationEnabled;

        default:
            return false;
    }
}

bool NotificationPrefs::isEnabled() const {
    return m_enabled;
}

void NotificationPrefs::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit preferencesChanged();
    }
}

bool NotificationPrefs::isErrorNotificationEnabled() const {
    return m_errorNotificationEnabled;
}

void NotificationPrefs::setErrorNotificationEnabled(bool enabled) {
    if (m_errorNotificationEnabled != enabled) {
        m_errorNotificationEnabled = enabled;
        emit preferencesChanged();
    }
}

bool NotificationPrefs::isConfigLoadedNotificationEnabled() const {
    return m_configLoadedNotificationEnabled;
}

void NotificationPrefs::setConfigLoadedNotificationEnabled(bool enabled) {
    if (m_configLoadedNotificationEnabled != enabled) {
        m_configLoadedNotificationEnabled = enabled;
        emit preferencesChanged();
    }
}

bool NotificationPrefs::isStateChangeNotificationEnabled() const {
    return m_stateChangeNotificationEnabled;
}

void NotificationPrefs::setStateChangeNotificationEnabled(bool enabled) {
    if (m_stateChangeNotificationEnabled != enabled) {
        m_stateChangeNotificationEnabled = enabled;
        emit preferencesChanged();
    }
}

bool NotificationPrefs::isKeymapSwitchNotificationEnabled() const {
    return m_keymapSwitchNotificationEnabled;
}

void NotificationPrefs::setKeymapSwitchNotificationEnabled(bool enabled) {
    if (m_keymapSwitchNotificationEnabled != enabled) {
        m_keymapSwitchNotificationEnabled = enabled;
        emit preferencesChanged();
    }
}

bool NotificationPrefs::isFocusChangeNotificationEnabled() const {
    return m_focusChangeNotificationEnabled;
}

void NotificationPrefs::setFocusChangeNotificationEnabled(bool enabled) {
    if (m_focusChangeNotificationEnabled != enabled) {
        m_focusChangeNotificationEnabled = enabled;
        emit preferencesChanged();
    }
}

bool NotificationPrefs::isPerformanceNotificationEnabled() const {
    return m_performanceNotificationEnabled;
}

void NotificationPrefs::setPerformanceNotificationEnabled(bool enabled) {
    if (m_performanceNotificationEnabled != enabled) {
        m_performanceNotificationEnabled = enabled;
        emit preferencesChanged();
    }
}

int NotificationPrefs::errorTimeout() const {
    return m_errorTimeout;
}

void NotificationPrefs::setErrorTimeout(int ms) {
    int timeout = qBound(1000, ms, 60000);
    if (m_errorTimeout != timeout) {
        m_errorTimeout = timeout;
        emit preferencesChanged();
    }
}

int NotificationPrefs::infoTimeout() const {
    return m_infoTimeout;
}

void NotificationPrefs::setInfoTimeout(int ms) {
    int timeout = qBound(1000, ms, 30000);
    if (m_infoTimeout != timeout) {
        m_infoTimeout = timeout;
        emit preferencesChanged();
    }
}

void NotificationPrefs::loadSettings() {
    QSettings settings("YAMY", "YAMY");

    m_enabled = settings.value("notifications/desktop/enabled", true).toBool();
    // Errors always enabled by default
    m_errorNotificationEnabled = settings.value("notifications/desktop/onError", true).toBool();
    m_configLoadedNotificationEnabled = settings.value("notifications/desktop/onConfigLoaded", true).toBool();
    m_stateChangeNotificationEnabled = settings.value("notifications/desktop/onStateChange", true).toBool();
    m_keymapSwitchNotificationEnabled = settings.value("notifications/desktop/onKeymapSwitch", false).toBool();
    m_focusChangeNotificationEnabled = settings.value("notifications/desktop/onFocusChange", false).toBool();
    m_performanceNotificationEnabled = settings.value("notifications/desktop/onPerformance", false).toBool();
    m_errorTimeout = settings.value("notifications/desktop/errorTimeout", 10000).toInt();
    m_infoTimeout = settings.value("notifications/desktop/infoTimeout", 3000).toInt();

    qDebug() << "NotificationPrefs: Loaded settings - enabled:" << m_enabled
             << "errorNotif:" << m_errorNotificationEnabled
             << "configNotif:" << m_configLoadedNotificationEnabled
             << "stateNotif:" << m_stateChangeNotificationEnabled
             << "keymapNotif:" << m_keymapSwitchNotificationEnabled
             << "focusNotif:" << m_focusChangeNotificationEnabled
             << "perfNotif:" << m_performanceNotificationEnabled
             << "errorTimeout:" << m_errorTimeout
             << "infoTimeout:" << m_infoTimeout;
}

void NotificationPrefs::saveSettings() {
    QSettings settings("YAMY", "YAMY");

    settings.setValue("notifications/desktop/enabled", m_enabled);
    settings.setValue("notifications/desktop/onError", m_errorNotificationEnabled);
    settings.setValue("notifications/desktop/onConfigLoaded", m_configLoadedNotificationEnabled);
    settings.setValue("notifications/desktop/onStateChange", m_stateChangeNotificationEnabled);
    settings.setValue("notifications/desktop/onKeymapSwitch", m_keymapSwitchNotificationEnabled);
    settings.setValue("notifications/desktop/onFocusChange", m_focusChangeNotificationEnabled);
    settings.setValue("notifications/desktop/onPerformance", m_performanceNotificationEnabled);
    settings.setValue("notifications/desktop/errorTimeout", m_errorTimeout);
    settings.setValue("notifications/desktop/infoTimeout", m_infoTimeout);

    settings.sync();

    qDebug() << "NotificationPrefs: Settings saved";
}

void NotificationPrefs::resetToDefaults() {
    m_enabled = true;
    m_errorNotificationEnabled = true;  // Always enabled
    m_configLoadedNotificationEnabled = true;
    m_stateChangeNotificationEnabled = true;
    m_keymapSwitchNotificationEnabled = false;
    m_focusChangeNotificationEnabled = false;
    m_performanceNotificationEnabled = false;
    m_errorTimeout = 10000;
    m_infoTimeout = 3000;

    emit preferencesChanged();

    qDebug() << "NotificationPrefs: Reset to defaults";
}

} // namespace yamy::ui
