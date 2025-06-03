#pragma once
#include <QWidget>
#include <QTextEdit>  // 添加输出窗口控件

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