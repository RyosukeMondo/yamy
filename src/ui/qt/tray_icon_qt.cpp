#include "tray_icon_qt.h"
#include "dialog_settings_qt.h"
#include "dialog_log_qt.h"
#include "dialog_about_qt.h"
#include "dialog_investigate_qt.h"
#include "config_manager_dialog.h"
#include "global_hotkey.h"
#include "../../core/settings/config_manager.h"
#include <QApplication>
#include <QMessageBox>
#include <QSignalMapper>
#include <QSettings>
#include <QDebug>

// Stub Engine class definition (matches stub in main_qt.cpp)
// This will be replaced once core YAMY is refactored for Linux
class Engine {
public:
    virtual ~Engine() = default;
    bool getIsEnabled() const { return m_enabled; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
private:
    mutable bool m_enabled = true;
};

TrayIconQt::TrayIconQt(Engine* engine, QObject* parent)
    : QSystemTrayIcon(parent)
    , m_engine(engine)
    , m_quickSwitchHotkey(nullptr)
    , m_menu(nullptr)
    , m_configMenu(nullptr)
    , m_configActionGroup(nullptr)
    , m_actionEnable(nullptr)
    , m_actionReload(nullptr)
    , m_actionSettings(nullptr)
    , m_actionLog(nullptr)
    , m_actionInvestigate(nullptr)
    , m_actionAbout(nullptr)
    , m_actionExit(nullptr)
    , m_enabled(false)
{
    // Load icons
    loadIcons();

    // Create context menu
    createMenu();

    // Setup global hotkey for quick config switch
    setupGlobalHotkey();

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
    if (m_enabled) {
        m_engine->enable();
    } else {
        m_engine->disable();
    }

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

    // TODO: Implement configuration reload
    // This requires:
    // 1. Loading .mayu files from configured paths
    // 2. Parsing settings
    // 3. Calling engine->setSetting()
    // For now, show a placeholder message

    showNotification(
        "YAMY",
        "Configuration reload - Not yet implemented\n"
        "Please restart YAMY to reload configuration.",
        QSystemTrayIcon::Information
    );
}

void TrayIconQt::onSettings()
{
    DialogSettingsQt* dialog = new DialogSettingsQt();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TrayIconQt::onShowLog()
{
    DialogLogQt* dialog = new DialogLogQt();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TrayIconQt::onInvestigate()
{
    DialogInvestigateQt* dialog = new DialogInvestigateQt();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TrayIconQt::onAbout()
{
    DialogAboutQt* dialog = new DialogAboutQt();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec(); // Modal dialog for About
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

    // Configurations submenu
    m_configMenu = m_menu->addMenu("Configurations");
    m_configActionGroup = new QActionGroup(this);
    m_configActionGroup->setExclusive(true);

    // Populate the config menu
    populateConfigMenu();

    // Connect the config menu's aboutToShow signal to refresh the list
    connect(m_configMenu, &QMenu::aboutToShow, this, &TrayIconQt::populateConfigMenu);

    m_menu->addSeparator();

    // Settings
    m_actionSettings = m_menu->addAction("Settings...");
    connect(m_actionSettings, &QAction::triggered, this, &TrayIconQt::onSettings);

    // Log
    m_actionLog = m_menu->addAction("Log...");
    connect(m_actionLog, &QAction::triggered, this, &TrayIconQt::onShowLog);

    // Investigate
    m_actionInvestigate = m_menu->addAction("Investigate...");
    connect(m_actionInvestigate, &QAction::triggered, this, &TrayIconQt::onInvestigate);

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

void TrayIconQt::populateConfigMenu()
{
    if (!m_configMenu) {
        return;
    }

    // Clear existing actions from the action group and menu
    for (QAction* action : m_configActionGroup->actions()) {
        m_configActionGroup->removeAction(action);
    }
    m_configMenu->clear();

    // Get configurations from ConfigManager
    ConfigManager& configMgr = ConfigManager::instance();
    std::vector<ConfigEntry> configs = configMgr.listConfigs();
    int activeIndex = configMgr.getActiveIndex();

    // Add configuration entries
    if (configs.empty()) {
        QAction* noConfigAction = m_configMenu->addAction("(No configurations)");
        noConfigAction->setEnabled(false);
    } else {
        // Limit displayed configs to prevent menu overflow
        constexpr int MAX_DISPLAYED_CONFIGS = 20;
        int displayCount = std::min(static_cast<int>(configs.size()), MAX_DISPLAYED_CONFIGS);

        for (int i = 0; i < displayCount; ++i) {
            const ConfigEntry& config = configs[i];
            QString displayName = QString::fromStdString(config.name);

            // Show path for duplicate names or truncate long names
            if (displayName.length() > 30) {
                displayName = displayName.left(27) + "...";
            }

            QAction* action = m_configMenu->addAction(displayName);
            action->setCheckable(true);
            action->setChecked(i == activeIndex);
            action->setData(i);  // Store index for switching
            action->setEnabled(config.exists);  // Disable if file doesn't exist

            // Add tooltip with full path
            action->setToolTip(QString::fromStdString(config.path));

            m_configActionGroup->addAction(action);

            // Connect using lambda to capture index
            connect(action, &QAction::triggered, this, [this, i]() {
                onSwitchConfig(i);
            });
        }

        // Show indicator if more configs exist
        if (static_cast<int>(configs.size()) > MAX_DISPLAYED_CONFIGS) {
            m_configMenu->addSeparator();
            QAction* moreAction = m_configMenu->addAction(
                QString("... and %1 more").arg(configs.size() - MAX_DISPLAYED_CONFIGS));
            moreAction->setEnabled(false);
        }
    }

    // Add separator and "Manage Configurations..." action
    m_configMenu->addSeparator();
    QAction* manageAction = m_configMenu->addAction("Manage Configurations...");
    connect(manageAction, &QAction::triggered, this, &TrayIconQt::onManageConfigs);
}

void TrayIconQt::onSwitchConfig(int index)
{
    ConfigManager& configMgr = ConfigManager::instance();

    if (configMgr.setActiveConfig(index)) {
        std::vector<ConfigEntry> configs = configMgr.listConfigs();
        if (index >= 0 && index < static_cast<int>(configs.size())) {
            QString configName = QString::fromStdString(configs[index].name);

            // Show notification
            showNotification(
                "YAMY",
                QString("Switched to configuration: %1").arg(configName),
                QSystemTrayIcon::Information
            );

            // Update tooltip
            updateTooltip(QString("YAMY - %1").arg(configName));
        }
    } else {
        showNotification(
            "YAMY",
            "Failed to switch configuration",
            QSystemTrayIcon::Warning
        );
    }
}

void TrayIconQt::onManageConfigs()
{
    ConfigManagerDialog* dialog = new ConfigManagerDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TrayIconQt::onQuickSwitchHotkey()
{
    ConfigManager& configMgr = ConfigManager::instance();

    if (configMgr.setNextConfig()) {
        // Get the new config name
        std::vector<ConfigEntry> configs = configMgr.listConfigs();
        int newIndex = configMgr.getActiveIndex();

        if (newIndex >= 0 && newIndex < static_cast<int>(configs.size())) {
            QString configName = QString::fromStdString(configs[newIndex].name);

            // Show brief notification
            showNotification(
                "YAMY - Config Switch",
                QString("Configuration: %1").arg(configName),
                QSystemTrayIcon::Information,
                2000  // 2 second timeout for quick switch
            );

            // Update tooltip with config name
            updateTooltip(QString("YAMY - %1").arg(configName));
        }
    } else {
        // No configs available or only one config
        std::vector<ConfigEntry> configs = configMgr.listConfigs();
        if (configs.empty()) {
            showNotification(
                "YAMY",
                "No configurations available",
                QSystemTrayIcon::Warning,
                2000
            );
        }
        // If only one config, silently do nothing
    }
}

void TrayIconQt::setupGlobalHotkey()
{
    QSettings settings("YAMY", "YAMY");

    // Read hotkey settings
    bool hotkeyEnabled = settings.value("hotkeys/quickSwitch/enabled", true).toBool();
    QString hotkeySeq = settings.value("hotkeys/quickSwitch/sequence", "Ctrl+Alt+C").toString();

    // Create hotkey object if not exists
    if (!m_quickSwitchHotkey) {
        m_quickSwitchHotkey = new GlobalHotkey(this);

        // Connect signals
        connect(m_quickSwitchHotkey, &GlobalHotkey::activated,
                this, &TrayIconQt::onQuickSwitchHotkey);

        connect(m_quickSwitchHotkey, &GlobalHotkey::registrationFailed,
                this, [this](const QString& reason) {
            qWarning() << "Quick-switch hotkey registration failed:" << reason;
            // Don't show notification on startup failure to avoid spam
        });
    }

    // Configure the hotkey
    m_quickSwitchHotkey->setEnabled(hotkeyEnabled);
    if (hotkeyEnabled && !hotkeySeq.isEmpty()) {
        m_quickSwitchHotkey->setShortcut(QKeySequence(hotkeySeq));
    } else {
        m_quickSwitchHotkey->setShortcut(QKeySequence());
    }
}
