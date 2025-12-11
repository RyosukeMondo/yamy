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
    , m_btnSave(nullptr)
    , m_btnClose(nullptr)
    , m_chkAutoScroll(nullptr)
    , m_autoScroll(true)
    , m_minLevel(yamy::logging::LogLevel::Trace)
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

void DialogLogQt::subscribeToLogger()
{
    auto& logger = yamy::logging::Logger::getInstance();
    logger.addListener([this](const yamy::logging::LogEntry& entry) {
        // Copy entry data to capture by value for thread safety
        CachedLogEntry cached;
        cached.level = entry.level;
        cached.category = QString::fromStdString(entry.category);
        cached.formattedText = formatLogEntry(entry);

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
    cached.formattedText = formatLogEntry(entry);
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
        m_logView->append(m_allEntries.back().formattedText);
        if (m_autoScroll) {
            scrollToBottom();
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
            m_logView->append(entry.formattedText);
        }
    }

    if (m_autoScroll) {
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
}

void DialogLogQt::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
    m_chkAutoScroll->setChecked(enabled);
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

void DialogLogQt::onAutoScrollToggled(bool checked)
{
    m_autoScroll = checked;
    if (m_autoScroll) {
        scrollToBottom();
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
