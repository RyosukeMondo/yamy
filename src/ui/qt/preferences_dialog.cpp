#include "preferences_dialog.h"
#include "notification_prefs.h"
#include "notification_sound.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_btnApply(nullptr)
    // General tab
    , m_chkStartOnLogin(nullptr)
    , m_chkQuickSwitchEnabled(nullptr)
    , m_editQuickSwitchHotkey(nullptr)
    , m_btnClearHotkey(nullptr)
    , m_comboDefaultConfig(nullptr)
    , m_btnResetGeneral(nullptr)
    // Notifications tab
    , m_chkDesktopNotifEnabled(nullptr)
    , m_chkNotifOnError(nullptr)
    , m_chkNotifOnConfigLoaded(nullptr)
    , m_chkNotifOnStateChange(nullptr)
    , m_chkNotifOnKeymapSwitch(nullptr)
    , m_chkNotifOnFocusChange(nullptr)
    , m_chkNotifOnPerformance(nullptr)
    , m_chkSoundsEnabled(nullptr)
    , m_chkSoundOnError(nullptr)
    , m_chkSoundOnConfigLoaded(nullptr)
    , m_chkSoundOnStateChange(nullptr)
    , m_sliderVolume(nullptr)
    , m_labelVolumeValue(nullptr)
    , m_btnTestSound(nullptr)
    , m_btnResetNotifications(nullptr)
    // Logging tab
    , m_comboLogLevel(nullptr)
    , m_spinBufferSize(nullptr)
    , m_chkLogToFile(nullptr)
    , m_editLogFilePath(nullptr)
    , m_btnBrowseLogFile(nullptr)
    , m_btnResetLogging(nullptr)
    // Advanced tab
    , m_spinPerfMetricsInterval(nullptr)
    , m_chkDebugMode(nullptr)
    , m_chkShowPerformanceOverlay(nullptr)
    , m_btnResetAdvanced(nullptr)
{
    setWindowTitle(tr("Preferences"));
    setMinimumSize(550, 450);

    setupUI();
    loadSettings();
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(createGeneralTab(), tr("General"));
    m_tabWidget->addTab(createNotificationsTab(), tr("Notifications"));
    m_tabWidget->addTab(createLoggingTab(), tr("Logging"));
    m_tabWidget->addTab(createAdvancedTab(), tr("Advanced"));
    mainLayout->addWidget(m_tabWidget);

    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    m_btnApply = m_buttonBox->button(QDialogButtonBox::Apply);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::onOk);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::onCancel);
    connect(m_btnApply, &QPushButton::clicked, this, &PreferencesDialog::onApply);

    mainLayout->addWidget(m_buttonBox);
}

QWidget* PreferencesDialog::createGeneralTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    // Startup group
    auto* startupGroup = new QGroupBox(tr("Startup"));
    auto* startupLayout = new QVBoxLayout(startupGroup);

    m_chkStartOnLogin = new QCheckBox(tr("Start YAMY on system login"));
    m_chkStartOnLogin->setToolTip(tr("Automatically start YAMY when you log in to your desktop session"));
    startupLayout->addWidget(m_chkStartOnLogin);

    layout->addWidget(startupGroup);

    // Quick-switch hotkey group
    auto* hotkeyGroup = new QGroupBox(tr("Config Quick-Switch Hotkey"));
    auto* hotkeyLayout = new QVBoxLayout(hotkeyGroup);

    m_chkQuickSwitchEnabled = new QCheckBox(tr("Enable quick-switch hotkey"));
    m_chkQuickSwitchEnabled->setToolTip(tr("Press hotkey to cycle through configurations"));
    hotkeyLayout->addWidget(m_chkQuickSwitchEnabled);

    auto* hotkeyEditLayout = new QHBoxLayout();
    hotkeyEditLayout->addWidget(new QLabel(tr("Hotkey:")));

    m_editQuickSwitchHotkey = new QKeySequenceEdit();
    m_editQuickSwitchHotkey->setToolTip(tr("Click and press key combination to set hotkey"));
    hotkeyEditLayout->addWidget(m_editQuickSwitchHotkey);

    m_btnClearHotkey = new QPushButton(tr("Clear"));
    connect(m_btnClearHotkey, &QPushButton::clicked, m_editQuickSwitchHotkey, &QKeySequenceEdit::clear);
    hotkeyEditLayout->addWidget(m_btnClearHotkey);

    hotkeyEditLayout->addStretch();
    hotkeyLayout->addLayout(hotkeyEditLayout);

    connect(m_chkQuickSwitchEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        m_editQuickSwitchHotkey->setEnabled(checked);
        m_btnClearHotkey->setEnabled(checked);
    });

    layout->addWidget(hotkeyGroup);

    // Default configuration group
    auto* configGroup = new QGroupBox(tr("Default Configuration"));
    auto* configLayout = new QFormLayout(configGroup);

    m_comboDefaultConfig = new QComboBox();
    m_comboDefaultConfig->setToolTip(tr("Configuration to load on startup"));
    configLayout->addRow(tr("Load on startup:"), m_comboDefaultConfig);

    layout->addWidget(configGroup);

    // Reset button
    auto* resetLayout = new QHBoxLayout();
    resetLayout->addStretch();
    m_btnResetGeneral = new QPushButton(tr("Reset to Defaults"));
    connect(m_btnResetGeneral, &QPushButton::clicked, this, &PreferencesDialog::onResetGeneralDefaults);
    resetLayout->addWidget(m_btnResetGeneral);
    layout->addLayout(resetLayout);

    layout->addStretch();
    return widget;
}

