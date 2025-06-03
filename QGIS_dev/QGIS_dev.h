#pragma once
#include <MapCanvas.h>
#include <OutputWidget.h>
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
    MapCanvas* m_mapCanvas;
    OutputWidget* m_outputWidget;
};
