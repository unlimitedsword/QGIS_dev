#pragma once
#include <QWidget>

// 前向声明
class QTextEdit;

class OutputWidget : public QWidget
{
	Q_OBJECT

public:
	OutputWidget(QWidget* parent = nullptr);
	~OutputWidget();

	void displayMessage(const QString& message,const QString& type);

private:
	QTextEdit* m_outputConsole;
};