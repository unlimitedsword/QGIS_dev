#include <QtWidgets/QMainWindow>
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

class QGIS_dev : public QMainWindow
{
    Q_OBJECT

public:
    QGIS_dev(QWidget *parent = nullptr);
    ~QGIS_dev();

private slots:
    // -- 将要实现的槽函数 --
    void onAddVectorLayer();
    void onAddRasterLayer();

    // --- 地图工具激活槽函数 ---
    void onActivatePanTool();
    void onActivateSelectTool();

private:
    // 初始化函数
    void setupUI();
    void setupActions();
    void setupToolBar();

    // UI 组件
    MapCanvas* m_mapCanvas;
    CustomLayerTreeView* m_customLayerTreeView;
    OutputWidget* m_outputWidget;

    // Dock 窗口
    QDockWidget* m_layerTreeDock;
    QDockWidget* m_outputDock;

    // 菜单和菜单动作
    QMenu* m_fileMenu;
    QAction* m_addVectorAction;
    QAction* m_addRasterAction;

    // --- 工具栏和工具动作 ---
    QToolBar* m_toolBar;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_panAction;           // 平移动作
    QAction* m_selectAction;        // 选择要素动作
    QActionGroup* m_toolActionGroup; // 工具动作组

    // --- 地图工具实例 ---
    FeatureSelectionTool* m_selectionTool;
};
