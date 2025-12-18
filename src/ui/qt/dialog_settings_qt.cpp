#include "dialog_settings_qt.h"
#include "ipc_client_gui.h"
#include "core/platform/ipc_defs.h"
#include "notification_sound.h"
#include "notification_prefs.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <cstdlib>

DialogSettingsQt::DialogSettingsQt(IPCClientGUI* ipcClient, QWidget* parent)
    : QDialog(parent)
    , m_keymapList(nullptr)
    , m_btnAdd(nullptr)
    , m_btnEdit(nullptr)
    , m_btnRemove(nullptr)
    , m_btnBrowse(nullptr)
    , m_btnSave(nullptr)
    , m_btnCancel(nullptr)
    , m_editKeymapPath(nullptr)
    , m_editEditorCommand(nullptr)
    , m_btnBrowseEditor(nullptr)
    , m_labelStatus(nullptr)
    , m_chkQuickSwitchEnabled(nullptr)
    , m_editQuickSwitchHotkey(nullptr)
    , m_btnClearHotkey(nullptr)
    , m_chkSoundsEnabled(nullptr)
    , m_chkSoundOnError(nullptr)
    , m_chkSoundOnConfigLoaded(nullptr)
    , m_chkSoundOnStateChange(nullptr)
    , m_sliderVolume(nullptr)
    , m_labelVolumeValue(nullptr)
    , m_btnTestSound(nullptr)
    , m_chkDesktopNotifEnabled(nullptr)
    , m_chkNotifOnError(nullptr)
    , m_chkNotifOnConfigLoaded(nullptr)
    , m_chkNotifOnStateChange(nullptr)
    , m_chkNotifOnKeymapSwitch(nullptr)
    , m_chkNotifOnFocusChange(nullptr)
    , m_chkNotifOnPerformance(nullptr)
    , m_btnResetNotifDefaults(nullptr)
    , m_ipcClient(ipcClient)
    , m_updatingList(false)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(600);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();
    loadSettings();

    // Connect to IPC client signals
    if (m_ipcClient) {
        connect(m_ipcClient, &IPCClientGUI::configListReceived,
                this, &DialogSettingsQt::updateConfigList);
        // Request initial list
        m_ipcClient->sendGetStatus();
    }
}

DialogSettingsQt::~DialogSettingsQt()
{
}



void DialogSettingsQt::onAddKeymap()
{
    QString dir = m_editKeymapPath->text();
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Add Keymap Files"),
        dir,
        tr("JSON Config Files (*.json);;All Files (*)")
    );

    if (files.isEmpty()) {
        return;
    }

    for (const QString& file : files) {
        if (!m_ipcClient) continue;
        m_ipcClient->sendAddConfig(file);
    }
}

void DialogSettingsQt::onEditKeymap()
{
    QListWidgetItem* item = m_keymapList->currentItem();
    if (!item) {
        return;
    }

    QString file = item->text();
    openInEditor(file);
}

void DialogSettingsQt::openInEditor(const QString& path)
{
    QSettings settings("YAMY", "YAMY");
    QString editorCmd = settings.value("editor/command", "").toString();

    // Strategy 1: Use configured editor command
    if (!editorCmd.isEmpty()) {
        if (launchEditor(editorCmd, path)) {
            m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
            return;
        }
    }

    // Strategy 2: Try $EDITOR environment variable
    const char* envEditor = std::getenv("EDITOR");
    if (envEditor && strlen(envEditor) > 0) {
        QString envEditorCmd = QString::fromLocal8Bit(envEditor) + " %f";
        if (launchEditor(envEditorCmd, path)) {
            m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
            return;
        }
    }

    // Strategy 3: Try $VISUAL environment variable
    const char* envVisual = std::getenv("VISUAL");
    if (envVisual && strlen(envVisual) > 0) {
        QString visualCmd = QString::fromLocal8Bit(envVisual) + " %f";
        if (launchEditor(visualCmd, path)) {
            m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
            return;
        }
    }

    // Strategy 4: Use QDesktopServices
    QUrl fileUrl = QUrl::fromLocalFile(path);
    if (QDesktopServices::openUrl(fileUrl)) {
        m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
        return;
    }

    // Strategy 5: Platform fallback
#ifdef Q_OS_LINUX
    QProcess process;
    process.setProgram("xdg-open");
    process.setArguments({path});
    if (process.startDetached()) {
        m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
        return;
    }
#endif

    // All strategies failed
    QMessageBox::warning(
        this,
        "Editor Error",
        QString("Failed to open file in editor.\n\n"
                "File: %1\n\n"
                "Please configure an editor in the External Editor setting below.")
            .arg(path)
    );
}

