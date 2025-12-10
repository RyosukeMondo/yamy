#include "global_hotkey.h"
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <QGuiApplication>
#endif

GlobalHotkey::GlobalHotkey(QObject* parent)
    : QObject(parent)
    , m_registered(false)
    , m_enabled(true)
    , m_keycode(0)
    , m_modifiers(0)
{
    // Install the native event filter
    qApp->installNativeEventFilter(this);
}

GlobalHotkey::~GlobalHotkey()
{
    unregister();
    qApp->removeNativeEventFilter(this);
}

bool GlobalHotkey::setShortcut(const QKeySequence& sequence)
{
    // Unregister previous hotkey
    unregister();

    if (sequence.isEmpty()) {
        m_shortcut = QKeySequence();
        return true;
    }

    m_shortcut = sequence;

    if (m_enabled) {
        return registerHotkey();
    }

    return true;
}

void GlobalHotkey::unregister()
{
    if (m_registered) {
        unregisterHotkey();
        m_registered = false;
    }
}

void GlobalHotkey::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;

    if (enabled && !m_shortcut.isEmpty()) {
        registerHotkey();
    } else if (!enabled && m_registered) {
        unregisterHotkey();
        m_registered = false;
    }
}

#ifdef Q_OS_LINUX

unsigned int GlobalHotkey::qtModifiersToX11(Qt::KeyboardModifiers mods)
{
    unsigned int x11Mods = 0;

    if (mods & Qt::ShiftModifier) {
        x11Mods |= ShiftMask;
    }
    if (mods & Qt::ControlModifier) {
        x11Mods |= ControlMask;
    }
    if (mods & Qt::AltModifier) {
        x11Mods |= Mod1Mask;  // Alt is typically Mod1
    }
    if (mods & Qt::MetaModifier) {
        x11Mods |= Mod4Mask;  // Super/Meta is typically Mod4
    }

    return x11Mods;
}

unsigned int GlobalHotkey::qtKeyToX11(int key)
{
    // Map common Qt keys to X11 keysyms
    switch (key) {
        // Letters A-Z
        case Qt::Key_A: return XK_a;
        case Qt::Key_B: return XK_b;
        case Qt::Key_C: return XK_c;
        case Qt::Key_D: return XK_d;
        case Qt::Key_E: return XK_e;
        case Qt::Key_F: return XK_f;
        case Qt::Key_G: return XK_g;
        case Qt::Key_H: return XK_h;
        case Qt::Key_I: return XK_i;
        case Qt::Key_J: return XK_j;
        case Qt::Key_K: return XK_k;
        case Qt::Key_L: return XK_l;
        case Qt::Key_M: return XK_m;
        case Qt::Key_N: return XK_n;
        case Qt::Key_O: return XK_o;
        case Qt::Key_P: return XK_p;
        case Qt::Key_Q: return XK_q;
        case Qt::Key_R: return XK_r;
        case Qt::Key_S: return XK_s;
        case Qt::Key_T: return XK_t;
        case Qt::Key_U: return XK_u;
        case Qt::Key_V: return XK_v;
        case Qt::Key_W: return XK_w;
        case Qt::Key_X: return XK_x;
        case Qt::Key_Y: return XK_y;
        case Qt::Key_Z: return XK_z;

        // Numbers 0-9
        case Qt::Key_0: return XK_0;
        case Qt::Key_1: return XK_1;
        case Qt::Key_2: return XK_2;
        case Qt::Key_3: return XK_3;
        case Qt::Key_4: return XK_4;
        case Qt::Key_5: return XK_5;
        case Qt::Key_6: return XK_6;
        case Qt::Key_7: return XK_7;
        case Qt::Key_8: return XK_8;
        case Qt::Key_9: return XK_9;

        // Function keys
        case Qt::Key_F1: return XK_F1;
        case Qt::Key_F2: return XK_F2;
        case Qt::Key_F3: return XK_F3;
        case Qt::Key_F4: return XK_F4;
        case Qt::Key_F5: return XK_F5;
        case Qt::Key_F6: return XK_F6;
        case Qt::Key_F7: return XK_F7;
        case Qt::Key_F8: return XK_F8;
        case Qt::Key_F9: return XK_F9;
        case Qt::Key_F10: return XK_F10;
        case Qt::Key_F11: return XK_F11;
        case Qt::Key_F12: return XK_F12;

        // Special keys
        case Qt::Key_Space: return XK_space;
        case Qt::Key_Escape: return XK_Escape;
        case Qt::Key_Tab: return XK_Tab;
        case Qt::Key_Return: return XK_Return;
        case Qt::Key_Enter: return XK_Return;
        case Qt::Key_Backspace: return XK_BackSpace;
        case Qt::Key_Delete: return XK_Delete;
        case Qt::Key_Insert: return XK_Insert;
        case Qt::Key_Home: return XK_Home;
        case Qt::Key_End: return XK_End;
        case Qt::Key_PageUp: return XK_Page_Up;
        case Qt::Key_PageDown: return XK_Page_Down;
        case Qt::Key_Left: return XK_Left;
        case Qt::Key_Right: return XK_Right;
        case Qt::Key_Up: return XK_Up;
        case Qt::Key_Down: return XK_Down;

        // Punctuation
        case Qt::Key_Minus: return XK_minus;
        case Qt::Key_Equal: return XK_equal;
        case Qt::Key_BracketLeft: return XK_bracketleft;
        case Qt::Key_BracketRight: return XK_bracketright;
        case Qt::Key_Backslash: return XK_backslash;
        case Qt::Key_Semicolon: return XK_semicolon;
        case Qt::Key_Apostrophe: return XK_apostrophe;
        case Qt::Key_Comma: return XK_comma;
        case Qt::Key_Period: return XK_period;
        case Qt::Key_Slash: return XK_slash;
        case Qt::Key_QuoteLeft: return XK_grave;

        default:
            // For unmapped keys, try direct conversion
            if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                return XK_a + (key - Qt::Key_A);
            }
            return 0;
    }
}

