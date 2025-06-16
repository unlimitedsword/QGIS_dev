#pragma once
#include <QObject>
#include <QFile>
#include <QTextStream>

class FileLogger : public QObject
{
    Q_OBJECT

public:
    // 构造函数接收日志文件的完整路径
    explicit FileLogger(const QString& logFilePath, QObject* parent = nullptr);
    ~FileLogger();

public slots:
    // 这个槽将连接到 OutputManager::messageLogged 信号
    void onMessageLogged(const QString& message, const QString& type);

private:
    QFile m_logFile;
    QTextStream m_logStream;
};