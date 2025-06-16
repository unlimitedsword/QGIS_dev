#pragma once
#include "FileLogger.h"
#include <QDateTime>
#include <QDebug>

FileLogger::FileLogger(const QString& logFilePath, QObject* parent)
    : QObject(parent)
{
    m_logFile.setFileName(logFilePath);

    // 以追加模式打开文件，这样即使程序快速重启也不会覆盖日志
    // QIODevice::Text 会自动处理不同操作系统下的换行符
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        qWarning() << "无法打开日志文件进行写入:" << logFilePath;
        return;
    }

    // 将文本流与文件关联
    m_logStream.setDevice(&m_logFile);
    // 确保使用UTF-8编码，以支持中文字符
    m_logStream.setCodec("UTF-8");

    // (可选) 在日志文件开头写入一条启动信息
    QString startMessage = QString("===== 日志开始于 %1 =====\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    m_logStream << startMessage;
    m_logStream.flush(); // 立即写入
}

FileLogger::~FileLogger()
{
    // 在对象销毁时，确保所有缓冲数据都已写入文件并关闭文件
    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
}

void FileLogger::onMessageLogged(const QString& message, const QString& type)
{
    if (!m_logFile.isOpen()) {
        return; // 如果文件未成功打开，则不执行任何操作
    }

    // 格式化日志条目，与您在OutputWidget中的格式完全一致
    QString formattedMessage = QString("[%1][%2] %3")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(type)
        .arg(message);

    // 将格式化后的消息写入文件流，并添加换行符
    m_logStream << formattedMessage << Qt::endl;
}