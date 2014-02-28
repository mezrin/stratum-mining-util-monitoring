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

        //! Функция установки хоста.
        void setHost(const QString &host);

        //! Функция установки порта.
        void setPort(int port);

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

        QString _host;

        int _port;

        QTcpSocket *_socket;

    private slots:
        //! Слот приёма сетевых сообщений.
        void onSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных.
        void onSocketError();

};

#endif
