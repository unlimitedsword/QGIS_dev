#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <QToolBar>


class MapCanvas;
class CustomLayerTreeView;
class OutputWidget;
class QgsMapTool;
class FeatureSelectionTool; // 我们自定义的选择工具
class QActionGroup;       // 用于确保工具按钮的互斥性
class QLabel;
class QgsPointXY;
class QgsMapLayer;
class QgsCoordinateReferenceSystem;
class QPushButton;

class QGIS_dev : public QMainWindow
{
    Q_OBJECT

public:
    QGIS_dev(QWidget *parent = nullptr);
    ~QGIS_dev();

protected:
    // 添加关闭事件处理，用于检查未保存的更改
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 文件操作
    void onNewProject();
    void onOpenProject();
    bool onSaveProject();
    bool onSaveProjectAs();

    // 查看操作
    void onOpenLogFolder();

    // 内部槽函数，用于标记项目为已修改
    void onProjectDirty();

    // -- 将要实现的槽函数 --
    void onAddVectorLayer();
    void onAddRasterLayer();

    // --- 地图工具激活槽函数 ---
    void onActivatePanTool();
    void onActivateSelectTool();

    // +++ 新增的槽函数，用于更新状态栏 +++
    void updateCoordinates(const QgsPointXY& point);
    void updateScale(double scale);

    // 选择图层发生变化时接收信号
    void onCurrentLayerChanged(QgsMapLayer* layer);

    // +++ 新增槽函数 +++
    void updateProjectCrs(); // 用于更新状态栏的CRS显示

    // +++ 新增槽函数，用于响应CRS按钮的点击 +++
    void onChangeProjectCrs();

    // +++ 新增槽函数 +++
    void onAddDelimitedTextLayer();

private:
    // 初始化函数
    void setupUI();
    void setupActions();
    void setupToolBar();

    // 构件工程的辅助函数
    void updateWindowTitle();
    void rebuildLayerTreeFromProject(); // 从QgsProject重建图层树
    bool maybeSave(); // 检查是否需要保存，并执行保存逻辑
    bool copyLayerDataToProject(const QString& sourceFilePath, QString& newRelativePath); // 复制数据文件
    bool copyShapefile(const QString& sourceShpPath, const QString& destDir, QString& newFileName); // 专门处理Shapefile

    // UI 组件
    MapCanvas* m_mapCanvas;
    CustomLayerTreeView* m_customLayerTreeView;
    OutputWidget* m_outputWidget;

    // ... Dock 窗口 ...
    QDockWidget* m_layerTreeDock;
    QDockWidget* m_outputDock; // 我们保留输出Dock

    // +++ 新增空间分析工具的Dock和占位符Widget +++
    QDockWidget* m_analysisDock;
    QWidget* m_analysisToolsWidget;


    // 菜单和菜单动作
    QMenu* m_fileMenu;
    QAction* m_addVectorAction;
    QAction* m_addRasterAction;
    // +++ 新增菜单动作 +++
    QAction* m_addDelimitedTextLayerAction;
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;

    QMenu* m_checkMenu;
    QAction* m_checkLogsAction;

    // --- 工具栏和工具动作 ---
    QToolBar* m_toolBar;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_panAction;           // 平移动作
    QAction* m_selectAction;        // 选择要素动作
    QActionGroup* m_toolActionGroup; // 工具动作组

    // --- 地图工具实例 ---
    FeatureSelectionTool* m_selectionTool;

    // --- 项目状态 ---
    QString m_projectFilePath;
    bool m_isProjectDirty;

    // +++ 新增的成员变量，用于状态栏显示 +++
    QLabel* m_coordsLabel;
    QLabel* m_scaleLabel;

    // +++ 将 m_crsLabel 的类型从 QLabel* 修改为 QPushButton* +++
    bool resolveLayerCrs(QgsMapLayer* layer);
    QPushButton* m_crsButton;
    QgsMapLayer* m_currentLayer;
};
