#pragma once
#pragma warning(disable:4996) 
#include <QtWidgets/QMainWindow>
#include <qgsmapcanvas.h>
#include <qgsmaptoolpan.h>
#include <QPlainTextEdit>  // 添加输出窗口控件

class QGIS_dev : public QMainWindow
{
    Q_OBJECT

public:
    QGIS_dev(QWidget *parent = nullptr);
    ~QGIS_dev();

private:
    QgsMapCanvas* m_mapCanvas;
    QgsMapToolPan* m_panTool; // 声明平移工具
    // 添加输出信息窗口
    QPlainTextEdit* m_outputConsole;

    // 添加打印矢量信息方法
    void printVectorInfo(QgsVectorLayer* layer);
};
