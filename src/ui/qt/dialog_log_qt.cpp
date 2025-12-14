#include "dialog_log_qt.h"
#include "core/logging/logger.h"
#include "log_stats_panel.h"
#include <algorithm>
#include <QCheckBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCharFormat>
// QTextCodec removed in Qt 6 - QTextStream uses UTF-8 by default
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>

DialogLogQt::DialogLogQt(QWidget* parent)
    : QDialog(parent)
    , m_levelFilter(nullptr)
    , m_categoryGroup(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeSpinner(nullptr)
    , m_bufferLimitSpinner(nullptr)
    , m_timestampFormatCombo(nullptr)
    , m_statsPanel(nullptr)
    , m_logView(nullptr)
    , m_btnClear(nullptr)
    , m_btnPause(nullptr)
    , m_btnSave(nullptr)
    , m_btnClose(nullptr)
    , m_pauseIndicator(nullptr)
    , m_searchEdit(nullptr)
    , m_btnFindNext(nullptr)
    , m_btnFindPrev(nullptr)
    , m_caseSensitive(nullptr)
    , m_searchStatus(nullptr)
    , m_paused(false)
    , m_entriesWhilePaused(0)
    , m_minLevel(yamy::logging::LogLevel::Trace)
    , m_searchCaseSensitive(false)
    , m_currentMatchIndex(0)
    , m_totalMatches(0)
    , m_maxBufferSize(DEFAULT_MAX_BUFFER_SIZE)
    , m_timestampFormat(TimestampFormat::Absolute)
    , m_dialogStartTime(std::chrono::system_clock::now())
{
    setWindowTitle("YAMY Log Viewer");
    setMinimumSize(800, 600);

    setupUI();
    loadFontSettings();
    loadBufferSettings();
    loadTimestampSettings();
    subscribeToLogger();
}

DialogLogQt::~DialogLogQt() = default;

void DialogLogQt::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Filter controls at top
    setupFilterControls(mainLayout);

    // Search controls
    setupSearchControls(mainLayout);

    // Stats panel
    m_statsPanel = new yamy::ui::LogStatsPanel(this);
    connect(m_statsPanel, &yamy::ui::LogStatsPanel::clearStatsRequested,
            this, &DialogLogQt::clearLog);
    mainLayout->addWidget(m_statsPanel);

    // Log view
    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    // Font is set in loadFontSettings()
    m_logView->setLineWrapMode(QTextEdit::NoWrap);
    mainLayout->addWidget(m_logView);

    // Bottom controls
    auto* controlLayout = new QHBoxLayout();

    m_btnPause = new QPushButton("Pause");
    connect(m_btnPause, &QPushButton::clicked, this, &DialogLogQt::onPauseResume);
    controlLayout->addWidget(m_btnPause);

    m_pauseIndicator = new QLabel();
    m_pauseIndicator->setStyleSheet("QLabel { color: #FFA500; font-weight: bold; }");
    m_pauseIndicator->hide();
    controlLayout->addWidget(m_pauseIndicator);

    controlLayout->addStretch();

    m_btnClear = new QPushButton("Clear");
    connect(m_btnClear, &QPushButton::clicked, this, &DialogLogQt::onClear);
    controlLayout->addWidget(m_btnClear);

    m_btnSave = new QPushButton("Export...");
    connect(m_btnSave, &QPushButton::clicked, this, &DialogLogQt::onExport);
    controlLayout->addWidget(m_btnSave);

    m_btnClose = new QPushButton("Close");
    connect(m_btnClose, &QPushButton::clicked, this, &DialogLogQt::onClose);
    controlLayout->addWidget(m_btnClose);

    mainLayout->addLayout(controlLayout);
}