bool DialogSettingsQt::launchEditor(const QString& editorCmd, const QString& filePath)
{
    if (editorCmd.isEmpty()) {
        return false;
    }

    QString cmd = editorCmd;

    // Quote the file path if it contains spaces
    QString quotedPath = filePath;
    if (quotedPath.contains(' ') && !quotedPath.startsWith('"')) {
        quotedPath = "\"" + quotedPath + "\"";
    }

    // Replace %f placeholder or append file path
    if (cmd.contains("%f")) {
        cmd.replace("%f", quotedPath);
    } else {
        cmd += " " + quotedPath;
    }

    // Parse command line into program and arguments
    QString program;
    QStringList args;
    bool inQuote = false;
    QString currentArg;

    for (int i = 0; i < cmd.length(); ++i) {
        QChar c = cmd[i];

        if (c == '"') {
            inQuote = !inQuote;
        } else if (c == ' ' && !inQuote) {
            if (!currentArg.isEmpty()) {
                if (program.isEmpty()) {
                    program = currentArg;
                } else {
                    args.append(currentArg);
                }
                currentArg.clear();
            }
        } else {
            currentArg += c;
        }
    }

    if (!currentArg.isEmpty()) {
        if (program.isEmpty()) {
            program = currentArg;
        } else {
            args.append(currentArg);
        }
    }

    if (program.isEmpty()) {
        return false;
    }

    QProcess* process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(args);

    connect(process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            process, &QProcess::deleteLater);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);

    qint64 pid = 0;
    if (process->startDetached(&pid)) {
        return true;
    }

    delete process;
    return false;
}

void DialogSettingsQt::onRemoveKeymap()
{
    QList<QListWidgetItem*> items = m_keymapList->selectedItems();
    if (items.isEmpty()) {
        return;
    }

    for (auto* item : items) {
        QString path = item->text();
        if (m_ipcClient) {
            m_ipcClient->sendRemoveConfig(path);
        }
    }
}

void DialogSettingsQt::updateConfigList(const yamy::RspConfigListPayload& payload)
{
    m_updatingList = true;
    m_keymapFiles.clear();
    m_keymapList->clear();

    for (uint32_t i = 0; i < payload.count; ++i) {
        QString configName = QString::fromUtf8(payload.configs[i].data());
        if (!configName.isEmpty()) {
            m_keymapFiles.append(configName);
            m_keymapList->addItem(configName);
        }
    }

    m_updatingList = false;
    onKeymapSelectionChanged();
}

void DialogSettingsQt::onBrowseKeymap()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Keymap Directory",
        m_editKeymapPath->text().isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
            : m_editKeymapPath->text()
    );

    if (!dir.isEmpty()) {
        m_editKeymapPath->setText(dir);
    }
}

void DialogSettingsQt::onBrowseEditor()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "Select Editor Executable",
        "/usr/bin",
        "Executables (*)"
    );

    if (!file.isEmpty()) {
        // Quote the path if it contains spaces and add %f placeholder
        if (file.contains(' ')) {
            file = "\"" + file + "\"";
        }
        m_editEditorCommand->setText(file + " %f");
    }
}

void DialogSettingsQt::onKeymapSelectionChanged()
{
    bool hasSelection = m_keymapList->currentItem() != nullptr;
    m_btnEdit->setEnabled(hasSelection);
    m_btnRemove->setEnabled(hasSelection);
}

void DialogSettingsQt::onSave()
{
    saveSettings();
    m_labelStatus->setText("Settings saved successfully");

    QMessageBox::information(
        this,
        "YAMY",
        "Settings saved. Reload configuration to apply changes."
    );

    accept();
}

