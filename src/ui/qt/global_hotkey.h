#pragma once

#include <QObject>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>

/**
 * @brief Global hotkey manager for Linux/X11
 *
 * Registers global hotkeys that work even when the application
 * doesn't have focus. Uses X11 XGrabKey for Linux.
 *
 * Usage:
 *   GlobalHotkey* hotkey = new GlobalHotkey(this);
 *   hotkey->setShortcut(QKeySequence("Ctrl+Alt+C"));
 *   connect(hotkey, &GlobalHotkey::activated, this, &MyClass::onHotkeyPressed);
 */
class GlobalHotkey : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject* parent = nullptr);
    ~GlobalHotkey() override;

    /**
     * @brief Set the keyboard shortcut to register
     * @param sequence The key sequence (e.g., "Ctrl+Alt+C")
     * @return true if registration succeeded
     */
    bool setShortcut(const QKeySequence& sequence);

    /**
     * @brief Get the current shortcut
     */
    QKeySequence shortcut() const { return m_shortcut; }

    /**
     * @brief Check if the hotkey is currently registered
     */
    bool isRegistered() const { return m_registered; }

    /**
     * @brief Unregister the current hotkey
     */
    void unregister();

    /**
     * @brief Enable or disable the hotkey
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if the hotkey is enabled
     */
    bool isEnabled() const { return m_enabled; }

signals:
    /**
     * @brief Emitted when the hotkey is pressed
     */
    void activated();

    /**
     * @brief Emitted when hotkey registration fails
     * @param reason Description of the failure
     */
    void registrationFailed(const QString& reason);

protected:
    /**
     * @brief Native event filter implementation
     */
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#else
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
#endif

private:
    /**
     * @brief Register the hotkey with X11
     * @return true on success
     */
    bool registerHotkey();

    /**
     * @brief Unregister the hotkey from X11
     */
    void unregisterHotkey();

    /**
     * @brief Convert Qt key modifiers to X11 modifiers
     */
    static unsigned int qtModifiersToX11(Qt::KeyboardModifiers mods);

    /**
     * @brief Convert Qt key to X11 keycode
     */
    static unsigned int qtKeyToX11(int key);

    QKeySequence m_shortcut;
    bool m_registered;
    bool m_enabled;

    // X11-specific data
    unsigned int m_keycode;
    unsigned int m_modifiers;
};
