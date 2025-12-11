#include "dialog_log_qt.h"
#include "log_stats_panel.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QFile>
#include <QCheckBox>
#include <QScrollBar>
#include <QDateTime>

DialogLogQt::DialogLogQt(QWidget* parent)
    : QDialog(parent)
    , m_statsPanel(nullptr)
    , m_logView(nullptr)
    , m_btnClear(nullptr)
    , m_btnSave(nullptr)
    , m_btnClose(nullptr)
    , m_chkAutoScroll(nullptr)
    , m_autoScroll(true)
    , m_updateTimer(nullptr)
{
    setWindowTitle("YAMY Log Viewer");
    setMinimumSize(700, 500);

    setupUI();

    // Setup periodic log update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DialogLogQt::onUpdateLog);
    m_updateTimer->start(1000); // Update every second
}

DialogLogQt::~DialogLogQt()
{
}

void DialogLogQt::appendLog(const QString& message)
{
    // Add timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);

    m_logView->append(logEntry);

    // Update stats
    if (message.contains("ERROR", Qt::CaseInsensitive)) {
        m_statsPanel->incrementError();
    } else if (message.contains("WARNING", Qt::CaseInsensitive)) {
        m_statsPanel->incrementWarning();
    }

    // Limit log size
    QTextDocument* doc = m_logView->document();
    m_statsPanel->setTotalLines(doc->blockCount());
    if (doc->blockCount() > MAX_LOG_LINES) {
        QTextCursor cursor = m_logView->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
                          doc->blockCount() - MAX_LOG_LINES);
        cursor.removeSelectedText();
    }

    // Auto-scroll to bottom
    if (m_autoScroll) {
        scrollToBottom();
    }
}

void DialogLogQt::clearLog()
{
    m_logView->clear();
    m_statsPanel->reset();
}

void DialogLogQt::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
    m_chkAutoScroll->setChecked(enabled);
}

void DialogLogQt::onClear()
{
    int ret = QMessageBox::question(
        this,
        "Clear Log",
        "Clear all log messages?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (ret == QMessageBox::Yes) {
        clearLog();
    }
}

void DialogLogQt::onSave()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Log",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/yamy.log",
        "Log Files (*.log);;Text Files (*.txt);;All Files (*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(
            this,
            "Save Log",
            "Failed to save log file:\n" + file.errorString()
        );
        return;
    }

    QTextStream out(&file);
    out << m_logView->toPlainText();
    file.close();

    QMessageBox::information(
        this,
        "Save Log",
        "Log saved successfully to:\n" + fileName
    );
}

void DialogLogQt::onClose()
{
    close();
}

void DialogLogQt::onAutoScrollToggled(bool checked)
{
    m_autoScroll = checked;
    if (m_autoScroll) {
        scrollToBottom();
    }
}

void DialogLogQt::onUpdateLog()
{
    // TODO: Fetch new log entries from engine (Phase 6)
    // For now, this is a placeholder
}

void DialogLogQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Stats panel
    m_statsPanel = new yamy::ui::LogStatsPanel(this);
    mainLayout->addWidget(m_statsPanel);

    // Log view
    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Monospace", 9));
    m_logView->setLineWrapMode(QTextEdit::NoWrap);
    mainLayout->addWidget(m_logView);

    // Bottom controls
    QHBoxLayout* controlLayout = new QHBoxLayout();

    m_chkAutoScroll = new QCheckBox("Auto-scroll");
    m_chkAutoScroll->setChecked(m_autoScroll);
    connect(m_chkAutoScroll, &QCheckBox::toggled,
            this, &DialogLogQt::onAutoScrollToggled);
    controlLayout->addWidget(m_chkAutoScroll);

    controlLayout->addStretch();

    m_btnClear = new QPushButton("Clear");
    connect(m_btnClear, &QPushButton::clicked, this, &DialogLogQt::onClear);
    controlLayout->addWidget(m_btnClear);

    m_btnSave = new QPushButton("Save...");
    connect(m_btnSave, &QPushButton::clicked, this, &DialogLogQt::onSave);
    controlLayout->addWidget(m_btnSave);

    m_btnClose = new QPushButton("Close");
    connect(m_btnClose, &QPushButton::clicked, this, &DialogLogQt::onClose);
    controlLayout->addWidget(m_btnClose);

    mainLayout->addLayout(controlLayout);

    // Add some sample log messages
    appendLog("YAMY log viewer initialized");
    appendLog("Waiting for engine connection...");
}

void DialogLogQt::scrollToBottom()
{
    QScrollBar* scrollBar = m_logView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