QWidget* PreferencesDialog::createNotificationsTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    // Desktop notifications group
    auto* desktopGroup = new QGroupBox(tr("Desktop Notifications"));
    auto* desktopLayout = new QVBoxLayout(desktopGroup);

    m_chkDesktopNotifEnabled = new QCheckBox(tr("Enable desktop notifications"));
    m_chkDesktopNotifEnabled->setToolTip(tr("Show system notifications for YAMY events"));
    desktopLayout->addWidget(m_chkDesktopNotifEnabled);

    // Notification types - row 1
    auto* typesRow1 = new QHBoxLayout();

    m_chkNotifOnError = new QCheckBox(tr("Errors (always)"));
    m_chkNotifOnError->setChecked(true);
    m_chkNotifOnError->setEnabled(false);
    m_chkNotifOnError->setToolTip(tr("Error notifications are always shown for safety"));
    typesRow1->addWidget(m_chkNotifOnError);

    m_chkNotifOnStateChange = new QCheckBox(tr("Engine state changes"));
    m_chkNotifOnStateChange->setToolTip(tr("Show when engine starts/stops"));
    typesRow1->addWidget(m_chkNotifOnStateChange);

    m_chkNotifOnConfigLoaded = new QCheckBox(tr("Config changes"));
    m_chkNotifOnConfigLoaded->setToolTip(tr("Show when configuration is loaded"));
    typesRow1->addWidget(m_chkNotifOnConfigLoaded);

    typesRow1->addStretch();
    desktopLayout->addLayout(typesRow1);

    // Notification types - row 2
    auto* typesRow2 = new QHBoxLayout();

    m_chkNotifOnKeymapSwitch = new QCheckBox(tr("Keymap switches"));
    m_chkNotifOnKeymapSwitch->setToolTip(tr("Show when keymap is switched"));
    typesRow2->addWidget(m_chkNotifOnKeymapSwitch);

    m_chkNotifOnFocusChange = new QCheckBox(tr("Focus changes"));
    m_chkNotifOnFocusChange->setToolTip(tr("Show when active window changes (verbose)"));
    typesRow2->addWidget(m_chkNotifOnFocusChange);

    m_chkNotifOnPerformance = new QCheckBox(tr("Performance metrics"));
    m_chkNotifOnPerformance->setToolTip(tr("Show latency and CPU usage reports (verbose)"));
    typesRow2->addWidget(m_chkNotifOnPerformance);

    typesRow2->addStretch();
    desktopLayout->addLayout(typesRow2);

    connect(m_chkDesktopNotifEnabled, &QCheckBox::toggled,
            this, &PreferencesDialog::updateNotificationControlsState);

    layout->addWidget(desktopGroup);

    // Sound notifications group
    auto* soundGroup = new QGroupBox(tr("Notification Sounds"));
    auto* soundLayout = new QVBoxLayout(soundGroup);

    m_chkSoundsEnabled = new QCheckBox(tr("Enable notification sounds"));
    m_chkSoundsEnabled->setToolTip(tr("Play sounds on notification events"));
    soundLayout->addWidget(m_chkSoundsEnabled);

    // Sound types
    auto* soundTypesLayout = new QHBoxLayout();

    m_chkSoundOnError = new QCheckBox(tr("On error"));
    soundTypesLayout->addWidget(m_chkSoundOnError);

    m_chkSoundOnConfigLoaded = new QCheckBox(tr("On config loaded"));
    soundTypesLayout->addWidget(m_chkSoundOnConfigLoaded);

    m_chkSoundOnStateChange = new QCheckBox(tr("On state change"));
    soundTypesLayout->addWidget(m_chkSoundOnStateChange);

    soundTypesLayout->addStretch();
    soundLayout->addLayout(soundTypesLayout);

    // Volume slider
    auto* volumeLayout = new QHBoxLayout();
    volumeLayout->addWidget(new QLabel(tr("Volume:")));

    m_sliderVolume = new QSlider(Qt::Horizontal);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(70);
    volumeLayout->addWidget(m_sliderVolume);

    m_labelVolumeValue = new QLabel("70%");
    m_labelVolumeValue->setMinimumWidth(40);
    volumeLayout->addWidget(m_labelVolumeValue);

    m_btnTestSound = new QPushButton(tr("Test"));
    connect(m_btnTestSound, &QPushButton::clicked, this, &PreferencesDialog::onTestSound);
    volumeLayout->addWidget(m_btnTestSound);

    soundLayout->addLayout(volumeLayout);

    connect(m_sliderVolume, &QSlider::valueChanged, this, [this](int value) {
        m_labelVolumeValue->setText(QString("%1%").arg(value));
    });

    connect(m_chkSoundsEnabled, &QCheckBox::toggled,
            this, &PreferencesDialog::updateSoundControlsState);

    layout->addWidget(soundGroup);

    // Reset button
    auto* resetLayout = new QHBoxLayout();
    resetLayout->addStretch();
    m_btnResetNotifications = new QPushButton(tr("Reset to Defaults"));
    connect(m_btnResetNotifications, &QPushButton::clicked,
            this, &PreferencesDialog::onResetNotificationDefaults);
    resetLayout->addWidget(m_btnResetNotifications);
    layout->addLayout(resetLayout);

    layout->addStretch();
    return widget;
}

