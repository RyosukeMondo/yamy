#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief About dialog
 *
 * Displays:
 * - Application name and version
 * - Build information
 * - License information
 * - Credits
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

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Get version string
     */
    QString getVersionString() const;

    /**
     * @brief Get build info string
     */
    QString getBuildInfo() const;

    // UI Components
    QLabel* m_labelIcon;
    QLabel* m_labelTitle;
    QLabel* m_labelVersion;
    QLabel* m_labelBuild;
    QLabel* m_labelDescription;
    QLabel* m_labelLicense;
    QPushButton* m_btnClose;
};
