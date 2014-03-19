#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include <QtNetwork/QTcpSocket>

#include "astratummonitor.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AStratumMonitor::AStratumMonitor(QObject *parent)
    : AMonitor("stratumon", parent), _state(STATE_RDY_WRITE), _host("localhost")
    , _algorithm("scrypt"), _port(3337), _number_of_checks(100), _checks(0)
    , _socket(new QTcpSocket(this)) {

    connect(_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onSocketError()));
}


// ========================================================================== //
// Функция установки наименования алгоритма.
// ========================================================================== //
void AStratumMonitor::setAlgorithm(const QString &algo) {
    if(algo == "scrypt" || algo == "keccak") {
        const bool active = isActive();
        if(active) stop();

        _algorithm = algo;

        if(active) start();
    }
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
// Функция установки лимита проверок активности стратума.
// ========================================================================== //
void AStratumMonitor::setNumberOfChecks(int number_of_checks) {
    if(number_of_checks > 0) {
        const bool active = isActive();
        if(active) stop();

        _number_of_checks = number_of_checks;

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

            _timer->setInterval(_checking_timeout*1000);

            _state = STATE_RDY_READ;
        } break;

        case STATE_RDY_READ: {
            _timer->setInterval(_checking_interval*1000);

            logWarn("stratum is not bind"); _socket->abort();

            if(++_checks >= _number_of_checks) {qApp->quit(); return;}

            restartStratum();

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

    process->start(
        QString("twistd --algorithm=%1 -y launcher.tac").arg(_algorithm));
    process->waitForFinished();
    process->deleteLater();

    logInfo("stratum started");
}


// ========================================================================== //
// Слот приёма сетевых сообщений.
// ========================================================================== //
void AStratumMonitor::onSocketReadyRead() {
    _timer->setInterval(_checking_interval*1000);

    _checks = 0; onEnd(); logInfo("stratum is alive");
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных.
// ========================================================================== //
void AStratumMonitor::onSocketError() {
    _timer->setInterval(_checking_interval*1000);

    logWarn(_socket->errorString()); _socket->abort();

    if(++_checks >= _number_of_checks) {qApp->quit(); return;}

    restartStratum();
}