QWidget* PreferencesDialog::createLoggingTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    // Log level group
    auto* levelGroup = new QGroupBox(tr("Log Level"));
    auto* levelLayout = new QFormLayout(levelGroup);

    m_comboLogLevel = new QComboBox();
    m_comboLogLevel->addItem(tr("Error"), "error");
    m_comboLogLevel->addItem(tr("Warning"), "warning");
    m_comboLogLevel->addItem(tr("Info"), "info");
    m_comboLogLevel->addItem(tr("Debug"), "debug");
    m_comboLogLevel->addItem(tr("Trace"), "trace");
    m_comboLogLevel->setToolTip(tr("Minimum log level to display and record"));
    levelLayout->addRow(tr("Minimum level:"), m_comboLogLevel);

    layout->addWidget(levelGroup);

    // Buffer settings group
    auto* bufferGroup = new QGroupBox(tr("Log Buffer"));
    auto* bufferLayout = new QFormLayout(bufferGroup);

    m_spinBufferSize = new QSpinBox();
    m_spinBufferSize->setRange(1000, 100000);
    m_spinBufferSize->setSingleStep(1000);
    m_spinBufferSize->setValue(10000);
    m_spinBufferSize->setSuffix(tr(" entries"));
    m_spinBufferSize->setToolTip(tr("Maximum number of log entries to keep in memory"));
    bufferLayout->addRow(tr("Buffer size:"), m_spinBufferSize);

    layout->addWidget(bufferGroup);

    // File logging group
    auto* fileGroup = new QGroupBox(tr("File Logging"));
    auto* fileLayout = new QVBoxLayout(fileGroup);

    m_chkLogToFile = new QCheckBox(tr("Enable logging to file"));
    m_chkLogToFile->setToolTip(tr("Write log entries to a file on disk"));
    fileLayout->addWidget(m_chkLogToFile);

    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel(tr("Log file:")));

    m_editLogFilePath = new QLineEdit();
    m_editLogFilePath->setPlaceholderText(tr("~/.local/share/yamy/yamy.log"));
    m_editLogFilePath->setToolTip(tr("Path to the log file"));
    pathLayout->addWidget(m_editLogFilePath);

    m_btnBrowseLogFile = new QPushButton(tr("Browse..."));
    connect(m_btnBrowseLogFile, &QPushButton::clicked, this, [this]() {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                              + "/yamy/yamy.log";
        QString path = QFileDialog::getSaveFileName(
            this, tr("Select Log File"), defaultPath,
            tr("Log Files (*.log);;All Files (*)"));
        if (!path.isEmpty()) {
            m_editLogFilePath->setText(path);
        }
    });
    pathLayout->addWidget(m_btnBrowseLogFile);

    fileLayout->addLayout(pathLayout);

    connect(m_chkLogToFile, &QCheckBox::toggled, this, [this](bool checked) {
        m_editLogFilePath->setEnabled(checked);
        m_btnBrowseLogFile->setEnabled(checked);
    });

    layout->addWidget(fileGroup);

    // Reset button
    auto* resetLayout = new QHBoxLayout();
    resetLayout->addStretch();
    m_btnResetLogging = new QPushButton(tr("Reset to Defaults"));
    connect(m_btnResetLogging, &QPushButton::clicked,
            this, &PreferencesDialog::onResetLoggingDefaults);
    resetLayout->addWidget(m_btnResetLogging);
    layout->addLayout(resetLayout);

    layout->addStretch();
    return widget;
}

