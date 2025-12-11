#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTextBrowser>

/**
 * @brief About dialog
 *
 * Displays:
 * - Application name and version
 * - Build information (commit hash, date, compiler, Qt version)
 * - License information (full text in scrollable area)
 * - Contributors list
 * - Links to project resources
 */
class DialogAboutQt : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct about dialog
     * @param parent Parent widget
     */
    explicit DialogAboutQt(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DialogAboutQt() override;

private slots:
    /**
     * @brief Close dialog
     */
    void onClose();

    /**
     * @brief Handle link clicks
     */
    void onLinkClicked(const QUrl& url);

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Create the About tab content
     */
    QWidget* createAboutTab();

    /**
     * @brief Create the License tab content
     */
    QWidget* createLicenseTab();

    /**
     * @brief Create the Contributors tab content
     */
    QWidget* createContributorsTab();

    /**
     * @brief Get version string
     */
    QString getVersionString() const;

    /**
     * @brief Get detailed build info string
     */
    QString getBuildInfo() const;

    /**
     * @brief Get platform info string
     */
    QString getPlatformInfo() const;

    /**
     * @brief Get license text
     */
    QString getLicenseText() const;

    /**
     * @brief Get contributors HTML
     */
    QString getContributorsHtml() const;

    // UI Components
    QTabWidget* m_tabWidget;
    QPushButton* m_btnClose;
};
