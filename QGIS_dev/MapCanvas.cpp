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

    // --- 连接CRS和比例尺信号 ---
    m_qgsCanvas->setDestinationCrs(QgsProject::instance()->crs());
    connect(QgsProject::instance(), &QgsProject::crsChanged, m_qgsCanvas, [this]() {
        m_qgsCanvas->setDestinationCrs(QgsProject::instance()->crs());
        });

    // !! 新增 !!: 连接QgsMapCanvas的比例尺变化信号到我们的私有槽
    connect(m_qgsCanvas, &QgsMapCanvas::scaleChanged, this, &MapCanvas::onCanvasScaleChanged);

    // !! 新增 !!: 应用程序启动时，手动触发一次比例尺更新，以初始化显示
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
    m_qgsCanvas->zoomIn();
}

void MapCanvas::zoomOut()
{
    m_qgsCanvas->zoomOut();
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