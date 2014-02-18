#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include "adaemon.h"

static int _g_sig_hup_fd[2], _g_sig_term_fd[2];

// ========================================================================== //
// Функция сигнала потери соединения с управляющим терминалом.
// ========================================================================== //
void ADaemon::sigHupHandler(int) {
    char a = 1;
    ssize_t r = ::write(_g_sig_hup_fd[0], &a, sizeof(a));
    Q_UNUSED(r);
}


// ========================================================================== //
// Функция сигнала запроса завершения процесса.
// ========================================================================== //
void ADaemon::sigTermHandler(int) {
    char a = 1;
    ssize_t r = ::write(_g_sig_term_fd[0], &a, sizeof(a));
    Q_UNUSED(r);
}


// ========================================================================== //
// Конструктор.
// ========================================================================== //
ADaemon::ADaemon(QObject *parent)
    : QObject(parent), _sig_hup_socket_notifier(NULL)
    , _sig_term_socket_notifier(NULL), _stratum_host("localhost")
    , _stratum_dname(QCoreApplication::applicationDirPath())
    , _stratum_port(3337), _checking_interval(5), _successed(false) {

    struct sigaction hup;
    hup.sa_handler = ADaemon::sigHupHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if(sigaction(SIGHUP, &hup, 0) == 0) {
        if(::socketpair(AF_UNIX, SOCK_STREAM, 0, _g_sig_hup_fd))
            qFatal("Couldn't create HUP socketpair");

        _sig_hup_socket_notifier
            = new QSocketNotifier(_g_sig_hup_fd[1], QSocketNotifier::Read
                , this);
        connect(_sig_hup_socket_notifier, SIGNAL(activated(int))
            , this, SLOT(onSigHupHandle()));
    }

    struct sigaction term;
    term.sa_handler = ADaemon::sigTermHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if(sigaction(SIGTERM, &term, 0) == 0) {
        if(::socketpair(AF_UNIX, SOCK_STREAM, 0, _g_sig_term_fd))
            qFatal("Couldn't create TERM socketpair");

        _sig_term_socket_notifier
            = new QSocketNotifier(_g_sig_term_fd[1], QSocketNotifier::Read
                , this);
        connect(_sig_term_socket_notifier, SIGNAL(activated(int))
            , this, SLOT(onSigTermHandle()));
    }

    _socket = new QTcpSocket(this);
    connect(_socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onSocketError(QAbstractSocket::SocketError)));

    _process = new QProcess(this);
    _process->setWorkingDirectory(_stratum_dname);

    createPidFile();
}


// ========================================================================== //
// Деструктор.
// ========================================================================== //
ADaemon::~ADaemon() {
    QFile pidfile(_stratum_dname + "/stratumon.pid");
    if(pidfile.exists()) pidfile.remove();
}


// ========================================================================== //
// Функция установки хоста.
// ========================================================================== //
void ADaemon::setStratumHost(const QString &host) {
    if(!host.isEmpty()) _stratum_host = host;
}


// ========================================================================== //
// Функция установки порта.
// ========================================================================== //
void ADaemon::setStratumPort(int port) {if(port > 0) _stratum_port = port;}


// ========================================================================== //
// Функция установки директории.
// ========================================================================== //
void ADaemon::setStratumDirPath(const QString &dname) {
    if(!dname.isEmpty()) {
        _stratum_dname = dname; _process->setWorkingDirectory(dname);

        createPidFile();
    }
}


// ========================================================================== //
// Функция установки интервала проверки.
// ========================================================================== //
void ADaemon::setCheckingInterval(int interval) {
    if(interval > 0) _checking_interval = interval;
}


// ========================================================================== //
// Слот выполнения подключения.
// ========================================================================== //
void ADaemon::onConnectToStratum() {
    _socket->abort(); _socket->connectToHost(_stratum_host, _stratum_port);
}


