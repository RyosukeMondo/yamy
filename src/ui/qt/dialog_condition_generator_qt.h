#pragma once

#include <QDialog>
#include <QComboBox>
#include <QRadioButton>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class DialogConditionGeneratorQt : public QDialog {
    Q_OBJECT

public:
    explicit DialogConditionGeneratorQt(const QString& windowTitle, const QString& windowClass, QWidget* parent = nullptr);

private slots:
    void onGenerate();
    void onCopyToClipboard();

private:
    void setupUI();

    QString m_windowTitle;
    QString m_windowClass;

    QComboBox* m_matchType;
    QRadioButton* m_radioTitle;
    QRadioButton* m_radioClass;
    QRadioButton* m_radioBoth;
    QTextEdit* m_generatedCondition;
    QPushButton* m_btnCopyToClipboard;
};
