#include "Output_Manager.h"
#include <QDebug>  // 用于控制台输出，便于调试

OutputManager* OutputManager::m_instance = nullptr;


OutputManager::OutputManager(QObject* parent)
	:	QObject(parent) 
{
	qDebug() << "OutputManager instance created.";
}

OutputManager* OutputManager::instance()
{
	if (!m_instance) {
		m_instance = new OutputManager();
	}
	return m_instance;
}

void OutputManager::logMessage(const QString& message) {
	emit messageLogged(message, "INFO");
}

void OutputManager::logWarning(const QString& message) {
	emit messageLogged(message, "WARNING");
}

void OutputManager::logError(const QString& message) {
	emit messageLogged(message, "ERROR");
}

