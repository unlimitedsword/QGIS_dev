#include "CustomLayerTreeView.h"
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QVariant>
#include <QMenu>
#include <QColorDialog>
#include <qgsmaplayer.h>
#include <qgsvectorlayer.h>
#include <qgssinglesymbolrenderer.h>
#include <qgssymbol.h>
#include <qgsmapcanvas.h>
#include <QDebug>

const int LayerPtrRole = Qt::UserRole + 1;

CustomLayerTreeView::CustomLayerTreeView(QgsMapCanvas* canvas, QWidget* parent)
    : QWidget(parent), m_mapCanvas(canvas)
{
    m_treeView = new QTreeView(this);
    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);
    m_model->setHorizontalHeaderLabels({"Layers"});

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_model, &QStandardItemModel::itemChanged, this, &CustomLayerTreeView::onItemChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &CustomLayerTreeView::onCustomContextMenuRequested);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_treeView);
    setLayout(layout);
}

CustomLayerTreeView::~CustomLayerTreeView() {}

void CustomLayerTreeView::addLayer(QgsMapLayer* layer)
{
    if (!layer) return;

    QStandardItem* item = new QStandardItem(layer->name());
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);
    item->setData(QVariant::fromValue(static_cast<void*>(layer)), LayerPtrRole);

    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>(layer);
    if (vlayer) {
        QgsSingleSymbolRenderer* renderer = dynamic_cast<QgsSingleSymbolRenderer*>(vlayer->renderer());
        if (renderer) {
            updateLayerItemIcon(item, renderer->symbol()->color());
        }
    }
    
    m_model->appendRow(item);
    updateMapCanvasLayers();
}

// 当model的属性改变时立即同步
void CustomLayerTreeView::onItemChanged(QStandardItem* item)
{
    qDebug() << "Item changed:" << item->text() << "Check state:" << item->checkState();
    updateMapCanvasLayers();
}

// 右键菜单
void CustomLayerTreeView::onCustomContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QStandardItem* item = m_model->itemFromIndex(index);
    QgsMapLayer* layer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());

    QMenu contextMenu(this);

    // --- 修改颜色功能 (仅对矢量图层) ---
    if (qobject_cast<QgsVectorLayer*>(layer)) {
        QAction* changeColorAction = contextMenu.addAction("修改颜色...");
        connect(changeColorAction, &QAction::triggered, this, [=]() {
            this->onChangeLayerColor(layer);
            });
    }

    // --- !! 新增排序功能 !! ---
    contextMenu.addSeparator(); // 添加一个分隔线，让UI更清晰

    QAction* moveUpAction = contextMenu.addAction("上移一层");
    QAction* moveDownAction = contextMenu.addAction("下移一层");

    // 连接信号到新的槽函数，使用lambda传递索引
    connect(moveUpAction, &QAction::triggered, this, [=]() { this->onMoveLayerUp(index); });
    connect(moveDownAction, &QAction::triggered, this, [=]() { this->onMoveLayerDown(index); });

    // !! 关键的UX逻辑：根据图层位置决定动作是否可用 !!
    int currentRow = index.row();
    if (currentRow == 0) {
        // 如果是第一行，则“上移”不可用
        moveUpAction->setEnabled(false);
    }
    if (currentRow == m_model->rowCount() - 1) {
        // 如果是最后一行，则“下移”不可用
        moveDownAction->setEnabled(false);
    }

    contextMenu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

// !! 新增槽函数的完整实现 !!
void CustomLayerTreeView::onMoveLayerUp(const QModelIndex& index)
{
    if (!index.isValid()) return;

    int row = index.row();
    if (row > 0) { // 再次检查，确保不会移动到-1行
        // 1. 取出当前行
        QList<QStandardItem*> rowItems = m_model->takeRow(row);
        // 2. 将其插入到上一行
        m_model->insertRow(row - 1, rowItems);
        // 3. 更新画布的图层顺序
        updateMapCanvasLayers();
        // 4. (可选但推荐) 将选择焦点也移动到新的位置
        m_treeView->setCurrentIndex(m_model->index(row - 1, 0));
    }
}

// !! 新增槽函数的完整实现 !!
void CustomLayerTreeView::onMoveLayerDown(const QModelIndex& index)
{
    if (!index.isValid()) return;

    int row = index.row();
    if (row < m_model->rowCount() - 1) { // 再次检查，确保在有效范围内
        // 1. 取出当前行
        QList<QStandardItem*> rowItems = m_model->takeRow(row);
        // 2. 将其插入到下一行
        m_model->insertRow(row + 1, rowItems);
        // 3. 更新画布的图层顺序
        updateMapCanvasLayers();
        // 4. (可选但推荐) 将选择焦点也移动到新的位置
        m_treeView->setCurrentIndex(m_model->index(row + 1, 0));
    }
}

void CustomLayerTreeView::onChangeLayerColor(QgsMapLayer* layer)
{
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
    if (!vectorLayer) return;

    // !! 修正: 对非QObject的类使用 dynamic_cast !!
    QgsSingleSymbolRenderer* renderer = dynamic_cast<QgsSingleSymbolRenderer*>(vectorLayer->renderer());
    if (!renderer) {
        qDebug() << "Cannot change color: layer does not use a single symbol renderer.";
        return;
    }
    
    const QColor newColor = QColorDialog::getColor(renderer->symbol()->color(), this, "选择新颜色");

    if (newColor.isValid()) {
        renderer->symbol()->setColor(newColor);
        vectorLayer->triggerRepaint();

        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            QgsMapLayer* itemLayer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());
            if (itemLayer == layer) {
                updateLayerItemIcon(item, newColor);
                break;
            }
        }
    }
}

void CustomLayerTreeView::updateLayerItemIcon(QStandardItem* item, const QColor& color)
{
    if (!item) return;
    QPixmap pixmap(16, 16);
    pixmap.fill(color);
    item->setIcon(QIcon(pixmap));
}

void CustomLayerTreeView::updateMapCanvasLayers()
{
    QList<QgsMapLayer*> visibleLayers;
    for (int i = m_model->rowCount() - 1; i >= 0; --i)
    {
        QStandardItem* item = m_model->item(i);
        if (item && item->checkState() == Qt::Checked)
        {
            QVariant layerVariant = item->data(LayerPtrRole);
            if (layerVariant.isValid()) {
                QgsMapLayer* layer = static_cast<QgsMapLayer*>(layerVariant.value<void*>());
                visibleLayers.append(layer);
            }
        }
    }
    m_mapCanvas->setLayers(visibleLayers);
    m_mapCanvas->refresh();
    qDebug() << "Map canvas updated with" << visibleLayers.count() << "visible layers.";
}