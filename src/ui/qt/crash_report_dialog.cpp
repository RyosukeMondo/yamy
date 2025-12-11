//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// crash_report_dialog.cpp - Crash report dialog implementation

#include "crash_report_dialog.h"
#include "utils/crash_handler.h"

#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QFileInfo>
#include <QSysInfo>

namespace yamy {

namespace {

constexpr int MAX_PREVIEW_LINES = 20;
constexpr int MAX_GITHUB_BODY_LENGTH = 4000;  // GitHub has limits on URL length

const char* SETTINGS_KEY_DONT_SHOW = "CrashDialog/DontShowAgain";
const char* GITHUB_ISSUES_URL = "https://github.com/yamy-dev/yamy/issues/new";

} // anonymous namespace

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CrashReportDialog implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CrashReportDialog::CrashReportDialog(const std::vector<std::string>& crashReports,
                                     QWidget* parent)
    : QDialog(parent)
    , m_crashReports(crashReports)
    , m_iconLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_messageLabel(nullptr)
    , m_reportSummaryLabel(nullptr)
    , m_reportPreview(nullptr)
    , m_btnView(nullptr)
    , m_btnReportBug(nullptr)
    , m_btnDismiss(nullptr)
    , m_chkDontShowAgain(nullptr)
{
    if (!crashReports.empty()) {
        m_currentReport = crashReports[0];
    }

    setWindowTitle("YAMY Crash Report");
    setMinimumSize(500, 350);
    resize(550, 400);
    setModal(true);

    setupUI();
}

CrashReportDialog::~CrashReportDialog() = default;

bool CrashReportDialog::dontShowAgainChecked() const
{
    return m_chkDontShowAgain && m_chkDontShowAgain->isChecked();
}

bool CrashReportDialog::shouldShowCrashDialog()
{
    if (getDontShowAgain()) {
        return false;
    }
    return CrashHandler::hasCrashReports();
}

void CrashReportDialog::setDontShowAgain(bool value)
{
    QSettings settings;
    settings.setValue(SETTINGS_KEY_DONT_SHOW, value);
}

bool CrashReportDialog::getDontShowAgain()
{
    QSettings settings;
    return settings.value(SETTINGS_KEY_DONT_SHOW, false).toBool();
}

void CrashReportDialog::clearDontShowAgain()
{
    QSettings settings;
    settings.remove(SETTINGS_KEY_DONT_SHOW);
}

void CrashReportDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Header with icon and title
    QHBoxLayout* headerLayout = new QHBoxLayout();

    m_iconLabel = new QLabel();
    QIcon warningIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    m_iconLabel->setPixmap(warningIcon.pixmap(48, 48));
    headerLayout->addWidget(m_iconLabel);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    m_titleLabel = new QLabel("<b>YAMY crashed unexpectedly</b>");
    m_titleLabel->setStyleSheet("font-size: 14pt;");
    titleLayout->addWidget(m_titleLabel);

    QString reportCount = m_crashReports.size() == 1
        ? "A crash report was saved."
        : QString("%1 crash reports were saved.").arg(m_crashReports.size());
    m_messageLabel = new QLabel(reportCount);
    m_messageLabel->setStyleSheet("color: #666;");
    titleLayout->addWidget(m_messageLabel);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Report summary/preview
    m_reportSummaryLabel = new QLabel("<b>Crash Summary:</b>");
    mainLayout->addWidget(m_reportSummaryLabel);

    m_reportPreview = new QTextEdit();
    m_reportPreview->setReadOnly(true);
    m_reportPreview->setMaximumHeight(150);
    m_reportPreview->setStyleSheet(
        "QTextEdit { background-color: #f5f5f5; font-family: monospace; }"
    );
    m_reportPreview->setPlainText(loadReportSummary(m_currentReport));
    mainLayout->addWidget(m_reportPreview);

    // Info text
    QLabel* infoLabel = new QLabel(
        "You can view the full report, report a bug on GitHub, or dismiss this notification."
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666;");
    mainLayout->addWidget(infoLabel);

    // Don't show again checkbox
    m_chkDontShowAgain = new QCheckBox("Don't show crash notifications again");
    mainLayout->addWidget(m_chkDontShowAgain);

    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnView = new QPushButton("View Full Report...");
    m_btnView->setMinimumWidth(130);
    connect(m_btnView, &QPushButton::clicked, this, &CrashReportDialog::onViewReport);
    btnLayout->addWidget(m_btnView);

    m_btnReportBug = new QPushButton("Report Bug...");
    m_btnReportBug->setMinimumWidth(110);
    connect(m_btnReportBug, &QPushButton::clicked, this, &CrashReportDialog::onReportBug);
    btnLayout->addWidget(m_btnReportBug);

    btnLayout->addStretch();

    m_btnDismiss = new QPushButton("Dismiss");
    m_btnDismiss->setDefault(true);
    m_btnDismiss->setMinimumWidth(100);
    connect(m_btnDismiss, &QPushButton::clicked, this, &CrashReportDialog::onDismiss);
    btnLayout->addWidget(m_btnDismiss);

    mainLayout->addLayout(btnLayout);
}

QString CrashReportDialog::loadReportSummary(const std::string& path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "(Unable to load crash report)";
    }

