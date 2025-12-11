#pragma once

#include <QDateTime>
#include <QDialog>
#include <QListWidget>
#include <QMutex>
#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <deque>
#include "../../core/platform/ipc_defs.h"

namespace yamy::ui {

/**
 * @brief Entry representing a single notification in history
 */
struct NotificationEntry {
    QDateTime timestamp;
    yamy::MessageType type;
    QString data;

    /**
     * @brief Get human-readable name for message type
     */
    static QString typeName(yamy::MessageType type);

    /**
     * @brief Format entry for display
     * @return Formatted string: "[HH:MM:SS] TypeName: data"
     */
    QString format() const;
};

/**
 * @brief Thread-safe notification history storage
 *
 * Stores the most recent notifications (up to maxSize) in a rolling window.
 * Thread-safe for concurrent access from engine notifications.
 */
class NotificationHistory : public QObject {
    Q_OBJECT

public:
    static constexpr int DEFAULT_MAX_SIZE = 100;

    /**
     * @brief Get singleton instance
     */
    static NotificationHistory& instance();

    /**
     * @brief Add notification to history
     * Thread-safe, may be called from any thread.
     * @param type Message type
     * @param data Associated data string
     */
    void addNotification(yamy::MessageType type, const QString& data);

    /**
     * @brief Get all notifications (thread-safe copy)
     * @return Vector of notification entries, oldest first
     */
    std::vector<NotificationEntry> getNotifications() const;

    /**
     * @brief Get notification count
     */
    int count() const;

    /**
     * @brief Clear all notifications
     */
    void clear();

    /**
     * @brief Set maximum history size
     * @param size Maximum number of entries to keep (1-1000)
     */
    void setMaxSize(int size);

    /**
     * @brief Get maximum history size
     */
    int maxSize() const;

signals:
    /**
     * @brief Emitted when a new notification is added
     * @param entry The notification entry that was added
     */
    void notificationAdded(const NotificationEntry& entry);

    /**
     * @brief Emitted when history is cleared
     */
    void historyCleared();

private:
    NotificationHistory();
    ~NotificationHistory() = default;

    // Prevent copying
    NotificationHistory(const NotificationHistory&) = delete;
    NotificationHistory& operator=(const NotificationHistory&) = delete;

    mutable QMutex m_mutex;
    std::deque<NotificationEntry> m_entries;
    int m_maxSize;
};

/**
 * @brief Dialog for viewing notification history
 *
 * Displays a list of recent notifications with timestamps.
 * Updates in real-time as new notifications arrive.
 */
class NotificationHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit NotificationHistoryDialog(QWidget* parent = nullptr);
    ~NotificationHistoryDialog() override;

private slots:
    void onNotificationAdded(const NotificationEntry& entry);
    void onHistoryCleared();
    void onClear();
    void onClose();

private:
    void setupUI();
    void populateList();
    void addEntryToList(const NotificationEntry& entry);

    QListWidget* m_listWidget;
    QPushButton* m_btnClear;
    QPushButton* m_btnClose;
};

} // namespace yamy::ui

// Register NotificationEntry for Qt signal/slot system
Q_DECLARE_METATYPE(yamy::ui::NotificationEntry)
