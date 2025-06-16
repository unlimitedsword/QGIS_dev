// attributetabledialog.cpp

#include "attributetabledialog.h"
#include <qgsvectorlayer.h>
#include <qgsvectorlayercache.h> // +++ 需要包含这个头文件
#include <qgsattributetablemodel.h>
#include <qgsattributetablefiltermodel.h>
#include <qgsfeaturemodel.h>
#include <qgsmapcanvas.h> // +++ 需要包含这个头文件
#include <QTableView>
#include <QVBoxLayout>
#include <QDebug>

// +++ 修改构造函数实现 +++
AttributeTableDialog::AttributeTableDialog(QgsVectorLayer* layer, QgsMapCanvas* canvas, QWidget* parent)
    : QDialog(parent), m_layer(layer), m_canvas(canvas) // 初始化新成员
{
    if (!m_layer || !m_canvas) { // 同时检查 layer 和 canvas
        qWarning() << "AttributeTableDialog created with a null layer or canvas.";
        return;
    }

    setWindowTitle(QString("属性表 - %1").arg(m_layer->name()));
    resize(800, 600);

    setupUI();
}

AttributeTableDialog::~AttributeTableDialog()
{
    qDebug() << "AttributeTableDialog destroyed.";
}

void AttributeTableDialog::setupUI()
{
    m_tableView = new QTableView(this);

    // ====================== 核心修改：使用新的构造函数 ======================

   // 1. 使用 QgsVectorLayerCache 替代直接访问 dataStore
    QgsVectorLayerCache* layerCache = new QgsVectorLayerCache(m_layer, 1000, this);

    // 2. 创建 QgsAttributeTableModel，传入 layerCache
    m_tableModel = new QgsAttributeTableModel(layerCache, this);

    // 3. 创建 QgsAttributeTableFilterModel，传入 canvas 和 tableModel
    m_filterModel = new QgsAttributeTableFilterModel(m_canvas, m_tableModel, this);


   // 4. 将过滤模型设置为视图的模型（注意：不再需要调用 setSourceModel）
    //    因为新的构造函数已经完成了这个工作。
    m_tableView->setModel(m_filterModel);

    // 5. 加载数据
    m_tableModel->loadLayer();

    // ======================================================================

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_tableView);
    setLayout(layout);
}