#include "CustomLayerTreeView.h"
#include "Output_Manager.h" // 引入日志管理器
#include "AttributeTableDialog.h"
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QVariant>
#include <QMenu>
#include <QColorDialog>
#include <QItemSelection>
#include <qgsmaplayer.h>
#include <qgsvectorlayer.h>
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
    m_model->setHorizontalHeaderLabels({"Layers"});

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
    
    m_model->appendRow(item);
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
    if (!item) return; // 增加安全检查
    QgsMapLayer* layer = static_cast<QgsMapLayer*>(item->data(LayerPtrRole).value<void*>());
    if (!layer) return; // 增加安全检查


    QMenu contextMenu(this);

    // 只有矢量图层才有属性表
    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>(layer);
    if (vlayer) {
        QAction* openAttributeTableAction = contextMenu.addAction("打开属性表");
        connect(openAttributeTableAction, &QAction::triggered, this, [=]() {
            // ====================== 核心修改 ======================
            // 调用新的构造函数，传入 vlayer 和 m_mapCanvas
            AttributeTableDialog* dialog = new AttributeTableDialog(vlayer, m_mapCanvas, this->window());
            // ========================================================
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            });
        contextMenu.addSeparator();
    }

    // 添加重命名动作
    QAction* renameAction = contextMenu.addAction("重命名");
    connect(renameAction, &QAction::triggered, this, [=]() {
        m_treeView->edit(index); // <<< 核心：以编程方式启动编辑
        });
    contextMenu.addSeparator();

    // --- 修改颜色功能 (仅对矢量图层) ---
    if (qobject_cast<QgsVectorLayer*>(layer)) {
        QAction* changeColorAction = contextMenu.addAction("修改颜色...");
        connect(changeColorAction, &QAction::triggered, this, [=]() {
            this->onChangeLayerColor(layer);
            });
    }

    // --- 上移/下移功能 ---
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

    // --- !! 新增：删除图层功能 !! ---
    contextMenu.addSeparator();
    QAction* removeAction = contextMenu.addAction("删除图层");
    connect(removeAction, &QAction::triggered, this, [=]() { this->onRemoveLayer(index); });

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
    QList<QgsMapLayer*> visibleLayers;
    // 渲染顺序是“从下往上”，所以我们遍历的顺序也应该是从列表底部到顶部
    for (int i = 0; i < m_model->rowCount(); ++i)
    {
        QStandardItem* item = m_model->item(i);
        if (item && item->checkState() == Qt::Checked)
        {
            QVariant layerVariant = item->data(LayerPtrRole);
            if (layerVariant.isValid()) {
                QgsMapLayer* layer = static_cast<QgsMapLayer*>(layerVariant.value<void*>());
                // 注意：setLayers的顺序是绘制顺序，列表第一个元素在最底层
                // 我们模型的顺序是“上层”在第0行，所以需要反向添加
                visibleLayers.prepend(layer);
            }
        }
    }
    m_mapCanvas->setLayers(visibleLayers);
    // m_mapCanvas->refresh(); // setLayers() 内部通常会调用 refresh
    qDebug() << "Map canvas updated with" << visibleLayers.count() << "visible layers.";
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
