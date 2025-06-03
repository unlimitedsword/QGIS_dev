#include "QGIS_dev.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QAction>
#include <qgsrasterlayer.h>
#include <qgsvectorlayer.h>
#include <qgsproject.h>
#include <qgsmaptoolpan.h>
#include <qgsapplication.h>
#include <QSplitter>


QGIS_dev::QGIS_dev(QWidget* parent)
    : QMainWindow(parent)
{
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    // 地图画布（占70%空间）
    m_mapCanvas = new MapCanvas();
    splitter->addWidget(m_mapCanvas);

    // 输出控制台（占30%空间）
    m_outputWidget = new OutputWidget();
    splitter->addWidget(m_outputWidget);

    // 设置分割比例
    splitter->setSizes({ 700, 300 });
    setCentralWidget(splitter);

    // 使用原始字符串避免转义问题
    QString vectorLayerPath = R"(C:\Users\ASUS\Downloads\GDAL_test_data\中国标准地图-审图号GS(2020)4619号-shp格式\中国标准地图-审图号GS(2020)4619号-shp格式\中国地州界.shp)";

    m_mapCanvas->addVectorLayer(vectorLayerPath);
}

QGIS_dev::~QGIS_dev()
{
    // 清理QGIS资源（通常在main函数中处理）
    // QgsApplication::exitQgis();
}