void DialogLogQt::setupFilterControls(QVBoxLayout* mainLayout)
{
    auto* filterLayout = new QHBoxLayout();

    // Level filter
    auto* levelLabel = new QLabel("Level:");
    filterLayout->addWidget(levelLabel);

    m_levelFilter = new QComboBox();
    m_levelFilter->addItem("Trace", static_cast<int>(yamy::logging::LogLevel::Trace));
    m_levelFilter->addItem("Info", static_cast<int>(yamy::logging::LogLevel::Info));
    m_levelFilter->addItem("Warning", static_cast<int>(yamy::logging::LogLevel::Warning));
    m_levelFilter->addItem("Error", static_cast<int>(yamy::logging::LogLevel::Error));
    m_levelFilter->setCurrentIndex(0);
    connect(m_levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DialogLogQt::onLevelFilterChanged);
    filterLayout->addWidget(m_levelFilter);

    filterLayout->addSpacing(20);

    // Category filters
    m_categoryGroup = new QGroupBox("Categories");
    auto* categoryLayout = new QHBoxLayout(m_categoryGroup);
    categoryLayout->setContentsMargins(5, 2, 5, 2);

    for (size_t i = 0; i < CATEGORY_COUNT; ++i) {
        const char* cat = CATEGORIES[i];
        auto* checkbox = new QCheckBox(cat);
        checkbox->setChecked(true);
        connect(checkbox, &QCheckBox::toggled,
                this, &DialogLogQt::onCategoryFilterChanged);
        categoryLayout->addWidget(checkbox);
        m_categoryFilters[cat] = checkbox;
    }

    filterLayout->addWidget(m_categoryGroup);

    // Font controls
    setupFontControls(filterLayout);

    filterLayout->addStretch();

    mainLayout->addLayout(filterLayout);
}

void DialogLogQt::setupFontControls(QHBoxLayout* filterLayout)
{
    filterLayout->addSpacing(20);

    auto* fontLabel = new QLabel("Font:");
    filterLayout->addWidget(fontLabel);

    m_fontCombo = new QFontComboBox();
    m_fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    m_fontCombo->setMaximumWidth(150);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged,
            this, &DialogLogQt::onFontFamilyChanged);
    filterLayout->addWidget(m_fontCombo);

    auto* sizeLabel = new QLabel("Size:");
    filterLayout->addWidget(sizeLabel);

    m_fontSizeSpinner = new QSpinBox();
    m_fontSizeSpinner->setRange(6, 24);
    m_fontSizeSpinner->setValue(10);
    m_fontSizeSpinner->setSuffix(" pt");
    connect(m_fontSizeSpinner, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DialogLogQt::onFontSizeChanged);
    filterLayout->addWidget(m_fontSizeSpinner);

    filterLayout->addSpacing(20);

    auto* bufferLabel = new QLabel("Buffer:");
    filterLayout->addWidget(bufferLabel);

    m_bufferLimitSpinner = new QSpinBox();
    m_bufferLimitSpinner->setRange(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
    m_bufferLimitSpinner->setValue(DEFAULT_MAX_BUFFER_SIZE);
    m_bufferLimitSpinner->setSingleStep(1000);
    m_bufferLimitSpinner->setSuffix(" lines");
    m_bufferLimitSpinner->setToolTip("Maximum number of log entries to keep in memory");
    connect(m_bufferLimitSpinner, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DialogLogQt::onBufferLimitChanged);
    filterLayout->addWidget(m_bufferLimitSpinner);

    filterLayout->addSpacing(20);

    auto* timestampLabel = new QLabel("Time:");
    filterLayout->addWidget(timestampLabel);

    m_timestampFormatCombo = new QComboBox();
    m_timestampFormatCombo->addItem("Absolute", static_cast<int>(TimestampFormat::Absolute));
    m_timestampFormatCombo->addItem("Relative", static_cast<int>(TimestampFormat::Relative));
    m_timestampFormatCombo->addItem("None", static_cast<int>(TimestampFormat::None));
    m_timestampFormatCombo->setToolTip("Timestamp format: Absolute (HH:MM:SS.mmm), Relative (+MM:SS.mmm), None");
    connect(m_timestampFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DialogLogQt::onTimestampFormatChanged);
    filterLayout->addWidget(m_timestampFormatCombo);
}

