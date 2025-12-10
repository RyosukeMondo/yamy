#include "crosshair_widget_qt.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QApplication>
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

CrosshairWidget::CrosshairWidget(QWidget* parent)
    : QWidget(parent)
    , m_active(false)
    , m_updateTimer(nullptr)
{
    setupOverlay();
}

CrosshairWidget::~CrosshairWidget()
{
    if (m_active) {
        deactivate();
    }
}

void CrosshairWidget::setupOverlay()
{
    // Set window flags for overlay behavior
    setWindowFlags(Qt::WindowStaysOnTopHint |
                   Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::X11BypassWindowManagerHint);

    // Set attributes for transparency
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Enable mouse tracking for crosshair movement
    setMouseTracking(true);

    // Set crosshair cursor
    setCursor(Qt::CrossCursor);

    // Create timer for smooth crosshair updates
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });
}

void CrosshairWidget::activate()
{
    if (m_active) {
        return;
    }

    m_active = true;

    // Get the combined geometry of all screens
    QRect combinedGeometry;
    for (QScreen* screen : QApplication::screens()) {
        combinedGeometry = combinedGeometry.united(screen->geometry());
    }

    // Set geometry to cover all screens
    setGeometry(combinedGeometry);

    // Show fullscreen
    show();
    raise();

    // Grab mouse and keyboard
    grabMouse();
    grabKeyboard();
    setFocus();

    // Start update timer for smooth crosshair tracking
    m_updateTimer->start(16);  // ~60 FPS
}

void CrosshairWidget::deactivate()
{
    if (!m_active) {
        return;
    }

    m_active = false;

    // Stop update timer
    m_updateTimer->stop();

    // Release grabs
    releaseMouse();
    releaseKeyboard();

    // Hide the overlay
    hide();
}

void CrosshairWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Get cursor position relative to this widget
    QPoint center = mapFromGlobal(QCursor::pos());

    // Draw semi-transparent background to indicate selection mode
    painter.fillRect(rect(), QColor(0, 0, 0, 30));

    // Draw crosshair lines
    QPen pen(QColor(255, 0, 0, 200), 2);
    painter.setPen(pen);

    // Vertical line
    painter.drawLine(center.x(), 0, center.x(), height());

    // Horizontal line
    painter.drawLine(0, center.y(), width(), center.y());

    // Draw center dot
    painter.setBrush(QBrush(QColor(255, 0, 0)));
    painter.drawEllipse(center, 4, 4);

    // Draw coordinate info near cursor
    QString coordText = QString("(%1, %2)").arg(center.x()).arg(center.y());
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);

    // Position text offset from cursor
    QPoint textPos = center + QPoint(15, -15);

    // Draw text background for readability
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(coordText);
    textRect.moveTo(textPos);
    textRect.adjust(-4, -2, 4, 2);
    painter.fillRect(textRect, QColor(0, 0, 0, 180));

    // Draw text
    painter.setPen(Qt::white);
    painter.drawText(textPos, coordText);

    // Draw instruction text at top
    QString instructionText = "Click to select window | Press Escape to cancel";
    QRect topRect(0, 10, width(), 30);
    painter.setPen(Qt::white);
    painter.drawText(topRect, Qt::AlignHCenter, instructionText);
}

void CrosshairWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Get window under cursor
        yamy::platform::WindowHandle hwnd = getWindowAtCursor();
        deactivate();
        emit windowSelected(hwnd);
    } else if (event->button() == Qt::RightButton) {
        // Right-click cancels selection
        deactivate();
        emit selectionCancelled();
    }
}

void CrosshairWidget::mouseMoveEvent(QMouseEvent* /*event*/)
{
    // Update is handled by timer for smoother rendering
    // Just trigger immediate update if needed
    update();
}

void CrosshairWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        deactivate();
        emit selectionCancelled();
    }
}

yamy::platform::WindowHandle CrosshairWidget::getWindowAtCursor()
{
    Display* display = QX11Info::display();
    if (!display) {
        return nullptr;
    }

    Window root = DefaultRootWindow(display);

    int root_x = 0;
    int root_y = 0;
    int win_x = 0;
    int win_y = 0;
    unsigned int mask = 0;
    Window child = None;
    Window returnRoot = None;

    // Get the initial window under cursor
    XQueryPointer(display, root, &returnRoot, &child,
                  &root_x, &root_y, &win_x, &win_y, &mask);

    if (child == None) {
        // Cursor is on root window
        return reinterpret_cast<yamy::platform::WindowHandle>(root);
    }

    // Traverse to leaf window (deepest child under cursor)
    Window target = child;
    while (child != None) {
        target = child;
        XQueryPointer(display, target, &returnRoot, &child,
                      &root_x, &root_y, &win_x, &win_y, &mask);
    }

    // Skip our own overlay window
    if (target == winId()) {
        // Try to get the window just below our overlay
        // This is a bit tricky - we need to temporarily hide and re-query
        hide();
        XFlush(display);

        XQueryPointer(display, root, &returnRoot, &child,
                      &root_x, &root_y, &win_x, &win_y, &mask);

        target = child;
        while (child != None) {
            target = child;
            XQueryPointer(display, target, &returnRoot, &child,
                          &root_x, &root_y, &win_x, &win_y, &mask);
        }

        // We'll show again in deactivate(), which is called right after
    }

    return reinterpret_cast<yamy::platform::WindowHandle>(target);
}
