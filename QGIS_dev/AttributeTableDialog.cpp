#include "attributetabledialog.h"
#include "Output_Manager.h"

#include <qgsvectorlayer.h>
#include <qgsvectorlayercache.h>
#include <qgsattributetablemodel.h>
#include <qgsattributetablefiltermodel.h>
#include <qgsmapcanvas.h>
#include <qgsexpression.h>
#include <qgsexpressioncontext.h>
#include <qgsexpressioncontextutils.h> // 包含这个头文件以获取全局上下文
#include <QVariant>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QHeaderView>
#include <QButtonGroup>
#include <QDebug>
#include <QMessageBox>
#include <QItemSelectionModel>

// ... 构造函数和析构函数保持不变 ...
AttributeTableDialog::AttributeTableDialog(QgsVectorLayer* layer, QgsMapCanvas* canvas, QWidget* parent)
    : QDialog(parent), m_layer(layer), m_canvas(canvas), m_currentSortOrder(Qt::AscendingOrder), m_deleteWarningShown(false)
{
    // ...
    setWindowTitle(QString("属性表 - %1").arg(m_layer->name()));
    resize(1100, 700); // 增大默认尺寸以容纳新控件
    setupUI();
}
AttributeTableDialog::~AttributeTableDialog() {}


void AttributeTableDialog::setupUI()
{
    // --- 主控制面板布局 ---
    QHBoxLayout* controlLayout = new QHBoxLayout();

    // --- 1. 字段选择 (始终显示，最左侧) ---
    controlLayout->addWidget(new QLabel("选择字段:", this));
    m_fieldComboBox = new QComboBox(this);
    controlLayout->addWidget(m_fieldComboBox);

    // --- 2. 操作模式选择 (始终显示，字段选择右侧) ---
    QGroupBox* modeGroupBox = new QGroupBox("操作模式", this);
    QHBoxLayout* modeLayout = new QHBoxLayout();
    m_sortRadioButton = new QRadioButton("排序", this);
    m_searchRadioButton = new QRadioButton("搜索", this);
    QButtonGroup* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_sortRadioButton);
    modeGroup->addButton(m_searchRadioButton);
    modeLayout->addWidget(m_sortRadioButton);
    modeLayout->addWidget(m_searchRadioButton);
    modeGroupBox->setLayout(modeLayout);
    controlLayout->addWidget(modeGroupBox);

    // --- 3. 动态控件区域 (排序或搜索，会根据模式变化) ---
    // 我们创建一个 QHBoxLayout 来容纳这部分，这样可以控制它们的相对顺序
    QHBoxLayout* dynamicControlsLayout = new QHBoxLayout();

    // a. 排序控件 (默认添加到 dynamicControlsLayout)
    m_sortOrderButton = new QPushButton("升序", this);
    dynamicControlsLayout->addWidget(m_sortOrderButton);

    // b. 搜索控件 (默认添加到 dynamicControlsLayout)
    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setPlaceholderText("输入搜索关键字...");
    m_searchButton = new QPushButton("搜索", this);
    dynamicControlsLayout->addWidget(m_searchLineEdit);
    dynamicControlsLayout->addWidget(m_searchButton);

    // 将这个动态控件布局添加到主控制布局
    controlLayout->addLayout(dynamicControlsLayout);

    // --- 4. 右侧固定操作按钮 (在所有动态控件之后，用伸缩项隔开) ---
    controlLayout->addStretch(); // 添加一个伸缩项，将后续按钮推到最右边

    // a. 反向选择按钮
    m_invertSelectionButton = new QPushButton("反向选择", this);
    m_invertSelectionButton->setIcon(QIcon("resource/images/反选.png"));
    m_invertSelectionButton->setToolTip("选中所有当前未选中的行，并取消选中当前已选中的行");
    controlLayout->addWidget(m_invertSelectionButton);

    // b. 删除按钮 (最右侧)
    m_deleteButton = new QPushButton("删除选中", this);
    m_deleteButton->setIcon(QIcon("resource/images/删除.png"));
    m_deleteButton->setToolTip("删除表格中所有选中的行");
    QgsVectorDataProvider* provider = m_layer->dataProvider();
    if (!provider || !(provider->capabilities())) {
        m_deleteButton->setEnabled(false);
        m_deleteButton->setToolTip("该图层的数据源不支持删除操作。");
    }
    controlLayout->addWidget(m_deleteButton);


    // --- 2. 创建并设置 Table View ---
    m_tableView = new QTableView(this); // **先创建**
    m_tableView->setSortingEnabled(false);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection); // **后设置**
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);


    // --- 3. 创建并设置数据模型 ---
    QgsVectorLayerCache* layerCache = new QgsVectorLayerCache(m_layer, 1000, this);
    m_tableModel = new QgsAttributeTableModel(layerCache, this);
    m_filterModel = new QgsAttributeTableFilterModel(m_canvas, m_tableModel, this);
    m_tableView->setModel(m_filterModel);
    m_tableModel->loadLayer();

    // 在模型加载数据后，立即尝试同步图层中已有的选择到表格中
    synchronizeTableSelectionWithLayer();

    // --- 4. 组装主布局 ---
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_tableView);
    setLayout(mainLayout);


    // --- 5. 连接信号和槽 ---
    connect(m_fieldComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AttributeTableDialog::onFieldSelectionChanged);
    connect(m_sortOrderButton, &QPushButton::clicked, this, &AttributeTableDialog::onSortOrderToggle);
    connect(m_searchButton, &QPushButton::clicked, this, &AttributeTableDialog::onSearchButtonClicked);
    connect(m_sortRadioButton, &QRadioButton::toggled, this, &AttributeTableDialog::onFilterModeChanged);
    connect(m_deleteButton, &QPushButton::clicked, this, &AttributeTableDialog::onDeleteSelectedFeatures);
    connect(m_layer, &QgsVectorLayer::selectionChanged, this, &AttributeTableDialog::synchronizeTableSelectionWithLayer);
    connect(m_invertSelectionButton, &QPushButton::clicked, this, &AttributeTableDialog::onInvertSelection); // +++ 连接新信号槽 +++

    // --- 6. 设置初始状态 ---
    populateFieldsComboBox();
    m_sortRadioButton->setChecked(true);
    updateControlsState();
}

