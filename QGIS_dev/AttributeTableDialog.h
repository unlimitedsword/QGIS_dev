#include <QDialog>
#include <QItemSelectionModel> // +++ 新增，用于获取选中行

// 前向声明
class QgsVectorLayer;
class QgsMapCanvas;
class QTableView;
class QgsAttributeTableModel;
class QgsAttributeTableFilterModel;
class QComboBox;
class QRadioButton;
class QPushButton;
class QLineEdit;

class AttributeTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttributeTableDialog(QgsVectorLayer* layer, QgsMapCanvas* canvas, QWidget* parent = nullptr);
    ~AttributeTableDialog();

private slots:
    // +++ 新增的槽函数 +++
    void onFieldSelectionChanged(int index);
    void onSortOrderToggle();
    void onSearchButtonClicked();
    void onFilterModeChanged(); // 响应排序/搜索模式的切换

    // +++ 新增槽函数 +++
    void onDeleteSelectedFeatures();

    void synchronizeTableSelectionWithLayer();

    void onInvertSelection();

private:
    void setupUI();
    void populateFieldsComboBox();
    void updateControlsState(); // 一个用于更新控件启用/禁用状态的辅助函数

    // UI 控件
    QTableView* m_tableView;
    QComboBox* m_fieldComboBox;
    QRadioButton* m_sortRadioButton;
    QRadioButton* m_searchRadioButton;
    QPushButton* m_sortOrderButton;
    QLineEdit* m_searchLineEdit;
    QPushButton* m_searchButton;
    QPushButton* m_deleteButton; // +++ 新增删除按钮 +++
    QPushButton* m_invertSelectionButton; // +++ 新增反选按钮 +++

    // 数据模型
    QgsVectorLayer* m_layer;
    QgsMapCanvas* m_canvas;
    QgsAttributeTableModel* m_tableModel;
    QgsAttributeTableFilterModel* m_filterModel;

    // 状态变量
    Qt::SortOrder m_currentSortOrder;

    // +++ 新增状态变量，用于控制删除提示只显示一次 +++
    bool m_deleteWarningShown;
};