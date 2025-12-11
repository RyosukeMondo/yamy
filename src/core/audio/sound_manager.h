#pragma once

#if defined(QT_CORE_LIB)
#include <QSoundEffect>
#endif
#include <map>

namespace yamy {
namespace audio {

enum class NotificationType {
    Success,
    Error,
    Warning,
};

#if defined(QT_CORE_LIB)
class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    void playSound(NotificationType type);
    void setVolume(int percent);
    void setEnabled(bool enabled);

private:
    bool m_enabled;
    std::map<NotificationType, QSoundEffect*> m_sounds;
};
#else
class SoundManager {
public:
    SoundManager() {}
    ~SoundManager() {}
    void playSound(NotificationType) {}
    void setVolume(int) {}
    void setEnabled(bool) {}
};
#endif

} // namespace audio
} // namespace yamy