// +++ 实现新增的槽函数和辅助函数 +++

void AttributeTableDialog::populateFieldsComboBox()
{
    m_fieldComboBox->blockSignals(true); // 避免在填充时触发信号
    m_fieldComboBox->clear();
    m_fieldComboBox->addItem("--- (不选择字段) ---", -1); // 添加一个默认/空选项

    const QgsFields fields = m_layer->fields();
    for (int i = 0; i < fields.count(); ++i) {
        m_fieldComboBox->addItem(fields.field(i).name(), i); // 显示字段名，存储字段索引
    }
    m_fieldComboBox->blockSignals(false);
}

void AttributeTableDialog::updateControlsState()
{
    bool fieldSelected = (m_fieldComboBox->currentIndex() > 0);

    m_sortRadioButton->setEnabled(fieldSelected);
    m_searchRadioButton->setEnabled(fieldSelected);

    if (!fieldSelected) {
        m_sortRadioButton->setChecked(false);
        m_searchRadioButton->setChecked(false);
    }

    bool sortMode = m_sortRadioButton->isChecked();
    bool searchMode = m_searchRadioButton->isChecked();

    m_sortOrderButton->setVisible(sortMode && fieldSelected);
    m_searchLineEdit->setVisible(searchMode && fieldSelected);
    m_searchButton->setVisible(searchMode && fieldSelected);
}

// 当用户在下拉框中选择一个新字段时
void AttributeTableDialog::onFieldSelectionChanged(int index)
{
    // 清除排序
    m_filterModel->sort(-1, Qt::AscendingOrder);

    // 清除过滤，并强制刷新视图
    m_filterModel->setFilterMode(QgsAttributeTableFilterModel::ShowAll);
    m_filterModel->setFilterExpression(QgsExpression(), QgsExpressionContext());
    m_filterModel->invalidate(); // ++ 正确的刷新方式 ++

    if (index > 0) {
        m_sortRadioButton->setChecked(true);
    }
    updateControlsState();
}


