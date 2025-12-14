#ifdef HAVE_APPINDICATOR3

#include "system_tray_appindicator.h"
#include "../../core/engine/engine.h"
#include "../../core/settings/config_manager.h"
#include "global_hotkey.h"
#include "dialog_settings_qt.h"
#include "dialog_log_qt.h"
#include "dialog_investigate_qt.h"
#include "notification_history.h"
#include "dialog_about_qt.h"
#include "config_manager_dialog.h"
#include "dialog_shortcuts_qt.h"
#include "dialog_examples_qt.h"
#include "preferences_dialog.h"

#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QPixmap>
#include <QDebug>

// Temporarily undefine Qt's signals/slots macros to avoid conflicts with GTK headers
#ifdef signals
#undef signals
#endif
#ifdef slots
#undef slots
#endif

// Include GTK and AppIndicator headers
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#ifdef HAVE_AYATANA_APPINDICATOR3
#include <libayatana-appindicator/app-indicator.h>
#else
#include <libappindicator/app-indicator.h>
#endif

// Restore Qt macros
#define signals Q_SIGNALS
#define slots Q_SLOTS

// GTK callback wrapper - forwards to Qt slot
extern "C" {
    static void gtk_menu_item_activated(GtkMenuItem* menuitem, gpointer user_data) {
        auto* action = static_cast<QAction*>(user_data);
        if (action) {
            action->trigger();
        }
    }
}

SystemTrayAppIndicator::SystemTrayAppIndicator(Engine* engine, QObject* parent)
    : QObject(parent)
    , m_indicator(nullptr)
    , m_engine(engine)
    , m_quickSwitchHotkey(nullptr)
    , m_menu(nullptr)
    , m_configMenu(nullptr)
    , m_configActionGroup(nullptr)
    , m_helpMenu(nullptr)
    , m_actionLocalDocs(nullptr)
    , m_actionEnable(nullptr)
    , m_actionReload(nullptr)
    , m_actionSettings(nullptr)
    , m_actionPreferences(nullptr)
    , m_actionLog(nullptr)
    , m_actionInvestigate(nullptr)
    , m_actionNotificationHistory(nullptr)
    , m_actionAbout(nullptr)
    , m_actionExit(nullptr)
    , m_enabled(false)
    , m_currentState(yamy::MessageType::EngineStarting)
{
    // Initialize GTK if not already initialized
    if (!gtk_init_check(nullptr, nullptr)) {
        qWarning() << "Failed to initialize GTK - system tray will not be available";
        return;
    }

    // Initialize libnotify for notifications
    if (!notify_init("YAMY")) {
        qWarning() << "Failed to initialize libnotify - notifications will not be available";
    }

    // Setup icon files and theme
    setupIcons();

    // Create the AppIndicator
    createIndicator();

    // Create menu
    createMenu();

    // Setup global hotkey
    setupGlobalHotkey();
}

SystemTrayAppIndicator::~SystemTrayAppIndicator()
{
    if (m_indicator) {
        g_object_unref(m_indicator);
        m_indicator = nullptr;
    }

    if (m_quickSwitchHotkey) {
        delete m_quickSwitchHotkey;
        m_quickSwitchHotkey = nullptr;
    }

    // Cleanup libnotify
    notify_uninit();
}

void SystemTrayAppIndicator::setEngine(Engine* engine)
{
    m_engine = engine;
    updateMenuState();
}

void SystemTrayAppIndicator::show()
{
    if (m_indicator) {
        app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);
    }
}

void SystemTrayAppIndicator::hide()
{
    if (m_indicator) {
        app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_PASSIVE);
    }
}

bool SystemTrayAppIndicator::isSystemTrayAvailable()
{
    // Check if GTK and AppIndicator are available
    return gtk_init_check(nullptr, nullptr);
}

void SystemTrayAppIndicator::setupIcons()
{
    // Create temp directory for icon theme
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_iconThemePath = tempDir + "/yamy-icons";

    QDir dir;
    dir.mkpath(m_iconThemePath + "/hicolor/48x48/apps");
    dir.mkpath(m_iconThemePath + "/hicolor/scalable/apps");

    // Install icon files
    installIconFiles();

    // Set icon names (without extension - AppIndicator looks in theme)
    m_iconEnabled = "yamy-enabled";
    m_iconDisabled = "yamy-disabled";
    m_iconError = "yamy-error";
}

