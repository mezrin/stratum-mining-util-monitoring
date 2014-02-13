#ifndef ADAEMON_H
#define ADAEMON_H

#include <QtCore/QSocketNotifier>
#include <QtCore/QObject>

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

    private:
        QSocketNotifier *_sig_hup_socket_notifier, *_sig_term_socket_notifier;

        QString _stratum_host, _stratum_dname;

        int _stratum_port;

        int _checking_interval;

    private slots:
        //! Слот сигнала потери соединения с управляющим терминалом.
        void onSigHupHandle();

        //! Слот сигнала запроса завершения процесса.
        void onSigTermHandle();

};

#endif