void DialogLogQt::setupSearchControls(QVBoxLayout* mainLayout)
{
    auto* searchLayout = new QHBoxLayout();

    auto* searchLabel = new QLabel("Search:");
    searchLayout->addWidget(searchLabel);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Enter text to search...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &DialogLogQt::onSearchTextChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed,
            this, &DialogLogQt::onFindNext);
    searchLayout->addWidget(m_searchEdit);

    m_btnFindPrev = new QPushButton("◀ Previous");
    m_btnFindPrev->setEnabled(false);
    connect(m_btnFindPrev, &QPushButton::clicked,
            this, &DialogLogQt::onFindPrevious);
    searchLayout->addWidget(m_btnFindPrev);

    m_btnFindNext = new QPushButton("Next ▶");
    m_btnFindNext->setEnabled(false);
    connect(m_btnFindNext, &QPushButton::clicked,
            this, &DialogLogQt::onFindNext);
    searchLayout->addWidget(m_btnFindNext);

    m_caseSensitive = new QCheckBox("Case sensitive");
    connect(m_caseSensitive, &QCheckBox::toggled,
            this, &DialogLogQt::onCaseSensitiveToggled);
    searchLayout->addWidget(m_caseSensitive);

    m_searchStatus = new QLabel();
    m_searchStatus->setMinimumWidth(100);
    searchLayout->addWidget(m_searchStatus);

    searchLayout->addStretch();

    mainLayout->addLayout(searchLayout);
}

void DialogLogQt::subscribeToLogger()
{
    auto& logger = yamy::logging::Logger::getInstance();
    logger.addListener([this](const yamy::logging::LogEntry& entry) {
        // Copy entry data to capture by value for thread safety
        CachedLogEntry cached;
        cached.level = entry.level;
        cached.category = QString::fromStdString(entry.category);
        cached.message = QString::fromStdString(entry.message);
        cached.timestamp = entry.timestamp;
        cached.plainText = formatLogEntry(entry);
        cached.htmlText = formatLogEntryHtml(entry);

        // Use QMetaObject::invokeMethod for thread-safe UI update
        QMetaObject::invokeMethod(this, [this, cached = std::move(cached)]() {
            processLogEntry(cached);
        }, Qt::QueuedConnection);
    });
}

void DialogLogQt::onLogEntry(const yamy::logging::LogEntry& entry)
{
    CachedLogEntry cached;
    cached.level = entry.level;
    cached.category = QString::fromStdString(entry.category);
    cached.message = QString::fromStdString(entry.message);
    cached.timestamp = entry.timestamp;
    cached.plainText = formatLogEntry(entry);
    cached.htmlText = formatLogEntryHtml(entry);
    processLogEntry(cached);
}

void DialogLogQt::processLogEntry(const CachedLogEntry& entry)
{
    m_allEntries.push_back(entry);

    // Update stats by level
    switch (entry.level) {
        case yamy::logging::LogLevel::Trace:
            m_statsPanel->incrementTrace();
            break;
        case yamy::logging::LogLevel::Info:
            m_statsPanel->incrementInfo();
            break;
        case yamy::logging::LogLevel::Warning:
            m_statsPanel->incrementWarning();
            break;
        case yamy::logging::LogLevel::Error:
            m_statsPanel->incrementError();
            break;
    }

    // Update stats by category
    m_statsPanel->incrementCategory(entry.category);

    // Trim buffer if needed (removes 10% when limit reached)
    trimBufferIfNeeded();

    // Update buffer usage display
    updateBufferUsageDisplay();

    // Display if passes filter
    if (shouldDisplay(m_allEntries.back())) {
        m_logView->append(m_allEntries.back().htmlText);
        if (!m_paused) {
            scrollToBottom();
        } else {
            // Update paused indicator with entry count
            ++m_entriesWhilePaused;
            updatePauseIndicator();
        }
    }
}

QString DialogLogQt::formatLogEntry(const yamy::logging::LogEntry& entry) const
{
    QString levelStr;
    switch (entry.level) {
        case yamy::logging::LogLevel::Trace:   levelStr = "TRACE"; break;
        case yamy::logging::LogLevel::Info:    levelStr = "INFO"; break;
        case yamy::logging::LogLevel::Warning: levelStr = "WARN"; break;
        case yamy::logging::LogLevel::Error:   levelStr = "ERROR"; break;
    }

    QString timestampStr = formatTimestamp(entry.timestamp);
    QString result;

    if (!timestampStr.isEmpty()) {
        result = QString("%1 [%2] [%3] %4")
            .arg(timestampStr)
            .arg(levelStr, -5)
            .arg(QString::fromStdString(entry.category), -8)
            .arg(QString::fromStdString(entry.message));
    } else {
        result = QString("[%1] [%2] %3")
            .arg(levelStr, -5)
            .arg(QString::fromStdString(entry.category), -8)
            .arg(QString::fromStdString(entry.message));
    }

    return result;
}

