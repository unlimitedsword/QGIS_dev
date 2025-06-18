// AnalysisToolboxWidget.h
#ifndef ANALYSISTOOLBOXWIDGET_H
#define ANALYSISTOOLBOXWIDGET_H

#include <QWidget>
#include <QStandardItemModel> // 我们用QStandardItemModel来构建树

// 前向声明
class QTreeView;
class QStandardItem;
class QModelIndex;

// 定义一个枚举或常量来唯一标识每个工具
enum class AnalysisToolId {
    SpatialJoin,
    Buffer,
    RasterStats,
    RasterClip,
    RasterResample,
    RasterReproject,
    ZonalStats
    // ... 可以继续添加
};
// 将工具ID存储在QStandardItem的UserRole中
const int ToolIdRole = Qt::UserRole + 1;


class AnalysisToolboxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnalysisToolboxWidget(QWidget* parent = nullptr);
    ~AnalysisToolboxWidget();

private slots:
    // 当用户双击树中的项时调用
    void onItemDoubleClicked(const QModelIndex& index);

private:
    void setupUI();
    void populateToolTree(); // 填充工具树的内容

    QTreeView* m_toolTreeView;
    QStandardItemModel* m_toolModel;
};

#endif // ANALYSISTOOLBOXWIDGET_H