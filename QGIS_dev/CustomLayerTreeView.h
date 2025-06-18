#include <QWidget>
#include <QPoint>

// 前向声明
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QgsMapLayer;
class QgsMapCanvas;
class QModelIndex;
class QItemSelection;

class CustomLayerTreeView : public QWidget
{
    Q_OBJECT

public:
    explicit CustomLayerTreeView(QgsMapCanvas* canvas, QWidget* parent = nullptr);
    ~CustomLayerTreeView();
    void updateMapCanvasLayers();

signals:
    void modelChanged(); // 新增信号
    // +++ 新增信号，当选中项改变时发出 +++
    void currentLayerChanged(QgsMapLayer* layer);

public slots:
    void addLayer(QgsMapLayer* layer);
    void clear();

    // +++ 新增槽函数，响应QTreeView自身的选中事件 +++
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private slots:
    void onItemChanged(QStandardItem* item);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onChangeLayerColor(QgsMapLayer* layer);

    // -- !! 新增的私有槽函数，用于处理排序 !! --
    void onMoveLayerUp(const QModelIndex& index);
    void onMoveLayerDown(const QModelIndex& index);

    // 置于顶层/底层
    void onMoveToTop(const QModelIndex& index);
    void onMoveToBottom(const QModelIndex& index);


private:
    void updateLayerItemIcon(QStandardItem* item, const QColor& color);
    void onRemoveLayer(const QModelIndex& index);

private:
    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QgsMapCanvas* m_mapCanvas;
};

