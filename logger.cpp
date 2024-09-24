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
    QDir logDir(QString("logs"));
    if (!logDir.exists())
        logDir.mkpath(".");
    QFile *curLogFile = new QFile(logDir.filePath((logType == Command) ? "commands.txt" : "messages.txt"));
    if (!curLogFile->open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file: " << curLogFile->errorString();
        delete curLogFile;
        return;
    }
    QTextStream out(curLogFile);
    QString timestamp = QDateTime::currentDateTime().toString("MM-dd-yyyy hh:mm:ss");
    QString type = (logType == Command) ? "Command" : "Message";
    out << "[" << timestamp << "] User: " << user << "\t| " << type << ": " << content << "\n";
    out.flush();
    curLogFile->close();
    delete curLogFile;
}

QString Logger::GetLog() {
    QDir logDir(QString("logs"));
    QFile logFile(logDir.filePath("messages.txt"));
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open log file: " << logFile.errorString();
        return QString(); 
    }
    QTextStream in(&logFile);
    QString logContent = in.readAll();
    logFile.close();
    return logContent;
}
