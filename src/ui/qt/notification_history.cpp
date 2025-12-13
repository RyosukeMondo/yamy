#include "notification_history.h"
#include <QLabel>
#include <QMetaObject>

namespace yamy::ui {

QString NotificationEntry::typeName(yamy::MessageType type)
{
    switch (type) {
        case yamy::MessageType::EngineStarting:
            return QStringLiteral("Engine Starting");
        case yamy::MessageType::EngineStarted:
            return QStringLiteral("Engine Started");
        case yamy::MessageType::EngineStopping:
            return QStringLiteral("Engine Stopping");
        case yamy::MessageType::EngineStopped:
            return QStringLiteral("Engine Stopped");
        case yamy::MessageType::EngineError:
            return QStringLiteral("Engine Error");
        case yamy::MessageType::ConfigLoading:
            return QStringLiteral("Config Loading");
        case yamy::MessageType::ConfigLoaded:
            return QStringLiteral("Config Loaded");
        case yamy::MessageType::ConfigError:
            return QStringLiteral("Config Error");
        case yamy::MessageType::ConfigValidating:
            return QStringLiteral("Config Validating");
        case yamy::MessageType::KeymapSwitched:
            return QStringLiteral("Keymap Switched");
        case yamy::MessageType::FocusChanged:
            return QStringLiteral("Focus Changed");
        case yamy::MessageType::ModifierChanged:
            return QStringLiteral("Modifier Changed");
        case yamy::MessageType::LatencyReport:
            return QStringLiteral("Latency Report");
        case yamy::MessageType::CpuUsageReport:
            return QStringLiteral("CPU Usage Report");
        default:
            return QStringLiteral("Unknown (%1)").arg(static_cast<uint32_t>(type));
    }
}

QString NotificationEntry::format() const
{
    QString result = QStringLiteral("[%1] %2")
        .arg(timestamp.toString(QStringLiteral("HH:mm:ss")))
        .arg(typeName(type));

    if (!data.isEmpty()) {
        result += QStringLiteral(": %1").arg(data);
    }

    return result;
}

NotificationHistory& NotificationHistory::instance()
{
    static NotificationHistory instance;
    return instance;
}

NotificationHistory::NotificationHistory()
    : QObject(nullptr)
    , m_maxSize(DEFAULT_MAX_SIZE)
{
}

void NotificationHistory::addNotification(yamy::MessageType type, const QString& data)
{
    NotificationEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.type = type;
    entry.data = data;

    {
        QMutexLocker locker(&m_mutex);

        m_entries.push_back(entry);

        // Trim old entries if over limit
        while (static_cast<int>(m_entries.size()) > m_maxSize) {
            m_entries.pop_front();
        }
    }

    // Emit signal (will be queued if receiver is in different thread)
    emit notificationAdded(entry);
}

std::vector<NotificationEntry> NotificationHistory::getNotifications() const
{
    QMutexLocker locker(&m_mutex);
    return std::vector<NotificationEntry>(m_entries.begin(), m_entries.end());
}

int NotificationHistory::count() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<int>(m_entries.size());
}

void NotificationHistory::clear()
{
    {
        QMutexLocker locker(&m_mutex);
        m_entries.clear();
    }

    emit historyCleared();
}

void NotificationHistory::setMaxSize(int size)
{
    if (size < 1) {
        size = 1;
    } else if (size > 1000) {
        size = 1000;
    }

    QMutexLocker locker(&m_mutex);
    m_maxSize = size;

    // Trim if necessary
    while (static_cast<int>(m_entries.size()) > m_maxSize) {
        m_entries.pop_front();
    }
}

int NotificationHistory::maxSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxSize;
}

// --- NotificationHistoryDialog Implementation ---

NotificationHistoryDialog::NotificationHistoryDialog(QWidget* parent)
    : QDialog(parent)
    , m_listWidget(nullptr)
    , m_btnClear(nullptr)
    , m_btnClose(nullptr)
{
    // Register NotificationEntry for queued signal connections
    static bool metaTypeRegistered = false;
    if (!metaTypeRegistered) {
        qRegisterMetaType<NotificationEntry>("NotificationEntry");
        metaTypeRegistered = true;
    }

    setupUI();
    populateList();

    // Connect to history updates
    NotificationHistory& history = NotificationHistory::instance();
    connect(&history, &NotificationHistory::notificationAdded,
            this, &NotificationHistoryDialog::onNotificationAdded,
            Qt::QueuedConnection);
    connect(&history, &NotificationHistory::historyCleared,
            this, &NotificationHistoryDialog::onHistoryCleared,
            Qt::QueuedConnection);
}

NotificationHistoryDialog::~NotificationHistoryDialog() = default;

void NotificationHistoryDialog::setupUI()
{
    setWindowTitle(tr("Notification History"));
    setMinimumSize(500, 400);

    auto* layout = new QVBoxLayout(this);

    // Info label
    auto* infoLabel = new QLabel(
        tr("Recent notifications (last %1):")
            .arg(NotificationHistory::DEFAULT_MAX_SIZE));
    layout->addWidget(infoLabel);

    // List widget
    m_listWidget = new QListWidget();
    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setFont(QFont(QStringLiteral("monospace")));
    layout->addWidget(m_listWidget, 1);

    // Button row
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_btnClear = new QPushButton(tr("Clear"));
    connect(m_btnClear, &QPushButton::clicked, this, &NotificationHistoryDialog::onClear);
    buttonLayout->addWidget(m_btnClear);

    m_btnClose = new QPushButton(tr("Close"));
    connect(m_btnClose, &QPushButton::clicked, this, &NotificationHistoryDialog::onClose);
    buttonLayout->addWidget(m_btnClose);

    layout->addLayout(buttonLayout);
}

void NotificationHistoryDialog::populateList()
{
    m_listWidget->clear();

    auto notifications = NotificationHistory::instance().getNotifications();
    for (const auto& entry : notifications) {
        addEntryToList(entry);
    }

    // Scroll to bottom (most recent)
    if (m_listWidget->count() > 0) {
        m_listWidget->scrollToBottom();
    }
}

void NotificationHistoryDialog::addEntryToList(const NotificationEntry& entry)
{
    auto* item = new QListWidgetItem(entry.format());

    // Color code by type
    switch (entry.type) {
        case yamy::MessageType::EngineError:
        case yamy::MessageType::ConfigError:
            item->setForeground(Qt::red);
            break;

        case yamy::MessageType::EngineStarted:
        case yamy::MessageType::ConfigLoaded:
            item->setForeground(QColor(0, 128, 0));  // Dark green
            break;

        case yamy::MessageType::EngineStarting:
        case yamy::MessageType::EngineStopping:
        case yamy::MessageType::ConfigLoading:
        case yamy::MessageType::ConfigValidating:
            item->setForeground(QColor(0, 0, 180));  // Dark blue
            break;

        default:
            break;  // Default text color
    }

    m_listWidget->addItem(item);
}

void NotificationHistoryDialog::onNotificationAdded(const NotificationEntry& entry)
{
    addEntryToList(entry);

    // Auto-scroll to bottom
    m_listWidget->scrollToBottom();
}

void NotificationHistoryDialog::onHistoryCleared()
{
    m_listWidget->clear();
}

void NotificationHistoryDialog::onClear()
{
    NotificationHistory::instance().clear();
}

void NotificationHistoryDialog::onClose()
{
    close();
}

} // namespace yamy::ui