// ========================================================================== //
// Функция создания PID-файла.
// ========================================================================== //
void ADaemon::createPidFile() {
    QString fname = _stratum_dname + "/stratumon.pid";
    if(QFile::exists(fname)) {
        QFile file(fname);
        if(file.open(QFile::ReadOnly)) {
            bool ok = false;
            int pid = file.readAll().toInt(&ok);
            if(ok) {
                _process->start("ps -p " + QString::number(pid));
                _process->waitForFinished();
                if(_process->readAll().contains(QByteArray::number(pid))) {
                    _process->start("kill " + QString::number(pid));
                    _process->waitForFinished();
                }
            }

            file.close(); file.remove();
        }
    }

    QFile file(fname);
    if(file.open(QFile::WriteOnly)) {
        file.write(QByteArray::number(qApp->applicationPid()));
        file.close();
    }
}


// ========================================================================== //
// Слот сигнала потери соединения с управляющим терминалом.
// ========================================================================== //
void ADaemon::onSigHupHandle() {
    _sig_hup_socket_notifier->setEnabled(false);

    char tmp;
    ssize_t r = ::read(_g_sig_hup_fd[1], &tmp, sizeof(tmp));
    Q_UNUSED(r);

    _sig_hup_socket_notifier->setEnabled(true);

    emit sighup();
}


// ========================================================================== //
// Слот сигнала запроса завершения процесса.
// ========================================================================== //
void ADaemon::onSigTermHandle() {
    _sig_term_socket_notifier->setEnabled(false);

    char tmp;
    ssize_t r = ::read(_g_sig_term_fd[1], &tmp, sizeof(tmp));
    Q_UNUSED(r);

    _sig_term_socket_notifier->setEnabled(true);

    emit sigterm();
}


// ========================================================================== //
// Слот подключения сокета.
// ========================================================================== //
void ADaemon::onSocketConnected() {
    qDebug("Connect to Stratum is successed!");
    qDebug("Send data...");

    _successed = false;

    QTimer::singleShot(5000, this, SLOT(onSocketDataWaitingError()));

    QByteArray data;
    data.append("{");
    data.append("\"id\": 1");
    data.append(", \"method\": \"mining.subscribe\"");
    data.append(", \"params\": []");
    data.append("}\n");
    _socket->write(data);
}


// ========================================================================== //
// Слот приёма сетевых сообщений.
// ========================================================================== //
void ADaemon::onSocketReadyRead() {
    qDebug("Stratum is alive!");

    _successed = true; _socket->close();

    QTimer::singleShot(_checking_interval*1000
        , this, SLOT(onConnectToStratum()));
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных.
// ========================================================================== //
void ADaemon::onSocketError(QAbstractSocket::SocketError error) {
    switch(error) {
        case QAbstractSocket::RemoteHostClosedError:
            QTimer::singleShot(_checking_interval*1000
                , this, SLOT(onConnectToStratum()));

            return;
        break;

        case QAbstractSocket::HostNotFoundError:
            qWarning("The host was not found. Please check the host name" \
                " and port settings.");

        break;

        case QAbstractSocket::ConnectionRefusedError:
            qWarning("The connection was refused by the peer.");

            QTimer::singleShot(_checking_interval*1000
                , this, SLOT(onConnectToStratum()));

            return;
        break;

        default:
            qWarning("The unknown error occurred. Please check" \
                " the host name and port settings.");
        break;
    }

    qApp->quit();
}


// ========================================================================== //
// Слот безуспешного ожидания данных.
// ========================================================================== //
void ADaemon::onSocketDataWaitingError() {
    if(_successed) return;

    QFile stratum_pidfile(_stratum_dname + "/twistd.pid");
    if(stratum_pidfile.open(QFile::ReadOnly)) {
        QByteArray pid_data = stratum_pidfile.readAll();

        stratum_pidfile.close();

        bool ok = false;
        int pid = pid_data.toInt(&ok);
        if(ok) {
            _process->start("kill -9 " + QString::number(pid));
            _process->waitForFinished();
            _process->start("twistd -y launcher.tac");
            _process->waitForFinished();
        }
    }
}
