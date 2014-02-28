#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSocketNotifier>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "amonitor.h"
#include "alogger.h"

static int _g_sig_term_fd[2];

// ========================================================================== //
// Функция сигнала запроса завершения процесса.
// ========================================================================== //
void AMonitor::sigTermHandler(int) {
    char a = 1;
    ssize_t r = ::write(_g_sig_term_fd[0], &a, sizeof(a));
    Q_UNUSED(r);
}


// ========================================================================== //
// Конструктор.
// ========================================================================== //
AMonitor::AMonitor(const QString &process_name, QObject *parent)
    : QObject(parent), _sig_term_notifier(NULL), _active(false)
    , _timer(new QTimer(this))
    , _work_path(QCoreApplication::applicationDirPath())
    , _process_name(process_name) {

    struct sigaction term;
    term.sa_handler = AMonitor::sigTermHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if(sigaction(SIGTERM, &term, 0) == 0) {
        if(::socketpair(AF_UNIX, SOCK_STREAM, 0, _g_sig_term_fd))
            qFatal("Couldn't create TERM socketpair");

        _sig_term_notifier
            = new QSocketNotifier(_g_sig_term_fd[1], QSocketNotifier::Read
                , this);
        connect(_sig_term_notifier, SIGNAL(activated(int))
            , this, SLOT(onSigTermHandle()));
    }

    _timer->setInterval(5000);
    _timer->setSingleShot(true);

    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
}


// ========================================================================== //
// Функция возврата флага активности мониторинга.
// ========================================================================== //
bool AMonitor::isActive() const {return _active;}


// ========================================================================== //
// Функция установки интервала проверки.
// ========================================================================== //
void AMonitor::setCheckingInterval(int interval) {
    if(interval > 0) _timer->setInterval(interval);
}


// ========================================================================== //
// Функция возврата директории.
// ========================================================================== //
QString AMonitor::workPath() const {return _work_path;}


// ========================================================================== //
// Функция установки директории.
// ========================================================================== //
void AMonitor::setWorkPath(const QString &path) {
    if(!path.isEmpty() && QDir(path).exists()) {
        const bool active = _active;
        if(active) stop();

        _work_path = path;

        if(active) start();
    }
}


// ========================================================================== //
// Слот запуска мониторинга.
// ========================================================================== //
void AMonitor::start() {
    if(_active) return;

    logInfo("stratumon started");

    createPidFile(); _timer->start(); _active = true; onBegin(); emit started();
}


// ========================================================================== //
// Слот остановки мониторинга.
// ========================================================================== //
void AMonitor::stop() {
    if(!_active) return;

    logInfo("stratumon stopped");

    _timer->stop(); removePidFile(); _active = false; onEnd(); emit stopped();
}


// ========================================================================== //
// Функция создания PID-файла.
// ========================================================================== //
void AMonitor::createPidFile() {
    QString fname = QString("%1/%2.pid").arg(_work_path).arg(_process_name);

    if(QFile::exists(fname)) {
        QFile file(fname);
        if(file.open(QFile::ReadOnly)) {
            bool ok = false;
            int pid = file.readAll().toInt(&ok);
            if(ok) {
                QProcess *process = new QProcess(this);
                process->start("ps -p " + QString::number(pid));
                process->waitForFinished();
                if(process->readAll().contains(QByteArray::number(pid))) {
                    process->start("kill " + QString::number(pid));
                    process->waitForFinished();
                }

                process->deleteLater();
            }

            file.close();
        }

        file.remove();
    }

    QFile file(fname);
    if(file.open(QFile::WriteOnly)) {
        file.write(QByteArray::number(qApp->applicationPid()));
        file.close();
    }
}


// ========================================================================== //
// Функция удаления PID-файла.
// ========================================================================== //
void AMonitor::removePidFile() {
    QFile file(QString("%1/%2.pid").arg(_work_path).arg(_process_name));
    if(file.exists()) file.remove();
}


// ========================================================================== //
// Слот сигнала запроса завершения процесса.
// ========================================================================== //
void AMonitor::onSigTermHandle() {
    if(!_sig_term_notifier) return;

    _sig_term_notifier->setEnabled(false);

    char tmp;
    ssize_t r = ::read(_g_sig_term_fd[1], &tmp, sizeof(tmp));
    Q_UNUSED(r);

    _sig_term_notifier->setEnabled(true);

    emit sigterm();
}


// ========================================================================== //
// Слот активации таймера.
// ========================================================================== //
void AMonitor::onTimerTimeout() {
    onCheck(); if(_active) _timer->start();
}
