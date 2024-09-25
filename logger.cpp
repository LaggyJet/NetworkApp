#include "logger.h"
#include <QDir>
#include <QDateTime>

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    QDir logs("logs");
    if (!logs.exists())
        logs.mkpath(".");
}

void Logger::Log(LogType logType, const QString &content, const QString &user) {
    QDir logDir("logs");
    if (!logDir.exists())
        logDir.mkpath(".");
    QString logFileName = (logType == Command) ? "commands.txt" : "messages.txt";
    QFile logFile(logDir.filePath(logFileName));
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file: " << logFile.errorString();
        return;
    }
    QTextStream out(&logFile);
    QString timestamp = QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm:ss");
    QString type = (logType == Command) ? "Command" : "Message";
    out << "[" << timestamp << "] User: " << user << "\t| " << type << ": " << content << "\n";
}

QString Logger::GetLog() {
    QDir logDir("logs");
    QFile logFile(logDir.filePath("messages.txt"));
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open log file: " << logFile.errorString();
        return QString();
    }
    QTextStream in(&logFile);
    QString logContent = in.readAll();
    return logContent;
}