QWidget* PreferencesDialog::createAdvancedTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    // Performance group
    auto* perfGroup = new QGroupBox(tr("Performance Monitoring"));
    auto* perfLayout = new QFormLayout(perfGroup);

    m_spinPerfMetricsInterval = new QSpinBox();
    m_spinPerfMetricsInterval->setRange(100, 10000);
    m_spinPerfMetricsInterval->setSingleStep(100);
    m_spinPerfMetricsInterval->setValue(1000);
    m_spinPerfMetricsInterval->setSuffix(tr(" ms"));
    m_spinPerfMetricsInterval->setToolTip(tr("How often to collect and report performance metrics"));
    perfLayout->addRow(tr("Metrics interval:"), m_spinPerfMetricsInterval);

    m_chkShowPerformanceOverlay = new QCheckBox(tr("Show performance overlay in tray tooltip"));
    m_chkShowPerformanceOverlay->setToolTip(tr("Display latency and CPU usage in system tray tooltip"));
    perfLayout->addRow(m_chkShowPerformanceOverlay);

    layout->addWidget(perfGroup);

    // Debug group
    auto* debugGroup = new QGroupBox(tr("Debugging"));
    auto* debugLayout = new QVBoxLayout(debugGroup);

    m_chkDebugMode = new QCheckBox(tr("Enable debug mode"));
    m_chkDebugMode->setToolTip(tr("Enable additional debugging features and verbose output"));
    debugLayout->addWidget(m_chkDebugMode);

    auto* debugHelp = new QLabel(tr("Debug mode enables additional logging, "
                                    "performance profiling, and diagnostic features. "
                                    "May impact performance."));
    debugHelp->setWordWrap(true);
    debugHelp->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    debugLayout->addWidget(debugHelp);

    layout->addWidget(debugGroup);

    // Reset button
    auto* resetLayout = new QHBoxLayout();
    resetLayout->addStretch();
    m_btnResetAdvanced = new QPushButton(tr("Reset to Defaults"));
    connect(m_btnResetAdvanced, &QPushButton::clicked,
            this, &PreferencesDialog::onResetAdvancedDefaults);
    resetLayout->addWidget(m_btnResetAdvanced);
    layout->addLayout(resetLayout);

    layout->addStretch();
    return widget;
}

void PreferencesDialog::loadSettings()
{
    loadGeneralSettings();
    loadNotificationSettings();
    loadLoggingSettings();
    loadAdvancedSettings();
    populateConfigDropdown();
}

