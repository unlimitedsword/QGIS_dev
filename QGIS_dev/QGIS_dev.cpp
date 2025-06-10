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
#include <QMessageBox>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h> // 需要包含栅格图层头文件
#include <qgslayertree.h>
#include <qgsproject.h>
#include <qgsmaptoolpan.h>




QGIS_dev::QGIS_dev(QWidget *parent)
    : QMainWindow(parent), m_isProjectDirty(false) // 初始化项目状态
{
    // 调用辅助函数来创建和设置UI
    setupUI();
    setupActions();
    setupToolBar();

    // 默认激活平移工具
    m_panAction->setChecked(true);
    onActivatePanTool(); // 确保程序启动时，平移工具是激活状态

    // 连接 CustomLayerTreeView 的修改信号，以更新“脏”状态
    // (这需要在 CustomLayerTreeView 中添加一个信号)
    connect(m_customLayerTreeView, &CustomLayerTreeView::modelChanged, this, &QGIS_dev::onProjectDirty);

    // 在 QGIS_dev 构造函数中

    connect(QgsProject::instance(), &QgsProject::layersAdded, this, &QGIS_dev::onProjectDirty);
    connect(QgsProject::instance(), &QgsProject::layersAdded, this, [this](const QList<QgsMapLayer*>& layers) {
        for (QgsMapLayer* layer : layers) {
            m_customLayerTreeView->addLayer(layer);
        }
        // 标记为脏状态的连接已经有了，这里专门处理UI更新
        });
    connect(QgsProject::instance(), QOverload<const QStringList&>::of(&QgsProject::layersWillBeRemoved), this, [this]() {
        this->onProjectDirty();
        });

    updateWindowTitle();
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

    // 新建项目动作
    m_newAction = new QAction("新建项目", this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, &QGIS_dev::onNewProject);

    // 打开项目动作
    m_openAction = new QAction("打开项目...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &QGIS_dev::onOpenProject);

    // 保存项目动作
    m_saveAction = new QAction("保存项目", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &QGIS_dev::onSaveProject);

    // 另存为动作
    m_saveAsAction = new QAction("项目另存为...", this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &QGIS_dev::onSaveProjectAs);

    // 将新动作添加到菜单栏
    m_fileMenu->addAction(m_newAction);
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_saveAction);
    m_fileMenu->addAction(m_saveAsAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_addVectorAction);
    m_fileMenu->addAction(m_addRasterAction);
}

// ---- 新的项目管理槽函数 ----

void QGIS_dev::onNewProject()
{
    if (maybeSave()) {
        QgsProject::instance()->clear();
        m_customLayerTreeView->clear(); // 需要在 CustomLayerTreeView 中实现 clear()
        m_projectFilePath.clear();
        m_isProjectDirty = false;
        updateWindowTitle();
        OutputManager::instance()->logMessage("新项目已创建。");
    }
}

void QGIS_dev::onOpenProject()
{
    if (maybeSave()) {
        QString filePath = QFileDialog::getOpenFileName(this, "打开QGIS项目", QDir::homePath(), "QGIS Projects (*.qgz *.qgs)");
        if (filePath.isEmpty()) return;

        QgsProject::instance()->clear(); // 清除旧项目
        m_customLayerTreeView->clear();

        if (QgsProject::instance()->read(filePath)) {
            m_projectFilePath = filePath;
            rebuildLayerTreeFromProject(); // 核心：从项目重建UI
            m_isProjectDirty = false;
            updateWindowTitle();
            OutputManager::instance()->logMessage("项目已打开: " + filePath);
        }
        else {
            QMessageBox::critical(this, "错误", "无法读取项目文件: " + filePath);
            m_projectFilePath.clear();
            updateWindowTitle();
        }
    }
}

bool QGIS_dev::onSaveProject()
{
    if (m_projectFilePath.isEmpty()) {
        return onSaveProjectAs();
    }
    else {
        // 设置项目路径，以便QGIS保存相对路径
        QgsProject::instance()->setFileName(QFileInfo(m_projectFilePath).path());
        if (QgsProject::instance()->write(m_projectFilePath)) {
            m_isProjectDirty = false;
            updateWindowTitle();
            OutputManager::instance()->logMessage("项目已保存: " + m_projectFilePath);
            return true;
        }
        else {
            QMessageBox::critical(this, "错误", "项目保存失败！");
            return false;
        }
    }
}

