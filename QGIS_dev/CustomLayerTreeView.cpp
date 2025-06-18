#include "CustomLayerTreeView.h"
#include "Output_Manager.h" // 引入日志管理器
#include "AttributeTableDialog.h"
#include "RasterLayerPropertiesDialog.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QVariant>
#include <QMenu>
#include <QColorDialog>
#include <QItemSelection>
#include <qgsmaplayer.h>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <qgssinglesymbolrenderer.h>
#include <qgssymbol.h>
#include <qgsmapcanvas.h>
#include <QDebug>
#include <QMessageBox>
#include <qgsproject.h> // 需要它来移除图层

const int LayerPtrRole = Qt::UserRole + 1;
CustomLayerTreeView::CustomLayerTreeView(QgsMapCanvas* canvas, QWidget* parent)
    : QWidget(parent), m_mapCanvas(canvas)
{
    m_treeView = new QTreeView(this);
    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);
    m_model->setHorizontalHeaderLabels({ "Layers" });
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_model, &QStandardItemModel::itemChanged, this, &CustomLayerTreeView::onItemChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &CustomLayerTreeView::onCustomContextMenuRequested);
    // +++ 连接QTreeView的selectionChanged信号到我们的新槽函数 +++
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &CustomLayerTreeView::onSelectionChanged);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_treeView);
    setLayout(layout);

}
CustomLayerTreeView::~CustomLayerTreeView()
{
}
void CustomLayerTreeView::addLayer(QgsMapLayer* layer)
{
    if (!layer) return;
    QStandardItem* item = new QStandardItem(layer->name());
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);
    item->setData(QVariant::fromValue(static_cast<void*>(layer)), LayerPtrRole);

    item->setEditable(true);// 设置为可编辑

    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>(layer);
    if (vlayer) {
        QgsSingleSymbolRenderer* renderer = dynamic_cast<QgsSingleSymbolRenderer*>(vlayer->renderer());
        if (renderer) {
            updateLayerItemIcon(item, renderer->symbol()->color());
        }
    }

    // 将新图层插入到模型的第0行
    m_model->insertRow(0, item);
}
// 当model的属性改变时立即同步
void CustomLayerTreeView::onItemChanged(QStandardItem* item)
{
    if (!item) return;
    // 获取关联的图层
    QgsMapLayer* layer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());
    if (!layer) return;

    // ==================== 关键修改 2: 处理重命名 ====================
    // 检查是否是名称发生了变化
    QString newName = item->text();
    if (layer->name() != newName) {
        QString oldName = layer->name();
        layer->setName(newName); // <<< 核心：更新QGIS图层对象的名称
        OutputManager::instance()->logMessage(QString("图层 '%1' 已重命名为 '%2'").arg(oldName).arg(newName));
        emit modelChanged(); // <<< 核心：发出信号，通知主窗口项目已变“脏”
        return; // 处理完重命名后直接返回，避免下面的逻辑重复执行
    }
    // ================================================================

    // 处理可见性变化（原有的逻辑）
    qDebug() << "Item changed:" << item->text() << "Check state:" << item->checkState();
    updateMapCanvasLayers();
    emit modelChanged(); // 可见性变化也应该标记为“脏”
}