void PreferencesDialog::loadGeneralSettings()
{
    QSettings settings("YAMY", "YAMY");

    bool startOnLogin = settings.value("general/startOnLogin", false).toBool();
    m_chkStartOnLogin->setChecked(startOnLogin);

    bool hotkeyEnabled = settings.value("hotkeys/quickSwitch/enabled", true).toBool();
    QString hotkeySeq = settings.value("hotkeys/quickSwitch/sequence", "Ctrl+Alt+C").toString();

    m_chkQuickSwitchEnabled->setChecked(hotkeyEnabled);
    m_editQuickSwitchHotkey->setKeySequence(QKeySequence(hotkeySeq));
    m_editQuickSwitchHotkey->setEnabled(hotkeyEnabled);
    m_btnClearHotkey->setEnabled(hotkeyEnabled);

    QString defaultConfig = settings.value("general/defaultConfig", "").toString();
    int idx = m_comboDefaultConfig->findData(defaultConfig);
    if (idx >= 0) {
        m_comboDefaultConfig->setCurrentIndex(idx);
    }
}

void PreferencesDialog::loadNotificationSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Desktop notifications from NotificationPrefs singleton
    auto& notifPrefs = yamy::ui::NotificationPrefs::instance();
    m_chkDesktopNotifEnabled->setChecked(notifPrefs.isEnabled());
    m_chkNotifOnError->setChecked(notifPrefs.isErrorNotificationEnabled());
    m_chkNotifOnConfigLoaded->setChecked(notifPrefs.isConfigLoadedNotificationEnabled());
    m_chkNotifOnStateChange->setChecked(notifPrefs.isStateChangeNotificationEnabled());
    m_chkNotifOnKeymapSwitch->setChecked(notifPrefs.isKeymapSwitchNotificationEnabled());
    m_chkNotifOnFocusChange->setChecked(notifPrefs.isFocusChangeNotificationEnabled());
    m_chkNotifOnPerformance->setChecked(notifPrefs.isPerformanceNotificationEnabled());

    updateNotificationControlsState();

    // Sound notifications
    bool soundsEnabled = settings.value("notifications/sounds/enabled", false).toBool();
    bool soundOnError = settings.value("notifications/sounds/onError", true).toBool();
    bool soundOnConfigLoaded = settings.value("notifications/sounds/onConfigLoaded", true).toBool();
    bool soundOnStateChange = settings.value("notifications/sounds/onStateChange", false).toBool();
    int soundVolume = settings.value("notifications/sounds/volume", 70).toInt();

    m_chkSoundsEnabled->setChecked(soundsEnabled);
    m_chkSoundOnError->setChecked(soundOnError);
    m_chkSoundOnConfigLoaded->setChecked(soundOnConfigLoaded);
    m_chkSoundOnStateChange->setChecked(soundOnStateChange);
    m_sliderVolume->setValue(soundVolume);
    m_labelVolumeValue->setText(QString("%1%").arg(soundVolume));

    updateSoundControlsState();
}

void PreferencesDialog::loadLoggingSettings()
{
    QSettings settings("YAMY", "YAMY");

    QString logLevel = settings.value("logging/level", "info").toString();
    int idx = m_comboLogLevel->findData(logLevel);
    if (idx >= 0) {
        m_comboLogLevel->setCurrentIndex(idx);
    }

    int bufferSize = settings.value("logging/bufferSize", 10000).toInt();
    m_spinBufferSize->setValue(bufferSize);

    bool logToFile = settings.value("logging/toFile", false).toBool();
    QString logFilePath = settings.value("logging/filePath", "").toString();

    m_chkLogToFile->setChecked(logToFile);
    m_editLogFilePath->setText(logFilePath);
    m_editLogFilePath->setEnabled(logToFile);
    m_btnBrowseLogFile->setEnabled(logToFile);
}

void PreferencesDialog::loadAdvancedSettings()
{
    QSettings settings("YAMY", "YAMY");

    int perfInterval = settings.value("advanced/perfMetricsInterval", 1000).toInt();
    m_spinPerfMetricsInterval->setValue(perfInterval);

    bool showPerfOverlay = settings.value("advanced/showPerformanceOverlay", false).toBool();
    m_chkShowPerformanceOverlay->setChecked(showPerfOverlay);

    bool debugMode = settings.value("advanced/debugMode", false).toBool();
    m_chkDebugMode->setChecked(debugMode);
}