QString DialogLogQt::escapeHtml(const QString& text)
{
    QString escaped = text;
    escaped.replace('&', "&amp;");
    escaped.replace('<', "&lt;");
    escaped.replace('>', "&gt;");
    return escaped;
}

QString DialogLogQt::highlightKeywords(const QString& text)
{
    QString result = text;
    // Highlight DOWN and UP in bold
    result.replace(" DOWN ", " <b>DOWN</b> ");
    result.replace(" UP ", " <b>UP</b> ");
    // Highlight HANDLED in green
    result.replace("HANDLED", "<span style='color:#228B22;'>HANDLED</span>");
    // PASSED remains default (no change needed)
    return result;
}

QString DialogLogQt::formatLogEntryHtml(const yamy::logging::LogEntry& entry) const
{
    QString levelStr;
    QString levelColor;
    switch (entry.level) {
        case yamy::logging::LogLevel::Trace:
            levelStr = "TRACE";
            levelColor = "#808080";  // Gray
            break;
        case yamy::logging::LogLevel::Info:
            levelStr = "INFO";
            levelColor.clear();  // Default (no color)
            break;
        case yamy::logging::LogLevel::Warning:
            levelStr = "WARN";
            levelColor = "#FFA500";  // Orange
            break;
        case yamy::logging::LogLevel::Error:
            levelStr = "ERROR";
            levelColor = "#FF0000";  // Red
            break;
    }

    // Escape HTML characters in message
    QString escapedMessage = escapeHtml(QString::fromStdString(entry.message));
    QString escapedCategory = escapeHtml(QString::fromStdString(entry.category));

    // Apply keyword highlighting
    escapedMessage = highlightKeywords(escapedMessage);

    // Format the log entry with timestamp
    QString timestampHtml = formatTimestampHtml(entry.timestamp);
    QString formattedEntry;

    if (!timestampHtml.isEmpty()) {
        formattedEntry = QString("%1[%2] [%3] %4")
            .arg(timestampHtml)
            .arg(levelStr, -5)
            .arg(escapedCategory, -8)
            .arg(escapedMessage);
    } else {
        formattedEntry = QString("[%1] [%2] %3")
            .arg(levelStr, -5)
            .arg(escapedCategory, -8)
            .arg(escapedMessage);
    }

    // Wrap in color span if needed
    if (!levelColor.isEmpty()) {
        return QString("<span style='color:%1;'>%2</span>")
            .arg(levelColor)
            .arg(formattedEntry);
    }

    return formattedEntry;
}

bool DialogLogQt::shouldDisplay(const CachedLogEntry& entry) const
{
    // Check level filter
    if (entry.level < m_minLevel) {
        return false;
    }

    // Check category filter
    auto it = m_categoryFilters.find(entry.category);
    if (it != m_categoryFilters.end()) {
        return it.value()->isChecked();
    }

    // Unknown category - show by default
    return true;
}

