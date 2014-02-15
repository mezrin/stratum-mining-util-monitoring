#ifndef ADAEMON_H
#define ADAEMON_H

#include <QtCore/QSocketNotifier>
#include <QtCore/QObject>

#include <QtNetwork/QTcpSocket>

class QProcess;

class ADaemon : public QObject {
    Q_OBJECT

    signals:
        void sighup();
        void sigterm();

    public:
        //! Функция сигнала потери соединения с управляющим терминалом.
        static void sigHupHandler(int);

        //! Функция сигнала запроса завершения процесса.
        static void sigTermHandler(int);

        //! Конструктор.
        explicit ADaemon(QObject *parent = NULL);

        //! Деструктор.
        virtual ~ADaemon() {}

        //! Функция установки хоста.
        void setStratumHost(const QString &host);

        //! Функция установки порта.
        void setStratumPort(int port);

        //! Функция установки директории.
        void setStratumDirPath(const QString &dname);

        //! Функция установки интервала проверки.
        void setCheckingInterval(int interval);

    public slots:
        //! Слот выполнения подключения.
        void onConnectToStratum();

    private:
        QSocketNotifier *_sig_hup_socket_notifier, *_sig_term_socket_notifier;

        QString _stratum_host, _stratum_dname;

        int _stratum_port;

        int _checking_interval;

        bool _successed;

        QTcpSocket *_socket;

        QProcess *_process;

        //! Функция создания PID-файла.
        void createPidFile();

    private slots:
        //! Слот сигнала потери соединения с управляющим терминалом.
        void onSigHupHandle();

        //! Слот сигнала запроса завершения процесса.
        void onSigTermHandle();

        //! Слот подключения сокета.
        void onSocketConnected();

        //! Слот приёма сетевых сообщений.
        void onSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных.
        void onSocketError(QAbstractSocket::SocketError error);

        //! Слот безуспешного ожидания данных.
        void onSocketDataWaitingError();

};

#endif
