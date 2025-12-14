#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

#include "ui/qt/main_window_gui.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("yamy-gui"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("YAMY GUI front-end"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption serverNameOpt(
        QStringList{QStringLiteral("s"), QStringLiteral("server-name")},
        QStringLiteral("Override IPC server name (default: yamy-engine)."),
        QStringLiteral("name"));
    parser.addOption(serverNameOpt);

    parser.process(app);

    const QString serverName = parser.value(serverNameOpt);

    MainWindowGUI window(serverName);
    window.show();

    return app.exec();
}