void DialogLogQt::rebuildLogView()
{
    m_logView->clear();

    for (const auto& entry : m_allEntries) {
        if (shouldDisplay(entry)) {
            // Regenerate HTML with current timestamp format
            QString levelStr;
            QString levelColor;
            switch (entry.level) {
                case yamy::logging::LogLevel::Trace:
                    levelStr = "TRACE";
                    levelColor = "#808080";
                    break;
                case yamy::logging::LogLevel::Info:
                    levelStr = "INFO";
                    levelColor.clear();
                    break;
                case yamy::logging::LogLevel::Warning:
                    levelStr = "WARN";
                    levelColor = "#FFA500";
                    break;
                case yamy::logging::LogLevel::Error:
                    levelStr = "ERROR";
                    levelColor = "#FF0000";
                    break;
            }

            QString escapedCategory = escapeHtml(entry.category);
            QString escapedMessage = escapeHtml(entry.message);
            escapedMessage = highlightKeywords(escapedMessage);

            QString timestampHtml = formatTimestampHtml(entry.timestamp);
            QString formattedEntry;

            if (!timestampHtml.isEmpty()) {
                formattedEntry = QString("%1[%2] [%3] %4")
                    .arg(timestampHtml)
                    .arg(levelStr, -5)
                    .arg(escapedCategory, -8)
                    .arg(escapedMessage);
            } else {
                formattedEntry = QString("[%1] [%2] %3")
                    .arg(levelStr, -5)
                    .arg(escapedCategory, -8)
                    .arg(escapedMessage);
            }

            if (!levelColor.isEmpty()) {
                m_logView->append(QString("<span style='color:%1;'>%2</span>")
                    .arg(levelColor)
                    .arg(formattedEntry));
            } else {
                m_logView->append(formattedEntry);
            }
        }
    }

    if (!m_paused) {
        scrollToBottom();
    }
}

void DialogLogQt::onLevelFilterChanged(int index)
{
    m_minLevel = static_cast<yamy::logging::LogLevel>(
        m_levelFilter->itemData(index).toInt());
    rebuildLogView();
}

void DialogLogQt::onCategoryFilterChanged(bool /*checked*/)
{
    rebuildLogView();
}

void DialogLogQt::appendLog(const QString& message)
{
    // Legacy method for manually appending logs
    yamy::logging::LogEntry entry(
        yamy::logging::LogLevel::Info,
        "UI",
        message.toStdString()
    );
    onLogEntry(entry);
}

void DialogLogQt::clearLog()
{
    m_logView->clear();
    m_allEntries.clear();
    m_statsPanel->reset();
    m_entriesWhilePaused = 0;
    updateBufferUsageDisplay();
    if (m_paused) {
        updatePauseIndicator();
    }
}

void DialogLogQt::setAutoScroll(bool enabled)
{
    m_paused = !enabled;
    if (m_paused) {
        m_btnPause->setText("Resume");
        m_entriesWhilePaused = 0;
        updatePauseIndicator();
        m_pauseIndicator->show();
    } else {
        m_btnPause->setText("Pause");
        m_pauseIndicator->hide();
        scrollToBottom();
    }
}

void DialogLogQt::onClear()
{
    if (m_allEntries.size() > 1000) {
        int ret = QMessageBox::question(
            this,
            "Clear Log",
            QString("Clear all %1 log messages?").arg(m_allEntries.size()),
            QMessageBox::Yes | QMessageBox::No
        );
        if (ret != QMessageBox::Yes) {
            return;
        }
    }
    clearLog();
}

void DialogLogQt::onExport()
{
    // Ask user whether to export all or filtered logs
    QMessageBox exportChoice(this);
    exportChoice.setWindowTitle("Export Logs");
    exportChoice.setText("Choose which logs to export:");
    exportChoice.setIcon(QMessageBox::Question);

    QPushButton* allBtn = exportChoice.addButton("All Logs", QMessageBox::AcceptRole);
    QPushButton* filteredBtn = exportChoice.addButton("Filtered Only", QMessageBox::AcceptRole);
    exportChoice.addButton(QMessageBox::Cancel);

    exportChoice.exec();

    QAbstractButton* clicked = exportChoice.clickedButton();
    if (clicked != allBtn && clicked != filteredBtn) {
        return; // User cancelled
    }

    bool exportFiltered = (clicked == filteredBtn);

    // Generate timestamped filename
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString defaultFileName = QString("logs_%1.txt").arg(timestamp);
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/" + defaultFileName;

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export Log",
        defaultPath,
        "Text Files (*.txt);;Log Files (*.log);;All Files (*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(
            this,
            "Export Log",
            "Failed to export log file:\n" + file.errorString()
        );
        return;
    }

    // Use QTextStream with UTF-8 encoding
    QTextStream out(&file);
    // Qt 6: QTextStream uses UTF-8 by default, no need to set codec
    out.setEncoding(QStringConverter::Utf8);

    int exportedCount = 0;

    if (exportFiltered) {
        // Export only entries that pass the current filter
        for (const auto& entry : m_allEntries) {
            if (shouldDisplay(entry)) {
                out << entry.plainText << "\n";
                ++exportedCount;
            }
        }
    } else {
        // Export all entries
        for (const auto& entry : m_allEntries) {
            out << entry.plainText << "\n";
            ++exportedCount;
        }
    }

    file.close();

    QString filterInfo = exportFiltered ? " (filtered)" : "";
    QMessageBox::information(
        this,
        "Export Log",
        QString("Successfully exported %1 log entries%2 to:\n%3")
            .arg(exportedCount)
            .arg(filterInfo)
            .arg(fileName)
    );
}