// 当用户点击“升序/降序”按钮时
void AttributeTableDialog::onSortOrderToggle()
{
    if (m_currentSortOrder == Qt::AscendingOrder) {
        m_currentSortOrder = Qt::DescendingOrder;
        m_sortOrderButton->setText("降序");
    }
    else {
        m_currentSortOrder = Qt::AscendingOrder;
        m_sortOrderButton->setText("升序");
    }

    int fieldIndex = m_fieldComboBox->currentData().toInt();
    if (fieldIndex >= 0) {
        // 排序前，确保是对所有行进行操作
        m_filterModel->setFilterMode(QgsAttributeTableFilterModel::ShowAll);
        m_filterModel->setFilterExpression(QgsExpression(), QgsExpressionContext()); // 清除可能存在的过滤器

        // sort() 方法会自动处理视图更新
        m_filterModel->sort(fieldIndex, m_currentSortOrder);
    }
}

void AttributeTableDialog::onFilterModeChanged()
{
    // 当从“搜索”模式切换回“排序”模式时，清除过滤器
    if (m_sortRadioButton->isChecked()) {
        if (m_filterModel->filterMode() != QgsAttributeTableFilterModel::ShowAll ||
            !m_filterModel->filterExpression().isEmpty()) {

            qDebug() << "Mode switched to Sort. Clearing filter.";
            m_filterModel->setFilterMode(QgsAttributeTableFilterModel::ShowAll);
            m_filterModel->setFilterExpression(QgsExpression(), QgsExpressionContext());
            m_filterModel->invalidate();
        }
    }
    updateControlsState();
}


// 当用户点击“搜索”按钮时
void AttributeTableDialog::onSearchButtonClicked()
{
    qDebug() << "------------------------------------------";
    qDebug() << "[START] onSearchButtonClicked() called.";

    // 字段选择检查
    int fieldIndex = m_fieldComboBox->currentData().toInt();
    qDebug() << "[DEBUG] Current field index from ComboBox:" << fieldIndex;
    if (fieldIndex < 0) {
        QMessageBox::information(this, "提示", "请先从下拉框中选择一个要搜索的字段。");
        qDebug() << "------------------------------------------";
        return;
    }

    // 获取搜索文本
    QString searchText = m_searchLineEdit->text().trimmed();
    qDebug() << "[DEBUG] Search text from LineEdit:" << "\"" << searchText << "\"";

    // 处理空搜索
    if (searchText.isEmpty()) {
        qDebug() << "[ACTION] Clearing filter because search text is empty.";
        m_filterModel->setFilterMode(QgsAttributeTableFilterModel::ShowAll);
        m_filterModel->invalidate();

        // 更新视图
        if (m_tableView) {
            m_tableView->viewport()->update();
        }

        qDebug() << "------------------------------------------";
        return;
    }

    // 获取字段信息
    QString fieldName = m_layer->fields().field(fieldIndex).name();
    QMetaType::Type fieldType = static_cast<QMetaType::Type>(m_layer->fields().field(fieldIndex).type());

    qDebug() << "[DEBUG] Field name:" << fieldName;
    qDebug() << "[DEBUG] Field type (QVariant type):" << fieldType;
    qDebug() << "[DEBUG] Field type name:" << m_layer->fields().field(fieldIndex).typeName();

    // 根据字段类型构建表达式
    QString filterString;
    if (fieldType == QMetaType::Int || fieldType == QMetaType::LongLong ||
        fieldType == QMetaType::Double || fieldType == QMetaType::ULongLong) {
        bool isNumeric;
        double numericValue = searchText.toDouble(&isNumeric);

        if (isNumeric) {
            filterString = QString("\"%1\" = %2").arg(fieldName).arg(numericValue);
            qDebug() << "[INFO] Numeric field. Using exact match:" << filterString;
        }
        else {
            filterString = QString("to_string(\"%1\") ILIKE '%%2%'").arg(fieldName).arg(searchText);
            qDebug() << "[INFO] Numeric field with non-numeric search. Using string conversion:" << filterString;
        }
    }
    else {
        filterString = QString("\"%1\" ILIKE '%%2%'").arg(fieldName).arg(searchText);
        qDebug() << "[INFO] String field. Using ILIKE:" << filterString;
    }

    qDebug() << "[DEBUG] Constructed filter string:" << filterString;

    // 创建表达式
    QgsExpression expression(filterString);
    QgsExpressionContext context;
    context.appendScopes(QgsExpressionContextUtils::globalProjectLayerScopes(m_layer));

    if (!expression.prepare(&context)) {
        qDebug() << "[ERROR] Expression preparation failed:" << expression.parserErrorString();
        QMessageBox::critical(this, "表达式错误",
            QString("无法解析搜索表达式:\n%1\n\n错误信息: %2")
            .arg(expression.expression())
            .arg(expression.parserErrorString()));
        return;
    }

    // ===== 基于官方API的解决方案 =====
    QgsFeatureRequest request;
    request.setFilterExpression(expression);
    request.setExpressionContext(context);

    // 获取匹配的特征ID
    QgsFeatureIds matchingIds;
    QgsFeatureIterator it = m_layer->getFeatures(request);
    QgsFeature feature;
    while (it.nextFeature(feature)) {
        matchingIds.insert(feature.id());
    }

    qDebug() << "[DEBUG] Found" << matchingIds.size() << "matching features";

    // 使用官方API设置过滤特征
    m_filterModel->setFilterMode(QgsAttributeTableFilterModel::ShowFilteredList);
    m_filterModel->setFilteredFeatures(matchingIds);

    // 强制刷新模型 - 官方推荐的更新方式
    m_filterModel->invalidate();

    // 可选：触发特征过滤信号
    emit m_filterModel->featuresFiltered();
    // ================================

    // 更新视图
    if (m_tableView) {
        m_tableView->viewport()->update();
    }

    // 调试输出
    qDebug() << "[DEBUG] After setting filtered features:";
    qDebug() << "  - Filter mode: " << m_filterModel->filterMode();
    qDebug() << "  - Filtered features count: " << m_filterModel->filteredFeatures().size();
    qDebug() << "  - Row count in filter model: " << m_filterModel->rowCount();

    qDebug() << "[END] Filter applied and view updated.";
    qDebug() << "------------------------------------------";
}