bool QGIS_dev::onSaveProjectAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, "项目另存为", m_projectFilePath.isEmpty() ? QDir::homePath() : m_projectFilePath, "QGIS Projects (*.qgz)");
    if (filePath.isEmpty()) return false;

    // 如果没有.qgz后缀，则添加
    if (!filePath.endsWith(".qgz", Qt::CaseInsensitive)) {
        filePath += ".qgz";
    }

    m_projectFilePath = filePath;
    return onSaveProject();
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

// QGIS_dev.cpp

void QGIS_dev::onAddVectorLayer()
{
    // ... (前面的项目保存检查逻辑不变) ...
    if (m_projectFilePath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先保存您的项目，然后才能添加图层。");
        if (!onSaveProjectAs()) {
            return;
        }
    }

    QString filter = "Shapefile (*.shp);;GeoPackage (*.gpkg);;All files (*.*)";
    QString sourceFilePath = QFileDialog::getOpenFileName(this, "选择一个矢量文件", QDir::homePath(), filter);

    if (sourceFilePath.isEmpty()) return;

    QString copiedFileRelativePath;
    // 复制文件到项目文件夹
    if (!copyLayerDataToProject(sourceFilePath, copiedFileRelativePath)) {
        OutputManager::instance()->logError("复制图层数据失败！");
        return;
    }

    // ================== 关键修改 ==================
    // 1. 获取复制后文件的【绝对路径】
    QDir projectDir(QFileInfo(m_projectFilePath).path());
    QString copiedFileAbsolutePath = projectDir.absoluteFilePath(copiedFileRelativePath);

    // 2. 使用【绝对路径】加载图层
    QString layerName = QFileInfo(copiedFileAbsolutePath).baseName();
    QgsVectorLayer* vectorLayer = new QgsVectorLayer(copiedFileAbsolutePath, layerName, "ogr");
    // ===============================================

    if (vectorLayer->isValid()) {
        OutputManager::instance()->logMessage("成功加载矢量图层: " + copiedFileAbsolutePath);
        QgsProject::instance()->addMapLayer(vectorLayer);
        // 您需要连接 QgsProject::layersAdded 信号来更新图层树，
        // 而不是在这里手动调用 m_customLayerTreeView->addLayer
        m_mapCanvas->zoomToLayer(vectorLayer);
        // onProjectDirty(); // 连接了 layersAdded 信号，这句可以省略，但保留也无害
    }
    else {
        // ================== 关键修改 ==================
        // 添加错误处理，这样才能看到失败的原因
        QString errorMsg = "加载矢量图层失败: " + copiedFileAbsolutePath;
        OutputManager::instance()->logError(errorMsg);
        OutputManager::instance()->logError("错误详情: " + vectorLayer->error().message());
        delete vectorLayer; // 释放内存
        // ===============================================
    }
}

void QGIS_dev::onAddRasterLayer()
{
    if (m_projectFilePath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先保存您的项目，然后才能添加图层。");
        if (!onSaveProjectAs()) return;
    }

    QString filter = "GeoTIFF (*.tif *.tiff);;Erdas Imagine (*.img);;All files (*.*)";
    QString sourceFilePath = QFileDialog::getOpenFileName(this, "选择一个栅格文件", QDir::homePath(), filter);

    if (sourceFilePath.isEmpty()) return;

    QString copiedFileRelativePath;
    if (!copyLayerDataToProject(sourceFilePath, copiedFileRelativePath)) {
        OutputManager::instance()->logError("复制栅格数据失败！");
        return;
    }

    // ================== 关键修改 ==================
    // 1. 获取复制后文件的【绝对路径】
    QDir projectDir(QFileInfo(m_projectFilePath).path());
    QString copiedFileAbsolutePath = projectDir.absoluteFilePath(copiedFileRelativePath);

    // 2. 使用【绝对路径】加载图层
    QString layerName = QFileInfo(copiedFileAbsolutePath).baseName();
    // 注意: QGIS 3.x 推荐用 "gdal" provider for rasters
    QgsRasterLayer* rasterLayer = new QgsRasterLayer(copiedFileAbsolutePath, layerName, "gdal");
    // ===============================================

    if (rasterLayer->isValid()) {
        OutputManager::instance()->logMessage("成功加载栅格图层: " + copiedFileAbsolutePath);
        QgsProject::instance()->addMapLayer(rasterLayer);
        m_mapCanvas->zoomToLayer(rasterLayer);
        // onProjectDirty(); // 同上，可省略
    }
    else {
        // ================== 关键修改 ==================
        // 添加错误处理
        QString errorMsg = "加载栅格图层失败: " + copiedFileAbsolutePath;
        OutputManager::instance()->logError(errorMsg);
        OutputManager::instance()->logError("错误详情: " + rasterLayer->error().message());
        delete rasterLayer;
        // ===============================================
    }
}

