#include "tray_icon_qt.h"
#include "dialog_settings_qt.h"
#include "dialog_log_qt.h"
#include "dialog_about_qt.h"
#include "dialog_investigate_qt.h"
#include "config_manager_dialog.h"
#include "global_hotkey.h"
#include "notification_history.h"
#include "notification_sound.h"
#include "../../core/settings/config_manager.h"
#include "../../core/engine/engine.h"
#include <QApplication>
#include <QMessageBox>
#include <QSignalMapper>
#include <QSettings>
#include <QDebug>
#include <QPainter>
#include <QPixmap>

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
    , m_actionNotificationHistory(nullptr)
    , m_actionAbout(nullptr)
    , m_actionExit(nullptr)
    , m_enabled(true)
    , m_currentState(yamy::MessageType::EngineStopped)
{
    // Load icons for different states
    loadIcons();

    // Set initial state
    setIcon(m_iconStopped);
    setToolTip("YAMY - Stopped");

    // Create context menu
    createMenu();

    // Setup global hotkey for quick config switch
    setupGlobalHotkey();

    // Connect activation signal
    connect(this, &QSystemTrayIcon::activated,
            this, &TrayIconQt::onActivated);

    // Show the tray icon
    show();
}

TrayIconQt::~TrayIconQt()
{
    delete m_quickSwitchHotkey;
    delete m_menu;
}

void TrayIconQt::setEngine(Engine* engine)
{
    m_engine = engine;
    updateMenuState();
}

void TrayIconQt::updateIcon(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        setIcon(m_iconRunning);
    } else {
        setIcon(m_iconDisabled);
    }
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
        case QSystemTrayIcon::DoubleClick:
            // Toggle enable/disable on double-click
            onToggleEnable();
            break;
        case QSystemTrayIcon::Trigger:
            // Left click - could show status or menu
            break;
        case QSystemTrayIcon::MiddleClick:
            // Middle click - reload config
            onReload();
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

    bool newState = !m_engine->getIsEnabled();
    m_engine->enable(newState);
    updateIcon(newState);
    updateMenuState();

    QString stateStr = newState ? "enabled" : "disabled";
    showNotification("YAMY", QString("YAMY is now %1").arg(stateStr),
                    QSystemTrayIcon::Information, 2000);
}

