#pragma once

#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/platform/types.h"
#include "core/platform/window_system_interface.h"

#include <memory>

#include "core/ipc_messages.h"
#include "core/platform/ipc_channel_interface.h"

class CrosshairWidget;
class Engine;

/**
 * @brief Investigate dialog for inspecting windows and keymap status
 *
 * Provides:
 * - Crosshair-based window selection
 * - Window information panel (handle, title, class, geometry, state)
 * - Process information (name, path)
 * - Keymap status panel (matched regex, active keymap, modifiers)
 * - Live log panel for real-time key events
 */
class DialogInvestigateQt : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct investigate dialog
     * @param engine Pointer to keyboard remapping engine (can be nullptr)
     * @param parent Parent widget
     */
    explicit DialogInvestigateQt(Engine* engine = nullptr, QWidget* parent = nullptr);

    /**
     * @brief Set the engine instance
     * @param engine Pointer to engine
     */
    void setEngine(Engine* engine);

    /**
     * @brief Set the window system for testing
     * @param ws Window system instance
     */
    void setWindowSystem(std::unique_ptr<yamy::platform::IWindowSystem> ws) { m_windowSystem = std::move(ws); }

    /**
     * @brief Set the IPC channel for testing
     * @param ipc IPC channel instance
     */
    void setIpcChannel(std::unique_ptr<yamy::platform::IIPCChannel> ipc) { m_ipcChannel = std::move(ipc); }

    /**
     * @brief Destructor
     */
    ~DialogInvestigateQt() override;

signals:
    /**
     * @brief Emitted when a window is selected for investigation
     * @param hwnd Handle of the selected window
     */
    void windowInvestigated(yamy::platform::WindowHandle hwnd);

private slots:
    /**
     * @brief Handle Select Window button click
     */
    void onSelectWindow();

    /**
     * @brief Handle window selection from crosshair
     * @param hwnd Handle of the selected window
     */
    void onWindowSelected(yamy::platform::WindowHandle hwnd);

    /**
     * @brief Handle selection cancelled (Escape pressed)
     */
    void onSelectionCancelled();

    /**
     * @brief Handle IPC messages from the engine
     * @param message The received message
     */
    void onIpcMessageReceived(const yamy::ipc::Message& message);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Create window information panel
     * @return Group box containing window info labels
     */
    QGroupBox* createWindowInfoPanel();

    /**
     * @brief Create keymap status panel
     * @return Group box containing keymap status labels
     */
    QGroupBox* createKeymapStatusPanel();

    /**
     * @brief Create live log panel
     * @return Group box containing live log text edit
     */
    QGroupBox* createLiveLogPanel();

    /**
     * @brief Clear all information panels
     */
    void clearPanels();

    /**
     * @brief Update window information panel with data from selected window
     * @param hwnd Handle of the window to retrieve info from
     *
     * Uses IWindowSystem to query window properties:
     * - Title (getWindowText)
     * - Class name (getClassName)
     * - Geometry (getWindowRect)
     * - Visibility state (getShowCommand)
     * - Process name and path (via /proc filesystem)
     * Handles invalid windows gracefully.
     */
    void updateWindowInfo(yamy::platform::WindowHandle hwnd);

    /**
     * @brief Get process name from PID by reading /proc/{pid}/comm
     * @param pid Process ID
     * @return Process name or empty string if unavailable
     */
    QString getProcessName(uint32_t pid);

    /**
     * @brief Get process executable path from PID by reading /proc/{pid}/exe
     * @param pid Process ID
     * @return Full path to executable or empty string if unavailable
     */
    QString getProcessPath(uint32_t pid);

    /**
     * @brief Update keymap status panel with data from engine
     * @param hwnd Handle of the window to query keymap for
     * @param className Window class name
     * @param titleName Window title name
     *
     * Queries the Engine for the active keymap for the given window
     * and updates the KeymapStatusPanel with:
     * - Keymap name
     * - Matched regex pattern (class and/or title)
     * - Active modifiers
     */
    void updateKeymapStatus(yamy::platform::WindowHandle hwnd,
                            const std::string& className,
                            const std::string& titleName);

    // Engine instance (not owned)
    Engine* m_engine;

    // IPC channel for communicating with the engine
    std::unique_ptr<yamy::platform::IIPCChannel> m_ipcChannel;

    // Window system interface for platform abstraction
    std::unique_ptr<yamy::platform::IWindowSystem> m_windowSystem;

    // Crosshair widget for window selection
    CrosshairWidget* m_crosshair;

    // Window info panel labels
    QLabel* m_labelHandle;
    QLabel* m_labelTitle;
    QLabel* m_labelClass;
    QLabel* m_labelProcess;
    QLabel* m_labelProcessPath;
    QLabel* m_labelGeometry;
    QLabel* m_labelState;

    // Keymap status panel labels
    QLabel* m_labelKeymapName;
    QLabel* m_labelMatchedRegex;
    QLabel* m_labelModifiers;

    // Live log panel
    QTextEdit* m_liveLog;

    // Buttons
    QPushButton* m_btnSelectWindow;
    QPushButton* m_btnCopyToClipboard;
    QPushButton* m_btnGenerateCondition;
    QPushButton* m_btnClose;

    // Currently selected window
    yamy::platform::WindowHandle m_selectedWindow;
};