void SystemTrayAppIndicator::installIconFiles()
{
    // Copy icon resources to temp theme directory
    // AppIndicator requires icons to be in icon theme structure

    QString iconBasePath = m_iconThemePath + "/hicolor/48x48/apps/";

    // Try to copy from resources or use fallback
    QFile::copy(":/icons/yamy_enabled.png", iconBasePath + "yamy-enabled.png");
    QFile::copy(":/icons/yamy_disabled.png", iconBasePath + "yamy-disabled.png");
    QFile::copy(":/icons/yamy_error.png", iconBasePath + "yamy-error.png");

    // If resources don't exist, create simple colored squares as fallback
    if (!QFile::exists(iconBasePath + "yamy-enabled.png")) {
        QPixmap enabledPixmap(48, 48);
        enabledPixmap.fill(Qt::green);
        enabledPixmap.save(iconBasePath + "yamy-enabled.png");
    }

    if (!QFile::exists(iconBasePath + "yamy-disabled.png")) {
        QPixmap disabledPixmap(48, 48);
        disabledPixmap.fill(Qt::gray);
        disabledPixmap.save(iconBasePath + "yamy-disabled.png");
    }

    if (!QFile::exists(iconBasePath + "yamy-error.png")) {
        QPixmap errorPixmap(48, 48);
        errorPixmap.fill(Qt::red);
        errorPixmap.save(iconBasePath + "yamy-error.png");
    }
}

void SystemTrayAppIndicator::createIndicator()
{
    // Create AppIndicator instance
    m_indicator = app_indicator_new(
        "yamy",                              // id
        m_iconDisabled.toUtf8().constData(), // icon name
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS
    );

    if (!m_indicator) {
        qWarning() << "Failed to create AppIndicator";
        return;
    }

    // Set icon theme path
    app_indicator_set_icon_theme_path(m_indicator, m_iconThemePath.toUtf8().constData());

    // Set initial status
    app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);

    // Set title (tooltip equivalent)
    app_indicator_set_title(m_indicator, "YAMY Keyboard Remapper");
}

GtkMenu* SystemTrayAppIndicator::convertQMenuToGtkMenu(QMenu* qmenu)
{
    if (!qmenu) {
        return nullptr;
    }

    GtkWidget* gtkMenu = gtk_menu_new();

    // Iterate through QMenu actions and create corresponding GTK menu items
    for (QAction* action : qmenu->actions()) {
        if (action->isSeparator()) {
            GtkWidget* separator = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), separator);
            gtk_widget_show(separator);
        } else if (action->menu()) {
            // Submenu
            GtkWidget* submenuItem = gtk_menu_item_new_with_label(action->text().remove('&').toUtf8().constData());
            GtkMenu* submenu = convertQMenuToGtkMenu(action->menu());
            if (submenu) {
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenuItem), GTK_WIDGET(submenu));
            }
            gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), submenuItem);
            gtk_widget_show(submenuItem);
        } else {
            // Regular action
            GtkWidget* menuItem;
            if (action->isCheckable()) {
                menuItem = gtk_check_menu_item_new_with_label(action->text().remove('&').toUtf8().constData());
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuItem), action->isChecked());
            } else {
                menuItem = gtk_menu_item_new_with_label(action->text().remove('&').toUtf8().constData());
            }

            gtk_widget_set_sensitive(menuItem, action->isEnabled());
            g_signal_connect(menuItem, "activate", G_CALLBACK(gtk_menu_item_activated), action);
            gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), menuItem);
            gtk_widget_show(menuItem);
        }
    }

    return GTK_MENU(gtkMenu);
}

