// ReprojectRasterDialog.h
#ifndef REPROJECTRASTERDIALOG_H
#define REPROJECTRASTERDIALOG_H

#include <QDialog>
#include <qgscoordinatereferencesystem.h> // 包含QgsCoordinateReferenceSystem

// 前向声明
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QgsRasterLayer;

class ReprojectRasterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReprojectRasterDialog(QWidget* parent = nullptr);
    ~ReprojectRasterDialog();

private slots:
    void onSelectInputRasterClicked();
    void onSelectOutputRasterClicked();
    void onInputRasterChanged(int index);
    void onTargetCrsComboChanged(int index); // 当目标CRS下拉框改变时
    void onOkClicked();

private:
    void setupUI();
    void populateInputRasterComboBox();
    void populateTargetCrsComboBox(); // 新函数：填充目标CRS下拉框
    void updateDefaultOutputPath();

    // UI 控件
    QComboBox* m_inputRasterCombo;
    QPushButton* m_btnSelectInputRasterFile;
    QLabel* m_lblSelectedRasterInfo;
    QLabel* m_lblCurrentCrsInfo;

    // === 修改：使用QComboBox替代QgsProjectionSelectionWidget ===
    QLabel* m_lblTargetCrs;
    QComboBox* m_targetCrsCombo;
    // =======================================================

    QLineEdit* m_editOutputRasterPath;
    QPushButton* m_btnSelectOutputRaster;

    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;

    // 数据
    QString m_currentInputRasterPath;
    QgsCoordinateReferenceSystem m_currentTargetCrs; // 存储当前选中的目标CRS
};

#endif // REPROJECTRASTERDIALOG_H