void PreferencesDialog::saveSettings()
{
    saveGeneralSettings();
    saveNotificationSettings();
    saveLoggingSettings();
    saveAdvancedSettings();
}

void PreferencesDialog::saveGeneralSettings()
{
    QSettings settings("YAMY", "YAMY");

    settings.setValue("general/startOnLogin", m_chkStartOnLogin->isChecked());
    settings.setValue("hotkeys/quickSwitch/enabled", m_chkQuickSwitchEnabled->isChecked());
    settings.setValue("hotkeys/quickSwitch/sequence",
                      m_editQuickSwitchHotkey->keySequence().toString());

    QString defaultConfig = m_comboDefaultConfig->currentData().toString();
    settings.setValue("general/defaultConfig", defaultConfig);

    settings.sync();
}

void PreferencesDialog::saveNotificationSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Save desktop notifications to NotificationPrefs singleton
    auto& notifPrefs = yamy::ui::NotificationPrefs::instance();
    notifPrefs.setEnabled(m_chkDesktopNotifEnabled->isChecked());
    notifPrefs.setErrorNotificationEnabled(m_chkNotifOnError->isChecked());
    notifPrefs.setConfigLoadedNotificationEnabled(m_chkNotifOnConfigLoaded->isChecked());
    notifPrefs.setStateChangeNotificationEnabled(m_chkNotifOnStateChange->isChecked());
    notifPrefs.setKeymapSwitchNotificationEnabled(m_chkNotifOnKeymapSwitch->isChecked());
    notifPrefs.setFocusChangeNotificationEnabled(m_chkNotifOnFocusChange->isChecked());
    notifPrefs.setPerformanceNotificationEnabled(m_chkNotifOnPerformance->isChecked());
    notifPrefs.saveSettings();

    // Save sound notifications
    settings.setValue("notifications/sounds/enabled", m_chkSoundsEnabled->isChecked());
    settings.setValue("notifications/sounds/onError", m_chkSoundOnError->isChecked());
    settings.setValue("notifications/sounds/onConfigLoaded", m_chkSoundOnConfigLoaded->isChecked());
    settings.setValue("notifications/sounds/onStateChange", m_chkSoundOnStateChange->isChecked());
    settings.setValue("notifications/sounds/volume", m_sliderVolume->value());

    settings.sync();

    // Apply sound settings to singleton
    auto& soundMgr = yamy::ui::NotificationSound::instance();
    soundMgr.setEnabled(m_chkSoundsEnabled->isChecked());
    soundMgr.setErrorSoundEnabled(m_chkSoundOnError->isChecked());
    soundMgr.setConfigLoadedSoundEnabled(m_chkSoundOnConfigLoaded->isChecked());
    soundMgr.setStateChangeSoundEnabled(m_chkSoundOnStateChange->isChecked());
    soundMgr.setVolume(m_sliderVolume->value());
}

void PreferencesDialog::saveLoggingSettings()
{
    QSettings settings("YAMY", "YAMY");

    settings.setValue("logging/level", m_comboLogLevel->currentData().toString());
    settings.setValue("logging/bufferSize", m_spinBufferSize->value());
    settings.setValue("logging/toFile", m_chkLogToFile->isChecked());
    settings.setValue("logging/filePath", m_editLogFilePath->text());

    settings.sync();
}

void PreferencesDialog::saveAdvancedSettings()
{
    QSettings settings("YAMY", "YAMY");

    settings.setValue("advanced/perfMetricsInterval", m_spinPerfMetricsInterval->value());
    settings.setValue("advanced/showPerformanceOverlay", m_chkShowPerformanceOverlay->isChecked());
    settings.setValue("advanced/debugMode", m_chkDebugMode->isChecked());

    settings.sync();
}

