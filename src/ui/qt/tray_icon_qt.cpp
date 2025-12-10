#include "tray_icon_qt.h"
#include <QApplication>
#include <QMessageBox>

// Forward declaration - will be properly included in Phase 6
class Engine {
public:
    bool getIsEnabled() const { return true; }
    void setIsEnabled(bool enabled) {}
    bool reload() { return true; }
};

TrayIconQt::TrayIconQt(Engine* engine, QObject* parent)
    : QSystemTrayIcon(parent)
    , m_engine(engine)
    , m_menu(nullptr)
    , m_actionEnable(nullptr)
    , m_actionReload(nullptr)
    , m_actionSettings(nullptr)
    , m_actionLog(nullptr)
    , m_actionAbout(nullptr)
    , m_actionExit(nullptr)
    , m_enabled(false)
{
    // Load icons
    loadIcons();

    // Create context menu
    createMenu();

    // Connect activation signal
    connect(this, &QSystemTrayIcon::activated,
            this, &TrayIconQt::onActivated);

    // Set initial icon state
    updateIcon(m_enabled);

    // Set initial tooltip
    updateTooltip("YAMY - Keyboard Remapper");
}

TrayIconQt::~TrayIconQt()
{
    // QObject hierarchy will clean up menu and actions
}

void TrayIconQt::setEngine(Engine* engine)
{
    m_engine = engine;
    if (m_engine) {
        updateMenuState();
    }
}

void TrayIconQt::updateIcon(bool enabled)
{
    m_enabled = enabled;
    setIcon(enabled ? m_iconEnabled : m_iconDisabled);
}

void TrayIconQt::updateTooltip(const QString& text)
{
    setToolTip(text);
}

void TrayIconQt::showNotification(
    const QString& title,
    const QString& message,
    QSystemTrayIcon::MessageIcon icon,
    int millisecondsTimeoutHint)
{
    showMessage(title, message, icon, millisecondsTimeoutHint);
}

void TrayIconQt::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        // Single left-click (on some platforms)
        // Do nothing by default
        break;

    case QSystemTrayIcon::DoubleClick:
        // Double-click - toggle enable state (similar to Windows)
        onToggleEnable();
        break;

    case QSystemTrayIcon::MiddleClick:
        // Middle-click
        // Could be used for quick reload
        break;

    case QSystemTrayIcon::Context:
        // Right-click - context menu (handled automatically by Qt)
        break;

    default:
        break;
    }
}

void TrayIconQt::onToggleEnable()
{
    if (!m_engine) {
        return;
    }

    m_enabled = !m_enabled;

    // Update engine state
    m_engine->setIsEnabled(m_enabled);

    // Update icon
    updateIcon(m_enabled);

    // Update menu
    updateMenuState();

    // Update tooltip
    updateTooltip(QString("YAMY - %1").arg(m_enabled ? "Enabled" : "Disabled"));

    // Show notification
    showNotification(
        "YAMY",
        m_enabled ? "Keyboard remapping enabled" : "Keyboard remapping disabled",
        QSystemTrayIcon::Information
    );
}

void TrayIconQt::onReload()
{
    if (!m_engine) {
        return;
    }

    bool success = m_engine->reload();

    showNotification(
        "YAMY",
        success ? "Configuration reloaded successfully" : "Failed to reload configuration",
        success ? QSystemTrayIcon::Information : QSystemTrayIcon::Warning
    );
}

void TrayIconQt::onSettings()
{
    // Will be implemented in Phase 5 (Dialogs)
    showNotification(
        "YAMY",
        "Settings dialog - Coming in Phase 5",
        QSystemTrayIcon::Information
    );
}

void TrayIconQt::onShowLog()
{
    // Will be implemented in Phase 5 (Dialogs)
    showNotification(
        "YAMY",
        "Log viewer - Coming in Phase 5",
        QSystemTrayIcon::Information
    );
}

void TrayIconQt::onAbout()
{
    // Will be implemented in Phase 5 (Dialogs)
    QMessageBox::about(
        nullptr,
        "About YAMY",
        "<h3>YAMY - Yet Another Mado tsukai no Yuutsu</h3>"
        "<p>Version 0.04</p>"
        "<p>Keyboard remapping utility for Windows and Linux</p>"
        "<p>Qt GUI for Linux</p>"
    );
}

void TrayIconQt::onExit()
{
    // Quit application
    QApplication::quit();
}

void TrayIconQt::createMenu()
{
    m_menu = new QMenu();

    // Enable/Disable toggle
    m_actionEnable = m_menu->addAction("Enable");
    m_actionEnable->setCheckable(true);
    m_actionEnable->setChecked(m_enabled);
    connect(m_actionEnable, &QAction::triggered, this, &TrayIconQt::onToggleEnable);

    m_menu->addSeparator();

    // Reload
    m_actionReload = m_menu->addAction("Reload");
    connect(m_actionReload, &QAction::triggered, this, &TrayIconQt::onReload);

    // TODO: Keymap submenu will be added in Phase 4

    m_menu->addSeparator();

    // Settings
    m_actionSettings = m_menu->addAction("Settings...");
    connect(m_actionSettings, &QAction::triggered, this, &TrayIconQt::onSettings);

    // Log
    m_actionLog = m_menu->addAction("Log...");
    connect(m_actionLog, &QAction::triggered, this, &TrayIconQt::onShowLog);

    // About
    m_actionAbout = m_menu->addAction("About...");
    connect(m_actionAbout, &QAction::triggered, this, &TrayIconQt::onAbout);

    m_menu->addSeparator();

    // Exit
    m_actionExit = m_menu->addAction("Exit");
    connect(m_actionExit, &QAction::triggered, this, &TrayIconQt::onExit);

    // Set menu
    setContextMenu(m_menu);
}

void TrayIconQt::loadIcons()
{
    // Load icons from Qt resources
    m_iconEnabled = QIcon(":/icons/yamy_enabled.png");
    m_iconDisabled = QIcon(":/icons/yamy_disabled.png");

    // Fallback to system icons if resources not found
    if (m_iconEnabled.isNull()) {
        m_iconEnabled = QIcon::fromTheme("preferences-desktop-keyboard");
    }
    if (m_iconDisabled.isNull()) {
        m_iconDisabled = QIcon::fromTheme("preferences-desktop-keyboard-shortcuts");
    }
}

void TrayIconQt::updateMenuState()
{
    if (!m_engine || !m_actionEnable) {
        return;
    }

    // Update enable/disable checkmark
    bool engineEnabled = m_engine->getIsEnabled();
    m_actionEnable->setChecked(engineEnabled);
    m_enabled = engineEnabled;
}