    QTextStream in(&file);
    QStringList lines;
    int lineCount = 0;

    while (!in.atEnd() && lineCount < MAX_PREVIEW_LINES) {
        lines << in.readLine();
        ++lineCount;
    }

    QString summary = lines.join("\n");
    if (!in.atEnd()) {
        summary += "\n\n... (truncated, click 'View Full Report' to see more)";
    }

    return summary;
}

QString CrashReportDialog::sanitizeReportForGitHub(const QString& report)
{
    QString sanitized = report;

    // Remove potentially sensitive paths (home directory)
    QString homeDir = QDir::homePath();
    sanitized.replace(homeDir, "~");

    // Truncate if too long
    if (sanitized.length() > MAX_GITHUB_BODY_LENGTH) {
        sanitized = sanitized.left(MAX_GITHUB_BODY_LENGTH) +
                    "\n\n... (report truncated)";
    }

    return sanitized;
}

QString CrashReportDialog::buildGitHubIssueUrl(const QString& report)
{
    QString sanitizedReport = sanitizeReportForGitHub(report);

    QString title = "Crash Report: YAMY " + QApplication::applicationVersion();

    QString body = QString(
        "## Crash Report\n\n"
        "**YAMY Version:** %1\n"
        "**Platform:** %2\n\n"
        "### What I was doing when the crash occurred\n"
        "(Please describe what you were doing when YAMY crashed)\n\n"
        "### Crash Report\n"
        "```\n%3\n```\n"
    ).arg(QApplication::applicationVersion(),
          QSysInfo::prettyProductName(),
          sanitizedReport);

    QUrl url(GITHUB_ISSUES_URL);
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("body", body);
    query.addQueryItem("labels", "bug,crash");
    url.setQuery(query);

    return url.toString();
}

void CrashReportDialog::onViewReport()
{
    m_action = Action::ViewReport;

    // Save "don't show again" preference
    if (dontShowAgainChecked()) {
        setDontShowAgain(true);
    }

    accept();
}

void CrashReportDialog::onReportBug()
{
    m_action = Action::ReportBug;

    // Load full report
    QFile file(QString::fromStdString(m_currentReport));
    QString fullReport;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fullReport = QTextStream(&file).readAll();
    } else {
        fullReport = m_reportPreview->toPlainText();
    }

    // Open GitHub issues page
    QString url = buildGitHubIssueUrl(fullReport);
    QDesktopServices::openUrl(QUrl(url));

    // Ask if user wants to delete the report after reporting
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Crash Report?",
        "Would you like to delete the crash report after reporting?\n"
        "(The report is no longer needed once submitted)",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply == QMessageBox::Yes) {
        CrashHandler::deleteCrashReport(m_currentReport);
    }

    // Save "don't show again" preference
    if (dontShowAgainChecked()) {
        setDontShowAgain(true);
    }

    accept();
}

void CrashReportDialog::onDismiss()
{
    m_action = Action::Dismiss;

    // Ask if user wants to delete the reports
    QString message = m_crashReports.size() == 1
        ? "Would you like to delete the crash report?"
        : QString("Would you like to delete all %1 crash reports?")
              .arg(m_crashReports.size());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Crash Reports?",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        CrashHandler::deleteAllCrashReports();
    }

    // Save "don't show again" preference
    if (dontShowAgainChecked()) {
        setDontShowAgain(true);
    }

    reject();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CrashReportViewerDialog implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CrashReportViewerDialog::CrashReportViewerDialog(const std::string& reportPath,
                                                 QWidget* parent)
    : QDialog(parent)
    , m_reportPath(reportPath)
    , m_reportText(nullptr)
    , m_btnReportBug(nullptr)
    , m_btnDelete(nullptr)
    , m_btnClose(nullptr)
{
    QFileInfo fileInfo(QString::fromStdString(reportPath));

    setWindowTitle("Crash Report - " + fileInfo.fileName());
    setMinimumSize(600, 500);
    resize(700, 600);

    setupUI();
}

