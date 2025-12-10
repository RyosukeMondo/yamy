#pragma once

#include <QWidget>
#include <QTimer>
#include "types.h"

/**
 * @brief Crosshair widget for window selection
 *
 * Provides a fullscreen transparent overlay with a crosshair cursor
 * that allows the user to select a window under the cursor.
 * Uses X11 XQueryPointer to get the actual window under the cursor
 * (traversing to the leaf window).
 */
class CrosshairWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct crosshair widget
     * @param parent Parent widget (usually nullptr for top-level)
     */
    explicit CrosshairWidget(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CrosshairWidget() override;

    /**
     * @brief Activate the crosshair overlay
     *
     * Shows the fullscreen overlay, grabs the mouse, and sets focus.
     * The crosshair will follow the cursor until a window is selected.
     */
    void activate();

    /**
     * @brief Deactivate the crosshair overlay
     *
     * Releases the mouse grab and hides the overlay.
     */
    void deactivate();

    /**
     * @brief Check if the widget is currently active
     * @return true if active, false otherwise
     */
    bool isActive() const { return m_active; }

signals:
    /**
     * @brief Emitted when a window is selected
     * @param hwnd Handle of the selected window
     */
    void windowSelected(yamy::platform::WindowHandle hwnd);

    /**
     * @brief Emitted when selection is cancelled (e.g., Escape key)
     */
    void selectionCancelled();

protected:
    /**
     * @brief Paint event - draws the crosshair
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Mouse press event - selects window under cursor
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Mouse move event - triggers repaint for crosshair
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Key press event - handles Escape to cancel
     */
    void keyPressEvent(QKeyEvent* event) override;

private:
    /**
     * @brief Get the window under the cursor using X11 API
     * @return Window handle of the window under cursor
     *
     * Traverses the window hierarchy to find the leaf window
     * under the current cursor position.
     */
    yamy::platform::WindowHandle getWindowAtCursor();

    /**
     * @brief Setup the widget for overlay mode
     */
    void setupOverlay();

    bool m_active{false};
    QTimer* m_updateTimer{nullptr};
};
