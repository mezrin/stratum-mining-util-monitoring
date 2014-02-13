#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QLocale>

#include "adaemon.h"

// ========================================================================== //
//
// ========================================================================== //
int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("AlexisSoft");
    QCoreApplication::setOrganizationDomain("free-lance.ru");
    QCoreApplication::setApplicationName("stratumon");

    QCoreApplication app(argc, argv);

    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::RussianFederation));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    ADaemon daemon(&app);
    QObject::connect(&daemon, SIGNAL(sigterm()), &app, SLOT(quit()));

    QCommandLineParser cmd_line_parser;
    cmd_line_parser.setApplicationDescription("stratumon");
    cmd_line_parser.addHelpOption();

    QCommandLineOption host_option(
        QStringList() << "i" << "stratum-host",
            QCoreApplication::translate("main"
                , "Domain or IP-address, default localhost"),
            QCoreApplication::translate("main", "host"));

    QCommandLineOption port_option(
        QStringList() << "p" << "stratum-port",
            QCoreApplication::translate("main"
                , "Number of network port, default 3337"),
            QCoreApplication::translate("main", "port"));

    QCommandLineOption dir_option(
        QStringList() << "d" << "stratum-dir",
            QCoreApplication::translate("main"
                , "Stratum directory, default current dir"),
            QCoreApplication::translate("main", "dir"));

    QCommandLineOption checking_interval_option(
        QStringList() << "c" << "checking-interval",
            QCoreApplication::translate("main"
                , "Stratum checking interval, default 5"),
            QCoreApplication::translate("main", "seconds"));

    cmd_line_parser.addOption(host_option);
    cmd_line_parser.addOption(port_option);
    cmd_line_parser.addOption(dir_option);
    cmd_line_parser.addOption(checking_interval_option);
    cmd_line_parser.process(app);

    daemon.setStratumHost(cmd_line_parser.value(host_option));
    daemon.setStratumPort(cmd_line_parser.value(port_option).toInt());
    daemon.setStratumDirPath(cmd_line_parser.value(dir_option));
    daemon.setCheckingInterval(
        cmd_line_parser.value(checking_interval_option).toInt());

    return app.exec();
}
