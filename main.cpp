#include <sys/types.h>
#include <sys/stat.h>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QLocale>

#include "adaemon.h"

// ========================================================================== //
// Функция запуска процесса.
// ========================================================================== //
int startProcess(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("AlexisSoft");
    QCoreApplication::setOrganizationDomain("free-lance.ru");
    QCoreApplication::setApplicationName("stratumon");

    QCoreApplication app(argc, argv);

    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::RussianFederation));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

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

    QCommandLineOption service_option(
        QStringList() << "t" << "terminal",
            QCoreApplication::translate("main"
                , "Start application in interactive mode."));

    cmd_line_parser.addOption(host_option);
    cmd_line_parser.addOption(port_option);
    cmd_line_parser.addOption(dir_option);
    cmd_line_parser.addOption(checking_interval_option);
    cmd_line_parser.addOption(service_option);
    cmd_line_parser.process(app);

    if(!cmd_line_parser.isSet(service_option)) {
        ADaemon daemon(&app);
        QObject::connect(&daemon, SIGNAL(sigterm()), &app, SLOT(quit()));

        daemon.setStratumHost(cmd_line_parser.value(host_option));
        daemon.setStratumPort(cmd_line_parser.value(port_option).toInt());
        daemon.setStratumDirPath(cmd_line_parser.value(dir_option));
        daemon.setCheckingInterval(
            cmd_line_parser.value(checking_interval_option).toInt());

        QMetaObject::invokeMethod(&daemon, "onConnectToStratum"
            , Qt::QueuedConnection);
    }

    return app.exec();
}


// ========================================================================== //
//
// ========================================================================== //
int main(int argc, char *argv[]) {
    bool has_terminal = false;
    for(int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if(arg == "-t" || arg == "--terminal"
            || arg == "-h" || arg == "--help") {

            has_terminal = true; break;
        }
    }

    if(has_terminal) return startProcess(argc, argv);

    int pid = fork();
    if(pid == -1) return -1;

    if(!pid) {
        umask(0); setsid(); int r = chdir("/"); Q_UNUSED(r);

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        return startProcess(argc, argv);
    }

    return 0;
}