void SystemTrayAppIndicator::createMenu()
{
    // Create Qt menu (for signal/slot handling)
    m_menu = new QMenu();

    // Enable/Disable action
    m_actionEnable = m_menu->addAction("Enable");
    m_actionEnable->setCheckable(true);
    m_actionEnable->setChecked(false);
    connect(m_actionEnable, &QAction::triggered, this, &SystemTrayAppIndicator::onToggleEnable);

    m_menu->addSeparator();

    // Reload action
    m_actionReload = m_menu->addAction("Reload");
    connect(m_actionReload, &QAction::triggered, this, &SystemTrayAppIndicator::onReload);

    m_menu->addSeparator();

    // Configurations submenu
    m_configMenu = m_menu->addMenu("Configurations");
    m_configActionGroup = new QActionGroup(this);
    m_configActionGroup->setExclusive(true);

    QAction* manageConfigsAction = m_configMenu->addAction("Manage Configurations...");
    connect(manageConfigsAction, &QAction::triggered, this, &SystemTrayAppIndicator::onManageConfigs);
    m_configMenu->addSeparator();

    populateConfigMenu();

    m_menu->addSeparator();

    // Settings action
    m_actionSettings = m_menu->addAction("Settings...");
    connect(m_actionSettings, &QAction::triggered, this, &SystemTrayAppIndicator::onSettings);

    // Preferences action
    m_actionPreferences = m_menu->addAction("Preferences...");
    connect(m_actionPreferences, &QAction::triggered, this, &SystemTrayAppIndicator::onPreferences);

    m_menu->addSeparator();

    // Log action
    m_actionLog = m_menu->addAction("Log...");
    connect(m_actionLog, &QAction::triggered, this, &SystemTrayAppIndicator::onShowLog);

    // Investigate action
    m_actionInvestigate = m_menu->addAction("Investigate...");
    connect(m_actionInvestigate, &QAction::triggered, this, &SystemTrayAppIndicator::onInvestigate);

    // Notification History action
    m_actionNotificationHistory = m_menu->addAction("Notification History...");
    connect(m_actionNotificationHistory, &QAction::triggered, this, &SystemTrayAppIndicator::onNotificationHistory);

    m_menu->addSeparator();

    // Help submenu
    m_helpMenu = m_menu->addMenu("Help");

    QAction* actionOnlineDocs = m_helpMenu->addAction("Online Documentation");
    connect(actionOnlineDocs, &QAction::triggered, this, &SystemTrayAppIndicator::onOnlineDocumentation);

    m_actionLocalDocs = m_helpMenu->addAction("Local Documentation");
    connect(m_actionLocalDocs, &QAction::triggered, this, &SystemTrayAppIndicator::onLocalDocumentation);

    QString localDocsPath = findLocalDocumentationPath();
    m_actionLocalDocs->setVisible(!localDocsPath.isEmpty());

    m_helpMenu->addSeparator();

    QAction* actionShortcuts = m_helpMenu->addAction("Keyboard Shortcuts...");
    connect(actionShortcuts, &QAction::triggered, this, &SystemTrayAppIndicator::onKeyboardShortcuts);

    QAction* actionExamples = m_helpMenu->addAction("Configuration Examples...");
    connect(actionExamples, &QAction::triggered, this, &SystemTrayAppIndicator::onConfigExamples);

    m_helpMenu->addSeparator();

    QAction* actionReportBug = m_helpMenu->addAction("Report a Bug...");
    connect(actionReportBug, &QAction::triggered, this, &SystemTrayAppIndicator::onReportBug);

    // About action
    m_actionAbout = m_menu->addAction("About...");
    connect(m_actionAbout, &QAction::triggered, this, &SystemTrayAppIndicator::onAbout);

    m_menu->addSeparator();

    // Exit action
    m_actionExit = m_menu->addAction("Exit");
    connect(m_actionExit, &QAction::triggered, this, &SystemTrayAppIndicator::onExit);

    // Convert Qt menu to GTK menu and set it on the indicator
    if (m_indicator) {
        GtkMenu* gtkMenu = convertQMenuToGtkMenu(m_menu);
        if (gtkMenu) {
            app_indicator_set_menu(m_indicator, gtkMenu);
        }
    }
}

void SystemTrayAppIndicator::updateIcon(bool enabled)
{
    m_enabled = enabled;

    if (!m_indicator) {
        return;
    }

    QString iconName = enabled ? m_iconEnabled : m_iconDisabled;
    app_indicator_set_icon(m_indicator, iconName.toUtf8().constData());
}

void SystemTrayAppIndicator::forceIconRefresh()
{
    // Force icon update
    updateIcon(m_enabled);
}

void SystemTrayAppIndicator::updateTooltip(const QString& text)
{
    // AppIndicator doesn't support tooltips directly, use title instead
    if (m_indicator) {
        app_indicator_set_title(m_indicator, text.toUtf8().constData());
    }
}

void SystemTrayAppIndicator::showNotification(
    const QString& title,
    const QString& message,
    MessageIcon icon,
    int millisecondsTimeoutHint)
{
    if (!notify_is_initted()) {
        return;
    }

    // Create notification
    NotifyNotification* notification = notify_notification_new(
        title.toUtf8().constData(),
        message.toUtf8().constData(),
        nullptr  // icon (use default)
    );

    if (!notification) {
        qWarning() << "Failed to create notification";
        return;
    }

    // Set timeout
    notify_notification_set_timeout(notification, millisecondsTimeoutHint);

    // Set urgency based on icon type
    NotifyUrgency urgency = NOTIFY_URGENCY_NORMAL;
    switch (icon) {
        case Critical:
            urgency = NOTIFY_URGENCY_CRITICAL;
            break;
        case Warning:
            urgency = NOTIFY_URGENCY_NORMAL;
            break;
        case Information:
        case NoIcon:
        default:
            urgency = NOTIFY_URGENCY_LOW;
            break;
    }
    notify_notification_set_urgency(notification, urgency);

    // Show notification
    GError* error = nullptr;
    if (!notify_notification_show(notification, &error)) {
        qWarning() << "Failed to show notification:" << (error ? error->message : "unknown error");
        if (error) {
            g_error_free(error);
        }
    }

    // Cleanup
    g_object_unref(notification);
}

