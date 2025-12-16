#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QKeySequenceEdit>
#include <QCheckBox>
#include <QSlider>

class IPCClientGUI;
namespace yamy { struct RspConfigListPayload; }

/**
 * @brief Settings dialog for YAMY configuration
 *
 * Allows users to:
 * - Manage keymap files (add, edit, remove)
 * - Configure keymap search paths
 * - Set application preferences
 */
class DialogSettingsQt : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct settings dialog
     * @param ipcClient Pointer to IPC client for daemon communication
     * @param parent Parent widget
     */
    explicit DialogSettingsQt(IPCClientGUI* ipcClient, QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DialogSettingsQt() override;
    
    /**
     * @brief Update the list of keymap files from the daemon response
     * @param payload Config list payload
     */
    void updateConfigList(const yamy::RspConfigListPayload& payload);

private slots:
    /**
     * @brief Add new keymap file
     */
    void onAddKeymap();

    /**
     * @brief Edit selected keymap file
     */
    void onEditKeymap();

    /**
     * @brief Remove selected keymap file
     */
    void onRemoveKeymap();

    /**
     * @brief Browse for keymap file
     */
    void onBrowseKeymap();

    /**
     * @brief Browse for editor executable
     */
    void onBrowseEditor();

    /**
     * @brief Update button states based on selection
     */
    void onKeymapSelectionChanged();

    /**
     * @brief Save settings
     */
    void onSave();

    /**
     * @brief Cancel and close
     */
    void onCancel();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Load settings from configuration
     */
    void loadSettings();

    /**
     * @brief Save settings to configuration
     */
    void saveSettings();

    /**
     * @brief Open a file in external editor
     * @param path Path to the file to open
     *
     * Uses configured editor, $EDITOR/$VISUAL env vars, or system default
     */
    void openInEditor(const QString& path);

    /**
     * @brief Launch an editor with the specified command
     * @param editorCmd Editor command (may contain %f placeholder)
     * @param filePath Path to the file to open
     * @return true if editor was launched successfully
     */
    bool launchEditor(const QString& editorCmd, const QString& filePath);

    // UI Components
    QListWidget* m_keymapList;
    QPushButton* m_btnAdd;
    QPushButton* m_btnEdit;
    QPushButton* m_btnRemove;
    QPushButton* m_btnBrowse;
    QPushButton* m_btnSave;
    QPushButton* m_btnCancel;

    QLineEdit* m_editKeymapPath;
    QLineEdit* m_editEditorCommand;
    QPushButton* m_btnBrowseEditor;
    QLabel* m_labelStatus;

    // Quick-switch hotkey components
    QCheckBox* m_chkQuickSwitchEnabled;
    QKeySequenceEdit* m_editQuickSwitchHotkey;
    QPushButton* m_btnClearHotkey;

    // Notification sound components
    QCheckBox* m_chkSoundsEnabled;
    QCheckBox* m_chkSoundOnError;
    QCheckBox* m_chkSoundOnConfigLoaded;
    QCheckBox* m_chkSoundOnStateChange;
    QSlider* m_sliderVolume;
    QLabel* m_labelVolumeValue;
    QPushButton* m_btnTestSound;

    // Desktop notification components
    QCheckBox* m_chkDesktopNotifEnabled;
    QCheckBox* m_chkNotifOnError;
    QCheckBox* m_chkNotifOnConfigLoaded;
    QCheckBox* m_chkNotifOnStateChange;
    QCheckBox* m_chkNotifOnKeymapSwitch;
    QCheckBox* m_chkNotifOnFocusChange;
    QCheckBox* m_chkNotifOnPerformance;
    QPushButton* m_btnResetNotifDefaults;

    // Data
    QStringList m_keymapFiles;
    IPCClientGUI* m_ipcClient;
    
    // Config list state
    bool m_updatingList;
};