bool GlobalHotkey::registerHotkey()
{
    if (m_shortcut.isEmpty()) {
        return false;
    }

    // Parse the key sequence
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QKeyCombination keyComb = m_shortcut[0];
    int key = keyComb.key();
    Qt::KeyboardModifiers mods = keyComb.keyboardModifiers();
#else
    int key = m_shortcut[0] & ~(Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    Qt::KeyboardModifiers mods = Qt::KeyboardModifiers(m_shortcut[0] & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
#endif

    // Convert to X11
    unsigned int keysym = qtKeyToX11(key);
    if (keysym == 0) {
        emit registrationFailed("Unsupported key in shortcut");
        return false;
    }

    m_modifiers = qtModifiersToX11(mods);

    // Get X11 display
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        emit registrationFailed("Cannot open X11 display");
        return false;
    }

    // Convert keysym to keycode
    m_keycode = XKeysymToKeycode(display, keysym);
    if (m_keycode == 0) {
        XCloseDisplay(display);
        emit registrationFailed("Cannot convert key to keycode");
        return false;
    }

    // Grab the key on the root window
    Window root = DefaultRootWindow(display);

    // We need to grab with various lock key combinations
    // because NumLock, CapsLock, ScrollLock affect the modifiers
    unsigned int lockMasks[] = {
        0,              // No lock
        LockMask,       // CapsLock
        Mod2Mask,       // NumLock (typically)
        LockMask | Mod2Mask,  // Both
    };

    bool success = true;
    for (unsigned int lockMask : lockMasks) {
        int result = XGrabKey(display, m_keycode,
                              m_modifiers | lockMask,
                              root, True,
                              GrabModeAsync, GrabModeAsync);
        if (result == BadAccess || result == BadValue) {
            success = false;
            break;
        }
    }

    XSync(display, False);
    XCloseDisplay(display);

    if (!success) {
        emit registrationFailed("Key combination already in use by another application");
        return false;
    }

    m_registered = true;
    qDebug() << "GlobalHotkey: Registered" << m_shortcut.toString();
    return true;
}

void GlobalHotkey::unregisterHotkey()
{
    if (!m_registered || m_keycode == 0) {
        return;
    }

    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return;
    }

    Window root = DefaultRootWindow(display);

    // Ungrab with the same lock mask combinations
    unsigned int lockMasks[] = {
        0,
        LockMask,
        Mod2Mask,
        LockMask | Mod2Mask,
    };

    for (unsigned int lockMask : lockMasks) {
        XUngrabKey(display, m_keycode, m_modifiers | lockMask, root);
    }

    XSync(display, False);
    XCloseDisplay(display);

    qDebug() << "GlobalHotkey: Unregistered" << m_shortcut.toString();

    m_keycode = 0;
    m_modifiers = 0;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool GlobalHotkey::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* /*result*/)
#else
bool GlobalHotkey::nativeEventFilter(const QByteArray& eventType, void* message, long* /*result*/)
#endif
{
    if (!m_registered || !m_enabled) {
        return false;
    }

    // Handle XCB events (modern Qt on Linux uses XCB)
    if (eventType == "xcb_generic_event_t") {
        xcb_generic_event_t* event = static_cast<xcb_generic_event_t*>(message);
        int responseType = event->response_type & ~0x80;

        if (responseType == XCB_KEY_PRESS) {
            xcb_key_press_event_t* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(event);

            // Mask out lock keys from the state
            unsigned int stateMasked = keyEvent->state & ~(LockMask | Mod2Mask);

            if (keyEvent->detail == m_keycode && stateMasked == m_modifiers) {
                emit activated();
                return true;  // Consume the event
            }
        }
    }

    return false;
}

#else // Non-Linux fallback

bool GlobalHotkey::registerHotkey()
{
    emit registrationFailed("Global hotkeys not supported on this platform");
    return false;
}

void GlobalHotkey::unregisterHotkey()
{
    // No-op
}

unsigned int GlobalHotkey::qtModifiersToX11(Qt::KeyboardModifiers)
{
    return 0;
}

unsigned int GlobalHotkey::qtKeyToX11(int)
{
    return 0;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool GlobalHotkey::nativeEventFilter(const QByteArray&, void*, qintptr*)
#else
bool GlobalHotkey::nativeEventFilter(const QByteArray&, void*, long*)
#endif
{
    return false;
}

#endif // Q_OS_LINUX