void PreferencesDialog::populateConfigDropdown()
{
    QSettings settings("YAMY", "YAMY");
    QStringList keymapFiles = settings.value("keymaps/files").toStringList();

    m_comboDefaultConfig->clear();
    m_comboDefaultConfig->addItem(tr("(None)"), "");

    for (const QString& file : keymapFiles) {
        QFileInfo fi(file);
        m_comboDefaultConfig->addItem(fi.fileName(), file);
    }

    // Restore selection
    QString defaultConfig = settings.value("general/defaultConfig", "").toString();
    int idx = m_comboDefaultConfig->findData(defaultConfig);
    if (idx >= 0) {
        m_comboDefaultConfig->setCurrentIndex(idx);
    }
}

void PreferencesDialog::updateSoundControlsState()
{
    bool enabled = m_chkSoundsEnabled->isChecked();
    m_chkSoundOnError->setEnabled(enabled);
    m_chkSoundOnConfigLoaded->setEnabled(enabled);
    m_chkSoundOnStateChange->setEnabled(enabled);
    m_sliderVolume->setEnabled(enabled);
    m_btnTestSound->setEnabled(enabled);
}

void PreferencesDialog::updateNotificationControlsState()
{
    bool enabled = m_chkDesktopNotifEnabled->isChecked();
    // Error is always enabled but checkbox is disabled
    m_chkNotifOnStateChange->setEnabled(enabled);
    m_chkNotifOnConfigLoaded->setEnabled(enabled);
    m_chkNotifOnKeymapSwitch->setEnabled(enabled);
    m_chkNotifOnFocusChange->setEnabled(enabled);
    m_chkNotifOnPerformance->setEnabled(enabled);
}

void PreferencesDialog::onApply()
{
    saveSettings();
}

void PreferencesDialog::onOk()
{
    saveSettings();
    accept();
}

void PreferencesDialog::onCancel()
{
    reject();
}

void PreferencesDialog::onResetGeneralDefaults()
{
    m_chkStartOnLogin->setChecked(false);
    m_chkQuickSwitchEnabled->setChecked(true);
    m_editQuickSwitchHotkey->setKeySequence(QKeySequence("Ctrl+Alt+C"));
    m_editQuickSwitchHotkey->setEnabled(true);
    m_btnClearHotkey->setEnabled(true);
    m_comboDefaultConfig->setCurrentIndex(0);
}

void PreferencesDialog::onResetNotificationDefaults()
{
    // Desktop notifications defaults
    m_chkDesktopNotifEnabled->setChecked(true);
    m_chkNotifOnError->setChecked(true);
    m_chkNotifOnStateChange->setChecked(true);
    m_chkNotifOnConfigLoaded->setChecked(true);
    m_chkNotifOnKeymapSwitch->setChecked(false);
    m_chkNotifOnFocusChange->setChecked(false);
    m_chkNotifOnPerformance->setChecked(false);

    updateNotificationControlsState();

    // Sound notifications defaults
    m_chkSoundsEnabled->setChecked(false);
    m_chkSoundOnError->setChecked(true);
    m_chkSoundOnConfigLoaded->setChecked(true);
    m_chkSoundOnStateChange->setChecked(false);
    m_sliderVolume->setValue(70);
    m_labelVolumeValue->setText("70%");

    updateSoundControlsState();
}

void PreferencesDialog::onResetLoggingDefaults()
{
    int idx = m_comboLogLevel->findData("info");
    if (idx >= 0) {
        m_comboLogLevel->setCurrentIndex(idx);
    }
    m_spinBufferSize->setValue(10000);
    m_chkLogToFile->setChecked(false);
    m_editLogFilePath->clear();
    m_editLogFilePath->setEnabled(false);
    m_btnBrowseLogFile->setEnabled(false);
}

void PreferencesDialog::onResetAdvancedDefaults()
{
    m_spinPerfMetricsInterval->setValue(1000);
    m_chkShowPerformanceOverlay->setChecked(false);
    m_chkDebugMode->setChecked(false);
}

void PreferencesDialog::onTestSound()
{
    auto& soundMgr = yamy::ui::NotificationSound::instance();
    soundMgr.setVolume(m_sliderVolume->value());
    soundMgr.setEnabled(true);
    soundMgr.playForMessage(yamy::MessageType::ConfigLoaded);
    soundMgr.setEnabled(m_chkSoundsEnabled->isChecked());
}
