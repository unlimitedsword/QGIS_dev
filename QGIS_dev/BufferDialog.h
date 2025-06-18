// BufferDialog.h
#ifndef BUFFERDIALOG_H
#define BUFFERDIALOG_H

#include <QDialog>
#include <qgis.h> // For Qgis::BufferCapStyle, Qgis::BufferJoinStyle
#include <qgsunittypes.h> // For Qgis::DistanceUnit

// 前向声明
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QgsVectorLayer;
class QDoubleSpinBox;
class QSpinBox;     // 用于线段数量 (segments)
class QCheckBox;    // 用于溶解结果
class QgsColorButton; // 用于颜色选择 (QGIS提供)

class BufferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BufferDialog(QWidget* parent = nullptr);
    ~BufferDialog();

    // 公共接口获取参数
    QgsVectorLayer* selectedInputLayer() const;
    double bufferDistance() const;
    Qgis::DistanceUnit distanceUnits() const;
    int segments() const;
    bool dissolveResult() const;
    QString outputLayerPath() const;
    QColor fillColor() const;       // 获取填充颜色
    double fillOpacity() const;   // 获取填充透明度 (0-1)
    QColor strokeColor() const;     // 获取边界颜色
    double strokeWidth() const;   // 获取边界宽度

private slots:
    void onSelectInputLayerClicked();
    void onSelectOutputLayerClicked();
    void onInputLayerChanged(); // 当输入图层改变时，尝试更新单位
    void onOkClicked();

private:
    void setupUI();
    void populateInputLayerComboBox();
    void populateUnitComboBox();
    void updateDefaultOutputPath();


    // UI 控件
    QComboBox* m_inputLayerCombo;
    QPushButton* m_btnSelectInputLayerFile;
    QLabel* m_lblSelectedInputLayerInfo;

    QDoubleSpinBox* m_spinBufferDistance;
    QComboBox* m_unitCombo;             // 单位选择
    QSpinBox* m_spinSegments;           // 端点平滑度
    QCheckBox* m_chkDissolveResult;     // 是否溶解

    QLineEdit* m_editOutputLayerPath;
    QPushButton* m_btnSelectOutputLayer;

    // 可视化参数
    QLabel* m_lblFillColor;
    QgsColorButton* m_btnFillColor;
    QLabel* m_lblFillOpacity;
    QDoubleSpinBox* m_spinFillOpacity; // 0-1 or 0-100%

    QLabel* m_lblStrokeColor;
    QgsColorButton* m_btnStrokeColor;
    QLabel* m_lblStrokeWidth;
    QDoubleSpinBox* m_spinStrokeWidth;


    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;

    // 数据
    QgsVectorLayer* m_currentInputLayer; // 存储当前选择的输入矢量图层
};

#endif // BUFFERDIALOG_H