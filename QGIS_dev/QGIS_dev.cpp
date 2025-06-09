#include "QGIS_dev.h"
#include "Output_Manager.h"
#include "MapCanvas.h"
#include "OutputWidget.h"
#include "CustomLayerTreeView.h"
#include "FeatureSelectionTool.h"

#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>
#include <QFileInfo>
#include <QDebug>
#include <QFileDialog> // 用于文件对话框
#include <QDir>        // 用于获取默认路径
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h> // 需要包含栅格图层头文件
#include <qgsproject.h>
#include <qgsmaptoolpan.h>




QGIS_dev::QGIS_dev(QWidget *parent)
    : QMainWindow(parent)
{
    // 调用辅助函数来创建和设置UI
    setupUI();
    setupActions();
    setupToolBar();

    // 默认激活平移工具
    m_panAction->setChecked(true);
    onActivatePanTool(); // 确保程序启动时，平移工具是激活状态
}

QGIS_dev::~QGIS_dev()
{
}

// ---在此设置所有窗口UI---
void QGIS_dev::setupUI()
{
    // 创建核心组件
    m_mapCanvas = new MapCanvas(this); // MapCanvas保持不变

    // 将地图画布的指针传递给它
    m_customLayerTreeView = new CustomLayerTreeView(m_mapCanvas->getCanvas(), this);

    m_outputWidget = new OutputWidget(this);

    setCentralWidget(m_mapCanvas);

    // 将我们的 customLayerTreeView 放入DockWidget
    m_layerTreeDock = new QDockWidget("图层 (自定义)", this);
    m_layerTreeDock->setWidget(m_customLayerTreeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_layerTreeDock);

    // 创建并配置输出窗口DockWidget
    m_outputDock = new QDockWidget("输出", this);
    m_outputDock->setWidget(m_outputWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);
    
    // 创建菜单栏
    m_fileMenu = menuBar()->addMenu("文件(&F)"); // &F 设置快捷键 Alt+F

    // 创建工具栏
    m_toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, m_toolBar);
    m_toolBar->setFloatable(false);       // 设置是否浮动
    m_toolBar->setMovable(false);         // 设置工具栏不允许移动
}

// ---在此设置所有的菜单栏---
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

// --- 在此设置所有的工具栏 ---
void QGIS_dev::setupToolBar() 
{   
    //--- 放大缩小工具---
    m_zoomInAction = new QAction("放大", this);
    m_zoomInAction->setStatusTip("放大");
    m_zoomInAction->setIcon(QIcon("resource/images/放大.png"));
    connect(m_zoomInAction, &QAction::triggered, m_mapCanvas, &MapCanvas::zoomIn);

    m_zoomOutAction = new QAction("缩小", this);
    m_zoomOutAction->setStatusTip("缩小");
    m_zoomOutAction->setIcon(QIcon("resource/images/缩小.png"));
    connect(m_zoomOutAction, &QAction::triggered, m_mapCanvas, &MapCanvas::zoomOut);

    // --- 地图工具 ---
    m_selectionTool = new FeatureSelectionTool(m_mapCanvas->getCanvas());

    m_panAction = new QAction("平移", this);
    m_panAction->setIcon(QIcon("resource/images/平移.png"));
    m_panAction->setCheckable(true); // 工具按钮是状态性的，所以设为可选中
    m_panAction->setStatusTip("平移地图");
    connect(m_panAction, &QAction::triggered, this, &QGIS_dev::onActivatePanTool);

    m_selectAction = new QAction("选择要素", this);
    m_selectAction->setIcon(QIcon("resource/images/选择.png"));
    m_selectAction->setCheckable(true);
    m_selectAction->setStatusTip("点击以选择要素并查看信息");
    connect(m_selectAction, &QAction::triggered, this, &QGIS_dev::onActivateSelectTool);

    // 3. 使用 QActionGroup 确保同一时间只有一个工具被激活
    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->addAction(m_panAction);
    m_toolActionGroup->addAction(m_selectAction);

    m_toolBar->addAction(m_zoomInAction);
    m_toolBar->addAction(m_zoomOutAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_panAction);
    m_toolBar->addAction(m_selectAction);
}

// --- 新增：实现激活工具的槽函数 ---
void QGIS_dev::onActivatePanTool()
{
    m_mapCanvas->getCanvas()->setMapTool(m_mapCanvas->getPanTool());
}

void QGIS_dev::onActivateSelectTool()
{
    m_mapCanvas->getCanvas()->setMapTool(m_selectionTool);
}

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