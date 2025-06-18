// AnalysisToolboxWidget.cpp
#include "AnalysisToolboxWidget.h"
#include "ResampleDialog.h"
#include "ReprojectRasterDialog.h"
#include "RasterClipDialog.h"
#include "BufferDialog.h"
#include "SpatialJoinDialog.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QDebug>

// --- 引入未来会创建的工具对话框的头文件 (占位符) ---
// #include "SpatialJoinDialog.h"
// #include "BufferDialog.h"
// ... 等等

AnalysisToolboxWidget::AnalysisToolboxWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

AnalysisToolboxWidget::~AnalysisToolboxWidget()
{
}

void AnalysisToolboxWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_toolTreeView = new QTreeView(this);
    m_toolModel = new QStandardItemModel(this);
    m_toolModel->setHorizontalHeaderLabels({ "分析工具" }); // 设置表头
    m_toolTreeView->setModel(m_toolModel);
    m_toolTreeView->setHeaderHidden(false); // 显示表头，让它看起来更像一个工具箱
    m_toolTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑

    populateToolTree(); // 填充工具

    connect(m_toolTreeView, &QTreeView::doubleClicked, this, &AnalysisToolboxWidget::onItemDoubleClicked);

    mainLayout->addWidget(m_toolTreeView);
    setLayout(mainLayout);
}

void AnalysisToolboxWidget::populateToolTree()
{
    // --- 1. 空间分析 ---
    QStandardItem* spatialAnalysisGroup = new QStandardItem("空间分析");
    spatialAnalysisGroup->setEditable(false); // 组节点不可编辑
    // spatialAnalysisGroup->setIcon(QIcon(":/icons/group.png")); // 可选：设置图标
    m_toolModel->appendRow(spatialAnalysisGroup);

    QStandardItem* spatialJoinItem = new QStandardItem("空间连接 (点落入面)");
    spatialJoinItem->setData(static_cast<int>(AnalysisToolId::SpatialJoin), ToolIdRole);
    spatialJoinItem->setEditable(false);
    // spatialJoinItem->setIcon(QIcon(":/icons/spatial_join.png"));
    spatialAnalysisGroup->appendRow(spatialJoinItem);

    QStandardItem* bufferItem = new QStandardItem("缓冲区分析");
    bufferItem->setData(static_cast<int>(AnalysisToolId::Buffer), ToolIdRole);
    bufferItem->setEditable(false);
    spatialAnalysisGroup->appendRow(bufferItem);


    // --- 2. 栅格数据处理 ---
    QStandardItem* rasterGroup = new QStandardItem("栅格数据处理");
    rasterGroup->setEditable(false);
    m_toolModel->appendRow(rasterGroup);

    QStandardItem* rasterStatsItem = new QStandardItem("波段统计");
    rasterStatsItem->setData(static_cast<int>(AnalysisToolId::RasterStats), ToolIdRole);
    rasterStatsItem->setEditable(false);
    rasterGroup->appendRow(rasterStatsItem);

    QStandardItem* rasterClipItem = new QStandardItem("地图裁剪");
    rasterClipItem->setData(static_cast<int>(AnalysisToolId::RasterClip), ToolIdRole);
    rasterClipItem->setEditable(false);
    rasterGroup->appendRow(rasterClipItem);

    QStandardItem* rasterResampleItem = new QStandardItem("重采样");
    rasterResampleItem->setData(static_cast<int>(AnalysisToolId::RasterResample), ToolIdRole);
    rasterResampleItem->setEditable(false);
    rasterGroup->appendRow(rasterResampleItem);

    QStandardItem* rasterReprojectItem = new QStandardItem("投影转换");
    rasterReprojectItem->setData(static_cast<int>(AnalysisToolId::RasterReproject), ToolIdRole);
    rasterReprojectItem->setEditable(false);
    rasterGroup->appendRow(rasterReprojectItem);


    // --- 3. 分区统计 ---
    QStandardItem* zonalStatsGroup = new QStandardItem("分区统计");
    zonalStatsGroup->setEditable(false);
    m_toolModel->appendRow(zonalStatsGroup);

    QStandardItem* zonalStatsItem = new QStandardItem("栅格分区统计");
    zonalStatsItem->setData(static_cast<int>(AnalysisToolId::ZonalStats), ToolIdRole);
    zonalStatsItem->setEditable(false);
    zonalStatsGroup->appendRow(zonalStatsItem);

    // 自动展开所有顶级项
    for (int i = 0; i < m_toolModel->rowCount(); ++i) {
        m_toolTreeView->expand(m_toolModel->index(i, 0));
    }
}

void AnalysisToolboxWidget::onItemDoubleClicked(const QModelIndex& index)
{
    // 检查是否是叶子节点（即具体的工具项，而不是分组项）
    QStandardItem* item = m_toolModel->itemFromIndex(index);
    if (!item || item->hasChildren()) { // 如果是分组项，则不处理
        return;
    }

    AnalysisToolId toolId = static_cast<AnalysisToolId>(item->data(ToolIdRole).toInt());

    qDebug() << "Tool double clicked:" << item->text() << "with ID:" << static_cast<int>(toolId);

    // 根据工具ID，创建并显示对应的对话框
    // 这里是未来实现的核心，现在只是占位符
    switch (toolId)
    {
    case AnalysisToolId::SpatialJoin:
    {
        SpatialJoinDialog* sjDialog = new SpatialJoinDialog(this);
        sjDialog->setAttribute(Qt::WA_DeleteOnClose);
        sjDialog->show();
        qDebug() << "SpatialJoinDialog opened.";
    }
    break;
    case AnalysisToolId::Buffer:
    {
        BufferDialog* bufferDialog = new BufferDialog(this);
        bufferDialog->setAttribute(Qt::WA_DeleteOnClose);
        bufferDialog->show();
        qDebug() << "BufferDialog opened.";
    }
    break;

    case AnalysisToolId::RasterClip: // 假设这是您为裁剪定义的ID
    {
        RasterClipDialog* clipDialog = new RasterClipDialog(this);
        clipDialog->setAttribute(Qt::WA_DeleteOnClose);
        clipDialog->show();
        qDebug() << "RasterClipDialog opened.";
    }
    break;
    case AnalysisToolId::RasterResample: // 重采样定义的ID
    {
        ResampleDialog* resampleDialog = new ResampleDialog(this);
        resampleDialog->setAttribute(Qt::WA_DeleteOnClose);
        resampleDialog->show();
        qDebug() << "ResampleDialog opened.";
    }
    break;
    case AnalysisToolId::RasterReproject: // 假设这是您为投影转换定义的ID
    {
        ReprojectRasterDialog* reprojectDialog = new ReprojectRasterDialog(this);
        reprojectDialog->setAttribute(Qt::WA_DeleteOnClose);
        reprojectDialog->show();
        qDebug() << "ReprojectRasterDialog opened.";
    }
    break;
        // ... 其他工具的处理 ...
    default:
        qWarning() << "Unknown tool ID:" << static_cast<int>(toolId);
        break;
    }
}