// ====================== 新增：实现删除功能 ======================
void AttributeTableDialog::onDeleteSelectedFeatures()
{
    if (!m_layer || !m_filterModel || !m_tableView) return;

    // 1. 获取当前在表格视图中【实际被高亮选中】的行的模型索引
    QModelIndexList selectedViewIndexes = m_tableView->selectionModel()->selectedRows();

    if (selectedViewIndexes.isEmpty()) {
        QMessageBox::information(this, "提示", "没有选中任何行进行删除。");
        return;
    }

    // 2. 将这些视图索引转换为要素ID
    QgsFeatureIds idsToDelete;
    for (const QModelIndex& viewIndex : selectedViewIndexes) {
        if (viewIndex.isValid()) {
            QModelIndex sourceIndex = m_filterModel->mapToSource(viewIndex);
            idsToDelete.insert(m_tableModel->rowToId(sourceIndex.row()));
        }
    }

    if (idsToDelete.isEmpty()) {
        // 这通常不应该发生，但作为保险
        QMessageBox::information(this, "提示", "无法识别选中的要素。");
        return;
    }

    // 3. 弹出确认对话框 (只在本次打开属性表时提示一次)
    if (!m_deleteWarningShown) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this, "确认删除",
            QString("您确定要永久删除选中的 %1 个要素吗？\n\n此操作无法撤销！").arg(idsToDelete.size()),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            return;
        }
        m_deleteWarningShown = true;
    }

    // 4. 执行删除操作 (这部分逻辑不变，是正确的)
    if (!m_layer->isEditable()) {
        if (!m_layer->startEditing()) {
            QMessageBox::critical(this, "错误", "无法开启图层编辑模式，删除失败。");
            return;
        }
    }
    if (m_layer->dataProvider()->deleteFeatures(idsToDelete)) {
        if (!m_layer->commitChanges()) {
            QMessageBox::critical(this, "错误", "删除成功，但提交更改失败。请检查数据源。");
            m_layer->rollBack();
        }
        else {
            OutputManager::instance()->logMessage(QString("成功删除 %1 个要素。").arg(idsToDelete.size()));
        }
    }
    else {
        QMessageBox::critical(this, "错误", "从数据源删除要素失败。");
        m_layer->rollBack();
    }

    // 5. 刷新地图和属性表
    m_layer->triggerRepaint();
    // 删除后，模型数据源已改变，让模型自己通知视图刷新
    m_tableModel->loadLayer(); // 重新加载数据，这会强制刷新所有内容
    // 或者更温和的方式，如果上面的loadLayer太慢：
    // m_filterModel->invalidate(); 
}