// ---- 辅助函数实现 ----

void QGIS_dev::onProjectDirty() {
    m_isProjectDirty = true;
    updateWindowTitle();
}

void QGIS_dev::updateWindowTitle() {
    QString title = m_projectFilePath.isEmpty() ? "未命名项目" : QFileInfo(m_projectFilePath).fileName();
    if (m_isProjectDirty) {
        title += "*";
    }
    title += " - My GIS App";
    setWindowTitle(title);
}

// 检查并提示保存
bool QGIS_dev::maybeSave() {
    if (!m_isProjectDirty) return true;

    const QMessageBox::StandardButton ret = QMessageBox::warning(this, "未保存的更改",
        "当前项目有未保存的更改，您想保存吗？",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        return onSaveProject();
    }
    else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true; // Discard
}

// 关闭事件
void QGIS_dev::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        event->accept();
    }
    else {
        event->ignore();
    }
}

// 重新构建图层树
void QGIS_dev::rebuildLayerTreeFromProject() {
    m_customLayerTreeView->clear();
    // QgsProject::mapLayers() 返回的是一个 map，值是图层指针
    // 我们需要按照图层树的顺序来添加
    QgsLayerTree* layerTree = QgsProject::instance()->layerTreeRoot();
    for (QgsLayerTreeNode* node : layerTree->children()) {
        if (node->nodeType() == QgsLayerTreeNode::NodeLayer) {
            QgsLayerTreeLayer* layerNode = static_cast<QgsLayerTreeLayer*>(node);
            m_customLayerTreeView->addLayer(layerNode->layer());
        }
    }
    // 确保画布也更新
    m_customLayerTreeView->updateMapCanvasLayers();
}

// 复制数据文件到项目目录
bool QGIS_dev::copyLayerDataToProject(const QString& sourceFilePath, QString& newRelativePath) {
    QDir projectDir(QFileInfo(m_projectFilePath).path());
    if (!projectDir.exists("data")) {
        projectDir.mkdir("data");
    }
    QDir dataDir(projectDir.filePath("data"));

    QString fileName = QFileInfo(sourceFilePath).fileName();
    QString destFilePath = dataDir.filePath(fileName);

    // 特殊处理 Shapefile
    if (sourceFilePath.endsWith(".shp", Qt::CaseInsensitive)) {
        if (!copyShapefile(sourceFilePath, dataDir.path(), fileName)) {
            return false;
        }
    }
    else { // 处理单个文件 (如.tif, .gpkg)
        if (QFile::exists(destFilePath)) {
            // 可选：询问用户是否覆盖
        }
        if (!QFile::copy(sourceFilePath, destFilePath)) {
            return false;
        }
    }

    newRelativePath = "data/" + fileName;
    return true;
}

// 专门用来复制Shapefile及其所有组件的函数
bool QGIS_dev::copyShapefile(const QString& sourceShpPath, const QString& destDir, QString& newFileName) {
    QString baseName = QFileInfo(sourceShpPath).completeBaseName();
    QDir sourceDir = QFileInfo(sourceShpPath).dir();

    QStringList extensions = { ".shp", ".shx", ".dbf", ".prj", ".cpg", ".sbn", ".sbx" };
    for (const QString& ext : extensions) {
        QString sourceFile = sourceDir.filePath(baseName + ext);
        if (QFile::exists(sourceFile)) {
            QString destFile = QDir(destDir).filePath(QFileInfo(sourceFile).fileName());
            if (QFile::exists(destFile)) QFile::remove(destFile); // 先删除以防万一
            if (!QFile::copy(sourceFile, destFile)) {
                OutputManager::instance()->logError("无法复制文件: " + sourceFile);
                return false;
            }
        }
    }
    newFileName = QFileInfo(sourceShpPath).fileName();
    return true;
}