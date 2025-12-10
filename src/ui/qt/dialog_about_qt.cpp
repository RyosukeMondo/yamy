#include "dialog_about_qt.h"
#include <QPixmap>
#include <QApplication>

DialogAboutQt::DialogAboutQt(QWidget* parent)
    : QDialog(parent)
    , m_labelIcon(nullptr)
    , m_labelTitle(nullptr)
    , m_labelVersion(nullptr)
    , m_labelBuild(nullptr)
    , m_labelDescription(nullptr)
    , m_labelLicense(nullptr)
    , m_btnClose(nullptr)
{
    setWindowTitle("About YAMY");
    setFixedSize(450, 400);

    setupUI();
}

DialogAboutQt::~DialogAboutQt()
{
}

void DialogAboutQt::onClose()
{
    close();
}

void DialogAboutQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Icon
    m_labelIcon = new QLabel();
    QPixmap icon(":/icons/yamy_enabled.png");
    if (!icon.isNull()) {
        m_labelIcon->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_labelIcon->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_labelIcon);

    // Title
    m_labelTitle = new QLabel("<h2>YAMY</h2>");
    m_labelTitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_labelTitle);

    // Subtitle
    QLabel* subtitle = new QLabel("<i>Yet Another Mado tsukai no Yuutsu</i>");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("color: #666;");
    mainLayout->addWidget(subtitle);

    // Version
    m_labelVersion = new QLabel(getVersionString());
    m_labelVersion->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_labelVersion);

    // Build info
    m_labelBuild = new QLabel(getBuildInfo());
    m_labelBuild->setAlignment(Qt::AlignCenter);
    m_labelBuild->setStyleSheet("color: #888; font-size: 10pt;");
    mainLayout->addWidget(m_labelBuild);

    mainLayout->addSpacing(10);

    // Description
    m_labelDescription = new QLabel(
        "<p>YAMY is a keyboard remapping utility that allows you to "
        "customize your keyboard layout and create powerful key combinations.</p>"
        "<p>Originally designed for Windows, now with Linux support via Qt.</p>"
    );
    m_labelDescription->setWordWrap(true);
    m_labelDescription->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_labelDescription);

    mainLayout->addSpacing(10);

    // License
    m_labelLicense = new QLabel(
        "<b>License:</b> MIT License<br>"
        "<b>Platform:</b> Linux (Qt5 GUI)<br>"
        "<b>Qt Version:</b> " + QString(qVersion()) + "<br>"
    );
    m_labelLicense->setWordWrap(true);
    m_labelLicense->setAlignment(Qt::AlignCenter);
    m_labelLicense->setStyleSheet("color: #555; font-size: 9pt;");
    mainLayout->addWidget(m_labelLicense);

    mainLayout->addStretch();

    // Close button
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnClose = new QPushButton("Close");
    m_btnClose->setDefault(true);
    m_btnClose->setMinimumWidth(100);
    connect(m_btnClose, &QPushButton::clicked, this, &DialogAboutQt::onClose);
    btnLayout->addWidget(m_btnClose);

    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

QString DialogAboutQt::getVersionString() const
{
    return QString("<b>Version %1</b>").arg(QApplication::applicationVersion());
}

QString DialogAboutQt::getBuildInfo() const
{
    QString buildInfo = "Built with Qt " + QString(QT_VERSION_STR);

#ifdef __DATE__
    buildInfo += " on " + QString(__DATE__);
#endif

    return buildInfo;
}
