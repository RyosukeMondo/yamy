#pragma once

#ifdef HAVE_APPINDICATOR3

#include <QObject>
#include <QString>
#include <QMenu>
#include <QActionGroup>
#include <memory>
#include <libappindicator/app-indicator.h>
#include "../../core/platform/ipc_defs.h"

// Forward declarations
class Engine;
class ConfigManager;
class GlobalHotkey;
class DialogShortcutsQt;
class DialogExamplesQt;

/**
 * @brief Native Linux system tray using libappindicator3
 *
 * Replaces Qt's QSystemTrayIcon to avoid Qt/DBus crashes.
 * Uses libappindicator3 which provides native DBus integration
 * without going through Qt's marshalling layer.
 */
class SystemTrayAppIndicator : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Message icon types for notifications
     */
    enum MessageIcon {
        NoIcon,
        Information,
        Warning,
        Critical
    };

    /**
     * @brief Construct system tray using AppIndicator
     * @param engine Pointer to keyboard remapping engine (can be nullptr initially)
     * @param parent QObject parent
     */
    explicit SystemTrayAppIndicator(Engine* engine = nullptr, QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SystemTrayAppIndicator() override;

    /**
     * @brief Set the engine instance
     * @param engine Pointer to engine
     */
    void setEngine(Engine* engine);

    /**
     * @brief Show the system tray icon
     */
    void show();

    /**
     * @brief Hide the system tray icon
     */
    void hide();

    /**
     * @brief Check if system tray is available
     * @return true if system tray is supported
     */
    static bool isSystemTrayAvailable();

public slots:
    /**
     * @brief Update icon state (enabled/disabled)
     * @param enabled true for enabled icon, false for disabled
     */
    void updateIcon(bool enabled);

    /**
     * @brief Force refresh of the current icon
     */
    void forceIconRefresh();

    /**
     * @brief Update tooltip text (not supported by AppIndicator, uses title instead)
     * @param text Tooltip string
     */
    void updateTooltip(const QString& text);

    /**
     * @brief Show notification
     * @param title Notification title
     * @param message Notification message
     * @param icon Icon type (Information, Warning, Critical)
     * @param millisecondsTimeoutHint Display duration in milliseconds
     */
    void showNotification(
        const QString& title,
        const QString& message,
        MessageIcon icon = Information,
        int millisecondsTimeoutHint = 3000
    );

    /**
     * @brief Handle engine notification messages
     * Updates tray icon based on engine state
     * @param type Message type indicating engine state change
     * @param data Additional data associated with the message
     */
    void handleEngineMessage(yamy::MessageType type, const QString& data);

private slots:
    /**
     * @brief Toggle enable/disable state
     */
    void onToggleEnable();

    /**
     * @brief Reload keymap configuration
     */
    void onReload();

    /**
     * @brief Show settings dialog
     */
    void onSettings();

    /**
     * @brief Show log viewer dialog
     */
    void onShowLog();

    /**
     * @brief Show investigate dialog
     */
    void onInvestigate();

    /**
     * @brief Show notification history dialog
     */
    void onNotificationHistory();

    /**
     * @brief Show about dialog
     */
    void onAbout();

    /**
     * @brief Open online documentation in browser
     */
    void onOnlineDocumentation();

    /**
     * @brief Open local documentation in browser
     */
    void onLocalDocumentation();

    /**
     * @brief Show keyboard shortcuts dialog
     */
    void onKeyboardShortcuts();

    /**
     * @brief Show configuration examples dialog
     */
    void onConfigExamples();

    /**
     * @brief Open bug report page in browser
     */
    void onReportBug();

    /**
     * @brief Show preferences dialog
     */
    void onPreferences();

    /**
     * @brief Exit application
     */
    void onExit();

    /**
     * @brief Switch to a configuration by index
     * @param index Index of the configuration to switch to
     */
    void onSwitchConfig(int index);

    /**
     * @brief Open configuration manager dialog
     */
    void onManageConfigs();

    /**
     * @brief Handle quick-switch hotkey activation
     */
    void onQuickSwitchHotkey();

private:
    /**
     * @brief Create and setup AppIndicator
     */
    void createIndicator();

    /**
     * @brief Create and setup context menu
     */
    void createMenu();

    /**
     * @brief Setup icon theme paths and names
     */
    void setupIcons();

    /**
     * @brief Update menu items based on engine state
     */
    void updateMenuState();

    /**
     * @brief Populate the configurations submenu with available configs
     */
    void populateConfigMenu();

    /**
     * @brief Setup the global hotkey based on settings
     */
    void setupGlobalHotkey();

    /**
     * @brief Find local documentation path if available
     * @return Path to local docs, empty string if not found
     */
    QString findLocalDocumentationPath() const;

    /**
     * @brief Install icon files to temp directory for AppIndicator
     * AppIndicator requires icons to be in a theme directory structure
     */
    void installIconFiles();

    /**
     * @brief Convert QMenu to GtkMenu for AppIndicator
     * @param qmenu Qt menu to convert
     * @return GtkMenu* (caller must manage lifetime)
     */
    GtkMenu* convertQMenuToGtkMenu(QMenu* qmenu);

    // AppIndicator instance
    AppIndicator* m_indicator;

    // Engine instance (not owned)
    Engine* m_engine;

    // Global hotkey for quick config switch
    GlobalHotkey* m_quickSwitchHotkey;

    // Context menu (Qt version for slots/signals)
    QMenu* m_menu;

    // Configurations submenu
    QMenu* m_configMenu;
    QActionGroup* m_configActionGroup;

    // Help submenu
    QMenu* m_helpMenu;

    // Local documentation action
    QAction* m_actionLocalDocs;

    // Menu actions
    QAction* m_actionEnable;
    QAction* m_actionReload;
    QAction* m_actionSettings;
    QAction* m_actionPreferences;
    QAction* m_actionLog;
    QAction* m_actionInvestigate;
    QAction* m_actionNotificationHistory;
    QAction* m_actionAbout;
    QAction* m_actionExit;

    // Current state
    bool m_enabled;
    yamy::MessageType m_currentState;
    QString m_currentConfigName;

    // Icon names for AppIndicator theme
    QString m_iconEnabled;
    QString m_iconDisabled;
    QString m_iconError;

    // Temp directory for icon theme
    QString m_iconThemePath;
};

#endif // HAVE_APPINDICATOR3