// 右键菜单
void CustomLayerTreeView::onCustomContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item) return;
    QgsMapLayer* layer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());
    if (!layer) return;

    QMenu contextMenu(this);
    QAction* action = nullptr; // 用于接收创建的QAction指针

    // --- 1. 重命名 (通用) ---
    action = contextMenu.addAction("重命名");
    connect(action, &QAction::triggered, this, [=]() {
        m_treeView->edit(index);
        });

    // --- 2. 修改颜色 (仅矢量) ---
    if (auto vlayer = qobject_cast<QgsVectorLayer*>(layer)) {
        action = contextMenu.addAction("修改颜色...");
        connect(action, &QAction::triggered, this, [=]() {
            this->onChangeLayerColor(layer); // 内部会再次cast，安全
            });
    }
    contextMenu.addSeparator(); // 在编辑属性和移动顺序之间加分隔符

    // --- 3. 排序功能 (通用) ---
    action = contextMenu.addAction("上移一层");
    connect(action, &QAction::triggered, this, [=]() { this->onMoveLayerUp(index); });
    action->setEnabled(index.row() > 0);

    action = contextMenu.addAction("下移一层");
    connect(action, &QAction::triggered, this, [=]() { this->onMoveLayerDown(index); });
    action->setEnabled(index.row() < m_model->rowCount() - 1);

    contextMenu.addSeparator(); // 一层移动和顶/底移动之间加分隔符

    action = contextMenu.addAction("置于顶层");
    connect(action, &QAction::triggered, this, [=]() { this->onMoveToTop(index); });
    action->setEnabled(index.row() > 0);

    action = contextMenu.addAction("置于底层");
    connect(action, &QAction::triggered, this, [=]() { this->onMoveToBottom(index); });
    action->setEnabled(index.row() < m_model->rowCount() - 1);

    contextMenu.addSeparator(); // 在排序和打开属性/删除之间加分隔符

    // --- 4. 打开特定属性对话框 (根据类型) ---
    if (auto vlayer = qobject_cast<QgsVectorLayer*>(layer)) {
        action = contextMenu.addAction("打开属性表");
        connect(action, &QAction::triggered, this, [=]() {
            AttributeTableDialog* dialog = new AttributeTableDialog(vlayer, m_mapCanvas, this->window());
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            });
    }
    else if (auto rlayer = qobject_cast<QgsRasterLayer*>(layer)) {
        action = contextMenu.addAction("属性...");
        connect(action, &QAction::triggered, this, [=]() {
            RasterLayerPropertiesDialog* dialog = new RasterLayerPropertiesDialog(rlayer, this->window());
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            });
    }
    // (未来可以为其他图层类型添加 else if 分支)

    // 只有在确实添加了“打开属性表”或“属性...”后才加分隔符
    if (contextMenu.actions().size() > 0 && !contextMenu.actions().last()->isSeparator()) {
        contextMenu.addSeparator();
    }


    // --- 5. 删除图层 (通用，通常放最后) ---
    action = contextMenu.addAction("删除图层");
    connect(action, &QAction::triggered, this, [=]() { this->onRemoveLayer(index); });

    contextMenu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void CustomLayerTreeView::onMoveLayerUp(const QModelIndex& index)
{
    if (!index.isValid()) return;
    int row = index.row();
    if (row > 0) {
        QList<QStandardItem*> rowItems = m_model->takeRow(row);
        m_model->insertRow(row - 1, rowItems);
        updateMapCanvasLayers();
        m_treeView->setCurrentIndex(m_model->index(row - 1, 0));
        emit modelChanged(); // <<< 发出信号
    }
}
void CustomLayerTreeView::onMoveLayerDown(const QModelIndex& index)
{
    if (!index.isValid()) return;
    int row = index.row();
    if (row < m_model->rowCount() - 1) {
        QList<QStandardItem*> rowItems = m_model->takeRow(row);
        m_model->insertRow(row + 1, rowItems);
        updateMapCanvasLayers();
        m_treeView->setCurrentIndex(m_model->index(row + 1, 0));
        emit modelChanged(); // <<< 发出信号
    }
}

void CustomLayerTreeView::onMoveToTop(const QModelIndex& index)
{
    if (!index.isValid() || index.row() == 0) return; // 如果已经是顶层，则不操作

    // 从当前位置取出该行
    QList<QStandardItem*> rowItems = m_model->takeRow(index.row());
    // 将其插入到模型的第0行 (UI的顶层)
    m_model->insertRow(0, rowItems);

    updateMapCanvasLayers(); // 根据新的UI顺序刷新画布
    m_treeView->setCurrentIndex(m_model->index(0, 0)); // 更新选中项
    emit modelChanged();
}

