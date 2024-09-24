#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

class Logger : public QObject {
    Q_OBJECT

    public:
        enum LogType {
            Command = 0,
            Message
        };

        static Logger &GetInstance();
        void Log(LogType logType, const QString &content, const QString &user);
        QString GetLog();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

    private:
        explicit Logger();
        ~Logger() {}
};

#endif // LOGGER_H
