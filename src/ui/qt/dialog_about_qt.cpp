#include "dialog_about_qt.h"
#include <QPixmap>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>

DialogAboutQt::DialogAboutQt(QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_btnClose(nullptr)
{
    setWindowTitle("About YAMY");
    setMinimumSize(550, 450);
    resize(600, 500);

    setupUI();
}

DialogAboutQt::~DialogAboutQt()
{
}

void DialogAboutQt::onClose()
{
    close();
}

void DialogAboutQt::onLinkClicked(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

void DialogAboutQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tab widget
    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(createAboutTab(), "About");
    m_tabWidget->addTab(createLicenseTab(), "License");
    m_tabWidget->addTab(createContributorsTab(), "Contributors");
    mainLayout->addWidget(m_tabWidget);

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

QWidget* DialogAboutQt::createAboutTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(10);

    // Icon
    QLabel* labelIcon = new QLabel();
    QPixmap icon(":/icons/yamy_enabled.png");
    if (!icon.isNull()) {
        labelIcon->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    labelIcon->setAlignment(Qt::AlignCenter);
    layout->addWidget(labelIcon);

    // Title
    QLabel* labelTitle = new QLabel("<h2>YAMY</h2>");
    labelTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(labelTitle);

    // Subtitle
    QLabel* subtitle = new QLabel("<i>Yet Another Mado tsukai no Yuutsu</i>");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("color: #666;");
    layout->addWidget(subtitle);

    // Version
    QLabel* labelVersion = new QLabel(getVersionString());
    labelVersion->setAlignment(Qt::AlignCenter);
    layout->addWidget(labelVersion);

    layout->addSpacing(10);

    // Description
    QLabel* labelDesc = new QLabel(
        "<p>YAMY is a keyboard remapping utility that allows you to "
        "customize your keyboard layout and create powerful key combinations.</p>"
        "<p>Originally designed for Windows, now with Linux support via Qt.</p>"
    );
    labelDesc->setWordWrap(true);
    labelDesc->setAlignment(Qt::AlignCenter);
    layout->addWidget(labelDesc);

    layout->addSpacing(10);

    // Build info
    QLabel* labelBuild = new QLabel(getBuildInfo());
    labelBuild->setWordWrap(true);
    labelBuild->setAlignment(Qt::AlignCenter);
    labelBuild->setStyleSheet("color: #666; font-size: 9pt;");
    layout->addWidget(labelBuild);

    // Platform info
    QLabel* labelPlatform = new QLabel(getPlatformInfo());
    labelPlatform->setAlignment(Qt::AlignCenter);
    labelPlatform->setStyleSheet("color: #666; font-size: 9pt;");
    layout->addWidget(labelPlatform);

    layout->addSpacing(10);

    // Links
    QTextBrowser* links = new QTextBrowser();
    links->setOpenExternalLinks(false);
    links->setMaximumHeight(80);
    links->setHtml(
        "<p style='text-align: center;'>"
        "<b>Links:</b><br>"
        "<a href='https://github.com/yamy-dev/yamy'>GitHub Repository</a> | "
        "<a href='https://github.com/yamy-dev/yamy/wiki'>Documentation</a> | "
        "<a href='https://github.com/yamy-dev/yamy/issues'>Bug Tracker</a>"
        "</p>"
    );
    connect(links, &QTextBrowser::anchorClicked, this, &DialogAboutQt::onLinkClicked);
    layout->addWidget(links);

    layout->addStretch();

    return tab;
}

QWidget* DialogAboutQt::createLicenseTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QTextBrowser* browser = new QTextBrowser();
    browser->setOpenExternalLinks(true);
    browser->setPlainText(getLicenseText());
    layout->addWidget(browser);

    return tab;
}

QWidget* DialogAboutQt::createContributorsTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QTextBrowser* browser = new QTextBrowser();
    browser->setOpenExternalLinks(false);
    browser->setHtml(getContributorsHtml());
    connect(browser, &QTextBrowser::anchorClicked, this, &DialogAboutQt::onLinkClicked);
    layout->addWidget(browser);

    return tab;
}

QString DialogAboutQt::getVersionString() const
{
    QString version = QApplication::applicationVersion();
    if (version.isEmpty()) {
        version = "0.1.0";  // Default version
    }
    return QString("<b>Version %1</b>").arg(version);
}

QString DialogAboutQt::getBuildInfo() const
{
    QStringList info;

    // Qt version
    info << QString("Qt %1 (runtime: %2)").arg(QT_VERSION_STR, qVersion());

    // Build date
#ifdef __DATE__
    info << QString("Built: %1").arg(__DATE__);
#endif

    // Compiler
#if defined(__clang__)
    info << QString("Compiler: Clang %1.%2.%3")
            .arg(__clang_major__)
            .arg(__clang_minor__)
            .arg(__clang_patchlevel__);
#elif defined(__GNUC__)
    info << QString("Compiler: GCC %1.%2.%3")
            .arg(__GNUC__)
            .arg(__GNUC_MINOR__)
            .arg(__GNUC_PATCHLEVEL__);
#endif

    // Git commit (if defined via build system)
#ifdef GIT_COMMIT_HASH
    info << QString("Commit: %1").arg(GIT_COMMIT_HASH);
#endif

    return info.join("<br>");
}

QString DialogAboutQt::getPlatformInfo() const
{
    return QString("<b>Platform:</b> %1 %2 (%3)")
        .arg(QSysInfo::productType())
        .arg(QSysInfo::productVersion())
        .arg(QSysInfo::currentCpuArchitecture());
}

QString DialogAboutQt::getLicenseText() const
{
    return R"(MIT License

Copyright (c) YAMY Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)";
}

QString DialogAboutQt::getContributorsHtml() const
{
    return R"(<h3>Contributors</h3>
<p>YAMY is developed and maintained by a community of contributors.</p>

<h4>Original YAMY Project</h4>
<ul>
<li><b>applet</b> - Original author of YAMY for Windows</li>
<li><b>U-618</b> - Major contributor to the Windows version</li>
</ul>

<h4>Linux Port</h4>
<ul>
<li><b>YAMY Linux Team</b> - Linux port and Qt GUI implementation</li>
</ul>

<h4>Special Thanks</h4>
<ul>
<li>The <b>mayu</b> project for the original key remapping concept</li>
<li>The <b>Qt Project</b> for the excellent cross-platform framework</li>
<li>All users who reported bugs and suggested improvements</li>
</ul>

<p style='margin-top: 20px;'>
Want to contribute? Visit our
<a href='https://github.com/yamy-dev/yamy'>GitHub repository</a>!
</p>
)";
}
