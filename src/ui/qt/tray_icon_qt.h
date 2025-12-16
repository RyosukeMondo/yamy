#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QObject>
#include <QCheckBox>
#include <QActionGroup>
#include <QColor>
#include <memory>
#include "../../core/platform/ipc_defs.h"

// Forward declarations for help dialogs
class DialogShortcutsQt;
class DialogExamplesQt;

class IPCClientGUI;

// Forward declaration
class Engine;
class ConfigManager;
class GlobalHotkey;

/**
 * @brief Qt-based system tray icon for Linux
 *
 * Provides system tray integration similar to Windows Shell_NotifyIcon.
 * Manages icon state (enabled/disabled), tooltip, context menu, and notifications.
 */
class TrayIconQt : public QSystemTrayIcon {
    Q_OBJECT

public:
    /**
     * @brief Construct tray icon
     * @param engine Pointer to keyboard remapping engine (can be nullptr initially)
     * @param ipcClient Pointer to IPC client (can be nullptr)
     * @param parent QObject parent
     */
    explicit TrayIconQt(Engine* engine = nullptr, IPCClientGUI* ipcClient = nullptr, QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~TrayIconQt() override;

    /**
     * @brief Set the engine instance
     * @param engine Pointer to engine
     */
    void setEngine(Engine* engine);

    /**
     * @brief Override toolTip to ensure it never returns null
     * @return Valid tooltip string (never null)
     */
    QString toolTip() const;

protected:
    /**
     * @brief Override event handler to prevent Qt/DBus crashes
     * @param e Event to handle
     * @return true if event was handled
     */
    bool event(QEvent* e) override;

public slots:
    /**
     * @brief Update icon state (enabled/disabled)
     * @param enabled true for enabled icon, false for disabled
     */
    void updateIcon(bool enabled);

    /**
     * @brief Force refresh of the current icon
     * Used to fix tray icon rendering issues on Windows startup.
     */
    void forceIconRefresh();

    /**
     * @brief Update tooltip text
     * @param text Tooltip string
     */
    void updateTooltip(const QString& text);

    /**
     * @brief Show balloon notification (similar to Windows balloon tips)
     * @param title Notification title
     * @param message Notification message
     * @param icon Icon type (Information, Warning, Critical)
     * @param millisecondsTimeoutHint Display duration in milliseconds
     */
    void showNotification(
        const QString& title,
        const QString& message,
        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
        int millisecondsTimeoutHint = 3000
    );

    /**
     * @brief Handle engine notification messages
     * Updates tray icon and tooltip based on engine state
     * @param type Message type indicating engine state change
     * @param data Additional data associated with the message
     */
    void handleEngineMessage(yamy::MessageType type, const QString& data);

private slots:
    /**
     * @brief Handle tray icon activation (clicks)
     * @param reason Activation reason (Trigger, DoubleClick, MiddleClick, Context)
     */
    void onActivated(QSystemTrayIcon::ActivationReason reason);

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
     * Cycles to the next configuration
     */
    void onQuickSwitchHotkey();

private:
    /**
     * @brief Create and setup context menu
     */
    void createMenu();

    /**
     * @brief Load icons from resources
     */
    void loadIcons();

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
     * @brief Create state icon with overlay color
     * @param baseIcon Base icon to overlay
     * @param overlayColor Color for state indicator
     * @return Icon with state overlay
     */

    QIcon createStateIcon(const QIcon& baseIcon, const QColor& overlayColor);

    /**
     * @brief Generate a dynamic tray icon
     * @param bg Background color
     * @param text Text to display (e.g., "Y")
     * @return Generated QIcon
     */
    QIcon generateTrayIcon(const QColor& bg, const QString& text);

    /**
     * @brief Find local documentation path if available
     * @return Path to local docs, empty string if not found
     */
    QString findLocalDocumentationPath() const;

    // Engine instance (not owned)
    Engine* m_engine;
    
    // IPC Client instance (not owned)
    IPCClientGUI* m_ipcClient;

    // Cached tooltip to prevent Qt/DBus crashes
    mutable QString m_cachedTooltip;

    // Global hotkey for quick config switch
    GlobalHotkey* m_quickSwitchHotkey;

    // Context menu
    QMenu* m_menu;

    // Configurations submenu
    QMenu* m_configMenu;
    QActionGroup* m_configActionGroup;

    // Help submenu
    QMenu* m_helpMenu;

    // Local documentation action (only visible if local docs exist)
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

    // Icons for different states
    QIcon m_iconEnabled;
    QIcon m_iconDisabled;
    QIcon m_iconLoading;    // Loading/starting state
    QIcon m_iconRunning;    // Engine running normally
    QIcon m_iconStopped;    // Engine stopped
    QIcon m_iconError;      // Engine error state

    // Current state
    bool m_enabled;

    // Current engine state
    yamy::MessageType m_currentState;

    // Currently loaded config name (for tooltip)
    QString m_currentConfigName;
};