CrashReportViewerDialog::~CrashReportViewerDialog() = default;

void CrashReportViewerDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Title
    QLabel* titleLabel = new QLabel("<b>Crash Report Details</b>");
    titleLabel->setStyleSheet("font-size: 12pt;");
    mainLayout->addWidget(titleLabel);

    // File path
    QLabel* pathLabel = new QLabel(
        QString("<i>File: %1</i>").arg(QString::fromStdString(m_reportPath))
    );
    pathLabel->setStyleSheet("color: #666; font-size: 9pt;");
    pathLabel->setWordWrap(true);
    mainLayout->addWidget(pathLabel);

    // Report content
    m_reportText = new QTextEdit();
    m_reportText->setReadOnly(true);
    m_reportText->setStyleSheet(
        "QTextEdit { background-color: #f8f8f8; font-family: monospace; font-size: 10pt; }"
    );
    m_reportContent = loadReport();
    m_reportText->setPlainText(m_reportContent);
    mainLayout->addWidget(m_reportText);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnReportBug = new QPushButton("Report Bug on GitHub...");
    m_btnReportBug->setMinimumWidth(160);
    connect(m_btnReportBug, &QPushButton::clicked,
            this, &CrashReportViewerDialog::onReportBug);
    btnLayout->addWidget(m_btnReportBug);

    m_btnDelete = new QPushButton("Delete Report");
    m_btnDelete->setMinimumWidth(110);
    connect(m_btnDelete, &QPushButton::clicked,
            this, &CrashReportViewerDialog::onDeleteReport);
    btnLayout->addWidget(m_btnDelete);

    btnLayout->addStretch();

    m_btnClose = new QPushButton("Close");
    m_btnClose->setDefault(true);
    m_btnClose->setMinimumWidth(100);
    connect(m_btnClose, &QPushButton::clicked,
            this, &CrashReportViewerDialog::onClose);
    btnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(btnLayout);
}

QString CrashReportViewerDialog::loadReport()
{
    QFile file(QString::fromStdString(m_reportPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "(Unable to load crash report: " +
               QString::fromStdString(m_reportPath) + ")";
    }

    QTextStream in(&file);
    return in.readAll();
}

QString CrashReportViewerDialog::sanitizeReportForGitHub(const QString& report)
{
    QString sanitized = report;

    // Remove potentially sensitive paths (home directory)
    QString homeDir = QDir::homePath();
    sanitized.replace(homeDir, "~");

    // Truncate if too long
    if (sanitized.length() > MAX_GITHUB_BODY_LENGTH) {
        sanitized = sanitized.left(MAX_GITHUB_BODY_LENGTH) +
                    "\n\n... (report truncated)";
    }

    return sanitized;
}

QString CrashReportViewerDialog::buildGitHubIssueUrl(const QString& report)
{
    QString sanitizedReport = sanitizeReportForGitHub(report);

    QString title = "Crash Report: YAMY " + QApplication::applicationVersion();

    QString body = QString(
        "## Crash Report\n\n"
        "**YAMY Version:** %1\n"
        "**Platform:** %2\n\n"
        "### What I was doing when the crash occurred\n"
        "(Please describe what you were doing when YAMY crashed)\n\n"
        "### Crash Report\n"
        "```\n%3\n```\n"
    ).arg(QApplication::applicationVersion(),
          QSysInfo::prettyProductName(),
          sanitizedReport);

    QUrl url(GITHUB_ISSUES_URL);
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("body", body);
    query.addQueryItem("labels", "bug,crash");
    url.setQuery(query);

    return url.toString();
}

void CrashReportViewerDialog::onReportBug()
{
    QString url = buildGitHubIssueUrl(m_reportContent);
    QDesktopServices::openUrl(QUrl(url));

    // Ask if user wants to delete the report after reporting
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Crash Report?",
        "Would you like to delete this crash report after reporting?\n"
        "(The report is no longer needed once submitted)",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply == QMessageBox::Yes) {
        CrashHandler::deleteCrashReport(m_reportPath);
        accept();
    }
}

void CrashReportViewerDialog::onDeleteReport()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Crash Report?",
        "Are you sure you want to delete this crash report?\n"
        "This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (CrashHandler::deleteCrashReport(m_reportPath)) {
            accept();
        } else {
            QMessageBox::warning(
                this,
                "Error",
                "Failed to delete crash report."
            );
        }
    }
}

void CrashReportViewerDialog::onClose()
{
    reject();
}

} // namespace yamy