void DialogSettingsQt::onCancel()
{
    reject();
}

void DialogSettingsQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Keymap files group
    QGroupBox* keymapGroup = new QGroupBox("Keymap Files");
    QVBoxLayout* keymapLayout = new QVBoxLayout(keymapGroup);

    m_keymapList = new QListWidget();
    m_keymapList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_keymapList, &QListWidget::itemSelectionChanged,
            this, &DialogSettingsQt::onKeymapSelectionChanged);
    keymapLayout->addWidget(m_keymapList);

    // Button row
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnAdd = new QPushButton("Add...");
    connect(m_btnAdd, &QPushButton::clicked, this, &DialogSettingsQt::onAddKeymap);
    btnLayout->addWidget(m_btnAdd);

    m_btnEdit = new QPushButton("Edit...");
    m_btnEdit->setEnabled(false);
    connect(m_btnEdit, &QPushButton::clicked, this, &DialogSettingsQt::onEditKeymap);
    btnLayout->addWidget(m_btnEdit);

    m_btnRemove = new QPushButton("Remove");
    m_btnRemove->setEnabled(false);
    connect(m_btnRemove, &QPushButton::clicked, this, &DialogSettingsQt::onRemoveKeymap);
    btnLayout->addWidget(m_btnRemove);

    btnLayout->addStretch();
    keymapLayout->addLayout(btnLayout);
    mainLayout->addWidget(keymapGroup);

    // Keymap path group
    QGroupBox* pathGroup = new QGroupBox("Keymap Directory");
    QHBoxLayout* pathLayout = new QHBoxLayout(pathGroup);

    m_editKeymapPath = new QLineEdit();
    m_editKeymapPath->setPlaceholderText("Directory containing .json config files");
    pathLayout->addWidget(m_editKeymapPath);

    m_btnBrowse = new QPushButton("Browse...");
    connect(m_btnBrowse, &QPushButton::clicked, this, &DialogSettingsQt::onBrowseKeymap);
    pathLayout->addWidget(m_btnBrowse);

    mainLayout->addWidget(pathGroup);

    // Editor configuration group
    QGroupBox* editorGroup = new QGroupBox("External Editor");
    QVBoxLayout* editorLayout = new QVBoxLayout(editorGroup);

    QHBoxLayout* editorCmdLayout = new QHBoxLayout();
    m_editEditorCommand = new QLineEdit();
    m_editEditorCommand->setPlaceholderText("Leave empty to use system default ($EDITOR or xdg-open)");
    editorCmdLayout->addWidget(m_editEditorCommand);

    m_btnBrowseEditor = new QPushButton("Browse...");
    connect(m_btnBrowseEditor, &QPushButton::clicked, this, &DialogSettingsQt::onBrowseEditor);
    editorCmdLayout->addWidget(m_btnBrowseEditor);

    editorLayout->addLayout(editorCmdLayout);

    QLabel* editorHelp = new QLabel(
        "Specify a command to open configuration files. Use %f as a placeholder for the file path.\n"
        "Examples: code %f, gedit %f, vim %f, nano %f"
    );
    editorHelp->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    editorHelp->setWordWrap(true);
    editorLayout->addWidget(editorHelp);

    mainLayout->addWidget(editorGroup);

    // Quick-switch hotkey group
    QGroupBox* hotkeyGroup = new QGroupBox("Config Quick-Switch Hotkey");
    QVBoxLayout* hotkeyLayout = new QVBoxLayout(hotkeyGroup);

    m_chkQuickSwitchEnabled = new QCheckBox("Enable quick-switch hotkey");
    m_chkQuickSwitchEnabled->setChecked(true);
    hotkeyLayout->addWidget(m_chkQuickSwitchEnabled);

    QHBoxLayout* hotkeyEditLayout = new QHBoxLayout();

    QLabel* hotkeyLabel = new QLabel("Hotkey:");
    hotkeyEditLayout->addWidget(hotkeyLabel);

    m_editQuickSwitchHotkey = new QKeySequenceEdit();
    m_editQuickSwitchHotkey->setKeySequence(QKeySequence("Ctrl+Alt+C"));
    hotkeyEditLayout->addWidget(m_editQuickSwitchHotkey);

    m_btnClearHotkey = new QPushButton("Clear");
    connect(m_btnClearHotkey, &QPushButton::clicked, this, [this]() {
        m_editQuickSwitchHotkey->clear();
    });
    hotkeyEditLayout->addWidget(m_btnClearHotkey);

    hotkeyEditLayout->addStretch();
    hotkeyLayout->addLayout(hotkeyEditLayout);

    QLabel* hotkeyHelp = new QLabel(
        "Press the hotkey to cycle through available configurations.\n"
        "Default: Ctrl+Alt+C. Leave empty to disable."
    );
    hotkeyHelp->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    hotkeyHelp->setWordWrap(true);
    hotkeyLayout->addWidget(hotkeyHelp);

    // Connect checkbox to enable/disable hotkey edit
    connect(m_chkQuickSwitchEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        m_editQuickSwitchHotkey->setEnabled(checked);
        m_btnClearHotkey->setEnabled(checked);
    });

    mainLayout->addWidget(hotkeyGroup);

    // Notification sounds group
    QGroupBox* soundGroup = new QGroupBox("Notification Sounds");
    QVBoxLayout* soundLayout = new QVBoxLayout(soundGroup);

    m_chkSoundsEnabled = new QCheckBox("Enable notification sounds");
    m_chkSoundsEnabled->setChecked(false);
    soundLayout->addWidget(m_chkSoundsEnabled);

    // Individual sound type checkboxes
    QHBoxLayout* soundTypesLayout = new QHBoxLayout();

    m_chkSoundOnError = new QCheckBox("On error");
    m_chkSoundOnError->setChecked(true);
    m_chkSoundOnError->setEnabled(false);
    soundTypesLayout->addWidget(m_chkSoundOnError);

    m_chkSoundOnConfigLoaded = new QCheckBox("On config loaded");
    m_chkSoundOnConfigLoaded->setChecked(true);
    m_chkSoundOnConfigLoaded->setEnabled(false);
    soundTypesLayout->addWidget(m_chkSoundOnConfigLoaded);

    m_chkSoundOnStateChange = new QCheckBox("On state change");
    m_chkSoundOnStateChange->setChecked(false);
    m_chkSoundOnStateChange->setEnabled(false);
    soundTypesLayout->addWidget(m_chkSoundOnStateChange);

    soundTypesLayout->addStretch();
    soundLayout->addLayout(soundTypesLayout);

    // Volume slider
    QHBoxLayout* volumeLayout = new QHBoxLayout();

    QLabel* volumeLabel = new QLabel("Volume:");
    volumeLayout->addWidget(volumeLabel);

    m_sliderVolume = new QSlider(Qt::Horizontal);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(70);
    m_sliderVolume->setEnabled(false);
    volumeLayout->addWidget(m_sliderVolume);

    m_labelVolumeValue = new QLabel("70%");
    m_labelVolumeValue->setMinimumWidth(40);
    volumeLayout->addWidget(m_labelVolumeValue);

    m_btnTestSound = new QPushButton("Test");
    m_btnTestSound->setEnabled(false);
    connect(m_btnTestSound, &QPushButton::clicked, this, [this]() {
        yamy::ui::NotificationSound::instance().setVolume(m_sliderVolume->value());
        yamy::ui::NotificationSound::instance().setEnabled(true);
        yamy::ui::NotificationSound::instance().playForMessage(yamy::MessageType::ConfigLoaded);
        yamy::ui::NotificationSound::instance().setEnabled(m_chkSoundsEnabled->isChecked());
    });
    volumeLayout->addWidget(m_btnTestSound);

    soundLayout->addLayout(volumeLayout);

    // Connect checkbox to enable/disable sound controls
    connect(m_chkSoundsEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        m_chkSoundOnError->setEnabled(checked);
        m_chkSoundOnConfigLoaded->setEnabled(checked);
        m_chkSoundOnStateChange->setEnabled(checked);
        m_sliderVolume->setEnabled(checked);
        m_btnTestSound->setEnabled(checked);
    });

    // Connect slider to update label
    connect(m_sliderVolume, &QSlider::valueChanged, this, [this](int value) {
        m_labelVolumeValue->setText(QString("%1%").arg(value));
    });

    QLabel* soundHelp = new QLabel(
        "Play sounds on notification events. Sounds use system theme or bundled files.\n"
        "Sounds are brief and non-intrusive."
    );
    soundHelp->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    soundHelp->setWordWrap(true);
    soundLayout->addWidget(soundHelp);

    mainLayout->addWidget(soundGroup);

    // Desktop notifications group
    QGroupBox* desktopNotifGroup = new QGroupBox("Desktop Notifications");
    QVBoxLayout* desktopNotifLayout = new QVBoxLayout(desktopNotifGroup);

    m_chkDesktopNotifEnabled = new QCheckBox("Enable desktop notifications");
    m_chkDesktopNotifEnabled->setChecked(true);
    desktopNotifLayout->addWidget(m_chkDesktopNotifEnabled);

    // Notification type checkboxes - first row
    QHBoxLayout* notifTypesRow1 = new QHBoxLayout();

    m_chkNotifOnError = new QCheckBox("Show errors (always)");
    m_chkNotifOnError->setChecked(true);
    m_chkNotifOnError->setEnabled(false);  // Always enabled
    m_chkNotifOnError->setToolTip("Error notifications are always shown for safety");
    notifTypesRow1->addWidget(m_chkNotifOnError);

    m_chkNotifOnStateChange = new QCheckBox("Engine state changes");
    m_chkNotifOnStateChange->setChecked(true);
    m_chkNotifOnStateChange->setToolTip("Show when engine starts/stops");
    notifTypesRow1->addWidget(m_chkNotifOnStateChange);

    m_chkNotifOnConfigLoaded = new QCheckBox("Config changes");
    m_chkNotifOnConfigLoaded->setChecked(true);
    m_chkNotifOnConfigLoaded->setToolTip("Show when configuration is loaded");
    notifTypesRow1->addWidget(m_chkNotifOnConfigLoaded);

    notifTypesRow1->addStretch();
    desktopNotifLayout->addLayout(notifTypesRow1);

    // Notification type checkboxes - second row
    QHBoxLayout* notifTypesRow2 = new QHBoxLayout();

    m_chkNotifOnKeymapSwitch = new QCheckBox("Keymap switches");
    m_chkNotifOnKeymapSwitch->setChecked(false);
    m_chkNotifOnKeymapSwitch->setToolTip("Show when keymap is switched");
    notifTypesRow2->addWidget(m_chkNotifOnKeymapSwitch);

    m_chkNotifOnFocusChange = new QCheckBox("Focus changes");
    m_chkNotifOnFocusChange->setChecked(false);
    m_chkNotifOnFocusChange->setToolTip("Show when active window changes (verbose)");
    notifTypesRow2->addWidget(m_chkNotifOnFocusChange);

    m_chkNotifOnPerformance = new QCheckBox("Performance metrics");
    m_chkNotifOnPerformance->setChecked(false);
    m_chkNotifOnPerformance->setToolTip("Show latency and CPU usage reports (verbose)");
    notifTypesRow2->addWidget(m_chkNotifOnPerformance);

    notifTypesRow2->addStretch();
    desktopNotifLayout->addLayout(notifTypesRow2);

    // Reset to defaults button
    QHBoxLayout* resetLayout = new QHBoxLayout();
    resetLayout->addStretch();
    m_btnResetNotifDefaults = new QPushButton("Reset to Defaults");
    m_btnResetNotifDefaults->setToolTip("Reset notification preferences to defaults");
    connect(m_btnResetNotifDefaults, &QPushButton::clicked, this, [this]() {
        m_chkDesktopNotifEnabled->setChecked(true);
        m_chkNotifOnError->setChecked(true);
        m_chkNotifOnStateChange->setChecked(true);
        m_chkNotifOnConfigLoaded->setChecked(true);
        m_chkNotifOnKeymapSwitch->setChecked(false);
        m_chkNotifOnFocusChange->setChecked(false);
        m_chkNotifOnPerformance->setChecked(false);
        m_labelStatus->setText("Notification preferences reset to defaults");
    });
    resetLayout->addWidget(m_btnResetNotifDefaults);
    desktopNotifLayout->addLayout(resetLayout);

    // Connect master checkbox to enable/disable individual checkboxes
    connect(m_chkDesktopNotifEnabled, &QCheckBox::toggled, this, [this](bool checked) {
        // Error is always enabled but checkbox is always disabled
        m_chkNotifOnStateChange->setEnabled(checked);
        m_chkNotifOnConfigLoaded->setEnabled(checked);
        m_chkNotifOnKeymapSwitch->setEnabled(checked);
        m_chkNotifOnFocusChange->setEnabled(checked);
        m_chkNotifOnPerformance->setEnabled(checked);
        m_btnResetNotifDefaults->setEnabled(checked);
    });

    QLabel* desktopNotifHelp = new QLabel(
        "Desktop notifications appear in your system notification area.\n"
        "Errors are always shown (10s timeout). Other notifications use 3s timeout."
    );
    desktopNotifHelp->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    desktopNotifHelp->setWordWrap(true);
    desktopNotifLayout->addWidget(desktopNotifHelp);

    mainLayout->addWidget(desktopNotifGroup);

    // Status label
    m_labelStatus = new QLabel();
    m_labelStatus->setStyleSheet("QLabel { color: #666; }");
    mainLayout->addWidget(m_labelStatus);

    // Dialog buttons
    QHBoxLayout* dialogBtnLayout = new QHBoxLayout();
    dialogBtnLayout->addStretch();

    m_btnSave = new QPushButton("Save");
    m_btnSave->setDefault(true);
    connect(m_btnSave, &QPushButton::clicked, this, &DialogSettingsQt::onSave);
    dialogBtnLayout->addWidget(m_btnSave);

    m_btnCancel = new QPushButton("Cancel");
    connect(m_btnCancel, &QPushButton::clicked, this, &DialogSettingsQt::onCancel);
    dialogBtnLayout->addWidget(m_btnCancel);

    mainLayout->addLayout(dialogBtnLayout);
}