void SystemTrayAppIndicator::handleEngineMessage(yamy::MessageType type, const QString& data)
{
    m_currentState = type;

    switch (type) {
        case yamy::MessageType::EngineStarting:
            updateIcon(false);
            updateTooltip("YAMY - Starting...");
            break;

        case yamy::MessageType::EngineStarted:
            updateIcon(true);
            if (m_actionEnable) {
                m_actionEnable->setChecked(true);
            }
            updateTooltip(data.isEmpty() ? "YAMY - Enabled" : QString("YAMY - Enabled (%1)").arg(data));
            m_currentConfigName = data;
            break;

        case yamy::MessageType::EngineStopped:
            updateIcon(false);
            if (m_actionEnable) {
                m_actionEnable->setChecked(false);
            }
            updateTooltip("YAMY - Disabled");
            break;

        case yamy::MessageType::ConfigLoaded:
            m_currentConfigName = data;
            updateTooltip(m_enabled ? QString("YAMY - Enabled (%1)").arg(data) : QString("YAMY - Disabled (%1)").arg(data));
            populateConfigMenu();
            break;

        case yamy::MessageType::ConfigError:
            if (m_indicator) {
                app_indicator_set_icon(m_indicator, m_iconError.toUtf8().constData());
            }
            showNotification("Configuration Error", data, Critical);
            break;

        case yamy::MessageType::EngineError:
            if (m_indicator) {
                app_indicator_set_icon(m_indicator, m_iconError.toUtf8().constData());
            }
            showNotification("Engine Error", data, Critical);
            break;

        default:
            break;
    }

    updateMenuState();
}

void SystemTrayAppIndicator::updateMenuState()
{
    bool engineAvailable = (m_engine != nullptr);

    if (m_actionEnable) {
        m_actionEnable->setEnabled(engineAvailable);
    }
    if (m_actionReload) {
        m_actionReload->setEnabled(engineAvailable);
    }

    // Recreate GTK menu to reflect changes
    if (m_indicator && m_menu) {
        GtkMenu* gtkMenu = convertQMenuToGtkMenu(m_menu);
        if (gtkMenu) {
            app_indicator_set_menu(m_indicator, gtkMenu);
        }
    }
}

void SystemTrayAppIndicator::populateConfigMenu()
{
    if (!m_configMenu || !m_configActionGroup) {
        return;
    }

    // Clear existing config actions
    for (QAction* action : m_configActionGroup->actions()) {
        m_configMenu->removeAction(action);
        delete action;
    }

    // Get configs from ConfigManager singleton
    ConfigManager& configManager = ConfigManager::instance();
    auto configs = configManager.listConfigs();

    for (size_t i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];

        QString displayName = config.name.empty() ? QString("(unnamed)") : QString::fromStdString(config.name);
        if (displayName.isNull() || displayName.isEmpty()) {
            displayName = QString("Config %1").arg(i + 1);
        }

        QAction* action = m_configMenu->addAction(displayName);
        action->setCheckable(true);
        action->setActionGroup(m_configActionGroup);

        if (!m_currentConfigName.isEmpty() &&
            QString::fromStdString(config.name) == m_currentConfigName) {
            action->setChecked(true);
        }

        QString tooltipPath = config.path.empty() ? QString("(no path)") : QString::fromStdString(config.path);
        if (tooltipPath.isNull() || tooltipPath.isEmpty()) {
            tooltipPath = QString("Configuration file");
        }
        action->setToolTip(tooltipPath);

        connect(action, &QAction::triggered, this, [this, i]() {
            onSwitchConfig(static_cast<int>(i));
        });
    }

    // Recreate GTK menu to reflect changes
    updateMenuState();
}

void SystemTrayAppIndicator::setupGlobalHotkey()
{
    // TODO: Implement global hotkey for quick config switch
    // This requires X11 global hotkey registration
}

