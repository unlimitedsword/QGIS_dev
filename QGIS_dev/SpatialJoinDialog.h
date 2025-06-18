// SpatialJoinDialog.h
#ifndef SPATIALJOINDIALOG_H
#define SPATIALJOINDIALOG_H

#include <QDialog>

// 前向声明
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QgsVectorLayer;

class SpatialJoinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpatialJoinDialog(QWidget* parent = nullptr);
    ~SpatialJoinDialog();

private slots:
    void onSelectTargetLayerClicked(); // 选择目标图层 (面)
    void onSelectJoinLayerClicked();   // 选择连接图层 (点)
    void onSelectOutputLayerClicked();
    void onTargetLayerChanged();       // 更新目标图层信息
    void onJoinLayerChanged();         // 更新连接图层信息，并填充POI ID字段选择
    void onOkClicked();

private:
    void setupUI();
    void populateTargetLayerComboBox(); // 填充项目中已有的面图层
    void populateJoinLayerComboBox();   // 填充项目中已有点图层
    void populatePoiIdFieldComboBox();  // 根据连接图层填充可选的ID字段

    // UI 控件
    QComboBox* m_targetLayerCombo;       // 目标图层 (面)
    QPushButton* m_btnSelectTargetLayerFile;
    QLabel* m_lblSelectedTargetLayerInfo;

    QComboBox* m_joinLayerCombo;         // 连接图层 (点)
    QPushButton* m_btnSelectJoinLayerFile;
    QLabel* m_lblSelectedJoinLayerInfo;

    QLabel* m_lblPoiIdSourceField;       // 提示选择哪个字段作为PoiId的来源
    QComboBox* m_poiIdSourceFieldCombo; // 从连接图层中选择一个字段作为PoiId

    QLabel* m_lblOutputPoiIdFieldName;   // 输出图层中新字段的名称
    QLineEdit* m_editOutputPoiIdFieldName;

    QLineEdit* m_editOutputLayerPath;
    QPushButton* m_btnSelectOutputLayer;

    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;

    // 数据
    QgsVectorLayer* m_currentTargetLayer;
    QgsVectorLayer* m_currentJoinLayer;
};

#endif // SPATIALJOINDIALOG_H