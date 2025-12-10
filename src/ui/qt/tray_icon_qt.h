#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QObject>
#include <QCheckBox>
#include <QActionGroup>
#include <memory>

// Forward declaration
class Engine;
class ConfigManager;

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
     * @param parent QObject parent
     */
    explicit TrayIconQt(Engine* engine = nullptr, QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~TrayIconQt() override;

    /**
     * @brief Set the engine instance
     * @param engine Pointer to engine
     */
    void setEngine(Engine* engine);

public slots:
    /**
     * @brief Update icon state (enabled/disabled)
     * @param enabled true for enabled icon, false for disabled
     */
    void updateIcon(bool enabled);

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
     * @brief Show about dialog
     */
    void onAbout();

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

    // Engine instance (not owned)
    Engine* m_engine;

    // Context menu
    QMenu* m_menu;

    // Configurations submenu
    QMenu* m_configMenu;
    QActionGroup* m_configActionGroup;

    // Menu actions
    QAction* m_actionEnable;
    QAction* m_actionReload;
    QAction* m_actionSettings;
    QAction* m_actionLog;
    QAction* m_actionAbout;
    QAction* m_actionExit;

    // Icons for different states
    QIcon m_iconEnabled;
    QIcon m_iconDisabled;

    // Current state
    bool m_enabled;
};
