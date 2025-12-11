#pragma once

#include <QObject>
#include <QSettings>
#include <memory>
#include "../../core/platform/ipc_defs.h"

#ifdef QT_MULTIMEDIA_AVAILABLE
#include <QSoundEffect>
#endif

namespace yamy::ui {

/**
 * @brief Manages notification sound playback based on user preferences
 *
 * Plays sounds on specific notification events when enabled.
 * Uses QSoundEffect for cross-platform audio playback.
 * Preferences are stored via QSettings.
 */
class NotificationSound : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     */
    static NotificationSound& instance();

    /**
     * @brief Play sound for given message type if enabled
     * @param type The notification message type
     */
    void playForMessage(yamy::MessageType type);

    /**
     * @brief Check if sounds are globally enabled
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable all sounds
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if sound is enabled for error notifications
     */
    bool isErrorSoundEnabled() const;

    /**
     * @brief Enable or disable error sounds
     */
    void setErrorSoundEnabled(bool enabled);

    /**
     * @brief Check if sound is enabled for config loaded notifications
     */
    bool isConfigLoadedSoundEnabled() const;

    /**
     * @brief Enable or disable config loaded sounds
     */
    void setConfigLoadedSoundEnabled(bool enabled);

    /**
     * @brief Check if sound is enabled for state change notifications
     */
    bool isStateChangeSoundEnabled() const;

    /**
     * @brief Enable or disable state change sounds
     */
    void setStateChangeSoundEnabled(bool enabled);

    /**
     * @brief Get current volume (0-100)
     */
    int volume() const;

    /**
     * @brief Set volume (0-100)
     */
    void setVolume(int volume);

    /**
     * @brief Load preferences from settings
     */
    void loadSettings();

    /**
     * @brief Save preferences to settings
     */
    void saveSettings();

private:
    NotificationSound();
    ~NotificationSound() = default;

    // Prevent copying
    NotificationSound(const NotificationSound&) = delete;
    NotificationSound& operator=(const NotificationSound&) = delete;

    /**
     * @brief Initialize sound effects
     */
    void initializeSounds();

#ifdef QT_MULTIMEDIA_AVAILABLE
    /**
     * @brief Play specific sound effect
     * @param sound Sound effect to play
     */
    void playSound(QSoundEffect* sound);

    // Sound effects
    std::unique_ptr<QSoundEffect> m_errorSound;
    std::unique_ptr<QSoundEffect> m_successSound;
    std::unique_ptr<QSoundEffect> m_stateChangeSound;
#else
    /**
     * @brief Play system beep as fallback
     */
    void playSystemBeep();
#endif

    // Settings
    bool m_enabled;
    bool m_errorSoundEnabled;
    bool m_configLoadedSoundEnabled;
    bool m_stateChangeSoundEnabled;
    int m_volume;  // 0-100
};

} // namespace yamy::ui
