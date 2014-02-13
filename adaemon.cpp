#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <QtCore/QCoreApplication>

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
    , _stratum_port(3337), _checking_interval(5) {

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
    if(!dname.isEmpty()) _stratum_dname = dname;
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

#include <QDebug>
// ========================================================================== //
// Слот подключения сокета.
// ========================================================================== //
void ADaemon::onSocketConnected() {
    qWarning("connected - qwarning"); qDebug("connected - qdebug");
}


// ========================================================================== //
// Слот приёма сетевых сообщений.
// ========================================================================== //
void ADaemon::onSocketReadyRead() {
    QByteArray data;

    QDataStream in(_socket);
    in >> data;

    qDebug() << data;
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных.
// ========================================================================== //
void ADaemon::onSocketError(QAbstractSocket::SocketError error) {
    switch(error) {
        case QAbstractSocket::RemoteHostClosedError: break;

        case QAbstractSocket::HostNotFoundError:
            qWarning("The host was not found. Please check the host name" \
                " and port settings.");

        break;

        case QAbstractSocket::ConnectionRefusedError:
            qWarning("The connection was refused by the peer.");
        break;

        default: qWarning("The unknown error occurred."); break;
    }
}
