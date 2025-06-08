#include <QtWidgets/QMainWindow>
#include "MapCanvas.h"
#include "OutputWidget.h"
#include "CustomLayerTreeView.h"
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>

class QGIS_dev : public QMainWindow
{
    Q_OBJECT

public:
    QGIS_dev(QWidget *parent = nullptr);
    ~QGIS_dev();

private:
    void setupUI();      // 用于创建UI元素的辅助函数
    void setupActions(); // 用于创建和连接Action的辅助函数
    void setupToolBar(); // 用于创建工具栏

private slots:
    // -- 我们将要实现的槽函数 --
    void onAddVectorLayer();
    void onAddRasterLayer();

private:
    // -- 原有成员 --
    MapCanvas* m_mapCanvas;
    OutputWidget* m_outputWidget;
    CustomLayerTreeView* m_customLayerTreeView; // 使用新的成员
    QDockWidget* m_layerTreeDock;
    QDockWidget* m_outputDock;

    // -- 菜单 --
    QMenu* m_fileMenu;
    QAction* m_addVectorAction;
    QAction* m_addRasterAction;

    // -- 工具栏 --
    QToolBar* m_toolBar;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
};
