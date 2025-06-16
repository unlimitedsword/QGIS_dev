#pragma once
#include <QDialog>

class QgsVectorLayer;
class QTableView;
class QgsAttributeTableModel;
class QgsAttributeTableFilterModel;
class QgsMapCanvas; // +++ 前向声明 QgsMapCanvas

class AttributeTableDialog : public QDialog
{
    Q_OBJECT

public:
    // +++ 构造函数现在需要接收 layer 和 canvas +++
    explicit AttributeTableDialog(QgsVectorLayer* layer, QgsMapCanvas* canvas, QWidget* parent = nullptr);
    ~AttributeTableDialog();

private:
    void setupUI();

    QgsVectorLayer* m_layer;
    QgsMapCanvas* m_canvas; // +++ 添加一个成员变量来保存 canvas 指针
    QTableView* m_tableView;
    QgsAttributeTableModel* m_tableModel;
    QgsAttributeTableFilterModel* m_filterModel;
};