QString SystemTrayAppIndicator::findLocalDocumentationPath() const
{
    // Check common documentation paths
    QStringList possiblePaths = {
        "/usr/share/doc/yamy/index.html",
        "/usr/local/share/doc/yamy/index.html",
        QApplication::applicationDirPath() + "/../docs/index.html",
        QApplication::applicationDirPath() + "/docs/index.html"
    };

    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return QString();
}

// Slot implementations
void SystemTrayAppIndicator::onToggleEnable()
{
    if (m_engine) {
        m_enabled = !m_enabled;
        m_engine->enable(m_enabled);
        updateIcon(m_enabled);
    }
}

void SystemTrayAppIndicator::onReload()
{
    if (!m_engine) {
        return;
    }

    // Get current configuration from ConfigManager
    ConfigManager& configMgr = ConfigManager::instance();
    int activeIndex = configMgr.getActiveIndex();
    auto configs = configMgr.listConfigs();

    if (activeIndex >= 0 && activeIndex < static_cast<int>(configs.size())) {
        std::string currentConfigPath = configs[activeIndex].path;

        // Reload configuration via Engine
        bool success = m_engine->switchConfiguration(currentConfigPath);

        if (success) {
            showNotification("Configuration Reloaded",
                           QString::fromStdString(configs[activeIndex].name),
                           Information);
        } else {
            showNotification("Reload Failed",
                           "Failed to reload configuration",
                           Critical);
        }
    }
}

void SystemTrayAppIndicator::onSettings()
{
    DialogSettingsQt dialog(nullptr);
    dialog.exec();
}

void SystemTrayAppIndicator::onShowLog()
{
    static DialogLogQt* logDialog = nullptr;
    if (!logDialog) {
        logDialog = new DialogLogQt(nullptr);
    }
    logDialog->show();
    logDialog->raise();
    logDialog->activateWindow();
}

void SystemTrayAppIndicator::onInvestigate()
{
    static DialogInvestigateQt* investigateDialog = nullptr;
    if (!investigateDialog) {
        investigateDialog = new DialogInvestigateQt(m_engine);
    }
    investigateDialog->show();
    investigateDialog->raise();
    investigateDialog->activateWindow();
}

void SystemTrayAppIndicator::onNotificationHistory()
{
    yamy::ui::NotificationHistoryDialog dialog;
    dialog.exec();
}

void SystemTrayAppIndicator::onAbout()
{
    DialogAboutQt dialog;
    dialog.exec();
}

void SystemTrayAppIndicator::onOnlineDocumentation()
{
    QDesktopServices::openUrl(QUrl("https://github.com/ryosukemondo/yamy"));
}

void SystemTrayAppIndicator::onLocalDocumentation()
{
    QString localDocsPath = findLocalDocumentationPath();
    if (!localDocsPath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(localDocsPath));
    }
}

void SystemTrayAppIndicator::onKeyboardShortcuts()
{
    static DialogShortcutsQt* shortcutsDialog = nullptr;
    if (!shortcutsDialog) {
        shortcutsDialog = new DialogShortcutsQt();
    }
    shortcutsDialog->show();
    shortcutsDialog->raise();
    shortcutsDialog->activateWindow();
}

void SystemTrayAppIndicator::onConfigExamples()
{
    static DialogExamplesQt* examplesDialog = nullptr;
    if (!examplesDialog) {
        examplesDialog = new DialogExamplesQt();
    }
    examplesDialog->show();
    examplesDialog->raise();
    examplesDialog->activateWindow();
}

void SystemTrayAppIndicator::onReportBug()
{
    QDesktopServices::openUrl(QUrl("https://github.com/ryosukemondo/yamy/issues"));
}

void SystemTrayAppIndicator::onPreferences()
{
    PreferencesDialog dialog(nullptr);
    dialog.exec();
}

void SystemTrayAppIndicator::onExit()
{
    QApplication::quit();
}

void SystemTrayAppIndicator::onSwitchConfig(int index)
{
    ConfigManager& configManager = ConfigManager::instance();
    configManager.setActiveConfig(index);
}

void SystemTrayAppIndicator::onManageConfigs()
{
    ConfigManagerDialog dialog(nullptr);
    dialog.exec();

    // Refresh config menu after management
    populateConfigMenu();
}

void SystemTrayAppIndicator::onQuickSwitchHotkey()
{
    ConfigManager& configManager = ConfigManager::instance();

    // Cycle to next configuration
    auto configs = configManager.listConfigs();
    if (configs.empty()) {
        return;
    }

    int currentIndex = configManager.getActiveIndex();
    int nextIndex = (currentIndex + 1) % static_cast<int>(configs.size());
    configManager.setActiveConfig(nextIndex);
}

#endif // HAVE_APPINDICATOR3
