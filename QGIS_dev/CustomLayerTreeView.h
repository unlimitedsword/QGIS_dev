#include <QWidget>
#include <QPoint>

// 前向声明
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QgsMapLayer;
class QgsMapCanvas;
class QModelIndex;

class CustomLayerTreeView : public QWidget
{
    Q_OBJECT

public:
    explicit CustomLayerTreeView(QgsMapCanvas* canvas, QWidget* parent = nullptr);
    ~CustomLayerTreeView();

public slots:
    void addLayer(QgsMapLayer* layer);

private slots:
    void onItemChanged(QStandardItem* item);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onChangeLayerColor(QgsMapLayer* layer);

    // -- !! 新增的私有槽函数，用于处理排序 !! --
    void onMoveLayerUp(const QModelIndex& index);
    void onMoveLayerDown(const QModelIndex& index);

private:
    void updateMapCanvasLayers();
    void updateLayerItemIcon(QStandardItem* item, const QColor& color);
    void onRemoveLayer(const QModelIndex& index);

private:
    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QgsMapCanvas* m_mapCanvas;
};

