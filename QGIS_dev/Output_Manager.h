#pragma once
#include <QObject>

class OutputManager : public QObject 
{
	Q_OBJECT

private:
	// 私有构造函数，防止外部创建实例
	explicit OutputManager(QObject* parent = nullptr);

	// 静态私有成员，拥有唯一实例
	static OutputManager* m_instance;

public:
	// 公共静态方法，用于记录并发送内容
	static OutputManager* instance();

public slots:
	// 公共槽函数，用于记录发送内容
	void logMessage(const QString& message);
	void logWarning(const QString& message);
	void logError(const QString& message);

signals:
	// 信号，当有新消息时发送
	void messageLogged(const QString& message, const QString& type = "INFO"); // 信号类型，可以是INFO，WARNING，ERROR

};