void TrayIconQt::onReload()
{
    if (!m_engine) {
        return;
    }

    // Get current configuration
    ConfigManager& configMgr = ConfigManager::instance();
    int activeIndex = configMgr.getActiveIndex();
    std::vector<ConfigEntry> configs = configMgr.listConfigs();

    if (activeIndex >= 0 && activeIndex < static_cast<int>(configs.size())) {
        std::string currentConfigPath = configs[activeIndex].path;
        
        // Reload configuration via Engine
        // This will trigger a re-parse and update keymaps
        bool success = m_engine->switchConfiguration(currentConfigPath);
        
        if (success) {
            showNotification(
                "YAMY",
                "Configuration reloaded successfully.",
                QSystemTrayIcon::Information
            );
        } else {
            showNotification(
                "YAMY",
                "Failed to reload configuration. Check log for details.",
                QSystemTrayIcon::Warning
            );
        }
    } else {
        // No active config or invalid index
        showNotification(
            "YAMY",
            "No active configuration to reload.",
            QSystemTrayIcon::Warning
        );
    }
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
    DialogInvestigateQt* dialog = new DialogInvestigateQt(m_engine);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TrayIconQt::onNotificationHistory()
{
    auto* dialog = new yamy::ui::NotificationHistoryDialog();
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

    // Notification History
    m_actionNotificationHistory = m_menu->addAction("Notification History...");
    connect(m_actionNotificationHistory, &QAction::triggered, this, &TrayIconQt::onNotificationHistory);

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
    m_iconLoading = QIcon(":/icons/yamy_loading.png");
    m_iconRunning = QIcon(":/icons/yamy_running.png");
    m_iconStopped = QIcon(":/icons/yamy_stopped.png");
    m_iconError = QIcon(":/icons/yamy_error.png");

    // Fallback: Create state icons from enabled/disabled with overlays
    if (m_iconLoading.isNull()) {
        m_iconLoading = createStateIcon(m_iconEnabled, Qt::yellow);
    }
    if (m_iconRunning.isNull()) {
        // Use enabled icon as running icon
        m_iconRunning = m_iconEnabled;
        if (m_iconRunning.isNull()) {
            m_iconRunning = QIcon::fromTheme("preferences-desktop-keyboard");
        }
    }
    if (m_iconStopped.isNull()) {
        // Use disabled icon as stopped icon
        m_iconStopped = m_iconDisabled;
        if (m_iconStopped.isNull()) {
            m_iconStopped = QIcon::fromTheme("preferences-desktop-keyboard-shortcuts");
        }
    }
    if (m_iconError.isNull()) {
        m_iconError = createStateIcon(m_iconEnabled.isNull() ?
            QIcon::fromTheme("preferences-desktop-keyboard") : m_iconEnabled, Qt::red);
    }

    // Final fallbacks for enabled/disabled
    if (m_iconEnabled.isNull()) {
        m_iconEnabled = m_iconRunning;
    }
    if (m_iconDisabled.isNull()) {
        m_iconDisabled = m_iconStopped;
    }
}

QIcon TrayIconQt::createStateIcon(const QIcon& baseIcon, const QColor& overlayColor)
{
    // Create a state indicator overlay on the base icon
    QPixmap basePixmap = baseIcon.pixmap(22, 22);
    if (basePixmap.isNull()) {
        // Create a simple colored icon if base is null
        basePixmap = QPixmap(22, 22);
        basePixmap.fill(Qt::transparent);
    }

    QPainter painter(&basePixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a small colored circle in the bottom-right corner
    painter.setBrush(overlayColor);
    painter.setPen(Qt::black);
    painter.drawEllipse(14, 14, 7, 7);

    return QIcon(basePixmap);
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

void TrayIconQt::handleEngineMessage(yamy::MessageType type, const QString& data)
{
    // Store notification in history
    yamy::ui::NotificationHistory::instance().addNotification(type, data);

    // Play notification sound if enabled
    yamy::ui::NotificationSound::instance().playForMessage(type);

    m_currentState = type;

    switch (type) {
        case yamy::MessageType::EngineStarting:
            setIcon(m_iconLoading);
            setToolTip("YAMY - Starting...");
            break;

        case yamy::MessageType::EngineStarted:
            setIcon(m_iconRunning);
            if (m_currentConfigName.isEmpty()) {
                setToolTip("YAMY - Running");
            } else {
                setToolTip(QString("YAMY - Running (%1)").arg(m_currentConfigName));
            }
            m_enabled = true;
            updateMenuState();
            break;

        case yamy::MessageType::EngineStopping:
            setIcon(m_iconLoading);
            setToolTip("YAMY - Stopping...");
            break;

        case yamy::MessageType::EngineStopped:
            setIcon(m_iconStopped);
            setToolTip("YAMY - Stopped");
            m_enabled = false;
            updateMenuState();
            break;

        case yamy::MessageType::EngineError:
            setIcon(m_iconError);
            setToolTip(QString("YAMY - Error: %1").arg(data));
            // Show error notification with 5-second auto-dismiss
            showNotification(
                "YAMY Error",
                data.isEmpty() ? "An engine error occurred" : data,
                QSystemTrayIcon::Critical,
                5000
            );
            break;

        case yamy::MessageType::ConfigLoading:
            setIcon(m_iconLoading);
            setToolTip(QString("YAMY - Loading config: %1").arg(data));
            break;

        case yamy::MessageType::ConfigLoaded:
            m_currentConfigName = data;
            if (m_currentState == yamy::MessageType::EngineStarted ||
                m_currentState == yamy::MessageType::ConfigLoaded) {
                setIcon(m_iconRunning);
            }
            if (data.isEmpty()) {
                setToolTip("YAMY - Running");
            } else {
                setToolTip(QString("YAMY - %1").arg(data));
            }
            updateMenuState();
            break;

        case yamy::MessageType::ConfigError:
            setIcon(m_iconError);
            setToolTip(QString("YAMY - Config Error: %1").arg(data));
            showNotification(
                "YAMY Configuration Error",
                data.isEmpty() ? "Failed to load configuration" : data,
                QSystemTrayIcon::Warning,
                5000
            );
            break;

        case yamy::MessageType::KeymapSwitched:
            // Update tooltip with keymap info but don't change icon
            if (!data.isEmpty()) {
                QString tooltip = m_currentConfigName.isEmpty() ?
                    QString("YAMY - Running [%1]").arg(data) :
                    QString("YAMY - %1 [%2]").arg(m_currentConfigName, data);
                setToolTip(tooltip);
            }
            break;

        case yamy::MessageType::FocusChanged:
            // No visual change for focus events
            break;

        case yamy::MessageType::ModifierChanged:
            // No visual change for modifier events
            break;

        case yamy::MessageType::LatencyReport:
        case yamy::MessageType::CpuUsageReport:
            // Performance metrics - no visual change
            break;

        default:
            qDebug() << "TrayIconQt: Unknown message type:" << static_cast<uint32_t>(type);
            break;
    }
}
