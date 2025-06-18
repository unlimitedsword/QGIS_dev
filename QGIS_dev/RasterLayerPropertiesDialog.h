#include <QDialog>

// 前向声明
class QgsRasterLayer;
class QLabel;
class QTableWidget; // 我们仍然用表格来美观地显示信息
class QTextEdit;    // 用于显示多波段统计

class RasterLayerPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RasterLayerPropertiesDialog(QgsRasterLayer* layer, QWidget* parent = nullptr);
    ~RasterLayerPropertiesDialog();

private:
    void setupUI();
    void populateGeneralInfo();
    void populateAllBandStatistics(); // 新函数：填充所有波段的统计

    QgsRasterLayer* m_rasterLayer;

    // UI - 基本信息区
    QTableWidget* m_infoTableWidget;

    // UI - 多波段统计区
    QLabel* m_lblBandStatsTitle;
    QTextEdit* m_allBandStatsTextEdit; // 用一个简单的文本框显示所有波段统计
};
