#include "dialog_log_qt.h"
#include "core/logging/logger.h"
#include "log_stats_panel.h"
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
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>

DialogLogQt::DialogLogQt(QWidget* parent)
    : QDialog(parent)
    , m_levelFilter(nullptr)
    , m_categoryGroup(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeSpinner(nullptr)
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
{
    setWindowTitle("YAMY Log Viewer");
    setMinimumSize(800, 600);

    setupUI();
    loadFontSettings();
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

    m_btnSave = new QPushButton("Save...");
    connect(m_btnSave, &QPushButton::clicked, this, &DialogLogQt::onSave);
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
    cached.plainText = formatLogEntry(entry);
    cached.htmlText = formatLogEntryHtml(entry);
    processLogEntry(cached);
}

void DialogLogQt::processLogEntry(const CachedLogEntry& entry)
{
    // Limit buffer size
    if (m_allEntries.size() >= static_cast<size_t>(MAX_LOG_ENTRIES)) {
        m_allEntries.erase(m_allEntries.begin(),
                          m_allEntries.begin() + MAX_LOG_ENTRIES / 10);
    }

    m_allEntries.push_back(entry);

    // Update stats
    if (entry.level == yamy::logging::LogLevel::Error) {
        m_statsPanel->incrementError();
    } else if (entry.level == yamy::logging::LogLevel::Warning) {
        m_statsPanel->incrementWarning();
    }
    m_statsPanel->setTotalLines(static_cast<int>(m_allEntries.size()));

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

    auto timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm{};
    localtime_r(&timestamp, &tm);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);

    return QString("[%1] [%2] [%3] %4")
        .arg(timeBuf)
        .arg(levelStr, -5)
        .arg(QString::fromStdString(entry.category), -8)
        .arg(QString::fromStdString(entry.message));
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

    auto timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm{};
    localtime_r(&timestamp, &tm);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);

    // Escape HTML characters in message
    QString escapedMessage = escapeHtml(QString::fromStdString(entry.message));
    QString escapedCategory = escapeHtml(QString::fromStdString(entry.category));

    // Apply keyword highlighting
    escapedMessage = highlightKeywords(escapedMessage);

    // Format the log entry
    QString formattedEntry = QString("[%1] [%2] [%3] %4")
        .arg(timeBuf)
        .arg(levelStr, -5)
        .arg(escapedCategory, -8)
        .arg(escapedMessage);

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
            m_logView->append(entry.htmlText);
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
