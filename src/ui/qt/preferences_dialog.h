#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>

/**
 * @brief Centralized preferences dialog with tabbed interface
 *
 * Organizes all user preferences into tabs:
 * - General: Start on login, Quick-switch hotkey, Default config
 * - Notifications: Desktop notifications, Sound settings
 * - Logging: Log level, Buffer size, Log to file
 * - Advanced: Performance metrics interval, Debug mode
 */
class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog() override;

private slots:
    void onApply();
    void onOk();
    void onCancel();
    void onResetGeneralDefaults();
    void onResetNotificationDefaults();
    void onResetLoggingDefaults();
    void onResetAdvancedDefaults();
    void onTestSound();

private:
    void setupUI();
    QWidget* createGeneralTab();
    QWidget* createNotificationsTab();
    QWidget* createLoggingTab();
    QWidget* createAdvancedTab();

    void loadSettings();
    void saveSettings();

    void loadGeneralSettings();
    void loadNotificationSettings();
    void loadLoggingSettings();
    void loadAdvancedSettings();

    void saveGeneralSettings();
    void saveNotificationSettings();
    void saveLoggingSettings();
    void saveAdvancedSettings();

    void populateConfigDropdown();
    void updateSoundControlsState();
    void updateNotificationControlsState();

    // Tab widget
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_btnApply;

    // General tab controls
    QCheckBox* m_chkStartOnLogin;
    QCheckBox* m_chkQuickSwitchEnabled;
    QKeySequenceEdit* m_editQuickSwitchHotkey;
    QPushButton* m_btnClearHotkey;
    QComboBox* m_comboDefaultConfig;
    QPushButton* m_btnResetGeneral;

    // Notifications tab controls
    QCheckBox* m_chkDesktopNotifEnabled;
    QCheckBox* m_chkNotifOnError;
    QCheckBox* m_chkNotifOnConfigLoaded;
    QCheckBox* m_chkNotifOnStateChange;
    QCheckBox* m_chkNotifOnKeymapSwitch;
    QCheckBox* m_chkNotifOnFocusChange;
    QCheckBox* m_chkNotifOnPerformance;

    QCheckBox* m_chkSoundsEnabled;
    QCheckBox* m_chkSoundOnError;
    QCheckBox* m_chkSoundOnConfigLoaded;
    QCheckBox* m_chkSoundOnStateChange;
    QSlider* m_sliderVolume;
    QLabel* m_labelVolumeValue;
    QPushButton* m_btnTestSound;
    QPushButton* m_btnResetNotifications;

    // Logging tab controls
    QComboBox* m_comboLogLevel;
    QSpinBox* m_spinBufferSize;
    QCheckBox* m_chkLogToFile;
    QLineEdit* m_editLogFilePath;
    QPushButton* m_btnBrowseLogFile;
    QPushButton* m_btnResetLogging;

    // Advanced tab controls
    QSpinBox* m_spinPerfMetricsInterval;
    QCheckBox* m_chkDebugMode;
    QCheckBox* m_chkShowPerformanceOverlay;
    QPushButton* m_btnResetAdvanced;
};
