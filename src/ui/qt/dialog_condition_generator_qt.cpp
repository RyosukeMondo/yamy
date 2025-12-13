#include "dialog_condition_generator_qt.h"
#include <QClipboard>
#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRegularExpression>

DialogConditionGeneratorQt::DialogConditionGeneratorQt(const QString& windowTitle, const QString& windowClass, QWidget* parent)
    : QDialog(parent), m_windowTitle(windowTitle), m_windowClass(windowClass)
{
    setWindowTitle("Generate Window Condition");
    setMinimumSize(500, 300);
    setupUI();
    onGenerate();
}

void DialogConditionGeneratorQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* optionsGroup = new QGroupBox("Options");
    QGridLayout* optionsLayout = new QGridLayout(optionsGroup);

    optionsLayout->addWidget(new QLabel("Match by:"), 0, 0);
    m_radioTitle = new QRadioButton("Title");
    m_radioClass = new QRadioButton("Class");
    m_radioBoth = new QRadioButton("Both");
    m_radioTitle->setChecked(true);
    optionsLayout->addWidget(m_radioTitle, 0, 1);
    optionsLayout->addWidget(m_radioClass, 0, 2);
    optionsLayout->addWidget(m_radioBoth, 0, 3);

    optionsLayout->addWidget(new QLabel("Match type:"), 1, 0);
    m_matchType = new QComboBox();
    m_matchType->addItems({"Exact", "Contains", "Regex"});
    optionsLayout->addWidget(m_matchType, 1, 1, 1, 3);
    
    connect(m_radioTitle, &QRadioButton::toggled, this, &DialogConditionGeneratorQt::onGenerate);
    connect(m_radioClass, &QRadioButton::toggled, this, &DialogConditionGeneratorQt::onGenerate);
    connect(m_radioBoth, &QRadioButton::toggled, this, &DialogConditionGeneratorQt::onGenerate);
    connect(m_matchType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DialogConditionGeneratorQt::onGenerate);

    mainLayout->addWidget(optionsGroup);

    QGroupBox* generatedGroup = new QGroupBox("Generated Condition");
    QVBoxLayout* generatedLayout = new QVBoxLayout(generatedGroup);
    m_generatedCondition = new QTextEdit();
    m_generatedCondition->setReadOnly(true);
    QFont monoFont("monospace");
    monoFont.setStyleHint(QFont::Monospace);
    m_generatedCondition->setFont(monoFont);
    generatedLayout->addWidget(m_generatedCondition);
    mainLayout->addWidget(generatedGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_btnCopyToClipboard = new QPushButton("Copy to Clipboard");
    connect(m_btnCopyToClipboard, &QPushButton::clicked, this, &DialogConditionGeneratorQt::onCopyToClipboard);
    buttonLayout->addWidget(m_btnCopyToClipboard);
    QPushButton* closeButton = new QPushButton("Close");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
}

void DialogConditionGeneratorQt::onGenerate()
{
    QString condition;
    QString matchType = m_matchType->currentText();
    QString title = m_windowTitle;
    QString wclass = m_windowClass;

    if (matchType == "Exact") {
        title = QRegularExpression::escape(title);
        wclass = QRegularExpression::escape(wclass);
    }

    if (m_radioTitle->isChecked()) {
        condition = QString("window title \"%1\"").arg(title);
    } else if (m_radioClass->isChecked()) {
        condition = QString("window class \"%1\"").arg(wclass);
    } else if (m_radioBoth->isChecked()) {
        condition = QString("window title \"%1\", window class \"%2\"").arg(title).arg(wclass);
    }
    
    if (matchType == "Regex" || matchType == "Contains") {
        condition.replace(QString("\""), QString("/"));
        if (matchType == "Contains") {
             condition.replace(QString("/"), QString("/.*"));
             condition.replace(QString(".*\""), QString(".*/"));
        }
    }

    m_generatedCondition->setText(condition);
}

void DialogConditionGeneratorQt::onCopyToClipboard()
{
    QApplication::clipboard()->setText(m_generatedCondition->toPlainText());
}
