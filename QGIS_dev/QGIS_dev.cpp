#include "QGIS_dev.h"
#include "Output_Manager.h"
#include "MapCanvas.h"
#include "OutputWidget.h"
#include "CustomLayerTreeView.h"
#include "FeatureSelectionTool.h"
#include "AnalysisToolboxWidget.h"

#include <QDesktopServices>
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
#include <QLabel>
#include <QStatusBar>
#include <QPushButton> // 添加此行以包含 QPushButton 的完整定义
#include <QProcess>
#include <QUrl>
#include <QUrlQuery>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h> // 需要包含栅格图层头文件
#include <qgslayertree.h>
#include <qgsproject.h>
#include <qgsmaptoolpan.h>
#include <qgsMapLayer.h>
#include <qgsapplication.h> // 添加此行以包含 QgsApplication 的完整定义
#include <qgsprojectionselectiondialog.h>

const int LayerPtrRole = Qt::UserRole + 1;


QGIS_dev::QGIS_dev(QWidget *parent)
    : QMainWindow(parent), m_isProjectDirty(false),m_currentLayer(nullptr) // 初始化项目状态
{
    // 设置图标
    setWindowIcon(QIcon(":/QGIS_dev/resource/images/QuickGIS.ico"));


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
            // 在rebuildLayerTreeFromProject中已经有这个逻辑了，
            // layersAdded 信号应该只用来触发UI更新，而不是直接添加
            m_customLayerTreeView->addLayer(layer);
        }
        // 关键修改：添加图层后，手动更新一次画布，确保显示
        m_customLayerTreeView->updateMapCanvasLayers();
        });
    connect(QgsProject::instance(), QOverload<const QStringList&>::of(&QgsProject::layersWillBeRemoved), this, [this]() {
        this->onProjectDirty();
        });

    // 关键：连接地图画布的信号到新槽函数
    QgsMapCanvas* canvas = m_mapCanvas->getCanvas();
    connect(m_mapCanvas->getCanvas(), &QgsMapCanvas::scaleChanged, this, &QGIS_dev::updateScale);
    connect(canvas, &QgsMapCanvas::xyCoordinates, this, &QGIS_dev::updateCoordinates);
    connect(canvas, &QgsMapCanvas::scaleChanged, this, &QGIS_dev::updateScale);

    // +++ 将图层树的选中事件连接到主窗口的槽 +++
    connect(m_customLayerTreeView, &CustomLayerTreeView::currentLayerChanged,
        this, &QGIS_dev::onCurrentLayerChanged);

    // ====================== 核心修改：项目CRS固定为WGS 84 ======================
    QgsCoordinateReferenceSystem wgs84("EPSG:4326");
    if (wgs84.isValid()) {
        QgsProject::instance()->setCrs(wgs84); // 程序启动时就设定项目CRS
        OutputManager::instance()->logMessage("Project CRS permanently set to WGS 84 (EPSG:4326).");
    }
    else {
        OutputManager::instance()->logError("CRITICAL: Failed to create WGS 84 CRS. Projections will likely fail.");
    }
    // 不再连接 QgsProject::crsChanged，因为不希望它改变
    // connect(QgsProject::instance(), &QgsProject::crsChanged, this, &QGIS_dev::updateProjectCrs);

    updateProjectCrs(); // 手动调用一次以设置标签文本
    // =========================================================================

    updateWindowTitle();

    updateScale(m_mapCanvas->getCanvas()->scale());
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
    m_customLayerTreeView->setMinimumWidth(300);

    m_outputWidget = new OutputWidget(this);

    setCentralWidget(m_mapCanvas);

    // 将customLayerTreeView 放入DockWidget
    m_layerTreeDock = new QDockWidget("图层 (自定义)", this);
    m_layerTreeDock->setWidget(m_customLayerTreeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_layerTreeDock);

    // 创建两个右侧Dock
    // a. 创建“空间分析”Dock，并放入自定义Widget
    m_analysisToolsWidget = new AnalysisToolboxWidget(this);

    m_analysisDock = new QDockWidget("空间分析工具", this);
    m_analysisDock->setWidget(m_analysisToolsWidget);

    // 创建“输出”Dock
    m_outputDock = new QDockWidget("输出", this);
    m_outputDock->setWidget(m_outputWidget);

    // 先添加上面的（分析），再添加下面的（输出）
    addDockWidget(Qt::RightDockWidgetArea, m_analysisDock);
    addDockWidget(Qt::RightDockWidgetArea, m_outputDock);

    // 将它们垂直排列
    splitDockWidget(m_analysisDock, m_outputDock, Qt::Vertical);

    m_analysisDock->setMinimumWidth(350);
    m_outputDock->setMinimumWidth(350);

    // 创建菜单栏
    m_fileMenu = menuBar()->addMenu("文件(&F)"); // &F 设置快捷键 Alt+F
    m_checkMenu = menuBar()->addMenu("查看(&C)");

    // 创建工具栏
    m_toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, m_toolBar);
    m_toolBar->setFloatable(false);       // 设置是否浮动
    m_toolBar->setMovable(false);         // 设置工具栏不允许移动

    // 新增：设置状态栏 
    QStatusBar* sb = statusBar(); // 获取主窗口的状态栏

    // 创建坐标标签
    m_coordsLabel = new QLabel("坐标: (移动鼠标查看)", this);
    m_coordsLabel->setMinimumWidth(250); // 设置一个最小宽度，防止跳动
    m_coordsLabel->setFrameShape(QFrame::StyledPanel);
    m_coordsLabel->setFrameShadow(QFrame::Sunken);

    // 创建比例尺标签
    m_scaleLabel = new QLabel("比例尺: N/A", this);
    m_scaleLabel->setMinimumWidth(150);
    m_scaleLabel->setFrameShape(QFrame::StyledPanel);
    m_scaleLabel->setFrameShadow(QFrame::Sunken);

    // 将标签作为永久部件添加到状态栏（这样它们就不会被临时消息覆盖）
    sb->addPermanentWidget(m_coordsLabel);
    sb->addPermanentWidget(m_scaleLabel);

    // ====================== 核心修改：CRS显示为固定标签 ======================
    m_crsLabel = new QLabel("EPSG:4326", this); // 直接显示固定文本
    m_crsLabel->setToolTip("项目坐标参考系固定为 WGS 84 (EPSG:4326)");
    m_crsLabel->setMinimumWidth(120); // 根据需要调整宽度
    m_crsLabel->setFrameShape(QFrame::StyledPanel);
    m_crsLabel->setFrameShadow(QFrame::Sunken);
    // 不再需要 connect clicked 信号
    sb->addPermanentWidget(m_crsLabel);
    // =======================================================================
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

    // ====================== 新增动作 ======================
   // 创建“添加分隔符文本图层”动作
    m_addDelimitedTextLayerAction = new QAction("添加分隔符文本图层...", this);
    m_addDelimitedTextLayerAction->setStatusTip("从CSV或其他文本文件加载点图层");
    connect(m_addDelimitedTextLayerAction, &QAction::triggered, this, &QGIS_dev::onAddDelimitedTextLayer);
    // ======================================================

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
    m_fileMenu->addAction(m_addDelimitedTextLayerAction);

    m_checkLogsAction = new QAction("日志", this);
    connect(m_checkLogsAction, &QAction::triggered, this, &QGIS_dev::onOpenLogFolder);
    m_checkMenu->addAction(m_checkLogsAction); // 假设你想加到“查看”菜单

    // === 新增视图菜单 ===
    QMenu* viewMenu = menuBar()->addMenu("视图(&V)");

    m_viewLayerManagerAction = new QAction("图层管理器", this);
    m_viewLayerManagerAction->setCheckable(true);
    m_viewLayerManagerAction->setChecked(m_layerTreeDock->isVisible());
    connect(m_viewLayerManagerAction, &QAction::toggled, this, [this](bool checked) {
        m_layerTreeDock->setVisible(checked);
        });

    m_viewToolBarAction = new QAction("工具栏", this);
    m_viewToolBarAction->setCheckable(true);
    m_viewToolBarAction->setChecked(m_toolBar->isVisible());
    connect(m_viewToolBarAction, &QAction::toggled, this, [this](bool checked) {
        m_toolBar->setVisible(checked);
        });

    m_viewOutputDockAction = new QAction("输出栏", this);
    m_viewOutputDockAction->setCheckable(true);
    m_viewOutputDockAction->setChecked(m_outputDock->isVisible());
    connect(m_viewOutputDockAction, &QAction::toggled, this, [this](bool checked) {
        m_outputDock->setVisible(checked);
        });

    m_viewAnalysisDockAction = new QAction("空间分析工具", this);
    m_viewAnalysisDockAction->setCheckable(true);
    m_viewAnalysisDockAction->setChecked(m_analysisDock->isVisible());
    connect(m_viewAnalysisDockAction, &QAction::toggled, this, [this](bool checked) {
        m_analysisDock->setVisible(checked);
        });

    viewMenu->addAction(m_viewToolBarAction);
    viewMenu->addAction(m_viewLayerManagerAction);
    viewMenu->addAction(m_viewOutputDockAction);
    viewMenu->addAction(m_viewAnalysisDockAction);

    // 监听dock/toolbar关闭事件，自动更新Action的勾选状态
    connect(m_analysisDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!this->isMinimized()) // 只在主窗口不是最小化时同步
            m_viewAnalysisDockAction->setChecked(visible);
        });
    connect(m_layerTreeDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!this->isMinimized())
            m_viewLayerManagerAction->setChecked(visible);
        });
    connect(m_outputDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!this->isMinimized())
            m_viewOutputDockAction->setChecked(visible);
        });
    connect(m_toolBar, &QToolBar::visibilityChanged, this, [this](bool visible) {
        if (!this->isMinimized())
            m_viewToolBarAction->setChecked(visible);
        });


    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");

    QAction* qgisApiAction = new QAction("QGIS API", this);
    connect(qgisApiAction, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://docs.qgis.org/3.40/en/docs/index.html"));
        });

    QAction* qt5Action = new QAction("Qt5", this);
    connect(qt5Action, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://doc.qt.io/archives/qt-5.15/qtassistant-index.html"));
        });

    helpMenu->addAction(qgisApiAction);
    helpMenu->addAction(qt5Action);

}

