#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QLocale>

#include <QDebug>

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

    QString host = "localhost", dname = QCoreApplication::applicationDirPath();

    int port = 3337, checking_interval_seconds = 5;

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

    QString host_value = cmd_line_parser.value(host_option);
    if(!host_value.isEmpty()) host = host_value;

    bool ok = false;
    int port_value = cmd_line_parser.value(port_option).toInt(&ok);
    if(ok && port_value > 0) port = port_value;

    QString dname_value = cmd_line_parser.value(dir_option);
    if(!dname_value.isEmpty()) dname = dname_value;

    int checking_interval_value
        = cmd_line_parser.value(checking_interval_option).toInt(&ok);
    if(ok && checking_interval_value > 0)
        checking_interval_seconds = checking_interval_value;

qDebug() << host << port << dname << checking_interval_seconds;

    return app.exec();
}
