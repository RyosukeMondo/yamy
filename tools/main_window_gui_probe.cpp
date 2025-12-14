#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "ui/qt/main_window_gui.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("yamy_gui_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Probe MainWindowGUI against a running daemon"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption serverNameOpt(
        QStringList{QStringLiteral("s"), QStringLiteral("server-name")},
        QStringLiteral("Custom IPC server name (defaults to yamy-engine)."),
        QStringLiteral("name"));
    parser.addOption(serverNameOpt);

    QCommandLineOption durationOpt(
        QStringList{QStringLiteral("d"), QStringLiteral("duration")},
        QStringLiteral("How long to keep the window alive (ms)."),
        QStringLiteral("ms"),
        QStringLiteral("8000"));
    parser.addOption(durationOpt);

    parser.process(app);

    const QString serverName = parser.value(serverNameOpt);
    bool ok = false;
    const int durationMs = parser.value(durationOpt).toInt(&ok);
    const int runForMs = ok && durationMs > 0 ? durationMs : 8000;

    MainWindowGUI window(serverName);
    window.show();

    const QString resolvedServer = serverName.isEmpty()
        ? QStringLiteral("yamy-engine (default)")
        : serverName;
    qInfo().noquote() << "[Probe]" << "Connecting to" << resolvedServer
                      << "for" << runForMs << "ms";

    QTimer::singleShot(runForMs, &app, &QCoreApplication::quit);
    return app.exec();
}
