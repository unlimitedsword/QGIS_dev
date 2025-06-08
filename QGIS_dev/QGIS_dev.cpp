#include "QGIS_dev.h"
#include <QFileInfo>
#include <QDebug>
#include <QFileDialog> // 用于文件对话框
#include <QDir>        // 用于获取默认路径
#include "Output_Manager.h"
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h> // 需要包含栅格图层头文件
#include <qgsproject.h>

QGIS_dev::QGIS_dev(QWidget *parent)
    : QMainWindow(parent)
{
    // 调用辅助函数来创建和设置UI
    setupUI();
    setupActions();
    setupToolBar();
}

QGIS_dev::~QGIS_dev()
{
}

void QGIS_dev::setupUI()
{
    // 1. 创建核心组件
    m_mapCanvas = new MapCanvas(this); // MapCanvas保持不变

    // !! 关键修改: 实例化我们自己的图层树 !!
    // 并将地图画布的指针传递给它
    m_customLayerTreeView = new CustomLayerTreeView(m_mapCanvas->getCanvas(), this);

    m_outputWidget = new OutputWidget(this);

    setCentralWidget(m_mapCanvas);

    // 将我们的 customLayerTreeView 放入DockWidget
    m_layerTreeDock = new QDockWidget("图层 (自定义)", this);
    m_layerTreeDock->setWidget(m_customLayerTreeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_layerTreeDock);

    // 4. 创建并配置输出窗口DockWidget
    m_outputDock = new QDockWidget("输出", this);
    m_outputDock->setWidget(m_outputWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);
    
    // 5. 创建菜单栏
    m_fileMenu = menuBar()->addMenu("文件(&F)"); // &F 设置快捷键 Alt+F

    // 创建工具栏
    m_toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, m_toolBar);
    m_toolBar->setFloatable(false);       // 设置是否浮动
    m_toolBar->setMovable(false);         // 设置工具栏不允许移动
}

// 新增的Action设置函数
void QGIS_dev::setupActions()
{
    // 创建“添加矢量图层”动作
    m_addVectorAction = new QAction("添加矢量图层", this);
    m_addVectorAction->setStatusTip("从文件加载一个矢量图层 (如: .shp)");
    connect(m_addVectorAction, &QAction::triggered, this, &QGIS_dev::onAddVectorLayer);

    // 创建“添加栅格图层”动作
    m_addRasterAction = new QAction("添加栅格图层", this);
    m_addRasterAction->setStatusTip("从文件加载一个栅格图层 (如: .tif)");
    connect(m_addRasterAction, &QAction::triggered, this, &QGIS_dev::onAddRasterLayer);

    // 将动作添加到“文件”菜单
    m_fileMenu->addAction(m_addVectorAction);
    m_fileMenu->addAction(m_addRasterAction);
}

void QGIS_dev::setupToolBar() 
{
    m_zoomInAction = new QAction("放大", this);
    m_zoomInAction->setIcon(QIcon("resource/images/放大.png"));
    connect(m_zoomInAction, &QAction::triggered, m_mapCanvas, &MapCanvas::zoomIn);

    m_zoomOutAction = new QAction("缩小", this);
    m_zoomOutAction->setIcon(QIcon("resource/images/缩小.png"));
    connect(m_zoomOutAction, &QAction::triggered, m_mapCanvas, &MapCanvas::zoomOut);

    m_toolBar->addAction(m_zoomInAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_zoomOutAction);


}

// -- 实现槽函数 --

void QGIS_dev::onAddVectorLayer()
{
    // 1. 定义文件过滤器，只显示常见的矢量文件格式
    QString filter = "Shapefile (*.shp);;GeoPackage (*.gpkg);;All files (*.*)";
    
    // 2. 弹出文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(this, "选择一个矢量文件", QDir::homePath(), filter);

    // 3. 检查用户是否选择了文件
    if (filePath.isEmpty()) {
        OutputManager::instance()->logWarning("未选择文件，操作取消。");
        return;
    }

    // 4. 加载图层（逻辑与之前构造函数中的类似）
    QString layerName = QFileInfo(filePath).baseName();
    QgsVectorLayer* vectorLayer = new QgsVectorLayer(filePath, layerName, "ogr");

    if (vectorLayer->isValid()) {
        // 关键一步：将图层添加到项目中
        QgsProject::instance()->addMapLayer(vectorLayer);
        m_customLayerTreeView->addLayer(vectorLayer);

        // 缩放功能依然可用
        m_mapCanvas->zoomToLayer(vectorLayer);

        // MapCanvas和LayerTreeView会自动更新！
        OutputManager::instance()->logMessage("成功加载矢量图层: " + filePath);
    } else {
        QString errorMsg = "加载矢量图层失败: " + filePath;
        OutputManager::instance()->logError(errorMsg);
        delete vectorLayer; // 释放内存
    }
}

void QGIS_dev::onAddRasterLayer()
{
    // 1. 定义栅格文件过滤器
    QString filter = "GeoTIFF (*.tif *.tiff);;Erdas Imagine (*.img);;All files (*.*)";

    // 2. 弹出文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(this, "选择一个栅格文件", QDir::homePath(), filter);

    // 3. 检查用户是否选择了文件
    if (filePath.isEmpty()) {
        OutputManager::instance()->logMessage("未选择文件，操作取消。");
        return;
    }

    // 4. 加载栅格图层
    QString layerName = QFileInfo(filePath).baseName();
    QgsRasterLayer* rasterLayer = new QgsRasterLayer(filePath, layerName);

    if (rasterLayer->isValid()) {
        // 关键一步：将图层添加到项目中
        QgsProject::instance()->addMapLayer(rasterLayer);
        m_customLayerTreeView->addLayer(rasterLayer);

        m_mapCanvas->zoomToLayer(rasterLayer);

        OutputManager::instance()->logMessage("成功加载栅格图层: " + filePath);
    } else {
        QString errorMsg = "加载栅格图层失败: " + filePath;
        OutputManager::instance()->logError(errorMsg);
        delete rasterLayer;
    }
}