#ifndef AMONITOR_H
#define AMONITOR_H

#include <QtCore/QObject>

class QSocketNotifier;
class QTimer;

class AMonitor : public QObject {
    Q_OBJECT

    signals:
        void sigterm();

        void started();
        void stopped();

    public:
        //! Функция сигнала запроса завершения процесса.
        static void sigTermHandler(int);

        //! Конструктор.
        explicit AMonitor(const QString &process_name, QObject *parent = NULL);

        //! Деструктор.
        virtual ~AMonitor() {}

        //! Функция возврата флага активности мониторинга.
        bool isActive() const;

        //! Функция установки интервала проверки.
        void setCheckingInterval(int interval);

        //! Функция возврата директории.
        QString workPath() const;

        //! Функция установки директории.
        void setWorkPath(const QString &path);

    public slots:
        //! Слот запуска мониторинга.
        void start();

        //! Слот остановки мониторинга.
        void stop();

    protected:
        //! Функция запуска мониторинга.
        virtual void onBegin() = 0;

        //! Функция выполнения мониторинга.
        virtual void onCheck() = 0;

        //! Функция остановки мониторинга.
        virtual void onEnd() = 0;

    private:
        QSocketNotifier *_sig_term_notifier;

        bool _active;

        QTimer *_timer;

        QString _work_path, _process_name;

        //! Функция создания PID-файла.
        void createPidFile();

        //! Функция удаления PID-файла.
        void removePidFile();

    private slots:
        //! Слот сигнала запроса завершения процесса.
        void onSigTermHandle();

        //! Слот активации таймера.
        void onTimerTimeout();

};

#endif
