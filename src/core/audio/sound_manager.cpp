#include "sound_manager.h"

#if defined(QT_CORE_LIB)
#include <QUrl>

namespace yamy {
namespace audio {

SoundManager::SoundManager() : m_enabled(true) {
    // Placeholder paths - these will need to be updated with actual sound file locations
    m_sounds[NotificationType::Success] = new QSoundEffect();
    m_sounds[NotificationType::Success]->setSource(QUrl("qrc:/sounds/success.wav"));
    m_sounds[NotificationType::Error] = new QSoundEffect();
    m_sounds[NotificationType::Error]->setSource(QUrl("qrc:/sounds/error.wav"));
    m_sounds[NotificationType::Warning] = new QSoundEffect();
    m_sounds[NotificationType::Warning]->setSource(QUrl("qrc:/sounds/warning.wav"));
}

SoundManager::~SoundManager() {
    for (auto const& [key, val] : m_sounds) {
        delete val;
    }
}

void SoundManager::playSound(NotificationType type) {
    if (!m_enabled) {
        return;
    }

    auto it = m_sounds.find(type);
    if (it != m_sounds.end()) {
        it->second->play();
    }
}

void SoundManager::setVolume(int percent) {
    qreal volume = static_cast<qreal>(percent) / 100.0;
    for (auto const& [key, val] : m_sounds) {
        val->setVolume(volume);
    }
}

void SoundManager::setEnabled(bool enabled) {
    m_enabled = enabled;
}

} // namespace audio
} // namespace yamy
#endif // defined(QT_CORE_LIB)
