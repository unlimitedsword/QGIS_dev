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
    m_mapCanvas = new QgsMapCanvas();
    splitter->addWidget(m_mapCanvas);

    // 输出控制台（占30%空间）
    m_outputConsole = new QPlainTextEdit();
    m_outputConsole->setReadOnly(true);
    m_outputConsole->setWordWrapMode(QTextOption::NoWrap);
    splitter->addWidget(m_outputConsole);

    // 设置分割比例
    splitter->setSizes({ 700, 300 });
    setCentralWidget(splitter);

    // 初始化平移工具并设置为当前工具
    m_panTool = new QgsMapToolPan(m_mapCanvas);
    m_mapCanvas->setMapTool(m_panTool);
    m_panTool->setCursor(Qt::OpenHandCursor);
    // 使用原始字符串避免转义问题
    QString vectorLayerPath = R"(C:\Users\ASUS\Downloads\GDAL_test_data\中国标准地图-审图号GS(2020)4619号-shp格式\中国标准地图-审图号GS(2020)4619号-shp格式\中国地州界.shp)";

    // 检查文件是否存在
    if (!QFileInfo::exists(vectorLayerPath)) {
        qDebug() << "文件不存在：" << vectorLayerPath;
        return;
    }

    QgsVectorLayer* vectorLayer = new QgsVectorLayer(vectorLayerPath, "My Layer", "ogr");

    if (vectorLayer->isValid()) {
        QgsProject::instance()->addMapLayer(vectorLayer);
        m_mapCanvas->setLayers({ vectorLayer });
        m_mapCanvas->setExtent(vectorLayer->extent());

        // 打印图层信息到控制台
        printVectorInfo(vectorLayer);
    }

    // 在图层加载失败时显示错误信息
    if (!vectorLayer->isValid()) {
        QString errorMsg = QString("图层加载失败！\n错误类型: %1\n详细信息: %2")
            .arg(vectorLayer->error().summary())
            .arg(vectorLayer->error().message());

        m_outputConsole->setPlainText(errorMsg);
        m_outputConsole->setStyleSheet("color: red;");
        delete vectorLayer;
    }
}

QGIS_dev::~QGIS_dev()
{
    // 清理QGIS资源（通常在main函数中处理）
    // QgsApplication::exitQgis();
}

void QGIS_dev::printVectorInfo(QgsVectorLayer* layer)
{
    if (!layer || !layer->isValid()) return;

    QString info;
    QTextStream stream(&info);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    stream.setCodec("UTF-8");  // Qt5
#else
    stream.setEncoding(QStringConverter::Utf8); // Qt6
#endif

    stream << "=== Vector Layer Information ===" << "\n";
    stream << "Layer name: " << layer->name() << "\n";
    stream << "Source path: " << layer->source() << "\n";
    stream << "Feature count: " << layer->featureCount() << "\n";

    stream << "\n=== Layer Properties ===" << "\n";
    stream << "Geometry type: " << QgsWkbTypes::geometryDisplayString(layer->geometryType()) << "\n";
    stream << "Edit status: " << (layer->isEditable() ? "Editable" : "Read-only") << "\n";


    stream << "\nCoordinate Reference System: "
        << layer->crs().authid() << " - "
        << layer->crs().description() << "\n";

    stream << "\n=== Attribute Fields ===" << "\n";
    const QgsFields fields = layer->fields();
    for (int i = 0; i < fields.count(); ++i) {
        stream << QString("%1. %2 (%3)")
            .arg(i + 1, 2)
            .arg(fields.at(i).name(), -20)
            .arg(fields.at(i).typeName())
            << "\n";
    }

    QgsRectangle extent = layer->extent();
    stream << "\n=== Spatial Extent ===" << "\n"
        << "X minimum: " << QString::number(extent.xMinimum(), 'f', 6) << "\n"
        << "X maximum: " << QString::number(extent.xMaximum(), 'f', 6) << "\n"
        << "Y minimum: " << QString::number(extent.yMinimum(), 'f', 6) << "\n"
        << "Y maximum: " << QString::number(extent.yMaximum(), 'f', 6) << "\n";

    // Time Information
    stream << "\n=== Time Information ===" << "\n"
        << "Last modified: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";

    // Update output console
    m_outputConsole->setPlainText(info);

    // Highlight first line
    QTextCursor cursor = m_outputConsole->textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Bold);
    fmt.setForeground(Qt::blue);
    cursor.mergeCharFormat(fmt);
}