// +++ 新增一个辅助函数，用于同步表格选择 +++
void AttributeTableDialog::synchronizeTableSelectionWithLayer()
{
    if (!m_layer || !m_filterModel || !m_tableView) return;

    // 1. 清除表格中当前所有的选择
    m_tableView->selectionModel()->clearSelection();

    // 2. 获取图层当前选中的要素ID
    QgsFeatureIds selectedFids = m_layer->selectedFeatureIds();

    if (selectedFids.isEmpty()) {
        return; // 如果图层没有选中要素，则无需操作
    }

    // 3. 遍历选中的要素ID，在表格中找到对应的行并选中它们
    QItemSelection selection;
    for (const QgsFeatureId fid : selectedFids) {
        // QgsAttributeTableFilterModel 有一个 fidToIndexList 方法可以找到对应的 ModelIndex
        QModelIndexList viewIndexes = m_filterModel->fidToIndexList(fid);
        for (const QModelIndex& viewIndex : viewIndexes) {
            if (viewIndex.isValid()) {
                // 我们要选中整行
                QModelIndex left = m_filterModel->index(viewIndex.row(), 0);

                QModelIndex right = m_filterModel->index(viewIndex.row(), m_filterModel->columnCount(QModelIndex()) - 1);

                selection.select(left, right);
            }
        }
    }

    // 4. 应用新的选择到表格视图
    if (!selection.isEmpty()) {
        m_tableView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        // 可选：滚动到第一个选中的行
        if (!selection.indexes().isEmpty()) {
            m_tableView->scrollTo(selection.indexes().first());
        }
    }
}

void AttributeTableDialog::onInvertSelection()
{
    if (!m_layer || !m_filterModel || !m_tableView) return;

    qDebug() << "[START] onInvertSelection called.";

    // 1. 获取当前表格视图中【所有可见行】的模型索引
    QList<QModelIndex> allVisibleViewIndexes;
    for (int row = 0; row < m_filterModel->rowCount(); ++row) {
        allVisibleViewIndexes.append(m_filterModel->index(row, 0));
    }

    if (allVisibleViewIndexes.isEmpty()) {
        qDebug() << "[INFO] No rows in current view. Nothing to invert.";
        return;
    }

    // 2. 获取当前在表格视图中【实际被高亮选中】的行的模型索引
    QModelIndexList currentlySelectedViewIndexes = m_tableView->selectionModel()->selectedRows();

    // 3. 创建一个新的QItemSelection，用于存储反选后的结果
    QItemSelection newSelection;

    // 4. 遍历所有可见行
    for (const QModelIndex& viewIndex : allVisibleViewIndexes) {
        if (viewIndex.isValid()) {
            bool wasSelected = false;
            // 检查这一行之前是否被选中
            for (const QModelIndex& selectedIdx : currentlySelectedViewIndexes) {
                if (selectedIdx.row() == viewIndex.row()) {
                    wasSelected = true;
                    break;
                }
            }

            if (!wasSelected) {
                // 如果这行之前没被选中，现在就选中它
                QModelIndex left = m_filterModel->index(viewIndex.row(), 0);
                QModelIndex right = m_filterModel->index(viewIndex.row(), m_filterModel->columnCount(QModelIndex()) - 1);
                newSelection.select(left, right);
            }
            // 如果这行之前被选中了，那么在新的selection里它就是未选中状态 (因为我们从空selection开始构建)
        }
    }

    // 5. 应用新的选择到表格视图
    //    a. 先清除旧的视觉选择
    m_tableView->selectionModel()->clearSelection();
    //    b. 再应用新的视觉选择
    if (!newSelection.isEmpty()) {
        m_tableView->selectionModel()->select(newSelection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    // 6. **核心：根据表格UI的最新选择状态，去更新图层的选择集**
    QgsFeatureIds fidsToSelectInLayer;
    QModelIndexList finalSelectedViewIndexes = m_tableView->selectionModel()->selectedRows();
    for (const QModelIndex& viewIndex : finalSelectedViewIndexes) {
        if (viewIndex.isValid()) {
            QModelIndex sourceIndex = m_filterModel->mapToSource(viewIndex);
            fidsToSelectInLayer.insert(m_tableModel->rowToId(sourceIndex.row()));
        }
    }

    // 更新图层选择集（先清空，再设置）
    m_layer->removeSelection();
    if (!fidsToSelectInLayer.isEmpty()) {
        m_layer->selectByIds(fidsToSelectInLayer);
    }

    qDebug() << "[END] Invert selection finished. UI selection updated. Layer selected count:" << m_layer->selectedFeatureCount();
}