#include "MapCanvas.h"
#include <qgsmapcanvas.h>
#include <qgsmaptoolpan.h>
#include <qgsproject.h>
#include <qgsmaplayer.h>
#include <QVBoxLayout>
#include <QDebug>

MapCanvas::MapCanvas(QWidget* parent)
    : QWidget(parent)
{
    m_qgsCanvas = new QgsMapCanvas();
    m_qgsCanvas->setCanvasColor(Qt::white);
    m_qgsCanvas->enableAntiAliasing(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_qgsCanvas);
    setLayout(layout);

    // 平移工具
    m_panTool = new QgsMapToolPan(m_qgsCanvas);
    m_qgsCanvas->setMapTool(m_panTool);
    m_panTool->setCursor(Qt::OpenHandCursor);

    // ====================== 核心修正 ======================
    // 删除所有主动设置 QgsProject::instance()->setCrs() 的代码。
    // MapCanvas 的职责：只做一件事 —— 忠实地跟随 QgsProject 的CRS。

    // 1. 启动时，将自己的目标CRS设置为当前项目的CRS (此时可能无效)
    m_qgsCanvas->setDestinationCrs(QgsProject::instance()->crs());

    // 2. 建立一个永久的连接：只要项目CRS变了，画布就跟着变。
    connect(QgsProject::instance(), &QgsProject::crsChanged,
        m_qgsCanvas, [this]() {
            m_qgsCanvas->setDestinationCrs(QgsProject::instance()->crs());
        });

    // ========================================================

    // 连接比例尺信号 (这部分不变)
    connect(m_qgsCanvas, &QgsMapCanvas::scaleChanged, this, &MapCanvas::onCanvasScaleChanged);
    onCanvasScaleChanged(m_qgsCanvas->scale());
}

MapCanvas::~MapCanvas()
{
}

QgsMapCanvas* MapCanvas::getCanvas() const
{
    return m_qgsCanvas;
}

// 获取平移工具的实现
QgsMapToolPan* MapCanvas::getPanTool() const
{
    return m_panTool;
}

void MapCanvas::zoomToLayer(QgsMapLayer* layer)
{
    if (layer && layer->isValid()) {
        m_qgsCanvas->setExtent(layer->extent());
        m_qgsCanvas->refresh();
        // 注意：setExtent会自动改变比例尺，从而触发scaleChanged信号，所以这里无需额外操作
    }
}

// --- 新增接口的实现 ---

void MapCanvas::zoomIn()
{
    if (m_qgsCanvas) {
        m_qgsCanvas->zoomIn(); // 将调用传递给QGIS画布
    }
}

void MapCanvas::zoomOut()
{
    if (m_qgsCanvas) {
        m_qgsCanvas->zoomOut(); // 将调用传递给QGIS画布
    }
}

// --- 新增私有槽的实现 ---

void MapCanvas::onCanvasScaleChanged(double newScale)
{
    // 将double类型的比例尺分母格式化为 "1:XXXXX" 的字符串
    QString formattedScale = QString("比例尺 1:%1").arg(static_cast<int>(newScale));

    // 发射我们自己的、携带格式化字符串的信号
    emit scaleChanged(formattedScale);
    qDebug() << "Scale changed to:" << formattedScale;
}