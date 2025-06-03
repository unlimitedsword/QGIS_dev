#include "Output_Manager.h"
#include <OutputWidget.h>
#include <QVBoxLayout>
#include <QDateTime>
#include <QString>

OutputWidget::OutputWidget(QWidget* parent)
	: QWidget(parent)
{
    // 输出控制台
    m_outputConsole = new QTextEdit();
    m_outputConsole->setReadOnly(true);
    m_outputConsole->setWordWrapMode(QTextOption::NoWrap);

    // 添加布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_outputConsole);
    setLayout(layout);

    QString testMessage = "This is a test message!";
    m_outputConsole->append(testMessage);
    connect(OutputManager::instance(), &OutputManager::messageLogged, this, &OutputWidget::displayMessage);
}

void OutputWidget::displayMessage(const QString& message,const QString& type) {
    QString formattedMessage = QString("[%1] [%2] %3").arg(QDateTime::currentDateTime().toString("yyyy-MM-ddhh:mm:ss")).arg(type).arg(message);
    m_outputConsole->append(formattedMessage);
}

OutputWidget::~OutputWidget() {

}