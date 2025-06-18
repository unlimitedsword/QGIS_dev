// RasterClipDialog.h
#ifndef RASTERCLIPDIALOG_H
#define RASTERCLIPDIALOG_H

#include <QDialog>

// 前向声明
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QgsRasterLayer;
class QgsVectorLayer;

class RasterClipDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RasterClipDialog(QWidget* parent = nullptr);
    ~RasterClipDialog();

private slots:
    void onSelectInputRasterClicked();
    void onSelectClipFeatureClicked(); // 选择裁剪矢量图层
    void onSelectOutputRasterClicked();
    void onOkClicked();

private:
    void setupUI();
    void populateInputRasterComboBox();
    void populateClipFeatureComboBox(); // 填充项目中已有的矢量面图层

    // UI 控件
    QComboBox* m_inputRasterCombo;
    QPushButton* m_btnSelectInputRasterFile;
    QLabel* m_lblSelectedRasterInfo;

    QComboBox* m_clipFeatureCombo;       // 选择项目中已有的矢量面图层
    QPushButton* m_btnSelectClipFeatureFile; // 从文件选择矢量
    QLabel* m_lblSelectedClipFeatureInfo;

    QLineEdit* m_editOutputRasterPath;
    QPushButton* m_btnSelectOutputRaster;

    // +++ 新增控件 +++
    QLabel* m_lblOutputNoDataHandling;
    QComboBox* m_comboNoDataHandling;

    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;

    // 数据
    QString m_currentInputRasterPath;
    QString m_currentClipFeaturePath; // 存储当前选择的裁剪矢量路径
};

#endif // RASTERCLIPDIALOG_H