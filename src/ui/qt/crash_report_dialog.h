#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// crash_report_dialog.h - Crash report dialog for YAMY
//
// Displays crash reports on startup if a previous crash was detected.
// Allows users to view, report bugs, and dismiss crash reports.

#ifndef _CRASH_REPORT_DIALOG_H
#define _CRASH_REPORT_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <vector>
#include <string>

namespace yamy {

/// Crash report dialog shown on startup when crash reports are detected
class CrashReportDialog : public QDialog {
    Q_OBJECT

public:
    /// Construct crash report dialog
    /// @param crashReports List of crash report file paths
    /// @param parent Parent widget
    explicit CrashReportDialog(const std::vector<std::string>& crashReports,
                               QWidget* parent = nullptr);

    ~CrashReportDialog() override;

    /// Check if "Don't show again" was checked
    bool dontShowAgainChecked() const;

    /// Result actions
    enum class Action {
        ViewReport,     // User wants to view full report
        ReportBug,      // User wants to report bug
        Dismiss         // User dismissed the dialog
    };

    /// Get the selected action
    Action selectedAction() const { return m_action; }

    /// Get the current crash report path
    const std::string& currentReportPath() const { return m_currentReport; }

    /// Check if there are crash reports and user hasn't disabled notifications
    /// @return true if crash dialog should be shown
    static bool shouldShowCrashDialog();

    /// Mark crash dialog as "don't show again" in settings
    static void setDontShowAgain(bool value);

    /// Get "don't show again" setting
    static bool getDontShowAgain();

    /// Clear "don't show again" setting (re-enable crash dialogs)
    static void clearDontShowAgain();

private slots:
    void onViewReport();
    void onReportBug();
    void onDismiss();

private:
    void setupUI();
    QString loadReportSummary(const std::string& path);
    QString sanitizeReportForGitHub(const QString& report);
    QString buildGitHubIssueUrl(const QString& report);

    std::vector<std::string> m_crashReports;
    std::string m_currentReport;
    Action m_action = Action::Dismiss;

    // UI Components
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QLabel* m_reportSummaryLabel;
    QTextEdit* m_reportPreview;
    QPushButton* m_btnView;
    QPushButton* m_btnReportBug;
    QPushButton* m_btnDismiss;
    QCheckBox* m_chkDontShowAgain;
};

/// Full crash report viewer dialog
class CrashReportViewerDialog : public QDialog {
    Q_OBJECT

public:
    /// Construct viewer dialog
    /// @param reportPath Path to crash report file
    /// @param parent Parent widget
    explicit CrashReportViewerDialog(const std::string& reportPath,
                                     QWidget* parent = nullptr);

    ~CrashReportViewerDialog() override;

private slots:
    void onReportBug();
    void onDeleteReport();
    void onClose();

private:
    void setupUI();
    QString loadReport();
    QString sanitizeReportForGitHub(const QString& report);
    QString buildGitHubIssueUrl(const QString& report);

    std::string m_reportPath;
    QString m_reportContent;

    // UI Components
    QTextEdit* m_reportText;
    QPushButton* m_btnReportBug;
    QPushButton* m_btnDelete;
    QPushButton* m_btnClose;
};

} // namespace yamy

#endif // _CRASH_REPORT_DIALOG_H