void DialogSettingsQt::loadSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Keymap files are now loaded from the daemon via IPC
    // m_keymapFiles = settings.value("keymaps/files").toStringList();
    m_keymapList->clear();
    // m_keymapList->addItems(m_keymapFiles);

    // Load keymap directory
    QString keymapDir = settings.value("keymaps/directory",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.yamy"
    ).toString();
    m_editKeymapPath->setText(keymapDir);

    // Load editor command
    QString editorCmd = settings.value("editor/command", "").toString();
    m_editEditorCommand->setText(editorCmd);

    // Load quick-switch hotkey settings
    bool hotkeyEnabled = settings.value("hotkeys/quickSwitch/enabled", true).toBool();
    QString hotkeySeq = settings.value("hotkeys/quickSwitch/sequence", "Ctrl+Alt+C").toString();

    m_chkQuickSwitchEnabled->setChecked(hotkeyEnabled);
    m_editQuickSwitchHotkey->setKeySequence(QKeySequence(hotkeySeq));
    m_editQuickSwitchHotkey->setEnabled(hotkeyEnabled);
    m_btnClearHotkey->setEnabled(hotkeyEnabled);

    // Load notification sound settings
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

    // Enable/disable sound controls based on master switch
    m_chkSoundOnError->setEnabled(soundsEnabled);
    m_chkSoundOnConfigLoaded->setEnabled(soundsEnabled);
    m_chkSoundOnStateChange->setEnabled(soundsEnabled);
    m_sliderVolume->setEnabled(soundsEnabled);
    m_btnTestSound->setEnabled(soundsEnabled);

    // Load desktop notification settings from NotificationPrefs
    auto& notifPrefs = yamy::ui::NotificationPrefs::instance();
    bool desktopNotifEnabled = notifPrefs.isEnabled();
    m_chkDesktopNotifEnabled->setChecked(desktopNotifEnabled);
    m_chkNotifOnError->setChecked(notifPrefs.isErrorNotificationEnabled());
    m_chkNotifOnConfigLoaded->setChecked(notifPrefs.isConfigLoadedNotificationEnabled());
    m_chkNotifOnStateChange->setChecked(notifPrefs.isStateChangeNotificationEnabled());
    m_chkNotifOnKeymapSwitch->setChecked(notifPrefs.isKeymapSwitchNotificationEnabled());
    m_chkNotifOnFocusChange->setChecked(notifPrefs.isFocusChangeNotificationEnabled());
    m_chkNotifOnPerformance->setChecked(notifPrefs.isPerformanceNotificationEnabled());

    // Enable/disable controls based on master switch
    m_chkNotifOnStateChange->setEnabled(desktopNotifEnabled);
    m_chkNotifOnConfigLoaded->setEnabled(desktopNotifEnabled);
    m_chkNotifOnKeymapSwitch->setEnabled(desktopNotifEnabled);
    m_chkNotifOnFocusChange->setEnabled(desktopNotifEnabled);
    m_chkNotifOnPerformance->setEnabled(desktopNotifEnabled);
    m_btnResetNotifDefaults->setEnabled(desktopNotifEnabled);

    m_labelStatus->setText("Settings loaded");
}