// ---- 新的项目管理槽函数 ----

// **修改 onNewProject 函数**
// QgsProject::clear() 也会发出 crsChanged 信号，所以也无需手动调用
void QGIS_dev::onNewProject()
{
    if (maybeSave()) {
        QgsProject::instance()->clear();

        QgsCoordinateReferenceSystem defaultCrs("EPSG:4326");
        if (defaultCrs.isValid()) {
            QgsProject::instance()->setCrs(defaultCrs);
        }

        m_customLayerTreeView->clear();
        m_projectFilePath.clear();
        m_isProjectDirty = false;
        updateWindowTitle();
        updateProjectCrs();
        OutputManager::instance()->logMessage("新项目已创建，项目CRS为WGS 84。");
    }
}

void QGIS_dev::onOpenProject()
{
    if (maybeSave()) {
        QString filePath = QFileDialog::getOpenFileName(this, "打开QGIS项目", QDir::homePath(), "QGIS Projects (*.qgz *.qgs)");
        if (filePath.isEmpty()) return;

        QgsProject::instance()->clear();
        m_customLayerTreeView->clear();

        if (QgsProject::instance()->read(filePath)) {
            m_projectFilePath = filePath;

            // **项目加载后，强制将项目CRS设回WGS 84**
            QgsCoordinateReferenceSystem wgs84("EPSG:4326");
            if (wgs84.isValid()) {
                QgsProject::instance()->setCrs(wgs84);
                OutputManager::instance()->logMessage("项目已打开，并将项目CRS设置为WGS 84。");
            }
            else {
                OutputManager::instance()->logError("打开项目后无法设置WGS 84 CRS。");
            }

            rebuildLayerTreeFromProject();
            m_isProjectDirty = false;
            updateWindowTitle();
            updateProjectCrs(); // 更新UI标签
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

void QGIS_dev::onOpenLogFolder()
{
    // 获取可执行文件所在目录
    QDir exeDir = QCoreApplication::applicationDirPath(); // 修改为 QDir 类型  

    QString logDir = exeDir.filePath("logs");

    QDir dir(logDir);
    if (!dir.exists()) {
        QMessageBox::warning(this, "提示", "日志文件夹不存在: " + logDir);
        return;
    }
#if defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe", { QDir::toNativeSeparators(logDir) });
#elif defined(Q_OS_MAC)
    QProcess::startDetached("open", { logDir });
#else // Linux/Unix
    QProcess::startDetached("xdg-open", { logDir });
#endif
}

// --- 在此设置所有的工具栏 ---
void QGIS_dev::setupToolBar() 
{   
    m_saveProjectAction = new QAction("保存", this);
    m_saveProjectAction->setStatusTip("保存");
    m_saveProjectAction->setIcon(QIcon(":/QGIS_dev/resource/images/保存.png"));
    connect(m_saveProjectAction, &QAction::triggered, this, &QGIS_dev::onSaveProject);  
    
    m_openProjectAction = new QAction("打开文件", this);
    m_openProjectAction->setStatusTip("打开文件");
    m_openProjectAction->setIcon(QIcon(":/QGIS_dev/resource/images/打开文件.png"));
    connect(m_openProjectAction, &QAction::triggered, this, &QGIS_dev::onOpenProject);

    addVectorTool = new QAction("矢量", this);
    addVectorTool->setStatusTip("添加矢量数据");
    addVectorTool->setIcon(QIcon(":/QGIS_dev/resource/images/矢量数据集.png"));
    connect(addVectorTool, &QAction::triggered, this, &QGIS_dev::onAddVectorLayer);    
    
    addRasterTool = new QAction("栅格", this);
    addRasterTool->setStatusTip("添加栅格数据");
    addRasterTool->setIcon(QIcon(":/QGIS_dev/resource/images/栅格影像.png"));
    connect(addRasterTool, &QAction::triggered, this, &QGIS_dev::onAddRasterLayer);



    //--- 放大缩小工具---
    m_zoomInAction = new QAction("放大", this);
    m_zoomInAction->setStatusTip("放大");
    m_zoomInAction->setIcon(QIcon(":/QGIS_dev/resource/images/放大.png"));
    connect(m_zoomInAction, &QAction::triggered, m_mapCanvas, &MapCanvas::zoomIn);

    m_zoomOutAction = new QAction("缩小", this);
    m_zoomOutAction->setStatusTip("缩小");
    m_zoomOutAction->setIcon(QIcon(":/QGIS_dev/resource/images/缩小.png"));
    connect(m_zoomOutAction, &QAction::triggered, m_mapCanvas, &MapCanvas::zoomOut);

    // --- 地图工具 ---
    m_selectionTool = new FeatureSelectionTool(m_mapCanvas->getCanvas());

    m_panAction = new QAction("平移", this);
    m_panAction->setIcon(QIcon(":/QGIS_dev/resource/images/平移.png"));
    m_panAction->setCheckable(true); // 工具按钮是状态性的，所以设为可选中
    m_panAction->setStatusTip("平移地图");
    connect(m_panAction, &QAction::triggered, this, &QGIS_dev::onActivatePanTool);

    m_selectAction = new QAction("选择要素", this);
    m_selectAction->setIcon(QIcon(":/QGIS_dev/resource/images/选择.png"));
    m_selectAction->setCheckable(true);
    m_selectAction->setStatusTip("点击以选择要素并查看信息");
    connect(m_selectAction, &QAction::triggered, this, &QGIS_dev::onActivateSelectTool);

    // 3. 使用 QActionGroup 确保同一时间只有一个工具被激活
    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->addAction(m_panAction);
    m_toolActionGroup->addAction(m_selectAction);


    m_toolBar->addAction(m_saveProjectAction);
    m_toolBar->addAction(m_openProjectAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(addVectorTool);
    m_toolBar->addAction(addRasterTool);
    m_toolBar->addSeparator();
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
        if (vectorLayer->isValid()) {
            QgsProject::instance()->addMapLayer(vectorLayer);
            //updateProjectCrs();
            //m_mapCanvas->zoomToLayer(vectorLayer);
            m_mapCanvas->getCanvas()->zoomToFullExtent();
            OutputManager::instance()->logMessage("成功加载矢量图层: " + copiedFileAbsolutePath);
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
        QString errorString = QFile().errorString(); // 获取静态方法的返回值
        OutputManager::instance()->logError(
            QString("复制文件失败: %1 -> %2, 错误: %3")
            .arg(sourceFilePath)
            .arg(copiedFileRelativePath)
            .arg(errorString)
        );
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
        QgsProject::instance()->addMapLayer(rasterLayer);
        updateProjectCrs();
        //m_mapCanvas->zoomToLayer(rasterLayer);
        m_mapCanvas->getCanvas()->zoomToFullExtent();
        OutputManager::instance()->logMessage("成功加载栅格图层: " + copiedFileAbsolutePath);
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
    title += " - QuickGIS";
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
    OutputManager::instance()->logMessage(QString("Attempting to copy layer data from: %1").arg(sourceFilePath));

    // 1. 检查源文件是否存在且可读
    QFileInfo sourceInfo(sourceFilePath);
    if (!sourceInfo.exists()) {
        OutputManager::instance()->logError("Source file does not exist: " + sourceFilePath);
        return false;
    }
    if (!sourceInfo.isReadable()) {
        OutputManager::instance()->logError("Source file is not readable: " + sourceFilePath);
        return false;
    }

    // 2. 构建目标目录路径
    QDir projectDir(QFileInfo(m_projectFilePath).path());
    OutputManager::instance()->logMessage("Project base directory: " + projectDir.absolutePath());

    if (!projectDir.exists("data")) {
        OutputManager::instance()->logMessage("Data directory does not exist, attempting to create...");
        if (projectDir.mkdir("data")) {
            OutputManager::instance()->logMessage("Data directory created successfully: " + projectDir.filePath("data"));
        }
        else {
            OutputManager::instance()->logError("Failed to create data directory: " + projectDir.filePath("data"));
            return false;
        }
    }
    QDir dataDir(projectDir.filePath("data"));
    if (!dataDir.exists()) { // 双重检查
        OutputManager::instance()->logError("Data directory still does not exist after attempting creation: " + dataDir.absolutePath());
        return false;
    }
    OutputManager::instance()->logMessage("Target data directory: " + dataDir.absolutePath());


    // 3. 构建目标文件路径
    QString fileName = sourceInfo.fileName();
    QString destFilePath = dataDir.filePath(fileName);
    OutputManager::instance()->logMessage(QString("Target destination file path: %1").arg(destFilePath));

    // 4. 如果目标文件已存在，先尝试删除（或者您可以选择不覆盖，直接返回错误/成功）
    if (QFile::exists(destFilePath)) {
        OutputManager::instance()->logWarning("Destination file already exists: " + destFilePath + ". Attempting to remove it.");
        if (!QFile::remove(destFilePath)) {
            OutputManager::instance()->logError("Failed to remove existing destination file: " + destFilePath + ". QFile::copy might fail.");
            // 根据您的策略，这里可以选择返回false，或者继续尝试覆盖
            // return false; 
        }
        else {
            OutputManager::instance()->logMessage("Successfully removed existing destination file.");
        }
    }

    // 5. 执行复制
    bool copySuccess = false;
    if (sourceFilePath.endsWith(".shp", Qt::CaseInsensitive)) {
        // 对于Shapefile，文件名参数应该是新的文件名（不含路径）
        QString justFileNameForShapefile = fileName;
        copySuccess = copyShapefile(sourceFilePath, dataDir.path(), justFileNameForShapefile);
        if (copySuccess) fileName = justFileNameForShapefile; // 如果copyShapefile可能修改了文件名
    }
    else {
        OutputManager::instance()->logMessage(QString("Attempting QFile::copy from '%1' to '%2'").arg(sourceFilePath).arg(destFilePath));
        if (QFile::copy(sourceFilePath, destFilePath)) {
            copySuccess = true;
            OutputManager::instance()->logMessage("QFile::copy successful.");
        }
        else {
            // 获取更详细的错误信息
            QFile tempFile(sourceFilePath); // 用一个临时QFile对象来访问errorString
            OutputManager::instance()->logError(QString("QFile::copy FAILED. Source: '%1', Dest: '%2'. Error: %3 (Code: %4)")
                .arg(sourceFilePath).arg(destFilePath)
                .arg(tempFile.errorString()).arg(tempFile.error()));
        }
    }

    if (copySuccess) {
        newRelativePath = "data/" + fileName; // 相对路径是相对于项目文件的
        OutputManager::instance()->logMessage("File copy successful. New relative path: " + newRelativePath);
        return true;
    }
    else {
        OutputManager::instance()->logError("File copy failed for: " + sourceFilePath);
        return false;
    }
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

void QGIS_dev::onCurrentLayerChanged(QgsMapLayer* layer)
{
    m_currentLayer = layer;
    // 不再需要在这里更新任何状态栏的标签。
    // 状态栏的更新应该由它们各自的事件驱动。
    // 比如坐标由鼠标移动事件驱动，比例尺由画布缩放事件驱动，CRS由项目CRS改变事件驱动。
}


// 修正 updateCoordinates，去掉对 m_currentLayer 的依赖
void QGIS_dev::updateCoordinates(const QgsPointXY& point)
{
    // 这个函数之前的版本已经是正确的了，我们保持它
    try {
        const QgsCoordinateReferenceSystem destCrs = m_mapCanvas->getCanvas()->mapSettings().destinationCrs();

        if (!destCrs.isValid()) {
            m_coordsLabel->setText("坐标: (未知坐标系)");
            return;
        }

        m_coordsLabel->setText(QString::asprintf("坐标: %.4f, %.4f", point.x(), point.y()));
    }
    catch (const std::exception& e) {
        // ... (异常处理不变) ...
    }
}


// **核心修正：彻底解耦 updateScale 与 m_currentLayer**
void QGIS_dev::updateScale(double scale)
{
    // **删除 if (!m_currentLayer) 的判断**

    if (scale <= 0) {
        m_scaleLabel->setText("比例尺: N/A");
        return;
    }

    // 直接更新比例尺标签
    m_scaleLabel->setText(QString("比例尺 1:%1").arg(static_cast<long>(scale)));
}


// updateProjectCrs 函数实现修改
void QGIS_dev::updateProjectCrs()
{
    // 项目CRS永远是WGS 84
    QgsCoordinateReferenceSystem wgs84("EPSG:4326");
    if (m_crsLabel && wgs84.isValid()) { // 确保m_crsLabel已创建
        m_crsLabel->setText(wgs84.authid()); // 例如 "EPSG:4326"
        m_crsLabel->setToolTip(QString("项目坐标参考系: %1").arg(wgs84.description()));
    }
    else if (m_crsLabel) {
        m_crsLabel->setText("CRS错误");
        m_crsLabel->setToolTip("无法设置WGS 84作为项目CRS。");
    }
}



void QGIS_dev::onAddDelimitedTextLayer()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        "选择一个分隔符文本文件",
        QDir::homePath(),
        "CSV (逗号分隔) (*.csv);;文本文件 (*.txt);;所有文件 (*.*)");

    if (filePath.isEmpty()) {
        return;
    }

    // ====================== 核心加固 ======================

    // 1. 确保文件路径使用正斜杠
    QString forwardSlashPath = filePath;
    forwardSlashPath.replace('\\', '/');

    // 2. 使用 QUrl 来安全地构造URI，它会自动处理特殊字符和协议前缀
    QUrl fileUrl = QUrl::fromLocalFile(forwardSlashPath);

    // 3. 构建参数部分
    QUrlQuery query;
    query.addQueryItem("encoding", "UTF-8");
    query.addQueryItem("delimiter", ",");
    query.addQueryItem("xField", "lon");
    query.addQueryItem("yField", "lat");
    query.addQueryItem("crs", "epsg:4326");
    query.addQueryItem("spatialIndex", "yes");
    query.addQueryItem("subsetIndex", "no");
    query.addQueryItem("watchFile", "no");

    // 4. 将参数附加到URL上
    fileUrl.setQuery(query);

    // 5. 从 QUrl 获取最终的、格式正确的URI字符串
    QString uri = fileUrl.toString();

    OutputManager::instance()->logMessage("构造的URI: " + uri); // 打印URI到日志，方便调试
    // =======================================================

    QString layerName = QFileInfo(filePath).baseName();
    QgsVectorLayer* layer = new QgsVectorLayer(uri, layerName, "delimitedtext");

    if (!layer || !layer->isValid()) {
        // ... (错误信息对话框不变) ...
        // +++ 增加更详细的错误日志 +++
        OutputManager::instance()->logError("加载分隔符文本图层失败。");
        OutputManager::instance()->logError("Provider 错误: " + layer->error().message());
        delete layer;
        return;
    }

    QgsProject::instance()->addMapLayer(layer);
    m_mapCanvas->getCanvas()->zoomToFullExtent();
    OutputManager::instance()->logMessage("成功加载分隔符文本图层: " + filePath);
}