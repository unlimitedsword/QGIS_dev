#include "MapCanvas.h"
#include "Output_Manager.h"
#include <qgsrasterlayer.h>
#include <qgsvectorlayer.h>
#include <qgsproject.h>
#include <qgsapplication.h>
#include <QFileInfo>
#include <QDebug>
#include <QVBoxLayout>

MapCanvas::MapCanvas(QWidget* parent)
    : QWidget(parent) // 初始化父类
{
    // 初始化地图画布
    m_qgsCanvas = new QgsMapCanvas();

    // 添加布局并将 m_qgsCanvas 加入
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_qgsCanvas);
    setLayout(layout);

    // 初始化平移工具并设置为当前工具
    m_panTool = new QgsMapToolPan(m_qgsCanvas);
    m_qgsCanvas->setMapTool(m_panTool);
    m_panTool->setCursor(Qt::OpenHandCursor);
}

void MapCanvas::addVectorLayer(QString vectorLayerPath) {
    // 检查路径是否为空或全是空白
    if (vectorLayerPath.trimmed().isEmpty()) {
        qDebug() << "文件路径为空";
        return;
    }

    // 检查文件是否存在
    if (!QFileInfo::exists(vectorLayerPath)) {
        qDebug() << "文件不存在：" << vectorLayerPath;
        return;
    }

    qDebug() << "尝试加载矢量图层：" << vectorLayerPath;
    QgsVectorLayer* vectorLayer = new QgsVectorLayer(vectorLayerPath, "My Layer", "ogr");

    if (vectorLayer->isValid()) {
        QgsProject::instance()->addMapLayer(vectorLayer);
        m_qgsCanvas->setLayers({ vectorLayer });
        m_qgsCanvas->setExtent(vectorLayer->extent());
        OutputManager::instance()->logMessage("Add vectorlayer "+vectorLayerPath+" successfully!");
    }
    else {
        qDebug() << "图层无效：" << vectorLayerPath;
        delete vectorLayer;
    }
}

MapCanvas::~MapCanvas() {

}