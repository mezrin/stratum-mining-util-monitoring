#include <QtCore/QProcess>
#include <QtCore/QFile>

#include <QtNetwork/QTcpSocket>

#include "astratummonitor.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AStratumMonitor::AStratumMonitor(QObject *parent)
    : AMonitor("stratumon", parent), _state(STATE_RDY_WRITE), _host("localhost")
    , _port(3337), _socket(new QTcpSocket(this)) {

    connect(_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onSocketError()));
}


// ========================================================================== //
// Функция установки порта.
// ========================================================================== //
void AStratumMonitor::setPort(int port) {
    if(port > 0) {
        const bool active = isActive();
        if(active) stop();

        _port = port;

        if(active) start();
    }
}


// ========================================================================== //
// Функция запуска мониторинга.
// ========================================================================== //
void AStratumMonitor::onBegin() {
    _state = STATE_RDY_WRITE; _socket->connectToHost(_host, _port);
}


// ========================================================================== //
// Функция выполнения мониторинга.
// ========================================================================== //
void AStratumMonitor::onCheck() {
    if(_socket->state() != QAbstractSocket::ConnectedState) {
        if(_socket->state() == QAbstractSocket::UnconnectedState) onBegin();

        return;
    }

    switch(_state) {
        case STATE_RDY_WRITE: {
            QByteArray data;
            data.append("{");
            data.append("\"id\": 1");
            data.append(", \"method\": \"mining.subscribe\"");
            data.append(", \"params\": []");
            data.append("}\n");

            _socket->write(data);

            _state = STATE_RDY_READ;
        } break;

        case STATE_RDY_READ: {
            logWarn("stratum is not bind");

            _socket->abort(); restartStratum();

            _state = STATE_RDY_WRITE;
        } break;
    }
}


// ========================================================================== //
// Функция остановки мониторинга.
// ========================================================================== //
void AStratumMonitor::onEnd() {_socket->abort();}


// ========================================================================== //
// Функция перезапуска пула.
// ========================================================================== //
void AStratumMonitor::restartStratum() {
    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(workPath());

    QFile file(workPath() + "/twistd.pid");
    if(file.open(QFile::ReadOnly)) {
        QByteArray pid_data = file.readAll();

        file.close();

        bool ok = false;
        int pid = pid_data.toInt(&ok);
        if(ok) {
            process->start("kill -9 " + QString::number(pid));
            process->waitForFinished();

            logWarn("old stratum process killed");
        }
    }

    process->start("twistd -y launcher.tac");
    process->waitForFinished();
    process->deleteLater();

    logInfo("stratum started");
}


// ========================================================================== //
// Слот приёма сетевых сообщений.
// ========================================================================== //
void AStratumMonitor::onSocketReadyRead() {
    onEnd(); logInfo("stratum is alive");
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных.
// ========================================================================== //
void AStratumMonitor::onSocketError() {
    logWarn(_socket->errorString()); _socket->abort(); restartStratum();
}
