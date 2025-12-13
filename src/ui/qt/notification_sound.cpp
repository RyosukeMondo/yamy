#include "notification_sound.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

namespace yamy::ui {

NotificationSound& NotificationSound::instance() {
    static NotificationSound instance;
    return instance;
}

NotificationSound::NotificationSound()
    : QObject(nullptr)
    , m_enabled(false)
    , m_errorSoundEnabled(true)
    , m_configLoadedSoundEnabled(true)
    , m_stateChangeSoundEnabled(false)
    , m_volume(70) {
    loadSettings();
    initializeSounds();
}

void NotificationSound::initializeSounds() {
#ifdef QT_MULTIMEDIA_AVAILABLE
    // Try to find sound files in various locations
    QStringList searchPaths;

    // Application data directory
    searchPaths << QCoreApplication::applicationDirPath() + "/sounds";

    // Standard locations
    searchPaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sounds";
    searchPaths << QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/yamy/sounds";

    // System sound themes (freedesktop)
    searchPaths << "/usr/share/sounds/freedesktop/stereo";
    searchPaths << "/usr/share/sounds/ubuntu/stereo";
    searchPaths << "/usr/share/sounds";

    // Try Qt resource system first
    QString errorSoundPath = ":/sounds/error.wav";
    QString successSoundPath = ":/sounds/success.wav";
    QString stateChangeSoundPath = ":/sounds/click.wav";

    // Check if resource files exist, otherwise search in file system
    if (!QFile::exists(errorSoundPath)) {
        for (const QString& basePath : searchPaths) {
            // Try common error sound names
            QStringList errorNames = {"dialog-error.oga", "dialog-error.wav",
                                       "error.wav", "error.oga", "bell.oga", "bell.wav"};
            for (const QString& name : errorNames) {
                QString path = basePath + "/" + name;
                if (QFile::exists(path)) {
                    errorSoundPath = path;
                    break;
                }
            }
            if (QFile::exists(errorSoundPath) && !errorSoundPath.startsWith(":")) break;
        }
    }

    if (!QFile::exists(successSoundPath)) {
        for (const QString& basePath : searchPaths) {
            // Try common success sound names
            QStringList successNames = {"complete.oga", "complete.wav", "message.oga",
                                         "message.wav", "bell.oga", "bell.wav"};
            for (const QString& name : successNames) {
                QString path = basePath + "/" + name;
                if (QFile::exists(path)) {
                    successSoundPath = path;
                    break;
                }
            }
            if (QFile::exists(successSoundPath) && !successSoundPath.startsWith(":")) break;
        }
    }

    if (!QFile::exists(stateChangeSoundPath)) {
        for (const QString& basePath : searchPaths) {
            // Try common click sound names
            QStringList clickNames = {"button-pressed.oga", "button-pressed.wav",
                                       "menu-click.oga", "menu-click.wav",
                                       "click.wav", "click.oga"};
            for (const QString& name : clickNames) {
                QString path = basePath + "/" + name;
                if (QFile::exists(path)) {
                    stateChangeSoundPath = path;
                    break;
                }
            }
            if (QFile::exists(stateChangeSoundPath) && !stateChangeSoundPath.startsWith(":")) break;
        }
    }

    // Initialize sound effects
    m_errorSound = std::make_unique<QSoundEffect>(this);
    m_successSound = std::make_unique<QSoundEffect>(this);
    m_stateChangeSound = std::make_unique<QSoundEffect>(this);

    // Set sources if files exist
    if (QFile::exists(errorSoundPath) || errorSoundPath.startsWith(":")) {
        m_errorSound->setSource(QUrl::fromLocalFile(errorSoundPath));
        qDebug() << "NotificationSound: Error sound loaded from:" << errorSoundPath;
    } else {
        qDebug() << "NotificationSound: No error sound file found";
    }

    if (QFile::exists(successSoundPath) || successSoundPath.startsWith(":")) {
        m_successSound->setSource(QUrl::fromLocalFile(successSoundPath));
        qDebug() << "NotificationSound: Success sound loaded from:" << successSoundPath;
    } else {
        qDebug() << "NotificationSound: No success sound file found";
    }

    if (QFile::exists(stateChangeSoundPath) || stateChangeSoundPath.startsWith(":")) {
        m_stateChangeSound->setSource(QUrl::fromLocalFile(stateChangeSoundPath));
        qDebug() << "NotificationSound: State change sound loaded from:" << stateChangeSoundPath;
    } else {
        qDebug() << "NotificationSound: No state change sound file found";
    }

    // Set initial volume
    qreal vol = m_volume / 100.0;
    m_errorSound->setVolume(vol);
    m_successSound->setVolume(vol);
    m_stateChangeSound->setVolume(vol);
#else
    qDebug() << "NotificationSound: Qt Multimedia not available, using system beep fallback";
#endif
}

void NotificationSound::playForMessage(yamy::MessageType type) {
    if (!m_enabled) {
        return;
    }

    switch (type) {
        case yamy::MessageType::EngineError:
        case yamy::MessageType::ConfigError:
            if (m_errorSoundEnabled) {
#ifdef QT_MULTIMEDIA_AVAILABLE
                playSound(m_errorSound.get());
#else
                playSystemBeep();
#endif
            }
            break;

        case yamy::MessageType::ConfigLoaded:
            if (m_configLoadedSoundEnabled) {
#ifdef QT_MULTIMEDIA_AVAILABLE
                playSound(m_successSound.get());
#else
                playSystemBeep();
#endif
            }
            break;

        case yamy::MessageType::EngineStarted:
        case yamy::MessageType::EngineStopped:
            if (m_stateChangeSoundEnabled) {
#ifdef QT_MULTIMEDIA_AVAILABLE
                playSound(m_stateChangeSound.get());
#else
                playSystemBeep();
#endif
            }
            break;

        default:
            // No sound for other message types
            break;
    }
}

#ifdef QT_MULTIMEDIA_AVAILABLE
void NotificationSound::playSound(QSoundEffect* sound) {
    if (!sound || sound->source().isEmpty()) {
        // Fallback to system beep if no sound file
        QApplication::beep();
        return;
    }

    // Update volume before playing
    sound->setVolume(m_volume / 100.0);
    sound->play();
}
#else
void NotificationSound::playSystemBeep() {
    QApplication::beep();
}
#endif

bool NotificationSound::isEnabled() const {
    return m_enabled;
}

void NotificationSound::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool NotificationSound::isErrorSoundEnabled() const {
    return m_errorSoundEnabled;
}

void NotificationSound::setErrorSoundEnabled(bool enabled) {
    m_errorSoundEnabled = enabled;
}

bool NotificationSound::isConfigLoadedSoundEnabled() const {
    return m_configLoadedSoundEnabled;
}

void NotificationSound::setConfigLoadedSoundEnabled(bool enabled) {
    m_configLoadedSoundEnabled = enabled;
}

bool NotificationSound::isStateChangeSoundEnabled() const {
    return m_stateChangeSoundEnabled;
}

void NotificationSound::setStateChangeSoundEnabled(bool enabled) {
    m_stateChangeSoundEnabled = enabled;
}

int NotificationSound::volume() const {
    return m_volume;
}

void NotificationSound::setVolume(int volume) {
    m_volume = qBound(0, volume, 100);

#ifdef QT_MULTIMEDIA_AVAILABLE
    // Update all sound effects
    qreal vol = m_volume / 100.0;
    if (m_errorSound) m_errorSound->setVolume(vol);
    if (m_successSound) m_successSound->setVolume(vol);
    if (m_stateChangeSound) m_stateChangeSound->setVolume(vol);
#endif
}

void NotificationSound::loadSettings() {
    QSettings settings("YAMY", "YAMY");

    m_enabled = settings.value("notifications/sounds/enabled", false).toBool();
    m_errorSoundEnabled = settings.value("notifications/sounds/onError", true).toBool();
    m_configLoadedSoundEnabled = settings.value("notifications/sounds/onConfigLoaded", true).toBool();
    m_stateChangeSoundEnabled = settings.value("notifications/sounds/onStateChange", false).toBool();
    m_volume = settings.value("notifications/sounds/volume", 70).toInt();

    qDebug() << "NotificationSound: Loaded settings - enabled:" << m_enabled
             << "errorSound:" << m_errorSoundEnabled
             << "configSound:" << m_configLoadedSoundEnabled
             << "stateSound:" << m_stateChangeSoundEnabled
             << "volume:" << m_volume;
}

void NotificationSound::saveSettings() {
    QSettings settings("YAMY", "YAMY");

    settings.setValue("notifications/sounds/enabled", m_enabled);
    settings.setValue("notifications/sounds/onError", m_errorSoundEnabled);
    settings.setValue("notifications/sounds/onConfigLoaded", m_configLoadedSoundEnabled);
    settings.setValue("notifications/sounds/onStateChange", m_stateChangeSoundEnabled);
    settings.setValue("notifications/sounds/volume", m_volume);

    settings.sync();

    qDebug() << "NotificationSound: Settings saved";
}

} // namespace yamy::ui