void DialogLogQt::onClose()
{
    close();
}

void DialogLogQt::onPauseResume()
{
    m_paused = !m_paused;

    if (m_paused) {
        m_btnPause->setText("Resume");
        m_entriesWhilePaused = 0;
        updatePauseIndicator();
        m_pauseIndicator->show();
    } else {
        m_btnPause->setText("Pause");
        m_pauseIndicator->hide();
        m_entriesWhilePaused = 0;
        scrollToBottom();
    }
}

void DialogLogQt::updatePauseIndicator()
{
    if (m_entriesWhilePaused > 0) {
        m_pauseIndicator->setText(QString("(Paused - %1 new entries)").arg(m_entriesWhilePaused));
    } else {
        m_pauseIndicator->setText("(Paused)");
    }
}

void DialogLogQt::scrollToBottom()
{
    QScrollBar* scrollBar = m_logView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void DialogLogQt::onFontFamilyChanged(const QFont& /*font*/)
{
    applyFont();
    saveFontSettings();
}

void DialogLogQt::onFontSizeChanged(int /*size*/)
{
    applyFont();
    saveFontSettings();
}

void DialogLogQt::loadFontSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Get default system monospace font
    QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QString defaultFamily = defaultFont.family();
    int defaultSize = 10;

    // Load saved font settings
    QString fontFamily = settings.value("logviewer/fontFamily", defaultFamily).toString();
    int fontSize = settings.value("logviewer/fontSize", defaultSize).toInt();

    // Validate font size range
    fontSize = qBound(6, fontSize, 24);

    // Check if saved font family is available
    QFontDatabase fontDb;
    QStringList families = fontDb.families();
    if (!families.contains(fontFamily)) {
        // Fallback to default if saved font not available
        fontFamily = defaultFamily;
    }

    // Block signals while setting values to avoid double saves
    m_fontCombo->blockSignals(true);
    m_fontSizeSpinner->blockSignals(true);

    m_fontCombo->setCurrentFont(QFont(fontFamily));
    m_fontSizeSpinner->setValue(fontSize);

    m_fontCombo->blockSignals(false);
    m_fontSizeSpinner->blockSignals(false);

    // Apply font to log view
    applyFont();
}

void DialogLogQt::saveFontSettings()
{
    QSettings settings("YAMY", "YAMY");

    settings.setValue("logviewer/fontFamily", m_fontCombo->currentFont().family());
    settings.setValue("logviewer/fontSize", m_fontSizeSpinner->value());
    settings.sync();
}

void DialogLogQt::applyFont()
{
    QFont font = m_fontCombo->currentFont();
    font.setPointSize(m_fontSizeSpinner->value());
    m_logView->setFont(font);
}

void DialogLogQt::onSearchTextChanged(const QString& text)
{
    m_searchText = text;
    m_currentMatchIndex = 0;

    if (text.isEmpty()) {
        clearSearchHighlights();
        m_btnFindNext->setEnabled(false);
        m_btnFindPrev->setEnabled(false);
        m_searchStatus->clear();
        m_totalMatches = 0;
        return;
    }

    highlightAllMatches();
    updateSearchStatus();

    bool hasMatches = m_totalMatches > 0;
    m_btnFindNext->setEnabled(hasMatches);
    m_btnFindPrev->setEnabled(hasMatches);

    // Move cursor to first match if any
    if (hasMatches) {
        m_logView->moveCursor(QTextCursor::Start);
        findMatch(true);
    }
}

void DialogLogQt::onFindNext()
{
    findMatch(true);
}

void DialogLogQt::onFindPrevious()
{
    findMatch(false);
}