void DialogSettingsQt::saveSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Save keymap files
    // Note: Config list is now persisted by the daemon.
    // We don't save the list here anymore to avoid conflicts.
    // settings.setValue("keymaps/files", m_keymapFiles);

    // Save keymap directory
    settings.setValue("keymaps/directory", m_editKeymapPath->text());

    // Save editor command
    settings.setValue("editor/command", m_editEditorCommand->text());

    // Save quick-switch hotkey settings
    settings.setValue("hotkeys/quickSwitch/enabled", m_chkQuickSwitchEnabled->isChecked());
    settings.setValue("hotkeys/quickSwitch/sequence",
                      m_editQuickSwitchHotkey->keySequence().toString());

    // Save notification sound settings
    settings.setValue("notifications/sounds/enabled", m_chkSoundsEnabled->isChecked());
    settings.setValue("notifications/sounds/onError", m_chkSoundOnError->isChecked());
    settings.setValue("notifications/sounds/onConfigLoaded", m_chkSoundOnConfigLoaded->isChecked());
    settings.setValue("notifications/sounds/onStateChange", m_chkSoundOnStateChange->isChecked());
    settings.setValue("notifications/sounds/volume", m_sliderVolume->value());

    settings.sync();

    // Reload notification sound settings into the singleton
    auto& soundMgr = yamy::ui::NotificationSound::instance();
    soundMgr.setEnabled(m_chkSoundsEnabled->isChecked());
    soundMgr.setErrorSoundEnabled(m_chkSoundOnError->isChecked());
    soundMgr.setConfigLoadedSoundEnabled(m_chkSoundOnConfigLoaded->isChecked());
    soundMgr.setStateChangeSoundEnabled(m_chkSoundOnStateChange->isChecked());
    soundMgr.setVolume(m_sliderVolume->value());

    // Save desktop notification settings to NotificationPrefs
    auto& notifPrefs = yamy::ui::NotificationPrefs::instance();
    notifPrefs.setEnabled(m_chkDesktopNotifEnabled->isChecked());
    notifPrefs.setErrorNotificationEnabled(m_chkNotifOnError->isChecked());
    notifPrefs.setConfigLoadedNotificationEnabled(m_chkNotifOnConfigLoaded->isChecked());
    notifPrefs.setStateChangeNotificationEnabled(m_chkNotifOnStateChange->isChecked());
    notifPrefs.setKeymapSwitchNotificationEnabled(m_chkNotifOnKeymapSwitch->isChecked());
    notifPrefs.setFocusChangeNotificationEnabled(m_chkNotifOnFocusChange->isChecked());
    notifPrefs.setPerformanceNotificationEnabled(m_chkNotifOnPerformance->isChecked());
    notifPrefs.saveSettings();
}