void CustomLayerTreeView::onMoveToBottom(const QModelIndex& index)
{
    if (!index.isValid() || index.row() == m_model->rowCount() - 1) return; // 如果已经是底层，则不操作

    // 从当前位置取出该行
    QList<QStandardItem*> rowItems = m_model->takeRow(index.row());
    // 将其追加到模型的末尾 (UI的底层)
    m_model->appendRow(rowItems);

    updateMapCanvasLayers(); // 根据新的UI顺序刷新画布
    m_treeView->setCurrentIndex(m_model->index(m_model->rowCount() - 1, 0)); // 更新选中项
    emit modelChanged();
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
                emit modelChanged();
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
// --- !! 新增的槽函数实现 !! ---
void CustomLayerTreeView::onRemoveLayer(const QModelIndex& index)
{
    if (!index.isValid()) return;
    // 1. 获取图层和Item
    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item) return;

    QgsMapLayer* layer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());
    if (!layer) return;

    // 2. 弹出确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", QString("确定要删除图层 '%1' 吗？此操作无法撤销。").arg(layer->name()),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }

    // 3. 从 QGIS 项目中移除图层。
    //    这一步是核心，它会触发信号，让QGIS框架负责清理图层对象。
    QgsProject::instance()->removeMapLayer(layer->id());

    // 4. 从我们的视图模型中移除对应的行。
    //    此时，layer 指针已经是一个“悬垂指针”，QGIS很快会删除它。
    //    我们绝对不能再使用它，尤其是不能 delete 它。
    m_model->removeRow(index.row());

    // 5. 更新画布
    //    在 QGIS 3.x 中，removeMapLayer 信号通常会连接到画布的更新槽，
    //    所以这一步可能不是必需的，但为了保险起见，可以保留。
    updateMapCanvasLayers();
    m_mapCanvas->refresh();

}


void CustomLayerTreeView::updateMapCanvasLayers()
{
    QList<QgsMapLayer*> layersToRender;

    // 从UI模型的顶部 (index 0, 用户看到的顶层) 开始遍历
    for (int uiRow = 0; uiRow < m_model->rowCount(); ++uiRow)
    {
        QStandardItem* item = m_model->item(uiRow);
        if (item && item->checkState() == Qt::Checked)
        {
            QVariant layerVariant = item->data(LayerPtrRole);
            if (layerVariant.isValid()) {
                QgsMapLayer* layer = static_cast<QgsMapLayer*>(layerVariant.value<void*>());
                // 将UI列表中的图层，按顺序【追加】到渲染列表的【末尾】
                layersToRender.append(layer);
            }
        }
    }

    m_mapCanvas->setLayers(layersToRender);
    qDebug() << "Map canvas updated. UI Top (" << (m_model->rowCount() > 0 ? m_model->item(0)->text() : "N/A")
        << ") is Map Top. Render order (bottom to top):";
    for (QgsMapLayer* lyr : layersToRender) {
        qDebug() << "  - " << lyr->name();
    }
}



void CustomLayerTreeView::clear()
{
    m_model->clear();
    m_model->setHorizontalHeaderLabels({ "Layers" });
    updateMapCanvasLayers(); // 清空画布
}
// +++ 实现新槽函数的逻辑 +++
void CustomLayerTreeView::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected); // 不关心之前选中的是什么

    QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) {
        // 如果没有选中任何项（例如，清空图层后），发出一个nullptr信号
        emit currentLayerChanged(nullptr);
        return;
    }

    // 通常我们只关心第一个被选中的项
    QModelIndex currentIndex = indexes.first();
    QStandardItem* item = m_model->itemFromIndex(currentIndex);
    if (item) {
        QgsMapLayer* layer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());
        // 发出信号，将当前选中的图层指针传递出去
        emit currentLayerChanged(layer);
    }

}