void DialogLogQt::onCaseSensitiveToggled(bool checked)
{
    m_searchCaseSensitive = checked;
    if (!m_searchText.isEmpty()) {
        onSearchTextChanged(m_searchText);
    }
}

void DialogLogQt::highlightAllMatches()
{
    clearSearchHighlights();

    if (m_searchText.isEmpty()) {
        m_totalMatches = 0;
        return;
    }

    QTextDocument* document = m_logView->document();
    QTextCursor cursor(document);

    // Define highlight format
    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(QColor(255, 255, 0)); // Yellow background
    highlightFormat.setForeground(QColor(0, 0, 0));     // Black text

    QTextDocument::FindFlags flags;
    if (m_searchCaseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    m_totalMatches = 0;

    // Find and highlight all matches
    cursor = document->find(m_searchText, 0, flags);
    while (!cursor.isNull()) {
        cursor.mergeCharFormat(highlightFormat);
        ++m_totalMatches;
        cursor = document->find(m_searchText, cursor, flags);
    }
}

void DialogLogQt::clearSearchHighlights()
{
    // Reset all formatting by rebuilding the log view
    // This preserves the original HTML formatting
    if (!m_searchText.isEmpty() || m_totalMatches > 0) {
        rebuildLogView();
    }
}

void DialogLogQt::updateSearchStatus()
{
    if (m_searchText.isEmpty()) {
        m_searchStatus->clear();
        return;
    }

    if (m_totalMatches == 0) {
        m_searchStatus->setText("No matches");
        m_searchStatus->setStyleSheet("QLabel { color: #FF6B6B; }");
    } else {
        m_searchStatus->setText(QString("%1 of %2 matches")
            .arg(m_currentMatchIndex + 1)
            .arg(m_totalMatches));
        m_searchStatus->setStyleSheet("");
    }
}

int DialogLogQt::countMatches()
{
    if (m_searchText.isEmpty()) {
        return 0;
    }

    QTextDocument* document = m_logView->document();
    QTextDocument::FindFlags flags;
    if (m_searchCaseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    int count = 0;
    QTextCursor cursor(document);
    cursor = document->find(m_searchText, cursor, flags);
    while (!cursor.isNull()) {
        ++count;
        cursor = document->find(m_searchText, cursor, flags);
    }

    return count;
}

void DialogLogQt::findMatch(bool forward)
{
    if (m_searchText.isEmpty() || m_totalMatches == 0) {
        return;
    }

    QTextDocument::FindFlags flags;
    if (m_searchCaseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (!forward) {
        flags |= QTextDocument::FindBackward;
    }

    bool found = m_logView->find(m_searchText, flags);

    if (!found) {
        // Wrap around
        QTextCursor cursor = m_logView->textCursor();
        if (forward) {
            cursor.movePosition(QTextCursor::Start);
            m_currentMatchIndex = 0;
        } else {
            cursor.movePosition(QTextCursor::End);
            m_currentMatchIndex = m_totalMatches - 1;
        }
        m_logView->setTextCursor(cursor);
        m_logView->find(m_searchText, flags);
    } else {
        // Update match index
        if (forward) {
            m_currentMatchIndex = (m_currentMatchIndex + 1) % m_totalMatches;
        } else {
            m_currentMatchIndex = (m_currentMatchIndex - 1 + m_totalMatches) % m_totalMatches;
        }
    }

    // Ensure the match is visible
    m_logView->ensureCursorVisible();
    updateSearchStatus();
}

void DialogLogQt::loadBufferSettings()
{
    QSettings settings("YAMY", "YAMY");
    int savedLimit = settings.value("logviewer/bufferLimit", DEFAULT_MAX_BUFFER_SIZE).toInt();

    // Validate the saved limit
    m_maxBufferSize = qBound(MIN_BUFFER_SIZE, savedLimit, MAX_BUFFER_SIZE);

    // Update spinner without triggering save
    m_bufferLimitSpinner->blockSignals(true);
    m_bufferLimitSpinner->setValue(m_maxBufferSize);
    m_bufferLimitSpinner->blockSignals(false);

    // Update stats panel with initial buffer usage
    updateBufferUsageDisplay();
}

void DialogLogQt::saveBufferSettings()
{
    QSettings settings("YAMY", "YAMY");
    settings.setValue("logviewer/bufferLimit", m_maxBufferSize);
    settings.sync();
}

void DialogLogQt::onBufferLimitChanged(int value)
{
    m_maxBufferSize = value;
    saveBufferSettings();

    // If current buffer exceeds new limit, trim immediately
    trimBufferIfNeeded();
    updateBufferUsageDisplay();

    // Rebuild view if entries were trimmed
    if (!m_allEntries.empty()) {
        rebuildLogView();
    }
}

void DialogLogQt::trimBufferIfNeeded()
{
    if (m_allEntries.size() > static_cast<size_t>(m_maxBufferSize)) {
        // Remove oldest 10% when limit is exceeded to avoid frequent trimming
        size_t trimCount = static_cast<size_t>(m_maxBufferSize) / 10;
        if (trimCount < 1) {
            trimCount = 1;
        }

        // Calculate how many entries to remove to get back under limit
        size_t excess = m_allEntries.size() - static_cast<size_t>(m_maxBufferSize);
        size_t toRemove = std::max(trimCount, excess);

        m_allEntries.erase(m_allEntries.begin(),
                          m_allEntries.begin() + static_cast<std::ptrdiff_t>(toRemove));

        // Rebuild the view after trimming (only if not triggered from rebuildLogView)
        // Note: rebuildLogView will be called separately when needed
    }
}

void DialogLogQt::updateBufferUsageDisplay()
{
    m_statsPanel->setBufferUsage(static_cast<int>(m_allEntries.size()), m_maxBufferSize);
}

void DialogLogQt::loadTimestampSettings()
{
    QSettings settings("YAMY", "YAMY");
    int savedFormat = settings.value("logviewer/timestampFormat",
                                     static_cast<int>(TimestampFormat::Absolute)).toInt();

    // Validate the saved format
    if (savedFormat < 0 || savedFormat > 2) {
        savedFormat = static_cast<int>(TimestampFormat::Absolute);
    }

    m_timestampFormat = static_cast<TimestampFormat>(savedFormat);

    // Update combobox without triggering signal
    m_timestampFormatCombo->blockSignals(true);
    m_timestampFormatCombo->setCurrentIndex(savedFormat);
    m_timestampFormatCombo->blockSignals(false);
}

void DialogLogQt::saveTimestampSettings()
{
    QSettings settings("YAMY", "YAMY");
    settings.setValue("logviewer/timestampFormat", static_cast<int>(m_timestampFormat));
    settings.sync();
}

void DialogLogQt::onTimestampFormatChanged(int index)
{
    m_timestampFormat = static_cast<TimestampFormat>(
        m_timestampFormatCombo->itemData(index).toInt());
    saveTimestampSettings();
    rebuildLogView();
}

QString DialogLogQt::formatTimestamp(const std::chrono::system_clock::time_point& timestamp) const
{
    switch (m_timestampFormat) {
        case TimestampFormat::Absolute: {
            auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &time_t_val);
#else
            localtime_r(&time_t_val, &tm);
#endif

            // Get milliseconds
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch()) % 1000;

            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tm);
            return QString("[%1.%2]")
                .arg(timeBuf)
                .arg(ms.count(), 3, 10, QChar('0'));
        }
        case TimestampFormat::Relative: {
            auto duration = timestamp - m_dialogStartTime;
            auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            // Handle negative durations (entries from before dialog opened)
            if (totalMs < 0) {
                totalMs = 0;
            }

            auto minutes = totalMs / 60000;
            auto seconds = (totalMs % 60000) / 1000;
            auto ms = totalMs % 1000;

            return QString("[+%1:%2.%3]")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(ms, 3, 10, QChar('0'));
        }
        case TimestampFormat::None:
            return QString();
    }
    return QString();
}

QString DialogLogQt::formatTimestampHtml(const std::chrono::system_clock::time_point& timestamp) const
{
    QString ts = formatTimestamp(timestamp);
    if (ts.isEmpty()) {
        return QString();
    }
    // Wrap timestamp in a span with gray color for readability
    return QString("<span style='color:#666666;'>%1</span> ").arg(ts);
}
