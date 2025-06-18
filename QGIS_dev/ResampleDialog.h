#include <QDialog>
#include <qgis.h> // For Qgis::ResamplingMethod

// 前向声明
class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QgsRasterLayer;
class QDoubleSpinBox;

class ResampleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResampleDialog(QWidget* parent = nullptr);
    ~ResampleDialog();

    // 公共接口获取参数
    QString selectedInputRasterPath() const; // 返回输入栅格的路径
    Qgis::RasterResamplingMethod selectedResamplingMethod() const;
    QString outputRasterPath() const;
    double outputPixelSizeX() const; // -1 表示使用源分辨率
    double outputPixelSizeY() const; // -1 表示使用源分辨率

private slots:
    void onSelectInputRasterClicked();
    void onSelectOutputRasterClicked();
    void onInputLayerOrMethodChanged(); // 当输入图层或方法改变时，更新默认输出名
    void onOkClicked();

private:
    void setupUI();
    void populateInputRasterComboBox();
    void updateDefaultOutputPath();

    // UI 控件
    QComboBox* m_inputRasterCombo;
    QPushButton* m_btnSelectInputRasterFile;
    QLabel* m_lblSelectedRasterInfo;

    QComboBox* m_resampleMethodCombo;

    QLabel* m_lblOutputPixelSizeX;
    QDoubleSpinBox* m_spinOutputPixelSizeX;
    QLabel* m_lblOutputPixelSizeY;
    QDoubleSpinBox* m_spinOutputPixelSizeY;
    QLabel* m_lblPixelSizeHint; // 提示用户留空则使用源分辨率


    QLineEdit* m_editOutputRasterPath;
    QPushButton* m_btnSelectOutputRaster;

    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;

    // 数据
    QString m_currentInputRasterPath; // 存储当前选择的输入栅格路径
};
