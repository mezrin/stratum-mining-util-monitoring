#ifndef ASTRATUMMONITOR_H
#define ASTRATUMMONITOR_H

#include "amonitor.h"

class QTcpSocket;

class AStratumMonitor : public AMonitor {
    Q_OBJECT

    public:
        //! Конструктор.
        explicit AStratumMonitor(QObject *parent = NULL);

        //! Деструктор.
        virtual ~AStratumMonitor() {}

        //! Функция установки наименования алгоритма.
        void setAlgorithm(const QString &algo);

        //! Функция установки порта.
        void setPort(int port);

        //! Функция установки лимита проверок активности стратума.
        void setNumberOfChecks(int number_of_checks);

    protected:
        //! Функция запуска мониторинга.
        virtual void onBegin();

        //! Функция выполнения мониторинга.
        virtual void onCheck();

        //! Функция остановки мониторинга.
        virtual void onEnd();

    private:
        enum State {STATE_RDY_WRITE, STATE_RDY_READ};

        State _state;

        QString _host, _algorithm;

        int _port;

        int _number_of_checks, _checks;

        QTcpSocket *_socket;

        //! Функция перезапуска пула.
        void restartStratum();

    private slots:
        //! Слот приёма сетевых сообщений.
        void onSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных.
        void onSocketError();

};

#endif
